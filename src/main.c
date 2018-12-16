#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <workbench/icon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/syslimits.h>
#include <unistd.h>
#include <signal.h>

#include "sys.h"

#include <exec/execbase.h>

// TODO: Scripted tests!

// Helpers
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define ZERO (0L)

// Version tag
#define VERSTAG "\0$VER: IconSnap 0.3 (30.11.2018)"
unsigned char versiontag[] = VERSTAG;

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
struct RDArgs *rdargs = NULL;

// clean exit handling
void CleanExit();

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
char *AlignCurrentWorkingDirTempPath = NULL;
unsigned char *AlignDirFixedDirName = NULL;
unsigned char *AlignDirFullPath = NULL;
unsigned char *AlignIconFixedDiskObjectName = NULL;

unsigned int AlignCurrentWorkingDir();
unsigned int AlignDir(unsigned char *diskObjectName);
unsigned int AlignFile(unsigned char *diskObjectName);
unsigned int AlignDiskObject(struct DiskObject *diskObject, long iconWidth, long iconHeigth);
long Align(long orig, long pad, long align, long alignoffset);

int main(int argc, char **argv)
{
    atexit(CleanExit);

    short sysInitResult = SysInit(argc, argv);
    if (sysInitResult != RETURN_OK)
    {
        exit(sysInitResult);
    }

    // Allocate memory areas
    AlignCurrentWorkingDirTempPath = AllocVec(PATH_MAX, MEMF_ANY);
    AlignDirFixedDirName = AllocVec(PATH_MAX, MEMF_ANY);
    AlignDirFullPath = AllocVec(PATH_MAX, MEMF_ANY);
    AlignIconFixedDiskObjectName = AllocVec(PATH_MAX, MEMF_ANY);

    if (argc == 0)
    {
        // Opened from WB
        Information("Started from Workbench\n\n");

        // Read args from tooltip
        struct WBStartup *wbStartup = (struct WBStartup *)argv;
        struct WBArg iconSnap = wbStartup->sm_ArgList[0];
        if (iconSnap.wa_Lock != ZERO)
        {
            BPTR oldDir = CurrentDir(iconSnap.wa_Lock);

            struct DiskObject *iconSnapDiskObject = GetDiskObjectNew(iconSnap.wa_Name);
            if (iconSnapDiskObject != NULL)
            {
                STRPTR VerboseTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "VERBOSE");
                if (VerboseTT)
                {
                    SysSetVerboseEnabled(TRUE);
                }
                STRPTR ConsoleDelayTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "CONSOLEDELAY");
                if (ConsoleDelayTT)
                {
                    long consoleDelay = strtol(ConsoleDelayTT, NULL, 10);
                    SysSetConsoleDelay(consoleDelay);
                }
                STRPTR PadLeftTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "PADLEFT");
                if (PadLeftTT)
                {
                    // Information("PadLeftTT: %s\n", PadLeftTT);
                    PaddingLeft = strtol(PadLeftTT, NULL, 10);
                }
                STRPTR PadTopTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "PADTOP");
                if (PadTopTT)
                {
                    // Information("PadTopTT: %s\n", PadTopTT);
                    PaddingTop = strtol(PadTopTT, NULL, 10);
                }
                STRPTR AlignXTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "ALIGNX");
                if (AlignXTT)
                {
                    // Information("AlignXTT: %s\n", AlignXTT);
                    AlignX = strtol(AlignXTT, NULL, 10);
                }
                STRPTR AlignYTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "ALIGNY");
                if (AlignYTT)
                {
                    // Information("AlignYTT: %s\n", AlignYTT);
                    AlignY = strtol(AlignYTT, NULL, 10);
                }
                STRPTR CenterXTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "CENTERX");
                if (CenterXTT)
                {
                    CenterX = TRUE;
                }
                STRPTR BottomYTT = FindToolType(iconSnapDiskObject->do_ToolTypes, "BOTTOMY");
                if (BottomYTT)
                {
                    BottomY = TRUE;
                }
            }
            CurrentDir(oldDir);

            Verbose(" PADLEFT %li\n", PaddingLeft);
            Verbose(" PADTOP %li\n", PaddingTop);
            Verbose(" ALIGNX %li\n", AlignX);
            Verbose(" ALIGNY %li\n", AlignY);
            Verbose(" CENTERX %hi\n", CenterX);
            Verbose(" BOTTOMY %hi\n", BottomY);
        }

        LONG wbNumArgs = wbStartup->sm_NumArgs;
        Verbose("Number of Workbench arguments received = %ld\n", wbNumArgs);

        // No tools selected?
        if (wbNumArgs <= 1)
        {
            Information("Use IconSnap from Workbench by Shift-clicking on the Icons\n");
            Information("you want to SnapShot, and double click on IconSnap\n");
            exit(RETURN_OK);
        }

        // Iterate parameters
        for (LONG i = 1; i < wbNumArgs; i++)
        {
            //Verbose("Argument %ld name =  %s\n", i, wbStartup->sm_ArgList[i].wa_Name);
            if (wbStartup->sm_ArgList[i].wa_Lock != ZERO)
            {
                BPTR oldDir = CurrentDir(wbStartup->sm_ArgList[i].wa_Lock);

                Verbose("AlignFile %s\n", wbStartup->sm_ArgList[i].wa_Name);
                AlignFile(wbStartup->sm_ArgList[i].wa_Name);
                //     struct DiskObject *dobj;
                //     dobj = GetDiskObjectNew(wbStartup->sm_ArgList[i].wa_Name);
                //     if (dobj != NULL)
                //     {
                //         Information("Opened disk object.\n");
                //         STRPTR TTarg = NULL;
                //         TTarg = FindToolType(dobj->do_ToolTypes, "TEST");
                //         if (TTarg)
                //         {
                //             Verbose("              TEST found set to = >%s<\n", TTarg);
                //         }
                //     }
                CurrentDir(oldDir);
            }
            // Verbose("");
        }

        exit(RETURN_OK);
    }

    // check arguments
    rdargs = ReadArgs(argumentString, argArray, NULL);

    if (!rdargs)
    {
        PrintFault(IoErr(), NULL);
        return RETURN_ERROR;
    }

    short verbose2 = argArray[VERBOSE_OPTION_POS] == DOSTRUE;
    if (verbose2)
    {
        SysSetVerboseEnabled(TRUE);
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
        fixCount = AlignFile(fileOption);
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

    return RETURN_OK;
}

