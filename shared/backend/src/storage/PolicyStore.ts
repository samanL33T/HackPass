import { existsSync, readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { join, dirname }                                     from "node:path";

export interface ServerFlags {
  premium_active:             boolean;
  feature_export_plaintext:   boolean;
  feature_legacy_kdf_allowed: boolean;
  force_relock_required:      boolean;
  policy_message:             string | null;
  auto_lock_minutes:          number;
  device_status:              "trusted" | "suspect" | "revoked";
}

export class PolicyStore {
  private readonly filePath: string;
  private flags: ServerFlags = {
    premium_active:             false,
    feature_export_plaintext:   false,
    feature_legacy_kdf_allowed: false,
    force_relock_required:      false,
    policy_message:             null,
    auto_lock_minutes:          5,
    device_status:              "trusted",
  };

  constructor(dataDir: string) {
    this.filePath = join(dataDir, "policy.json");
    mkdirSync(dirname(this.filePath), { recursive: true });
    this.load();
  }

  get(): ServerFlags { return { ...this.flags }; }

  set(patch: Partial<ServerFlags>): ServerFlags {
    this.flags = { ...this.flags, ...patch };
    this.persist();
    return this.get();
  }

  private load(): void {
    if (!existsSync(this.filePath)) {
      this.persist();
      return;
    }
    try {
      this.flags = { ...this.flags, ...JSON.parse(readFileSync(this.filePath, "utf8")) };
    } catch (_e) {
      // start fresh on parse error
    }
  }

  private persist(): void {
    writeFileSync(this.filePath, JSON.stringify(this.flags, null, 2));
  }
}
