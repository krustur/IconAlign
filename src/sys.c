#include <exec/types.h>
// #include <dos/dos.h>
#include <libraries/dos.h>
// #include <workbench/workbench.h>
// #include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
// #include <proto/icon.h>
// #include <workbench/icon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Build Platform
#ifdef BUILD_PLATFORM_AMIGA
const char *BuildPlatform = "Amiga";
#elif BUILD_PLATFORM_WIN
const char *BuildPlatform = "Win";
#else
const char *BuildPlatform = "Unknown";
#endif

// VBCC
extern size_t __stack_usage;

// Library
unsigned long DosReqVersion = 37L;
struct Library *DosBase = NULL;
unsigned long IconReqVersion = 37L;
struct Library *IconBase = NULL;
short iconLibraryV44Enabled = FALSE;

// Logging
#define LOG_MAX (256)
// const char *ConsoleString = "CON:20/20/500/100/IconSnap/CLOSE";
const char *ConsoleString = "CON:20/20/500/100/IconSnap";
long ConsoleDelay = 100;
unsigned char *LogBuffer = NULL;
short verbose = FALSE;
BPTR wbcon = (BPTR)NULL;
// FILE *outputFile = NULL;
void Information(const char *fmt, ...);
void Verbose(const char *fmt, ...);

short SysInit(int argc, char **argv)
{
    // Open libraries
    DosBase = OpenLibrary(DOSNAME, DosReqVersion);
    if (!DosBase)
    {
        // Information("Failed to open dos.library %li\n", DosReqVersion);
        return RETURN_ERROR;
    }
    // Verbose("DosBase: %p\n", (void *)DosBase);

    IconBase = OpenLibrary("icon.library", IconReqVersion);
    if (!IconBase)
    {
        // Information("Failed to open icon.library %li\n", IconReqVersion);
        return RETURN_ERROR;
    }
    // Verbose("IconBase: %p\n", (void *)IconBase);
    if (IconBase->lib_Version >= 45)
    {
        iconLibraryV44Enabled = TRUE;
    }

    // Initialize logging
    LogBuffer = AllocVec(LOG_MAX, MEMF_ANY);

    if (argc == 0)
    {
        //Opened from WB
        wbcon = Open(ConsoleString, MODE_NEWFILE);
        SelectOutput(wbcon);
    }

    return RETURN_OK;
}

void SysCleanup(void)
{
    Verbose("Stack used: %lu\n", (unsigned long)__stack_usage);

    if (LogBuffer != NULL)
    {
        FreeVec(LogBuffer);
    }
    if (wbcon != (BPTR)NULL)
    {
        Delay(ConsoleDelay);
        Close(wbcon);
    }
    if (DosBase != NULL)
    {
        CloseLibrary(DosBase);
        DosBase = NULL;
    }
    if (IconBase != NULL)
    {
        CloseLibrary(IconBase);
        IconBase = NULL;
    }
}

void SysSetVerboseEnabled(short enabled)
{
    verbose = enabled;
    Verbose("Verbose logging active\n");
    Verbose("Build platform: %s\n", BuildPlatform);
    Verbose("dos.library version %li\n", DosBase->lib_Version);
    Verbose("dos.library opencnt %li\n", DosBase->lib_OpenCnt);
    Verbose("icon.library version %li\n", IconBase->lib_Version);
    Verbose("icon.library opencnt %li\n", IconBase->lib_OpenCnt);
    if (iconLibraryV44Enabled)
    {
        Verbose("Icon library V44 features enabled\n");
    }
}

void SysSetConsoleDelay(long delay)
{
    Verbose("ConsoleDelay changed from %li to %li\n", ConsoleDelay, delay);
    ConsoleDelay = delay;
}

// Logging
void Verbose(const char *fmt, ...)
{
    if (!verbose)
        return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(LogBuffer, LOG_MAX-1, fmt, args);
    Write(Output(), LogBuffer, strlen(LogBuffer));
    va_end(args);

    // va_list args;
    // // va_start(args, fmt);
    // Information(fmt, args);
    // // va_end(args);
}

void Information(const char *fmt, ...)
{
    if (LogBuffer == NULL)
    {
        Write(Output(), "Log error\n", 10);
        return;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(LogBuffer, LOG_MAX-1, fmt, args);
    Write(Output(), LogBuffer, strlen(LogBuffer));
    va_end(args);
}
