import { Hono } from "hono";
import type { PolicyStore } from "../storage/PolicyStore.ts";

export function policyRouter(policies: PolicyStore): Hono {
  const r = new Hono();
  r.get("/", (c) => c.json({ server_flags: policies.get() }));
  return r;
}
