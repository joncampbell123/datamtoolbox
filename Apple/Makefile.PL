#!/usr/bin/perl

$RELTOP = "..";

require "$RELTOP/makstep1.PL";

# directories we target
%DIRS = ("HFS","Image-DiskCopy","MFS","Macintosh-BootBlock","common");

require "$RELTOP/makstep2.PL";
require "$RELTOP/makstep3.PL";

