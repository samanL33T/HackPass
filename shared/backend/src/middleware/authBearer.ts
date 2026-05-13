import type { Context, Next } from "hono";
import type { DeviceRegistry } from "../storage/DeviceRegistry.ts";

export function authBearer(registry: DeviceRegistry) {
  return async (c: Context, next: Next) => {
    const header = c.req.header("Authorization") ?? "";
    if (!header.startsWith("Bearer ")) {
      return c.json({ error: "missing bearer" }, 401);
    }
    const token = header.slice("Bearer ".length).trim();
    const dev = registry.verifySession(token);
    if (!dev) {
      return c.json({ error: "invalid session" }, 401);
    }
    c.set("device", dev);
    await next();
  };
}
