// Combines existing PNGs from extension/icons/ into a multi-resolution
// Windows .ico at assets/icon.ico. Run once when the logo changes:
//
//   bun tools/generate-ico.ts
//
// Output: assets/icon.ico
//
// Background: Windows .ico is a tiny header + per-image directory entries
// pointing at raw image data. Modern Windows accepts PNG payloads inside
// .ico (Vista+), so we don't need BMP encoding - we just glue the PNGs
// we already have.

import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { join, dirname }                            from "node:path";
import { fileURLToPath }                            from "node:url";

const here    = dirname(fileURLToPath(import.meta.url));
const repo    = dirname(here);
const iconDir = join(repo, "extension", "icons");
const outFile = join(repo, "assets", "icon.ico");

interface IcoImage {
  width:  number;  // 0 means 256
  height: number;
  data:   Buffer;
}

function loadPng(size: number): IcoImage | null {
  const path = join(iconDir, `icon-${size}.png`);
  if (!existsSync(path)) return null;
  return {
    width:  size === 256 ? 0 : size,
    height: size === 256 ? 0 : size,
    data:   readFileSync(path),
  };
}

const sizes  = [16, 32, 48, 128, 256];
const images = sizes.map(loadPng).filter((i): i is IcoImage => i !== null);

if (images.length === 0) {
  console.error(`No PNGs found in ${iconDir}. Run extension's icon generator first.`);
  process.exit(1);
}

// ICONDIR (6 bytes) + ICONDIRENTRY[N] (16 bytes each) + raw image data.
const headerSize = 6 + images.length * 16;
let   offset     = headerSize;

const header = Buffer.alloc(6);
header.writeUInt16LE(0, 0);             // reserved
header.writeUInt16LE(1, 2);             // type: 1 = icon
header.writeUInt16LE(images.length, 4); // count

const entries = Buffer.alloc(images.length * 16);
for (let i = 0; i < images.length; ++i) {
  const img    = images[i]!;
  const base   = i * 16;
  entries.writeUInt8 (img.width,         base + 0);
  entries.writeUInt8 (img.height,        base + 1);
  entries.writeUInt8 (0,                 base + 2); // color palette (0 for PNG)
  entries.writeUInt8 (0,                 base + 3); // reserved
  entries.writeUInt16LE(1,               base + 4); // color planes
  entries.writeUInt16LE(32,              base + 6); // bits per pixel
  entries.writeUInt32LE(img.data.length, base + 8); // bytes in resource
  entries.writeUInt32LE(offset,          base + 12); // offset in file
  offset += img.data.length;
}

const out = Buffer.concat([header, entries, ...images.map(i => i.data)]);
writeFileSync(outFile, out);
console.log(`Wrote ${outFile}  (${images.length} images, ${out.length} bytes)`);
