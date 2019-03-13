#!/bin/bash

echo "Compiling..."
make

echo "Stopping CUPS"
/etc/init.d/cups stop

echo "Compile drivers"
ppdc *.drv

echo "Copying files"
cp rastertozj /usr/lib/cups/filter/
mkdir -p /usr/share/cups/model/zjiang
cp ppd/zj58.ppd /usr/share/cups/model/zjiang/
cp ppd/zj80.ppd /usr/share/cups/model/zjiang/
cd /usr/lib/cups/filter
chmod 755 rastertozj
chown root:root rastertozj
cd -

echo "Starting CUPS"
/etc/init.d/cups start
