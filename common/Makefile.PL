#!/usr/bin/perl

$RELTOP = "..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/common-BlockDeviceFile$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT";
$MORECLEAN = "*.o";

# directories we target
%DIRS = ("zlib");

require "$RELTOP/makstep2.PL";

sub arr {
	print MAK "$_[0]: $_[1]\n";
	print MAK "\tar r $_[0] $_[1]\n\n";
}

sub cppr {
	print MAK "$_[0]: $_[1]\n";
	print MAK "\tg++ $MORECFLAGS -c -o $_[0] $_[1]\n\n";
}

arr("$LIBDIR/common-BlockDeviceFile$LIBEXT","BlockDeviceFile.o");
cppr("BlockDeviceFile.o","BlockDeviceFile.cpp");

arr("$LIBDIR/common-SomeFileStdio$LIBEXT","SomeFileStdio.o");
cppr("SomeFileStdio.o","SomeFileStdio.cpp");

arr("$LIBDIR/common-types$LIBEXT","Types.o");
cppr("Types.o","Types.cpp");

require "$RELTOP/makstep3.PL";

