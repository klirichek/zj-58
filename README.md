ZJ-58 (and ZJ-80)
=================

CUPS filter for thermal printer Zjiang ZJ-58 and JZ-80 with cutter.
These drivers are also work with chinese XPrinters models XP-58, XP-80, XP-N160II, etc. And I think it will work with almost any ESC/POS printers.

Preamble
--------

This repository is based on https://github.com/klirichek/zj-58 which is (at least for now) not maintained.

The linux driver provided on Zjiang site unfortunately doesn't work.
* First, it is 32-bit binary (and so, on x64 system need some x86 libs to be installed).
* Second, it's PPD is not correct - it just doesn't show any advanced settings because of it.
* Finally, raster filter just crashes on any try to run it!

But even if you run it, you'll see that it's working with printer is not optimal: for example, if it sees a blank line, 
it will send the blank raster to print (instead of just use 'feeding' command, which is definitely faster!)

This is the solution: PPD is fixed and works.
Filter is provided as src (you can found a list of packages need to be installed in order to build it in the header of source).
Also, printing of blank lines is optimized.

Installation
------------

1) Clone or download
2) Run sudo ./install
3) Go on installing printer with CUPS at http://localhost:631/

Writing own PPD
---------------

You may write own PPD file for your printer to use any media size you want. Just open
zj.drv file with any text editor and copy-paste printer section embraced with curly brackets.
Then change your printer model and correct media sizes at the end of section.

At most all chinese thermo printers has 203DPI resolution, so you need to recalculate media sizes
into points, i.e. 229points=80mm, 164points=58mm, etc.

Also you may change resolution value for your printer and recalculate all dimensions of your media.