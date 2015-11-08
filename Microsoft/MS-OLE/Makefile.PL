#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/microsoft-cmsolefs$LIBEXT cmsolefs_app";
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

arr("$LIBDIR/microsoft-cmsolefs$LIBEXT","cmsolefs.o");
cppr("cmsolefs.o","cmsolefs.cpp");

lud("cmsolefs_app","cmsolefs_app.o $LIBDIR/microsoft-cmsolefs$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT $LIBDIR/common-BlockDeviceFile$LIBEXT");
cppr("cmsolefs_app.o","main.cpp");

require "$RELTOP/makstep3.PL";

