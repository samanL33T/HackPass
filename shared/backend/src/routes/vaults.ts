import { Hono } from "hono";
import type { VaultStore }   from "../storage/VaultStore.ts";
import type { PolicyStore }  from "../storage/PolicyStore.ts";

export function vaultsRouter(vaults: VaultStore, policies: PolicyStore): Hono {
  const r = new Hono();

  r.get("/:vaultId", (c) => {
    const id  = c.req.param("vaultId");
    const rec = vaults.get(id);
    return c.json({
      vault_id:      id,
      vault:         rec?.blob   ?? "",
      version:       rec?.version ?? 0,
      last_modified: rec?.updated ?? 0,
      server_flags:  policies.get(),
    });
  });

  r.put("/:vaultId", async (c) => {
    const id   = c.req.param("vaultId");
    const body = await c.req.json().catch(() => null) as { vault?: string; version?: number } | null;
    if (!body || typeof body.vault !== "string") {
      return c.json({ error: "missing vault" }, 400);
    }
    const current = vaults.get(id);
    const expected = body.version ?? 0;
    if (current && current.version !== expected) {
      return c.json({ error: "version conflict", server_version: current.version }, 409);
    }
    const rec = vaults.put(id, body.vault, (current?.version ?? 0) + 1);
    return c.json({ vault_id: id, version: rec.version, last_modified: rec.updated });
  });

  return r;
}
