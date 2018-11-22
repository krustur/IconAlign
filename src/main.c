#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/syslimits.h>
#include <unistd.h>

// TODO: Parameters for padding and alignment
// TODO: Align bottom, not top!
// TODO: Readme.md
// TODO: Readme for Amiga
// TODO: Pack
// TODO: Upload to Aminet
// TODO: Scripted tests!


// VBCC
extern size_t __stack_usage;

// Version tag
#define VERSTAG "\0$VER: IconAlign 0.1 (18.11.2018)"
UBYTE versiontag[] = VERSTAG;

// Build Platform
#ifdef BUILD_PLATFORM_AMIGA
const char *BuildPlatform = "Amiga";
#elif BUILD_PLATFORM_WIN
const char *BuildPlatform = "Win";
#else
const char *BuildPlatform = "Unknown";
#endif

// Libraries
// int _IconBaseVer = 45;
struct Library *DosBase = NULL;
struct Library *IconBase = NULL;

// Logging
short verbose = FALSE;
void Information(const char *fmt, ...);
void Verbose(const char *fmt, ...);

// String/Path handling
size_t maxPathTreshold = PATH_MAX - 2;
short StringEndsWith(const char *str, const char *suffix);
void StringToLower(char *str, unsigned int strLen, unsigned int offset);

// Icon alignment
long LeftPadding = 4;
long TopPadding = 4;
long XAlignment = 16;
long YAlignment = 16;
unsigned int AlignCurrentWorkingDir();
unsigned int AlignDir(unsigned char *diskObjectName);
unsigned int AlignIcon(unsigned char *diskObjectName);

int main(int argc, char **argv)
{
    // Open libraries
    DosBase = OpenLibrary("dos.library", 40L);
    if (!DosBase)
    {
        Information("Failed to open dos.library 40\n");
        return RETURN_ERROR;
    }

    IconBase = OpenLibrary("icon.library", 45L);
    if (!IconBase)
    {
        Information("Failed to open icon.library 45\n");
        return RETURN_ERROR;
    }

    // check arguments
    long argArray[] =
        {
            0,
            // 0,
            0};
    struct RDArgs *rdargs = ReadArgs("FILE/K,DIR/K,VERBOSE/S", argArray, NULL);
    if (!rdargs)
    {
        // GetFa
        PrintFault(IoErr(), NULL);
        return RETURN_ERROR;
    }

    verbose = argArray[2] == DOSTRUE;
    if (verbose)
    {
        Verbose(" VERBOSE logging active\n");
    }
    STRPTR fileOption = (STRPTR)argArray[0];
    if (fileOption)
    {
        Verbose(" FILE=[%s]\n", fileOption);
    }
    unsigned char *dirOption = (unsigned char *)argArray[1];
    if (dirOption)
    {
        Verbose(" DIR=[%s]\n", dirOption);
    }
    // if (!fileOption && !folderOption)
    // {
    //     Information("please provide FILE or FOLDER option\n");
    //     return RETURN_ERROR;
    // }

    unsigned int fixCount = 0;
    if (!fileOption && !dirOption)
    {
        fixCount = AlignCurrentWorkingDir();
    }
    else if (fileOption)
    {
        fixCount = AlignIcon(fileOption);
    }
    else if (dirOption)
    {
        size_t dirOptionLen = strlen(dirOption);
        if (dirOptionLen == 0)
        {
            fixCount = AlignCurrentWorkingDir();
        }
        else
        {
            fixCount = AlignDir(dirOption);
        }
    }
    if (fixCount == 0)
    {
        Information("No icons found\n");
    }

    // if (argc != 2)
    // {
    //     printf("incorrect args!\n");
    //     printf("usage: IconAlign FILE\n");
    //     return RETURN_ERROR;
    // }

    if (rdargs)
    {
        FreeArgs(rdargs);
    }
    CloseLibrary(IconBase);

    Verbose("Build platform: %s\n", BuildPlatform);
    Verbose("Stack used: %lu\n", (unsigned long)__stack_usage);
    // Verbose("IconBase: %p\n", (void *)IconBase);
    // Verbose("SysBase: %p\n", (void *)SysBase);
    Verbose("DosBase: %p\n", (void *)DosBase);

    return RETURN_OK;
}

// Logging
void Verbose(const char *fmt, ...)
{
    if (!verbose)
        return;

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void Information(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

// String/Path handling
short StringEndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
    {
        return FALSE;
    }
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
    {
        return FALSE;
    }
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void StringToLower(char *str, unsigned int strLen, unsigned int offset)
{
    // Verbose("StringToLower from:   %s\n", str);
    if (offset >= strLen)
    {
        return;
    }
    for (int i = offset; i < strLen; i++)
    {
        unsigned char curr = str[i];
        if (curr >= 'A' && curr <= 'Z')
        {
            str[i] = curr + 32;
        }
        // TODO: Not really needed for this app! But åäö etc.
    }
    // Verbose("StringToLower result: %s\n", str);
}

// Icon management
unsigned int AlignCurrentWorkingDir()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        Verbose("Using current working dir: %s\n", cwd);
    }
    unsigned int fixCount = AlignDir(cwd);
    return fixCount;
}

