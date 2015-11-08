#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ();
%SRCFILES = ("adler32.c","compress.c","crc32.c","deflate.c","gzio.c","infback.c","inffast.c","inflate.c","inftrees.c","trees.c","uncompr.c","zutil.c");
$TARGET = "common-zlib.a";

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