void CleanExit()
{
    if (AlignCurrentWorkingDirTempPath != NULL)
    {
        FreeVec(AlignCurrentWorkingDirTempPath);
    }
    if (AlignDirFixedDirName != NULL)
    {
        FreeVec(AlignDirFixedDirName);
    }
    if (AlignDirFullPath != NULL)
    {
        FreeVec(AlignDirFullPath);
    }
    if (AlignIconFixedDiskObjectName != NULL)
    {
        FreeVec(AlignIconFixedDiskObjectName);
    }
    if (rdargs != NULL)
    {
        FreeArgs(rdargs);
        rdargs = NULL;
    }
    SysCleanup();
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
    //printf("sizeof(cwd): %ld\n",sizeof(cwd));
    if (getcwd(AlignCurrentWorkingDirTempPath, PATH_MAX) != NULL)
    {
        Verbose("Using current working dir: %s\n", AlignCurrentWorkingDirTempPath);
        fixCount = AlignDir(AlignCurrentWorkingDirTempPath);
    }
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
    strcpy(AlignDirFixedDirName, dirName);
    if (!StringEndsWith(dirName, "/") && !StringEndsWith(dirName, ":"))
    {
        Verbose("Appending \"/\" to dir\n");

        AlignDirFixedDirName[dirNameLen] = '/';
        AlignDirFixedDirName[dirNameLen + 1] = 0;
        dirNameLen += 1;
        Verbose("Fixed dir \"%s\"\n", AlignDirFixedDirName);
    }

    unsigned int fixCount = 0;
    DIR *dir = opendir(AlignDirFixedDirName);
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
                strcpy(AlignDirFullPath, AlignDirFixedDirName);
                strcat(AlignDirFullPath, direntry->d_name);
                StringToLower(AlignDirFullPath, fullLen, fullLen - 4);
                if (StringEndsWith(AlignDirFullPath, ".info"))
                {
                    // Verbose("AlignDirFullPath: %s\n", AlignDirFullPath);
                    fixCount += AlignFile(AlignDirFullPath);
                }
            }
            //printf("%s\n", direntry->d_name);
        }
        closedir(dir);
    }
    return fixCount;
}

unsigned int AlignFile(unsigned char *diskObjectName)
{
    unsigned long diskObjectNameLen = strlen(diskObjectName);
    // unsigned char *fixedDiskObjectName = malloc(PATH_MAX);
    strcpy(AlignIconFixedDiskObjectName, diskObjectName);
    StringToLower(AlignIconFixedDiskObjectName, diskObjectNameLen, diskObjectNameLen - 4);
    if (StringEndsWith(AlignIconFixedDiskObjectName, ".info"))
    {
        AlignIconFixedDiskObjectName[diskObjectNameLen - 5] = 0;
    }

    struct DiskObject *diskObject;

    long iconWidth = 0;
    long iconHeigth = 0;
    if (iconLibraryV44Enabled)
    {
        diskObject = GetIconTags(AlignIconFixedDiskObjectName, TAG_DONE); //ICONGETA_GetPaletteMappedIcon, FALSE, TAG_DONE);
        IconControl(diskObject, ICONCTRLA_GetWidth, &iconWidth, TAG_DONE);
        IconControl(diskObject, ICONCTRLA_GetHeight, &iconHeigth, TAG_DONE);
    }
    else
    {
        diskObject = GetDiskObject(AlignIconFixedDiskObjectName);
        iconWidth = diskObject->do_Gadget.Width;
        iconHeigth = diskObject->do_Gadget.Height;
    }
    if (diskObject)
    {
        if (iconLibraryV44Enabled)
        {
            Verbose("Icon found (>=V44): %s\n", AlignIconFixedDiskObjectName);
        }
        else
        {
            Verbose("Icon found (<V44): %s\n", AlignIconFixedDiskObjectName);
        }

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
            Information("Skip \"%s\" - no icon position\n", AlignIconFixedDiskObjectName);
            return 1;
        }

        short xaligned = FALSE;
        long origx = diskObject->do_CurrentX;
        long newx;
        if (diskObject->do_CurrentX != NO_ICON_POSITION)
        {
            if (CenterX)
            {
                newx = Align(origx, PaddingLeft, AlignX, iconWidth / 2);
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
                newy = Align(origy, PaddingTop, AlignY, iconHeigth);
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
                        AlignIconFixedDiskObjectName,
                        origx, origy);
            return 1;
        }

        PutDiskObject(AlignIconFixedDiskObjectName, diskObject);
        FreeDiskObject(diskObject);

        Information("Aligend \"%s\" (%i,%i) to (%i,%i)\n",
                    AlignIconFixedDiskObjectName,
                    origx, origy,
                    newx, newy);
        return 1;
    }
    Verbose("Skipped \"%s\" - icon not found\n", AlignIconFixedDiskObjectName);
    return 0;
}

unsigned int AlignDiskObject(struct DiskObject *diskObject, long iconWidth, long iconHeigth)
{
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
