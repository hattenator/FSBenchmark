This is a pretty simple benchmark for empty filesystems.  You can use it to check RAID layouts, block sizes, device-mapper settings, filesystem settings, and etc. have the performance you would expect from the hardware.  You can use it to side-by-side two different filesystems that should be comparable.

It has been a while since I've built this, but I think it's just

g++ -O -o dbFS dbFS.cpp

The flags are just a list of sizes for the block and file size you want to use:

writeBlock writes streamBlock streamCount offset dsync

e.g.

./dbFS 8192 100000 8388608 100 96636764160 dsync
