import { Hono } from "hono";

import { config }           from "./config.ts";
import { ensureTls }        from "./crypto/tlsCert.ts";
import { DeviceRegistry }   from "./storage/DeviceRegistry.ts";
import { VaultStore }       from "./storage/VaultStore.ts";
import { PolicyStore }      from "./storage/PolicyStore.ts";
import { requestLog }       from "./middleware/requestLog.ts";
import { devicesRouter }    from "./routes/devices.ts";
import { authRouter }       from "./routes/auth.ts";
import { vaultsRouter }     from "./routes/vaults.ts";
import { policyRouter }     from "./routes/policy.ts";
import { messagesRouter }   from "./routes/messages.ts";
import { licenseRouter }    from "./routes/license.ts";

const tls      = ensureTls(config.dataDir, config.regenerateCert);
const registry = new DeviceRegistry(config.dataDir);
const vaults   = new VaultStore(config.dataDir);
const policies = new PolicyStore(config.dataDir);

const app = new Hono();
app.use("*", requestLog);

const api = new Hono();
api.route("/devices",  devicesRouter(registry));
api.route("/auth",     authRouter(registry));
api.route("/vaults",   vaultsRouter(vaults, policies));
api.route("/policy",   policyRouter(policies));
api.route("/messages", messagesRouter(policies));
api.route("/license",  licenseRouter());
app.route("/api/v1",   api);

app.get("/health", (c) => c.json({ ok: true }));

// Loopback-only. The backend is the local companion to HackPass.exe on the
// same machine; nothing else has business reaching it.
Bun.serve({
  hostname: "127.0.0.1",
  port:     config.port,
  tls:      { cert: tls.cert, key: tls.key },
  fetch:    app.fetch,
});

console.log(`HackPass backend listening on https://127.0.0.1:${config.port}`);
