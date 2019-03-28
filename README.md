CUPS driver for thermal receipt printers (was: zj-58)
=====================================================

CUPS filter for thermal receipt printers for rolls 58 and 80mm.

Originally it was filter for Zijiang zj-58 with specific PPD,
but later it is revealed that it actually works with many other cheap 58mm printers, like
Xprinter XP-58, etc. Also support for 80mm printers added.

Printing is performed from cups-raster using ESC/POS sequence 'GS v 0 <x> <y>'.
Empty (zero) lines feeded as 'feed' command (to avoid send empty raster).

Also 2 Cash Drawers supported, with tunable impulse length for 'on' and 'off.'

Build
=====

You need toolchain, CMake and cups-devel.

It may be achieved, say (as example) by running
```
sudo apt install build-essential cmake libcups2-dev libcupsimage2-dev
```

After it the filter could be built by the sequence of commands:

```
  mkdir build && cd build && cmake ..
  make
```


Installation
============

Need administrative rights!

```
  sudo make install
```

'Sudo' is necessary to stop/restart cups service before operation, and also to place files with necessary rights.


Debian packaging
================

For simplicity done with the same script as installation, so also need adminstrative rights.
```
  sudo cpack -G DEB
```

That will stop/start cups as a side effect, however that is not critical.