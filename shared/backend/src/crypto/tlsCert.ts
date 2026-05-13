import { mkdirSync, existsSync, readFileSync } from "node:fs";
import { dirname, join }                       from "node:path";
import { execSync }                            from "node:child_process";

export interface TlsMaterial {
  cert: string;
  key:  string;
}

// Prefer the openssl staged next to the server binary, else PATH.
function opensslCmd(): string {
  const exeDir = dirname(process.execPath);
  for (const name of ["openssl.exe", "openssl"]) {
    const candidate = join(exeDir, name);
    if (existsSync(candidate)) return `"${candidate}"`;
  }
  return "openssl";
}

// Generates a self-signed cert if one doesn't already exist in dataDir/certs.
// Persists across restarts so HackPass's TOFU pin survives backend restarts
// and installer reinstalls (since the cert lives outside the install dir).
export function ensureTls(dataDir: string, regenerate: boolean): TlsMaterial {
  const certDir  = join(dataDir, "certs");
  const certPath = join(certDir, "server.crt");
  const keyPath  = join(certDir, "server.key");

  mkdirSync(certDir, { recursive: true });

  if (!regenerate && existsSync(certPath) && existsSync(keyPath)) {
    return { cert: readFileSync(certPath, "utf8"), key: readFileSync(keyPath, "utf8") };
  }

  // ECDSA P-256 self-signed cert valid for 5 years.
  execSync([
    `${opensslCmd()} req`,
    "-x509",
    "-newkey ec",
    "-pkeyopt ec_paramgen_curve:prime256v1",
    "-nodes",
    `-keyout "${keyPath}"`,
    `-out "${certPath}"`,
    "-days 1825",
    "-subj \"/CN=hackpass-backend\"",
    "-addext \"subjectAltName=DNS:localhost,DNS:hackpass-backend,IP:127.0.0.1\"",
  ].join(" "), { stdio: "ignore" });

  return { cert: readFileSync(certPath, "utf8"), key: readFileSync(keyPath, "utf8") };
}
