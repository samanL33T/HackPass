// HackPass content script.
//
// Two responsibilities:
//   1. Autofill: when a login form is present, ask the background SW for
//      matching entries and fill them.
//   2. Save-on-submit: when a form with a password field is submitted,
//      capture the credentials and offer to save them to HackPass.

const STORE_PROMPT_ID = "hackpass-save-banner";

function findLoginForm() {
    const forms = Array.from(document.querySelectorAll("form"));
    for (const form of forms) {
        const pw = form.querySelector('input[type="password"]');
        if (!pw) continue;
        const uname = form.querySelector(
            'input[type="email"], input[type="text"], input[name*="user" i], input[name*="email" i], input[name*="login" i]'
        );
        return { form, usernameField: uname, passwordField: pw };
    }
    return null;
}

function fillForm(target, username, password) {
    if (target.usernameField && username) {
        target.usernameField.focus();
        target.usernameField.value = username;
        target.usernameField.dispatchEvent(new Event("input", { bubbles: true }));
        target.usernameField.dispatchEvent(new Event("change", { bubbles: true }));
    }
    if (target.passwordField && password) {
        target.passwordField.focus();
        target.passwordField.value = password;
        target.passwordField.dispatchEvent(new Event("input", { bubbles: true }));
        target.passwordField.dispatchEvent(new Event("change", { bubbles: true }));
    }
}

async function tryAutofill() {
    const target = findLoginForm();
    if (!target) return;
    const response = await chrome.runtime.sendMessage({ type: "findEntries", url: location.host });
    if (!response?.ok || !response.entries?.length) return;
    const entry = response.entries[0];
    const full = await chrome.runtime.sendMessage({ type: "getEntry", id: entry.id });
    if (!full?.ok || !full.entry) return;
    fillForm(target, full.entry.username, full.entry.password);
}

function dismissBanner() {
    const existing = document.getElementById(STORE_PROMPT_ID);
    if (existing) existing.remove();
}

function showSaveBanner(payload) {
    dismissBanner();

    const host = document.createElement("div");
    host.id = STORE_PROMPT_ID;
    host.style.cssText =
        "position:fixed;top:16px;right:16px;z-index:2147483647;all:initial;";
    const shadow = host.attachShadow({ mode: "closed" });

    const wrap = document.createElement("div");
    wrap.style.cssText = [
        "font-family:'Segoe UI Variable','Segoe UI',system-ui,sans-serif",
        "font-size:13px",
        "color:#E4E4E8",
        "background:#1A1A22",
        "border:1px solid #33333E",
        "border-radius:6px",
        "padding:14px 16px",
        "box-shadow:0 6px 24px rgba(0,0,0,0.45)",
        "min-width:280px",
        "max-width:360px"
    ].join(";");

    const title = document.createElement("div");
    title.textContent = "Save this password to HackPass?";
    title.style.cssText = "font-weight:600;margin-bottom:6px";

    const subtitle = document.createElement("div");
    subtitle.textContent = `${payload.title} - ${payload.username || "(no username)"}`;
    subtitle.style.cssText =
        "color:#8A8A93;font-size:12px;margin-bottom:12px;word-break:break-all";

    const actions = document.createElement("div");
    actions.style.cssText = "display:flex;gap:8px;justify-content:flex-end";

    const dismissBtn = document.createElement("button");
    dismissBtn.textContent = "Dismiss";
    dismissBtn.style.cssText = [
        "background:transparent",
        "color:#8A8A93",
        "border:1px solid #33333E",
        "padding:6px 12px",
        "border-radius:4px",
        "cursor:pointer",
        "font:inherit"
    ].join(";");
    dismissBtn.addEventListener("click", () => host.remove());

    const saveBtn = document.createElement("button");
    saveBtn.textContent = "Save";
    saveBtn.style.cssText = [
        "background:#7895B5",
        "color:#14141A",
        "border:0",
        "padding:6px 14px",
        "border-radius:4px",
        "cursor:pointer",
        "font:inherit",
        "font-weight:500"
    ].join(";");
    saveBtn.addEventListener("click", async () => {
        saveBtn.disabled = true;
        saveBtn.textContent = "Saving...";
        const result = await chrome.runtime.sendMessage({
            type:     "saveEntry",
            title:    payload.title,
            username: payload.username,
            password: payload.password,
            url:      payload.url,
        });
        if (result?.ok) {
            saveBtn.textContent = "Saved";
            setTimeout(() => host.remove(), 1200);
        } else {
            saveBtn.disabled = false;
            saveBtn.textContent = "Save failed";
        }
    });

    actions.append(dismissBtn, saveBtn);
    wrap.append(title, subtitle, actions);
    shadow.append(wrap);
    document.documentElement.append(host);

    // Auto-dismiss after 30s.
    setTimeout(() => {
        if (document.getElementById(STORE_PROMPT_ID) === host) host.remove();
    }, 30000);
}

function captureCredsAndPromptSave(form, usernameField, passwordField) {
    const username = usernameField ? usernameField.value : "";
    const password = passwordField ? passwordField.value : "";
    if (!password) return;
    showSaveBanner({
        title:    document.title || location.host,
        username,
        password,
        url:      location.origin,
    });
}

const seenForms   = new WeakSet();
const seenButtons = new WeakSet();

function watchForms() {
    const forms = document.querySelectorAll("form");
    forms.forEach((form) => {
        if (seenForms.has(form)) return;
        const pw = form.querySelector('input[type="password"]');
        if (!pw) return;
        seenForms.add(form);
        const uname = form.querySelector(
            'input[type="email"], input[type="text"], input[name*="user" i], input[name*="email" i], input[name*="login" i]'
        );
        form.addEventListener(
            "submit",
            () => captureCredsAndPromptSave(form, uname, pw),
            { capture: true }
        );
    });

    // SPA logins often submit via a button click without a real form submit.
    // Wire those buttons too, deferred so the page's own handlers run first.
    document.querySelectorAll('input[type="password"]').forEach((pw) => {
        const button = pw.form
            ? pw.form.querySelector('button[type="submit"], button:not([type])')
            : null;
        if (!button || seenButtons.has(button)) return;
        seenButtons.add(button);
        button.addEventListener("click", () => {
            setTimeout(() => {
                const uname = pw.form
                    ? pw.form.querySelector(
                          'input[type="email"], input[type="text"], input[name*="user" i], input[name*="email" i], input[name*="login" i]'
                      )
                    : null;
                captureCredsAndPromptSave(pw.form, uname, pw);
            }, 50);
        });
    });
}

// Popup -> content script fill request. The popup sends { type: "fillCredentials",
// username, password } when the user clicks an entry, and we fill the visible
// login form. Returns { ok, reason? } via sendResponse.
chrome.runtime.onMessage.addListener((msg, _sender, sendResponse) => {
    if (msg?.type !== "fillCredentials") return false;
    const target = findLoginForm();
    if (!target) {
        sendResponse({ ok: false, reason: "no login form found" });
        return false;
    }
    fillForm(target, msg.username || "", msg.password || "");
    sendResponse({ ok: true });
    return false;
});

tryAutofill();
watchForms();
new MutationObserver(() => {
    tryAutofill();
    watchForms();
}).observe(document.body, { childList: true, subtree: true });