unsigned int AlignDir(unsigned char *dirName)
{
    size_t dirNameLen = strlen(dirName);
    if (dirNameLen > maxPathTreshold)
    {
        Information("path \"%s\" is too long - skipping!\n", dirName);
        return 0;
    }
    if (!StringEndsWith(dirName, "/") && !StringEndsWith(dirName, ":"))
    {
        Verbose("Appending \"/\" to dir\n");
        unsigned char fixedDirName[PATH_MAX];
        strcpy(fixedDirName, dirName);
        fixedDirName[dirNameLen] = '/';
        fixedDirName[dirNameLen + 1] = 0;
        dirName = fixedDirName;
        dirNameLen += 1;
    }

    unsigned int fixCount = 0;
    DIR *dir = opendir(dirName);
    struct dirent *direntry;
    if (dir)
    {
        // Verbose("dirNameLen: %i", dirNameLen);
        while ((direntry = readdir(dir)) != NULL)
        {
            size_t dirEntryLen = strlen(direntry->d_name);
            size_t fullLen = dirNameLen + dirEntryLen;
            if (fullLen > PATH_MAX - 2)
            {
                Information("full path to %s is too long - skipping!\n", direntry->d_name);
            }
            else
            {
                unsigned char fullPath[PATH_MAX];
                strcpy(fullPath, dirName);
                strcat(fullPath, direntry->d_name);
                StringToLower(fullPath, fullLen, fullLen - 4);
                if (StringEndsWith(fullPath, ".info"))
                {
                    fullPath[fullLen - 5] = 0;
                    // Verbose("fullPath: %s\n", fullPath);
                    fixCount += AlignIcon(fullPath);
                }
            }
            //printf("%s\n", direntry->d_name);
        }
        closedir(dir);
    }
    return fixCount;
}

unsigned int AlignIcon(unsigned char *diskObjectName)
{
    struct DiskObject *diskObject = GetDiskObject(diskObjectName);
    if (diskObject)
    {
        Verbose("Icon found: %s\n", diskObjectName);
        // Verbose("do_Magic: %x\n", diskObject->do_Magic);
        Verbose(" Version: %hi\n", diskObject->do_Version);
        Verbose(" do_Type: %i\n", (int)diskObject->do_Type);
        Verbose(" do_DefaultTool: %s\n", diskObject->do_DefaultTool);
        // Verbose("do_DefaultTool: ");
        // Verbose(diskObject->do_DefaultTool);
        // Verbose("\n");
        Verbose(" do_CurrentX: %li\n", diskObject->do_CurrentX);
        Verbose(" do_CurrentY: %li\n", diskObject->do_CurrentY);
        // Verbose(" Width: %hi\n", diskObject->);
        Verbose(" SpecialInfo: %p\n", diskObject->do_Gadget.SpecialInfo);
        Verbose(" UserData: %p\n", diskObject->do_Gadget.UserData);
        Verbose(" Width: %hi\n", diskObject->do_Gadget.Width);
        Verbose(" Height: %hi\n", diskObject->do_Gadget.Height);
        // Verbose(" GadgetRender: %p\n", diskObject->do_Gadget.GadgetRender);
        Verbose(" Flags: %hi\n", diskObject->do_Gadget.Flags);
        Verbose(" GadgetType: %hi\n", diskObject->do_Gadget.GadgetType);
        Verbose(" LeftEdge: %hi\n", diskObject->do_Gadget.LeftEdge);
        Verbose(" TopEdge: %hi\n", diskObject->do_Gadget.TopEdge);

        // diskObject->do_CurrentX += 10;
        if (diskObject->do_CurrentX == NO_ICON_POSITION && diskObject->do_CurrentX == NO_ICON_POSITION)
        {
            Information("Skip \"%s\" - no icon position\n", diskObjectName);
            return 0;
        }

        short xaligned = FALSE;
        if (diskObject->do_CurrentX != NO_ICON_POSITION)
        {
            long currx = (diskObject->do_CurrentX - LeftPadding) + (XAlignment / 2);
            long newx = LeftPadding + currx - (currx % XAlignment);
            if (newx != diskObject->do_CurrentX)
            {
                xaligned = TRUE;
            }
            diskObject->do_CurrentX = newx;
        }
        short yaligned = FALSE;
        if (diskObject->do_CurrentY != NO_ICON_POSITION)
        {
            long curry = (diskObject->do_CurrentY -TopPadding) + (YAlignment / 2);
            long newy = TopPadding + curry - (curry % YAlignment);
            if (newy != diskObject->do_CurrentY)
            {
                yaligned = TRUE;
                diskObject->do_CurrentY = newy;
            }
        }

        if (!xaligned && !yaligned)
        {
            Information("Already aligned \"%s\"\n", diskObjectName);
            return 0;
        }
        
        PutDiskObject(diskObjectName, diskObject);
        FreeDiskObject(diskObject);

        Information("Aligend \"%s\"\n", diskObjectName);
        return 1;
    }
    Verbose("Skipped \"%s\" - icon not found\n", diskObjectName);
    return 0;
}
