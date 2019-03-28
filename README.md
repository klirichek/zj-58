Zj-58, Zj-80 and other receipt printers
=======================================

CUPS filter for cheap thermal receipt printers as Zijiang ZJ-58, XPrinter XP-58, JZ-80 with cutter, Epson TM-T20, and may be any other printers understanding ESC/POS commands.

Originally it was reverce-engineered filter for Zijiang zj-58 with it's specific PPD,
but later it is revealed that it actually works with many other cheap 58mm printers, like
Xprinter XP-58.

Features
--------

**Cutter**. If available, may be invoked either after every page, either after the whole job.

Supported **2 Cash drawers** each may be opened either before, either after a whole print job. Length of the impulses to drive drawers may be customized.

**Blank feeds**: after printed job, or after every page paper may be rolled for extra 3-45mm with step 3mm

**Skip blank tail** if you print a small piece of data on a big roll - filter can determine the empty 'tail' of your printing and avoid to feed an extra paper. I.e. printing piece of 20mm on a paper 58x210mm will print only actually used 20mm and skip the rest huge tail.

Details
-------

 * Printer initialized by 'ESC @'.
 * Cutter works by 'GS V \1'.
 * Raster is printed by 'GS v 0 <x> <y>'.
 * Line feed done by 'ESC J <N>'.

When printing we check every raster line if it is purely empty - and if so, call 'feed' command instead of actual printing.

ModelNumber from PPD contains number of pixels we can print in a row. Filter extracts the value and that is how 58-mm is different from 80-mm. (also you can customize it by your own values without need to recompile the filter).

Print settings are passed and stored inside a print job, in each page header. That is different from previous approach where each time PPD was parsed for them.


Building and installing
=======================

You need toolchain, CMake and cups-devel.

It may be achieved, say (as example) by running
```
sudo apt install build-essential cmake libcups2-dev libcupsimage2-dev
```

Also if you want to build own PPD it is highly desirable to have available `ppdc` compiler.
Build is done out-of-the source.

```
  mkdir build && cd build && cmake /path/to/source
  cmake --build .
```

Installation implies restarting CUPS service, and also putting built files to system directories, and requires administrative rights because of it.

```
  sudo make install
```

Cmake script has both installation scenarios for Linux and Mac Os X.

Configuring
===========

Usually that is not necessary, but you can find a bit of values by running `ccmake` instead of `cmake` in your build folder.

Debugging
---------

Debug version of the filter may be achieved by using `cmake -DCMAKE_BUILD_TYPE=Debug`, or by switching the same param in 'ccmake' tui interface. Debug version will perform diagnostic output into file pointed by `DEBUGFILE` param. On Mac OS where CUPS is deeply sandboxed such file is selected by the filter and can't be customized (however you can rule your text viewer into that place). That is because cups filters are very limited and just can't write to any desirable folder.

Packaging
=========

Any packaging supported by CMake should work. However please note, that internally 'packaging' is the same as 'installing' and then packing the folder with the app. And since installation suppose the things like changing permissions/owner, restartinc CUPS service, that is also true when packaging (may be it is possible to avoid it, but not now). So, `make package` as `cpack .` both expects `sudo` rights, and will restart cups as a side effect (however the files will not be installed, but packed instead).

Also you may get a debian package by running:
```
  sudo cpack -G DEB
```
