# Usage

```
Usage: RomScanner  [-v] DB [ROM] [-o <int>] [-i <int>] [--help]
Options are:
  DB                        Database file
  ROM                       Rom file for scanning. If not provided, display the
                            database information
  -o, --voffset=<int>       Virtual offset. If not provided, the default value
                            in the database will be used
  -i, --index=<int>         Choose which database for scanning
  -v, --version             Show the version information
      --help                Print this help and exit
```

# Guide

1. Get code binary from rom.
   
   - You can extrct code binary with [ndstool](https://github.com/devkitPro/ndstool):
  
      `ndstool [XXX.nds] -x -9 code.bin`
  
    - Or dump it with [No$GBA](https://www.nogba.com/):
  ![](pic/no%24gba.jpg)

      After the rom is loaded, dump 4M bytes from address 0x02000000. (recommend this way)

2. Scan functions in this code binary and save the output to a sym file.
   
   `RomScanner nds.zdb code.bin > XXX.sym`
