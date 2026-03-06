# Agent Build Environment Setup (pokeemerald-soullink)

This document lists what an automated coding agent needs in the execution environment to run `make`, validate changes, and avoid the failures seen in fresh containers.

## Why `make` fails in a fresh environment

The first failing symptom in this environment was:

- `arm-none-eabi-gcc: command not found`

That means the ARM cross-toolchain is missing from `PATH`.

## Minimum system dependencies

For Ubuntu/Debian-based containers, preinstall:

```bash
apt-get update
apt-get install -y \
  build-essential \
  binutils-arm-none-eabi \
  gcc-arm-none-eabi \
  libnewlib-arm-none-eabi \
  git \
  libpng-dev \
  python3
```

These are aligned with this repo's Linux install docs and are sufficient for a working `make` build in this environment.

## Optional but recommended

- `ccache` (faster incremental builds)
- `pkg-config` (general tooling quality-of-life)
- `python3-venv` (if your agent installs python tooling)

## Environment variables and PATH expectations

Most agent runs work without extra variables if the distro packages above are installed.

If using a custom toolchain location instead of distro packages:

- Ensure binaries like `arm-none-eabi-gcc` and `arm-none-eabi-cpp` are on `PATH`, **or**
- Set:

```bash
export TOOLCHAIN=/path/to/toolchain/root
```

Where `$TOOLCHAIN/bin/arm-none-eabi-gcc` exists.

## Quick preflight checks for agents

Run these before coding to fail fast:

```bash
command -v make
command -v python3
command -v arm-none-eabi-gcc
arm-none-eabi-gcc --version | head -n 1
make -n | head -n 20
```

## Build validation workflow agents should run

```bash
make -j"$(nproc)"
```

Then verify artifacts exist:

```bash
test -f pokeemerald.gba && test -f pokeemerald.elf
```

## Notes for CI/container image maintainers

To make agents succeed right away, bake dependencies into the base image so each run does **not** need to install ~500MB+ of toolchain packages.

A reliable baseline is:

1. Include all packages from "Minimum system dependencies" above.
2. Ensure `/usr/bin` is present in `PATH`.
3. Give enough disk space for dependencies + build artifacts (several GB recommended).
4. Give enough CPU/RAM for large asset processing done by this project.

