const tokenInput = document.getElementById("token");
const status     = document.getElementById("status");

async function load() {
  const r = await chrome.storage.local.get(["hackpassToken"]);
  tokenInput.value = r.hackpassToken || "";
}

async function save() {
  await chrome.storage.local.set({ hackpassToken: tokenInput.value.trim() });
  await chrome.runtime.sendMessage({ type: "reconnect" });
  status.textContent = "Saved and reconnecting...";
  setTimeout(() => { status.textContent = ""; }, 2000);
}

async function test() {
  const r = await chrome.runtime.sendMessage({ type: "lockState" });
  status.textContent = r?.ok
      ? "Connected. Vault state: " + r.state
      : "Failed: " + (r?.error?.message ?? "unknown");
}

document.getElementById("save").addEventListener("click", save);
document.getElementById("test").addEventListener("click", test);
load();
