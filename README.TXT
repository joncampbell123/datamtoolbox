
THIS CODE IS PROVIDED AS-IS FOR REFERENCE, DATA BACKUP/RECOVERY, AND
DIGITAL ARCHEOLOGICAL PURPOSES UNDER NO WARRANTY WHATSOEVER.

All source code in this package should be compiled with the source tree
root specified as one of the include paths. All #includes are written
to assume this i.e. code that needs the common Block File classes
will say:

#include "common/BlockDeviceFile.h"

and code that refers to the MS-DOS FAT system will say:

#include "Microsoft/FAT/cdosfat.h"



To generate Makefiles for Linux, type "make Makefile.PL" followed by "make"

