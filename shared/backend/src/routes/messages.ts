import { Hono } from "hono";
import type { PolicyStore } from "../storage/PolicyStore.ts";

export function messagesRouter(policies: PolicyStore): Hono {
  const r = new Hono();
  r.get("/", (c) => {
    const msg = policies.get().policy_message;
    return c.json({ messages: msg ? [msg] : [] });
  });
  return r;
}
