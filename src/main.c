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

// TODO: Readme.md
// TODO: Readme for Amiga
// TODO: Pack
// TODO: Upload to Aminet
// TODO: Scripted tests!

// C helpers
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// VBCC
extern size_t __stack_usage;

// Version tag
#define VERSTAG "\0$VER: IconSnap 0.2 (26.11.2018)"
unsigned char versiontag[] = VERSTAG;

// Build Platform
#ifdef BUILD_PLATFORM_AMIGA
const char *BuildPlatform = "Amiga";
#elif BUILD_PLATFORM_WIN
const char *BuildPlatform = "Win";
#else
const char *BuildPlatform = "Unknown";
#endif

// Libraries
unsigned long DosVersion = 37L;
struct Library *DosBase = NULL;
unsigned long IconVersion = 37L;
struct Library *IconBase = NULL;
short iconLibraryV44Enabled = FALSE;

// Arguments
unsigned char FILE_OPTION_POS = 0;
unsigned char DIR_OPTION_POS = 1;
unsigned char PADLEFT_OPTION_POS = 2;
unsigned char PADTOP_OPTION_POS = 3;
unsigned char ALIGNX_OPTION_POS = 4;
unsigned char ALIGNY_OPTION_POS = 5;
unsigned char CENTERX_OPTION_POS = 6;
unsigned char BOTTOMY_OPTION_POS = 7;

unsigned char VERBOSE_OPTION_POS = 8;
long argArray[] =
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0};
unsigned char argumentString[] = "FILE/K,DIR/K,PADLEFT/N,PADTOP/N,ALIGNX/N,ALIGNY/N,CENTERX/S,BOTTOMY/S,VERBOSE/S";

// Logging
short verbose = FALSE;
void Information(const char *fmt, ...);
void Verbose(const char *fmt, ...);

// String/Path handling
size_t maxPathTreshold = PATH_MAX - 2;
short StringEndsWith(const char *str, const char *suffix);
void StringToLower(char *str, unsigned int strLen, unsigned int offset);

// Icon alignment
long PaddingLeft = 4;
long PaddingTop = 4;
long AlignX = 16;
long AlignY = 16;
short CenterX = FALSE;
short BottomY = FALSE;
unsigned int AlignCurrentWorkingDir();
unsigned int AlignDir(unsigned char *diskObjectName);
unsigned int AlignIcon(unsigned char *diskObjectName);
long Align(long orig, long pad, long align, long alignoffset);

