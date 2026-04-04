#!/usr/bin/env zsh
# ─────────────────────────────────────────────────────────────────────────────
# Entrypoint for the C++ dev container
#
# Interactive:    docker compose run --rm dev
#                   → drops into `nix develop` shell at /workspace
#
# Non-interactive: docker compose run --rm dev just build
#                   → runs `just build` inside the devshell and exits
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

NIX_SH="/nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh"
DEVENV_DIR="$HOME/.devenv"

[[ -f "$NIX_SH" ]] && source "$NIX_SH"

export PATH="/nix/var/nix/profiles/default/bin:$PATH"

# Start the Nix daemon (multi-user install needs it running)
sudo /nix/var/nix/profiles/default/bin/nix-daemon &>/dev/null &
sleep 1

# Always operate from the mounted workspace
cd /workspace

if [[ $# -eq 0 || ( $# -eq 1 && "$1" == "zsh" ) ]]; then
  # Interactive — enter the devshell
  exec nix develop "$DEVENV_DIR#default" --command zsh
else
  # Non-interactive — run the given command inside the devshell
  exec nix develop "$DEVENV_DIR#default" --command "$@"
fi
