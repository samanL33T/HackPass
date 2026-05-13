import { existsSync, readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { join, dirname }                                     from "node:path";
import { randomBytes }                                       from "node:crypto";
import { nanoid }                                            from "nanoid";

interface DeviceRecord {
  deviceId:     string;
  deviceToken:  string;
  registeredAt: number;
  sessionToken: string | null;
  sessionExpiresAt: number;
}

export class DeviceRegistry {
  private readonly filePath: string;
  private devices: Map<string, DeviceRecord> = new Map();

  constructor(dataDir: string) {
    this.filePath = join(dataDir, "devices.json");
    mkdirSync(dirname(this.filePath), { recursive: true });
    this.load();
  }

  registerNew(): DeviceRecord {
    const rec: DeviceRecord = {
      deviceId:    nanoid(),
      deviceToken: randomBytes(32).toString("hex"),
      registeredAt: Date.now(),
      sessionToken: null,
      sessionExpiresAt: 0,
    };
    this.devices.set(rec.deviceId, rec);
    this.persist();
    return rec;
  }

  authenticate(deviceId: string, deviceToken: string): string | null {
    const rec = this.devices.get(deviceId);
    if (!rec || rec.deviceToken !== deviceToken) {
      return null;
    }
    rec.sessionToken     = randomBytes(32).toString("hex");
    rec.sessionExpiresAt = Date.now() + (24 * 60 * 60 * 1000);  // 24h
    this.persist();
    return rec.sessionToken;
  }

  verifySession(sessionToken: string): DeviceRecord | null {
    for (const rec of this.devices.values()) {
      if (rec.sessionToken === sessionToken && rec.sessionExpiresAt > Date.now()) {
        return rec;
      }
    }
    return null;
  }

  private load(): void {
    if (!existsSync(this.filePath)) return;
    try {
      const raw = JSON.parse(readFileSync(this.filePath, "utf8")) as DeviceRecord[];
      for (const r of raw) this.devices.set(r.deviceId, r);
    } catch (_e) {
      // start fresh on parse error
    }
  }

  private persist(): void {
    const arr = Array.from(this.devices.values());
    writeFileSync(this.filePath, JSON.stringify(arr, null, 2));
  }
}
