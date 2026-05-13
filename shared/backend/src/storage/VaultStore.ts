import { existsSync, readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { join }                                              from "node:path";

interface VaultRecord {
  vaultId:  string;
  version:  number;
  blob:     string;  // base64 ciphertext as stored on disk
  updated:  number;
}

export class VaultStore {
  private readonly dataDir: string;

  constructor(dataDir: string) {
    this.dataDir = join(dataDir, "vaults");
    mkdirSync(this.dataDir, { recursive: true });
  }

  get(vaultId: string): VaultRecord | null {
    const path = this.pathFor(vaultId);
    if (!existsSync(path)) return null;
    try {
      return JSON.parse(readFileSync(path, "utf8")) as VaultRecord;
    } catch (_e) {
      return null;
    }
  }

  put(vaultId: string, blobBase64: string, version: number): VaultRecord {
    const rec: VaultRecord = {
      vaultId,
      version,
      blob:    blobBase64,
      updated: Date.now(),
    };
    writeFileSync(this.pathFor(vaultId), JSON.stringify(rec));
    return rec;
  }

  private pathFor(vaultId: string): string {
    const safe = vaultId.replace(/[^a-z0-9_\-]/gi, "_");
    return join(this.dataDir, `${safe}.json`);
  }
}
