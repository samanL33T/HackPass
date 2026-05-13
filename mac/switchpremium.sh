#!/usr/bin/env bash
# Open backend policy.json in the default editor. Restart HackPass after saving.

set -euo pipefail

DATA_DIR="$HOME/Library/Application Support/HackPass/backend-data"
POLICY="$DATA_DIR/policy.json"

mkdir -p "$DATA_DIR"

if [[ ! -f "$POLICY" ]]; then
    cat > "$POLICY" <<'JSON'
{
  "premium_active":             false,
  "feature_export_plaintext":   false,
  "feature_legacy_kdf_allowed": false,
  "force_relock_required":      false,
  "policy_message":             null,
  "auto_lock_minutes":          5,
  "device_status":              "trusted"
}
JSON
fi

open -W "$POLICY"
