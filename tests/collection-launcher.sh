#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
scratch=$(mktemp -d)
trap 'rm -rf -- "$scratch"' EXIT
cat > "$scratch/player" <<'PLAYER'
#!/bin/bash
printf '%s %s\n' "${OMADROP_ASCII-unset}" "$#"
PLAYER
chmod +x "$scratch/player"
export OMADROP_LIVE_EXECUTABLE="$scratch/player" XDG_CONFIG_HOME="$scratch/config"
unset OMADROP_ASCII
runner="$root/experiments/projectm-ascii/run-collection.sh"
[[ $("$runner") == '0 21' ]]
mkdir -p "$XDG_CONFIG_HOME/omadrop"
printf 'version=4\ndisplay=single\n' > "$XDG_CONFIG_HOME/omadrop/preferences.conf"
[[ $("$runner") == '0 21' ]]
printf 'ascii=1\n' >> "$XDG_CONFIG_HOME/omadrop/preferences.conf"
[[ $("$runner") == 'unset 21' ]]
[[ $(OMADROP_ASCII=0 "$runner") == '0 21' ]]
if "$runner" --invalid >/dev/null 2>&1; then exit 1; fi
echo 'Collection defaults, saved ASCII and explicit overrides passed'
