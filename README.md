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
   
   ### For NDS/DSi
   - Extrct code binary with [ndstool](https://github.com/devkitPro/ndstool):
  
      `ndstool [XXX.nds] -x -9 code.bin`
  
    - Or dump it with [No$GBA](https://www.nogba.com/) (recommend this way):
  
      ![](pic/no%24gba.jpg)

      After the rom is loaded, dump 4M bytes from address 0x02000000. 
   
   ### For 3DS
   - [Decrypt the CIA file](https://gbatemp.net/threads/batch-cia-3ds-decryptor-a-simple-batch-file-to-decrypt-cia-3ds.512385/)
   - [Extract the code.bin](https://github.com/ihaveamac/3DS-rom-tools/wiki/Extract-a-game-or-application-in-.cxi%2C-.cfa-or-.ncch-format) (make sure it's uncompressed)

2. Scan the binary file and save the output to a sym file.
   
   `RomScanner nds.zdb code.bin > XXX.sym`

# Tools description
| Name          | Description                                                                             |
|---------------|-----------------------------------------------------------------------------------------|
| collect.py    | search all function binary codes from static libraries, save the patterns to a txt file |
| GenDB         | Generate a datebase from txt files that collected by collect.py                         |
| RomScanner    | According to the database, search all function names in the code binary                 |
| ida_helper.py | import symbols to [ida](https://hex-rays.com/)                                          |

# Generate database
1. Put the static libs to directory xxx_v1, xxx_v2 ...
2. Collect information from static libs, generate xxx_v1.txt, xxx_v2.txt 
   
   `collect.py xxx_v1`
   
   `collect.py xxx_v2`

   `...`

3. Compile patterns, get database, it may take a long time
   
   `GenDB xxx.zdb xxx_v1.txt xxxv2.txt ... -voffset [Default Virtual Offset]`