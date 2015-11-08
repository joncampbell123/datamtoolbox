#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ();

# libraries we use
%LIBS = ("common-SomeFileStdio","common-types","apple-common-macintosh","common-BlockDeviceFile","apple-image-diskcopy");

# source files to compile
%SRCFILES = ("hfsdump.cpp");
$TARGET = "hfsdump";
$MORECFLAGS .= "";

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

