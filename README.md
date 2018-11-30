# IconSnap
## About
IconSnap is a command line utility that snaps Workbench icons to a virtual
grid.

Have you ever felt limited by Workbench's ability to layout the icons? This
util aids you in your struggles by allowing you to roughly position your
icons in Workbench any way you want, and then run this util to snap your
Icons to a given grid.

#### Features
- Snap Workbench icons to grid (x and y individually customizable)
- Add Padding to left and top (individually customizable)
- Snap single icon
- Snap all icons in entire folder or current directory
- Supports OS3.5 Icons (requires icon.library v44)
- Snap vertically to bottom
- Snap horizontally to center

## Installation
Copy IconSnap to C:

## How to use
Order your Icons in Workbench roughly the way you want them positioned, and
then snapshot their position. After that run this tool and point it to the
folder containg the Icons to be Snapshot.

### Command line usage
```
IconSnap    FILE/K,DIR/K,PADLEFT/N,PADTOP/N,ALIGNX/N,ALIGNY/N,
            CENTERX/S,BOTTOMY/S,VERBOSE/S

FILE/K:     Single File or Folder to Snap. Can point to either the
            actual file/folder or it's associated .info file.

DIR/K:      Folder containing Files and Folders to Snap. If you 
            want to snap the actual folder, use the FILE parameter
            instead.

PADLEFT/N:  Use this option to create some Padding to the left of
            the Snapped Icons.

PADTOP/N:   Use this option to create some Padding above the Snapped
            Icons.

ALIGNX/N:   Number of pixels in the virual grid on the X axis.

ALIGNY/N:   Number of pixels in the virual grid on the Y axis.

CENTERX/S:  Snap horizontally to center of the Icon.
            NOTE: A bug in the code, that reads the Icon width and 
            heigth incorrectly, results in this feature not behaving
            as expected!

BOTTOMY/S:  Snap vertically to the bottom of the Icon. 
            NOTE: A bug in the code, that reads the Icon width and 
            heigth incorrectly, results in this feature not behaving
            as expected!

VERBOSE/S:  Active verbose logging. Used for debugging purposes.
```

### Examples
```
IconSnap FILE Sys:Disk.info
IconSnap DIR Sys:Utilities/
IconSnap DIR Sys:Tools PADLEFT 15 PADTOP 15 ALIGNX 5 ALIGNY 5 CENTERX BOTTOMY 
```

## Version history
0.3 Support for OS3.5 Icons (requires icon.library v44)

0.2 First working version/release

## Bug reports and feature requests
Post your Bug reports and Feature requests on github: 
https://github.com/krustur/IconSnap/issues

