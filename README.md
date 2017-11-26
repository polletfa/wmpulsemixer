# wmpulsemixer (v0.1.0)

`wmpulsemixer` is a sleek volume control dockapp designed specifically for the Window Maker desktop environment. 

It is a fork of the classic `wmsmixer` (which was a fork of `wmmixer`).
This version completely strips out the obsolete Linux Open Sound System (OSS) kernel module backend (`/dev/mixer`)
and replaces it with a native, user-space control layer for modern audio servers (PulseAudio and PipeWire) via `pamixer`.

## Requirements
To build and run `wmpulsemixer`, you need the standard X11 development libraries and the `pamixer` command-line utility:

```bash
# Debian / Ubuntu / Mint
sudo apt install build-essential x11proto-core-dev libx11-dev libxext-dev libxpm-dev pamixer
```

## Compilation and Installation
The project utilizes `Imake` to generate system-specific Makefiles. Build it by running:

```bash
xmkmf
make
sudo make install
```

## Usage
Launch the dockapp within your Window Maker startup routine or session:

```bash
wmpulsemixer
wmpulsemixer -w           # Launch as a Dock app
wmpulsemixer -boost 1.5   # Allow volume up to 150%
```

See `wmpulsemixer --help` for a full list of options.

## Authors & Credits
- **Fabien Pollet** (2026) — Creator of `wmpulsemixer` (PulseAudio/PipeWire modern rewrite).
- **Damian Kramer** (2001-2003) — Creator of `wmsmixer` (Scrollwheel engine optimization).
- **Sam Hawker** (1998) — Creator of the original `wmmixer` core architecture

## Links

- [wmsmixer](https://repo.or.cz/dockapps.git/tree/HEAD:/wmsmixer)
- [wmmixer](https://repo.or.cz/dockapps.git/tree/HEAD:/wmmixer)