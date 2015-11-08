#!/usr/bin/perl

$RELTOP = "..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ("FAT","MSDOS-EXE","MSDOS-WinEXEstub","MS-OLE","MS-Partitions");

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

