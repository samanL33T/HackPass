import { Hono } from "hono";

export function licenseRouter(): Hono {
  const r = new Hono();
  r.post("/validate", async (c) => {
    const body = await c.req.json().catch(() => null) as { license_key?: string } | null;
    if (!body || !body.license_key) {
      return c.json({ valid: false, reason: "missing license_key" }, 400);
    }
    // Demo behavior: any non-empty license key is accepted as "pro" expiring in 1 year.
    // Real product would verify against a signed license database.
    const oneYear = Date.now() + (365 * 24 * 60 * 60 * 1000);
    return c.json({ valid: true, tier: "pro", expires_ms: oneYear });
  });
  return r;
}
