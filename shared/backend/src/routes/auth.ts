import { Hono } from "hono";
import type { DeviceRegistry } from "../storage/DeviceRegistry.ts";

export function authRouter(registry: DeviceRegistry): Hono {
  const r = new Hono();
  r.post("/login", async (c) => {
    const body = await c.req.json().catch(() => null) as { device_id?: string; device_token?: string } | null;
    if (!body || !body.device_id || !body.device_token) {
      return c.json({ error: "missing credentials" }, 400);
    }
    const sessionToken = registry.authenticate(body.device_id, body.device_token);
    if (!sessionToken) {
      return c.json({ error: "invalid credentials" }, 401);
    }
    return c.json({ session_token: sessionToken });
  });
  return r;
}