int main(int argc, char **argv)
{
    // Open libraries
    DosBase = OpenLibrary("dos.library", DosVersion);
    if (!DosBase)
    {
        Information("Failed to open dos.library %li\n", DosVersion);
        return RETURN_ERROR;
    }
    // Verbose("DosBase: %p\n", (void *)DosBase);

    IconBase = OpenLibrary("icon.library", IconVersion);
    if (!IconBase)
    {
        Information("Failed to open icon.library %li\n", IconVersion);
        return RETURN_ERROR;
    }
    // Verbose("IconBase: %p\n", (void *)IconBase);

    // check arguments

    struct RDArgs *rdargs = ReadArgs(argumentString, argArray, NULL);

    if (!rdargs)
    {
        PrintFault(IoErr(), NULL);
        return RETURN_ERROR;
    }

    verbose = argArray[VERBOSE_OPTION_POS] == DOSTRUE;
    if (verbose)
    {
        Verbose(" VERBOSE logging active\n");
    }
    Verbose("Build platform: %s\n", BuildPlatform);
    Verbose("dos.library version %li\n", DosBase->lib_Version);
    Verbose("icon.library version %li\n", IconBase->lib_Version);
    if (IconBase->lib_Version >= 45)
    {
        Verbose("Icon library V44 features enabled\n");
        iconLibraryV44Enabled = TRUE;
    }

    unsigned char *fileOption = (unsigned char *)argArray[FILE_OPTION_POS];
    if (fileOption)
    {
        Verbose(" FILE=[%s]\n", fileOption);
    }

    unsigned char *dirOption = (unsigned char *)argArray[DIR_OPTION_POS];
    if (dirOption)
    {
        Verbose(" DIR=[%s]\n", dirOption);
    }

    if ((long *)argArray[PADLEFT_OPTION_POS] != NULL)
    {
        PaddingLeft = *(long *)argArray[PADLEFT_OPTION_POS];
    }
    if ((long *)argArray[PADTOP_OPTION_POS] != NULL)
    {
        PaddingTop = *(long *)argArray[PADTOP_OPTION_POS];
    }

    if ((long *)argArray[ALIGNX_OPTION_POS] != NULL)
    {
        AlignX = *(long *)argArray[ALIGNX_OPTION_POS];
    }
    if ((long *)argArray[ALIGNY_OPTION_POS] != NULL)
    {
        AlignY = *(long *)argArray[ALIGNY_OPTION_POS];
    }

    CenterX = argArray[CENTERX_OPTION_POS] == DOSTRUE;
    BottomY = argArray[BOTTOMY_OPTION_POS] == DOSTRUE;

    Verbose(" PADLEFT %li\n", PaddingLeft);
    Verbose(" PADTOP %li\n", PaddingTop);
    Verbose(" ALIGNX %li\n", AlignX);
    Verbose(" ALIGNY %li\n", AlignY);
    Verbose(" CENTERX %hi\n", CenterX);
    Verbose(" BOTTOMY %hi\n", BottomY);
    // if (!fileOption && !folderOption)
    // {
    //     Information("please provide FILE or FOLDER option\n");
    //     return RETURN_ERROR;
    // }

    if (AlignX == 0 || AlignY == 0)
    {
        Information("Can't align to 0\n");
        return RETURN_ERROR;
    }

    unsigned int fixCount = 0;
    if (!fileOption && !dirOption)
    {
        // usage: IconSnap
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
            // usage: IconSnap DIR ""
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
    //     printf("usage: IconSnap FILE\n");
    //     return RETURN_ERROR;
    // }

    if (rdargs)
    {
        FreeArgs(rdargs);
    }
    CloseLibrary(DosBase);
    CloseLibrary(IconBase);

    Verbose("Stack used: %lu\n", (unsigned long)__stack_usage);

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
    unsigned int fixCount = 0;
    char *currentWorkingDir = malloc(PATH_MAX);
    //printf("sizeof(cwd): %ld\n",sizeof(cwd));
    if (getcwd(currentWorkingDir, PATH_MAX) != NULL)
    {
        Verbose("Using current working dir: %s\n", currentWorkingDir);
        fixCount = AlignDir(currentWorkingDir);
    }
    free(currentWorkingDir);
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
    unsigned char *fixedDirName = malloc(PATH_MAX);
    strcpy(fixedDirName, dirName);
    if (!StringEndsWith(dirName, "/") && !StringEndsWith(dirName, ":"))
    {
        Verbose("Appending \"/\" to dir\n");

        fixedDirName[dirNameLen] = '/';
        fixedDirName[dirNameLen + 1] = 0;
        dirNameLen += 1;
        Verbose("Fixed dir \"%s\"\n", fixedDirName);
    }

    unsigned int fixCount = 0;
    DIR *dir = opendir(fixedDirName);
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
                unsigned char *fullPath = malloc(PATH_MAX);
                strcpy(fullPath, fixedDirName);
                strcat(fullPath, direntry->d_name);
                StringToLower(fullPath, fullLen, fullLen - 4);
                if (StringEndsWith(fullPath, ".info"))
                {
                    // Verbose("fullPath: %s\n", fullPath);
                    fixCount += AlignIcon(fullPath);
                }
                free(fullPath);
            }
            //printf("%s\n", direntry->d_name);
        }
        closedir(dir);
    }
    free(fixedDirName);
    return fixCount;
}

