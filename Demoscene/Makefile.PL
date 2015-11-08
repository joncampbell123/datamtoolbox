#!/usr/bin/perl

$RELTOP = "..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ("FutureCrew-EXEdir","RealTech94-Res");

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

