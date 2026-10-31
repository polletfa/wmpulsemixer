# Changelog

## 0.2.1 - 31 Oct 2026

- Added -m option to set the color of the "mute" symbol.
- Removed call to deprecated GTK method.
- Stick to GNU standard of using two dashes for long options (old options still supported)
- A little refactoring for more readability.

## 0.2.0 - 29 Oct 2026

- Added feature to change the default sink/source: right-click on the icon to open a menu of available devices.
- Added GTK for the menu
- Refactored the event loop using GTK and libpulse-mainloop-glib

## 0.1.4 - 25 May 2026

- Minor changes to the build system (no changes in code)

## 0.1.3 - 24 May 2026

- Minor changes (code quality)

## 0.1.2 - 24 May 2026

- Use libpulse instead of pamixer (much more performant)
- Add mute/unmute feature (click on the icon)

## 0.1.1 - 23 May 2026

- Minor fix

## 0.1.0 - 23 May 2026

- Initial version based on wmsmixer 0.5.1 and using pamixer
