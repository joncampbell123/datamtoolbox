#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ();

# libraries we use
%LIBS = ();

# source files to compile
%SRCFILES = ("Apple-Macintosh-Common.cpp");
$TARGET = "apple-common-macintosh.a";
$MORECFLAGS .= "";

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