unsigned int AlignIcon(unsigned char *diskObjectName)
{
    unsigned long diskObjectNameLen = strlen(diskObjectName);
    unsigned char *fixedDiskObjectName = malloc(PATH_MAX);
    strcpy(fixedDiskObjectName, diskObjectName);
    StringToLower(fixedDiskObjectName, diskObjectNameLen, diskObjectNameLen - 4);
    if (StringEndsWith(fixedDiskObjectName, ".info"))
    {
        fixedDiskObjectName[diskObjectNameLen - 5] = 0;
    }

    struct DiskObject *diskObject = GetDiskObject(fixedDiskObjectName);
    if (diskObject)
    {
        Verbose("Icon found: %s\n", fixedDiskObjectName);
        // Verbose("do_Magic: %x\n", diskObject->do_Magic);
        // Verbose(" Version: %hi\n", diskObject->do_Version);
        // Verbose(" do_Type: %i\n", (int)diskObject->do_Type);
        // Verbose(" do_DefaultTool: %s\n", diskObject->do_DefaultTool);
        // Verbose("do_DefaultTool: ");
        // Verbose(diskObject->do_DefaultTool);
        // Verbose("\n");
        Verbose(" do_CurrentX: %li\n", diskObject->do_CurrentX);
        Verbose(" do_CurrentY: %li\n", diskObject->do_CurrentY);
        // Verbose(" Width: %hi\n", diskObject->);
        // Verbose(" SpecialInfo: %p\n", diskObject->do_Gadget.SpecialInfo);
        // Verbose(" UserData: %p\n", diskObject->do_Gadget.UserData);
        Verbose(" Width: %hi\n", diskObject->do_Gadget.Width);
        Verbose(" Height: %hi\n", diskObject->do_Gadget.Height);
        // Verbose(" GadgetRender: %p\n", diskObject->do_Gadget.GadgetRender);
        // Verbose(" SelectRender: %p\n", diskObject->do_Gadget.SelectRender);
        // struct Image *gadgetImage = (struct Image *)(diskObject->do_Gadget.GadgetRender);
        // Verbose(" gadgetImage Width: %hi\n", gadgetImage->Width);
        // Verbose(" gadgetImage Height: %hi\n", gadgetImage->Height);

        // struct Image *selectImage = (struct Image *)(diskObject->do_Gadget.SelectRender);
        // if (selectImage)
        // {
        //     Verbose(" selectImage Width: %hi\n", selectImage->Width);
        //     Verbose(" selectImage Height: %hi\n", selectImage->Height);
        // }
        // Verbose(" Flags: %hi\n", diskObject->U);
        // Verbose(" Flags: %hi\n", diskObject->do_Gadget.Flags);
        // Verbose(" GadgetType: %hi\n", diskObject->do_Gadget.GadgetType);
        // Verbose(" LeftEdge: %hi\n", diskObject->do_Gadget.LeftEdge);
        // Verbose(" TopEdge: %hi\n", diskObject->do_Gadget.TopEdge);
        // unsigned char *toolType = NULL;
        // toolType = FindToolType(diskObject->do_ToolTypes, "IM1");
        // if (toolType)
        // {
        //     Verbose("  toolType IM1 = \"%s\"\n", toolType);
        // }
        // else
        // {
        //     Verbose("  toolType IM1 not found :'(\n", toolType);
        // }

        // diskObject->do_CurrentX += 10;
        if (diskObject->do_CurrentX == NO_ICON_POSITION && diskObject->do_CurrentX == NO_ICON_POSITION)
        {
            Information("Skip \"%s\" - no icon position\n", fixedDiskObjectName);
            free(fixedDiskObjectName);
            return 1;
        }

        short xaligned = FALSE;
        long origx = diskObject->do_CurrentX;
        long newx;
        if (diskObject->do_CurrentX != NO_ICON_POSITION)
        {
            if (CenterX)
            {
                newx = Align(origx, PaddingLeft, AlignX, diskObject->do_Gadget.Width / 2);
            }
            else
            {
                newx = Align(origx, PaddingLeft, AlignX, 0);
            }
            // long currx = (origx - PaddingLeft) + (AlignX / 2);
            // newx = PaddingLeft + currx - (currx % AlignX);
            if (newx != origx)
            {
                xaligned = TRUE;
            }
            diskObject->do_CurrentX = newx;
        }
        short yaligned = FALSE;
        long origy = diskObject->do_CurrentY;
        long newy;
        if (diskObject->do_CurrentY != NO_ICON_POSITION)
        {
            if (BottomY)
            {
                newy = Align(origy, PaddingTop, AlignY, diskObject->do_Gadget.Height);
            }
            else
            {
                newy = Align(origy, PaddingTop, AlignY, 0);
            }
            // long curry = (origy - PaddingTop) + (AlignY / 2);
            // newy = PaddingTop + curry - (curry % AlignY);
            if (newy != origy)
            {
                yaligned = TRUE;
                diskObject->do_CurrentY = newy;
            }
        }

        if (!xaligned && !yaligned)
        {
            Information("Already aligned \"%s\" (%i,%i)\n",
                        fixedDiskObjectName,
                        origx, origy);
            free(fixedDiskObjectName);
            return 1;
        }

        PutDiskObject(fixedDiskObjectName, diskObject);
        FreeDiskObject(diskObject);

        Information("Aligend \"%s\" (%i,%i) to (%i,%i)\n",
                    fixedDiskObjectName,
                    origx, origy,
                    newx, newy);
        free(fixedDiskObjectName);
        return 1;
    }
    Verbose("Skipped \"%s\" - icon not found\n", fixedDiskObjectName);
    free(fixedDiskObjectName);
    return 0;
}

long Align(long orig, long pad, long align, long alignoffset)
{
    // printf("align %li %li %li %li\n", orig, pad, align, alignoffset);
    long max = MAX(alignoffset, (orig + alignoffset - pad));
    // printf("max %li\n", max);

    long temp = max + (align / 2);
    long alignedunpadded = temp - (temp % align);
    // printf("alignedunpadded %li\n", alignedunpadded);
    long aligned = alignedunpadded + pad - alignoffset;
    // printf("aligned %li\n", aligned);
    // printf("\n");
    return aligned;
}

// long Align(long orig, long pad, long align, long alignoffset)
// {
//     // printf("align %li %li %li\n", orig, pad, align);
//     long max = MAX(0,(orig - pad));
//     // printf("max %li\n", max);

//     long temp = max + alignoffset + (align / 2);
//     long alignedunpadded = temp - (temp % align);
//     // printf("alignedunpadded %li\n", alignedunpadded);
//     long aligned = alignedunpadded - alignoffset + PaddingLeft;
//     // printf("aligned %li\n", aligned);
//     // printf("\n");
//     return aligned;
// }
