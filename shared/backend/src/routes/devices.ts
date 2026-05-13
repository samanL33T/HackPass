import { Hono } from "hono";
import type { DeviceRegistry } from "../storage/DeviceRegistry.ts";

export function devicesRouter(registry: DeviceRegistry): Hono {
  const r = new Hono();
  r.post("/register", (c) => {
    const rec = registry.registerNew();
    return c.json({
      device_id:    rec.deviceId,
      device_token: rec.deviceToken,
    });
  });
  return r;
}
