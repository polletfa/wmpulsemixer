# wmpulsemixer

`wmpulsemixer` is a sleek volume control dockapp designed specifically for the Window Maker desktop environment. 

It is a fork of the classic `wmsmixer` (which was a fork of `wmmixer`).
This version completely strips out the obsolete Linux Open Sound System (OSS) kernel module backend (`/dev/mixer`)
and replaces it with a native, user-space control layer for modern audio servers (PulseAudio and PipeWire).

## Requirements
To build and run `wmpulsemixer`, you need the standard X11 development libraries and the `libpulse` library:

```bash
# Debian / Ubuntu / Mint
sudo apt install build-essential x11proto-core-dev libx11-dev libxext-dev libxpm-dev libpulse-dev
```

## Compilation and Installation

```bash
make
sudo make install
```

The binary will be installed in `/usr/local` unless you specify another location:

```
sudo make install PREFIX=/usr
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