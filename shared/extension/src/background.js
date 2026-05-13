// HackPass extension background service worker.
//
// Maintains a WebSocket to HackPass.exe on 127.0.0.1:8765, performs the token
// handshake with the value stored in chrome.storage.local, then dispatches
// JSON-RPC requests from content scripts and the popup.

const HACKPASS_URL  = "ws://127.0.0.1:8765";
const RECONNECT_MS  = 2000;
const HANDSHAKE_TAG = "[HackPass]";

let ws            = null;
let authed        = false;
let nextId        = 1;
const pending     = new Map();   // id -> { resolve, reject }
let token         = "";
let reconnectTimer = null;

async function loadToken() {
    try {
        const r = await chrome.storage.local.get(["hackpassToken"]);
        token   = r.hackpassToken || "";
    } catch (e) {
        console.warn(HANDSHAKE_TAG, "loadToken failed:", e);
        token = "";
    }
}

function rejectAllPending(reason) {
    for (const cb of pending.values()) {
        try { cb.reject(reason); } catch (_e) { /* ignore */ }
    }
    pending.clear();
}

function scheduleReconnect() {
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(() => {
        reconnectTimer = null;
        connect();
    }, RECONNECT_MS);
}

function connect() {
    if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) return;

    try {
        ws = new WebSocket(HACKPASS_URL);
    } catch (e) {
        console.warn(HANDSHAKE_TAG, "WebSocket constructor threw:", e);
        ws = null;
        scheduleReconnect();
        return;
    }

    ws.onopen = async () => {
        try {
            await loadToken();
            if (!token) {
                console.info(HANDSHAKE_TAG, "no token configured; extension idle until Options is set");
                return;
            }
            if (!ws || ws.readyState !== WebSocket.OPEN) {
                console.warn(HANDSHAKE_TAG, "socket closed before handshake send");
                return;
            }
            const reqId = nextId++;
            // Register the pending resolver BEFORE sending so we cannot lose a
            // fast-arriving response that beats the set().
            pending.set(reqId, {
                resolve: () => { authed = true;  console.info(HANDSHAKE_TAG, "handshake ok"); },
                reject:  (e) => { authed = false; console.warn(HANDSHAKE_TAG, "handshake rejected:", e); },
            });
            try {
                ws.send(JSON.stringify({
                    jsonrpc: "2.0",
                    id: reqId,
                    method: "handshake",
                    params: { token },
                }));
            } catch (e) {
                pending.delete(reqId);
                console.warn(HANDSHAKE_TAG, "ws.send failed:", e);
            }
        } catch (e) {
            console.error(HANDSHAKE_TAG, "onopen handler crashed:", e);
        }
    };

    ws.onmessage = (ev) => {
        let msg;
        try { msg = JSON.parse(ev.data); } catch (_e) { return; }
        if (typeof msg.id !== "number") return;
        const cb = pending.get(msg.id);
        pending.delete(msg.id);
        if (!cb) return;
        if ("error" in msg) cb.reject(msg.error);
        else                cb.resolve(msg.result);
    };

    ws.onclose = (ev) => {
        authed = false;
        ws     = null;
        rejectAllPending({ code: -1, message: `disconnected (code ${ev.code})` });
        scheduleReconnect();
    };

    ws.onerror = () => {
        // Suppress noisy default error logging; onclose handles the reconnect.
    };
}

function call(method, params) {
    return new Promise((resolve, reject) => {
        if (!ws || ws.readyState !== WebSocket.OPEN || !authed) {
            reject({ code: -1, message: "not connected" });
            return;
        }
        const id = nextId++;
        pending.set(id, { resolve, reject });
        try {
            ws.send(JSON.stringify({ jsonrpc: "2.0", id, method, params }));
        } catch (e) {
            pending.delete(id);
            reject({ code: -1, message: String(e) });
        }
    });
}

chrome.runtime.onMessage.addListener((msg, _sender, sendResponse) => {
    (async () => {
        try {
            switch (msg?.type) {
                case "findEntries": {
                    const r = await call("vault.findEntries", { url: msg.url || "" });
                    sendResponse({ ok: true, entries: r.entries || [] });
                    break;
                }
                case "getEntry": {
                    const r = await call("vault.getEntry", { id: msg.id });
                    sendResponse({ ok: true, entry: r });
                    break;
                }
                case "saveEntry": {
                    const r = await call("vault.saveEntry", {
                        title:    msg.title,
                        username: msg.username,
                        password: msg.password,
                        url:      msg.url,
                    });
                    sendResponse({ ok: true, id: r.id, updated: !!r.updated });
                    break;
                }
                case "lockState": {
                    const r = await call("vault.lockState", {});
                    sendResponse({ ok: true, state: r.state });
                    break;
                }
                case "reconnect": {
                    if (ws) try { ws.close(); } catch (_e) { /* ignore */ }
                    connect();
                    sendResponse({ ok: true });
                    break;
                }
                default:
                    sendResponse({ ok: false, error: "unknown type" });
            }
        } catch (err) {
            sendResponse({ ok: false, error: err });
        }
    })();
    return true;
});

chrome.storage.onChanged.addListener((changes) => {
    if (changes.hackpassToken) {
        if (ws) try { ws.close(); } catch (_e) { /* ignore */ }
        connect();
    }
});

connect();
