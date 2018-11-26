@REM echo off
@REM -v for verbose
vc src/main.c -v -stack-check -size -lamiga -lposix -IE:\Amiga\KrustWB3\Output\Dev\vbcc_posix\targets\m68k-amigaos\include -LE:\Amiga\KrustWB3\Output\Dev\vbcc_posix\targets\m68k-amigaos\lib -DBUILD_PLATFORM_WIN -o IconSnap
copy IconSnap r