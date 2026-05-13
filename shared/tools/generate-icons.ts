// Generates the four PNG icons the Chrome MV3 manifest requires.
// Pure Bun script - no Inkscape / ImageMagick needed.
//
// Run from the repo root:
//   bun run tools/generate-icons.ts
//
// Output: extension/icons/icon-{16,32,48,128}.png. Solid slate-blue rounded
// squares with a transparent background. Not the full Vesalius padlock from
// the SVG, but recognisable enough that the toolbar shows the brand color.

import { writeFileSync, mkdirSync } from "node:fs";
import { deflateSync }               from "node:zlib";

const CRC32_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) {
      c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
    }
    t[n] = c >>> 0;
  }
  return t;
})();

function crc32(buf: Buffer): number {
  let crc = 0xFFFFFFFF;
  for (const b of buf) {
    crc = CRC32_TABLE[(crc ^ b) & 0xFF]! ^ (crc >>> 8);
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

function chunk(type: string, data: Buffer): Buffer {
  const len = Buffer.alloc(4);
  len.writeUInt32BE(data.length, 0);
  const typeBuf = Buffer.from(type, "ascii");
  const crcBuf  = Buffer.alloc(4);
  crcBuf.writeUInt32BE(crc32(Buffer.concat([typeBuf, data])), 0);
  return Buffer.concat([len, typeBuf, data, crcBuf]);
}

function generateIcon(size: number): Buffer {
  const sig = Buffer.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]);

  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(size, 0);
  ihdr.writeUInt32BE(size, 4);
  ihdr[8]  = 8;  // bit depth
  ihdr[9]  = 6;  // color type: RGBA
  ihdr[10] = 0;
  ihdr[11] = 0;
  ihdr[12] = 0;

  // Slate-blue accent #7895B5; transparent corners; rounded square shape.
  const fg = [0x78, 0x95, 0xB5, 0xFF];
  const padding = Math.max(0, Math.floor(size * 0.05));
  const radius  = Math.max(2, Math.floor(size * 0.18));

  function inside(x: number, y: number): boolean {
    const left   = padding;
    const right  = size - padding;
    const top    = padding;
    const bottom = size - padding;
    if (x < left || x >= right || y < top || y >= bottom) return false;

    let cx: number | null = null;
    let cy: number | null = null;
    if (x < left + radius && y < top + radius) {
      cx = left + radius;       cy = top + radius;
    } else if (x >= right - radius && y < top + radius) {
      cx = right - 1 - radius;  cy = top + radius;
    } else if (x < left + radius && y >= bottom - radius) {
      cx = left + radius;       cy = bottom - 1 - radius;
    } else if (x >= right - radius && y >= bottom - radius) {
      cx = right - 1 - radius;  cy = bottom - 1 - radius;
    }
    if (cx === null || cy === null) return true;
    const dx = x - cx;
    const dy = y - cy;
    return dx * dx + dy * dy <= radius * radius;
  }

  // Optional inner padlock silhouette for sizes >= 32.
  const drawLock = size >= 32;
  const lockBodyTop    = Math.floor(size * 0.50);
  const lockBodyBottom = Math.floor(size * 0.85);
  const lockBodyLeft   = Math.floor(size * 0.28);
  const lockBodyRight  = Math.floor(size * 0.72);
  const shackleInnerR  = Math.floor(size * 0.16);
  const shackleOuterR  = Math.floor(size * 0.24);
  const shackleCx      = Math.floor(size * 0.5);
  const shackleCy      = lockBodyTop;
  function inLock(x: number, y: number): boolean {
    if (!drawLock) return false;
    if (y >= lockBodyTop && y < lockBodyBottom &&
        x >= lockBodyLeft && x < lockBodyRight) return true;
    if (y < lockBodyTop) {
      const dx = x - shackleCx;
      const dy = y - shackleCy;
      const r2 = dx * dx + dy * dy;
      return r2 <= shackleOuterR * shackleOuterR &&
             r2 >= shackleInnerR * shackleInnerR;
    }
    return false;
  }

  const stride   = size * 4;
  const rowBytes = 1 + stride;
  const pixels   = Buffer.alloc(rowBytes * size);
  const lockColor = [0x14, 0x14, 0x1A, 0xFF]; // dark navy carve-out

  for (let y = 0; y < size; y++) {
    const rs = y * rowBytes;
    pixels[rs] = 0;
    for (let x = 0; x < size; x++) {
      const i = rs + 1 + x * 4;
      if (!inside(x, y)) {
        pixels[i] = 0; pixels[i+1] = 0; pixels[i+2] = 0; pixels[i+3] = 0;
        continue;
      }
      const carve = inLock(x, y);
      const c = carve ? lockColor : fg;
      pixels[i]     = c[0]!;
      pixels[i + 1] = c[1]!;
      pixels[i + 2] = c[2]!;
      pixels[i + 3] = c[3]!;
    }
  }

  return Buffer.concat([
    sig,
    chunk("IHDR", ihdr),
    chunk("IDAT", deflateSync(pixels)),
    chunk("IEND", Buffer.alloc(0)),
  ]);
}

mkdirSync("extension/icons", { recursive: true });
for (const size of [16, 32, 48, 128]) {
  const png = generateIcon(size);
  writeFileSync(`extension/icons/icon-${size}.png`, png);
  console.log(`wrote extension/icons/icon-${size}.png (${png.length} bytes)`);
}
