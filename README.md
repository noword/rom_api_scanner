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

1. Scan this binary file for functions and save the output to a sym file.
   
   `RomScanner nds.zdb code.bin > XXX.sym`

# Generate database
1. Put static libs to directory xxx_v1, xxx_v2 ...
2. Collect information in static libs, generate xxx_v1.txt, xxx_v2.txt 
   
   `collect.py xxx_v1`
   
   `collect.py xxx_v2`

3. Compile patterns, get database
   
   `GenDB xxx.zdb xxx_v1.txt xxxv2.txt -v [Default Base Offset]`
