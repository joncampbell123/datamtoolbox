#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/microsoft-cpcpartitions$LIBEXT";
$MORECLEAN = "*.o";

# directories we target
%DIRS = ();
%LIBS = ("common-SomeFileStdio","common-types","common-BlockDeviceFile");

require "$RELTOP/makstep2.PL";

sub lud {
	print MAK "$_[0]: $_[1]\n";
	print MAK "\tg++ $MORECFLAGS -o $_[0] $_[1]\n\n";
}

sub arr {
	print MAK "$_[0]: $_[1]\n";
	print MAK "\tar r $_[0] $_[1]\n\n";
}

sub cppr {
	print MAK "$_[0]: $_[1]\n";
	print MAK "\tg++ $MORECFLAGS -c -o $_[0] $_[1]\n\n";
}

arr("$LIBDIR/microsoft-cpcpartitions$LIBEXT","cpcpartitions.o");
cppr("cpcpartitions.o","cpcpartitions.cpp");

require "$RELTOP/makstep3.PL";

