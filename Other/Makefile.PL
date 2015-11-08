#!/usr/bin/perl

$RELTOP = "..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ("MCO");

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

