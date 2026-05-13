// Mints a HackPass evaluation license blob for testing.
// Writes the binary to stdout (redirect to a file). The same HMAC key is
// embedded in app/src/settings/EmbeddedLicense.cpp; tester finding the key
// via static analysis can use this tool to mint their own license.
//
// Usage:
//   bun run tools/generate-license.ts <user> <tier> <days-until-expiry> > eval_license.bin

import { createHmac } from "node:crypto";

const args = process.argv.slice(2);
if (args.length < 3) {
  console.error("usage: generate-license.ts <user> <tier> <days>");
  process.exit(1);
}

const user  = args[0]!;
const tier  = args[1]!;  // "free" | "pro"
const days  = Number(args[2]!);

const expiresMs = Date.now() + (days * 24 * 60 * 60 * 1000);
const payload   = { user, expires_ms: expiresMs, tier };
const json      = JSON.stringify(payload);

// Must match app/src/settings/EmbeddedLicense.cpp:kEmbeddedKey exactly.
const key = "hackpass-eval-license-hmac-key-do-not-use-for-real";
const tag = createHmac("sha256", key).update(json).digest();

const b64url = (b: Buffer | string) =>
  Buffer.from(b).toString("base64").replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");

const blob = `${b64url(json)}.${b64url(tag)}`;
process.stdout.write(blob);
