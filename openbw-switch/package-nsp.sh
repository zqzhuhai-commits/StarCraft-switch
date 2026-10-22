#!/usr/bin/env bash
# Build a native application NSP containing the engine NSO (not an NRO forwarder).
set -euo pipefail
cd "$(dirname "$0")"
: "${KEYS_FILE:?Set KEYS_FILE to your local prod.keys path}"
: "${HACBREWPACK:?Set HACBREWPACK to your locally compiled hacbrewpack executable}"
KEYS_FILE=$(realpath "$KEYS_FILE")
HACBREWPACK=$(realpath "$HACBREWPACK")
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export PATH="$DEVKITPRO/tools/bin:$PATH"
# Old hacBrewPack parsers reject newer, unrelated extended keys.
# Pass only the two entries needed for first-generation application NCAs.
umask 077
KEY_SUBSET=$(mktemp)
trap 'rm -f "$KEY_SUBSET"' EXIT
python3 - "$KEYS_FILE" "$KEY_SUBSET" <<'PYKEYS'
import re, sys
from pathlib import Path
found = {}
for line in Path(sys.argv[1]).read_text().splitlines():
    match = re.match(r"\s*(header_key|key_area_key_application_00)\s*=\s*([0-9a-fA-F]+)\s*$", line)
    if match:
        found[match[1]] = match[2]
for name, length in [("header_key", 64), ("key_area_key_application_00", 32)]:
    if len(found.get(name, "")) != length:
        raise SystemExit("Required local packaging key is missing or invalid: " + name)
Path(sys.argv[2]).write_text("".join(name + " = " + value + "\n" for name, value in found.items()))
PYKEYS
test -s build/StarCraft-switch.nso
test -s build/StarCraft-switch.nacp
rm -rf build/package
mkdir -p build/package/exefs build/package/control build/package/output
cp build/StarCraft-switch.nso build/package/exefs/main
npdmtool npdm.json build/package/exefs/main.npdm
cp build/StarCraft-switch.nacp build/package/control/control.nacp
cp icon.jpg build/package/control/icon_AmericanEnglish.dat
ROMFS_ARGS=(--noromfs)
if [[ -n "${ROMFS_DIR:-}" ]]; then
  ROMFS_ARGS=(--romfsdir "$(realpath "$ROMFS_DIR")")
fi
"$HACBREWPACK" -k "$KEY_SUBSET" --keygeneration 1 --titleid 0100534352460000 \
  --titlename "星际争霸" --titlepublisher "StarCraft Switch Community" \
  --exefsdir build/package/exefs --controldir build/package/control \
  --nspdir build/package/output --tempdir build/package/temp \
  --ncadir build/package/nca --backupdir build/package/backup \
  "${ROMFS_ARGS[@]}" --nologo
cp build/package/output/0100534352460000.nsp build/StarCraft-Switch-0.3.0-skirmish-test.nsp
printf 'Created build/StarCraft-Switch-0.3.0-skirmish-test.nsp\n'
