; -v for verbose
vc src/main.c -stack-check -lamiga -lposix -IDev:vbcc_posix/targets/m68k-amigaos/include -LDev:vbcc_posix/targets/m68k-amigaos/lib -DBUILD_PLATFORM_AMIGA  -c99 -o IconAlign 
copy IconAlign r