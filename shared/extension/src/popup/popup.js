async function init() {
    const stateEl = document.getElementById("state");
    const emptyEl = document.getElementById("empty");
    const listEl  = document.getElementById("entries");

    document.getElementById("open-options").addEventListener("click", (e) => {
        e.preventDefault();
        chrome.runtime.openOptionsPage();
    });

    const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
    const host  = tab?.url ? new URL(tab.url).host : "";

    const response = await chrome.runtime.sendMessage({ type: "findEntries", url: host });
    if (!response?.ok) {
        stateEl.textContent = "disconnected";
        return;
    }
    stateEl.textContent = "connected";
    const entries = response.entries || [];
    if (entries.length === 0) {
        emptyEl.style.display = "block";
        return;
    }
    emptyEl.style.display = "none";

    for (const e of entries) {
        const li = document.createElement("li");
        const title = document.createElement("div");
        title.className   = "title";
        title.textContent = e.title || e.url;
        const username = document.createElement("div");
        username.className   = "username";
        username.textContent = e.username || e.url;
        li.appendChild(title);
        li.appendChild(username);

        li.addEventListener("click", async () => {
            const r = await chrome.runtime.sendMessage({ type: "getEntry", id: e.id });
            if (!r?.ok || !r.entry) return;

            // Ask the active tab's content script to fill the visible form.
            // Falls back to clipboard if no form is found (or content script
            // isn't injected, e.g. on chrome:// pages).
            const fillResp = await chrome.tabs.sendMessage(tab.id, {
                type:     "fillCredentials",
                username: r.entry.username,
                password: r.entry.password,
            }).catch(() => null);

            if (fillResp?.ok) {
                stateEl.textContent = "filled";
            } else {
                await navigator.clipboard.writeText(r.entry.password || "");
                stateEl.textContent = "copied (no form)";
            }
            setTimeout(() => { stateEl.textContent = "connected"; }, 1800);
        });

        listEl.appendChild(li);
    }
}

init();
