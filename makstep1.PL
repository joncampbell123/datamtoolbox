
$LIBDIR = "$RELTOP/lib";
$LIBEXT = ".a";
$MORECFLAGS .= "-I$RELTOP -DLINUX -Dstrcmpi=strcasecmp -Dstrnicmp=strncasecmp";

open(MAK,">Makefile") or die("Cannot create Makefile");

