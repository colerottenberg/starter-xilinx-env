# ─────────────────────────────────────────────────────────────────────────────
# C++ Dev Container — Debian + Nix, CLI-only
#
# Design:
#   • Debian bookworm-slim as the base (small, well-understood Linux userland)
#   • Nix installed via Determinate Systems installer (flakes on, container-safe)
#   • flake.nix pre-realised into a profile → instant shell entry
#   • Neovim included via Nix for in-container editing
#   • Your source tree mounts at /workspace via docker compose
# ─────────────────────────────────────────────────────────────────────────────
FROM debian:bookworm-slim

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

# ── System baseline ───────────────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
      bash \
      zsh \
      ca-certificates \
      curl \
      git \
      locales \
      procps \
      sudo \
      xz-utils \
    && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8

# ── Non-root user ─────────────────────────────────────────────────────────────
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd  --uid $USER_UID --gid $USER_GID -m -s /bin/zsh $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER $USERNAME
WORKDIR /home/$USERNAME

# ── Nix (Determinate Systems installer — flakes on by default, container-friendly) ──
RUN curl -fsSL https://install.determinate.systems/nix | sh -s -- install linux \
      --init none \
      --no-confirm

ENV PATH=/nix/var/nix/profiles/default/bin:/home/$USERNAME/.nix-profile/bin:$PATH

# ── Copy flake, resolve & pre-build the devshell ─────────────────────────────
# We copy only flake.nix (+ lock if present) — not your whole source tree.
# Source mounts in at runtime via docker compose volume.
COPY --chown=$USERNAME:$USERNAME flake.nix  /home/$USERNAME/.devenv/flake.nix
COPY --chown=$USERNAME:$USERNAME flake.lock* /home/$USERNAME/.devenv/

WORKDIR /home/$USERNAME/.devenv

RUN sudo /nix/var/nix/profiles/default/bin/nix-daemon &>/dev/null & \
    sleep 2 \
    && . /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh \
    && nix flake update \
    && nix develop --profile /home/$USERNAME/.devenv-profile --command true

# ── Shell init ────────────────────────────────────────────────────────────────
RUN cat >> /home/$USERNAME/.zshrc <<'EOF'

# ── Nix (Determinate Systems) ─────────────────────────────────────────────────
if [[ -f /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh ]]; then
  . /nix/var/nix/profiles/default/etc/profile.d/nix-daemon.sh
fi
export PATH="/nix/var/nix/profiles/default/bin:$PATH"
EOF

# ── Workspace ─────────────────────────────────────────────────────────────────
RUN sudo mkdir -p /workspace && sudo chown $USER_UID:$USER_GID /workspace
WORKDIR /workspace

COPY --chown=$USERNAME:$USERNAME entrypoint.sh /home/$USERNAME/entrypoint.sh
RUN chmod +x /home/$USERNAME/entrypoint.sh

ENTRYPOINT ["/home/dev/entrypoint.sh"]
CMD ["zsh"]
