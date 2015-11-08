#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/other-cmcofs$LIBEXT cmcofs_app";
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

arr("$LIBDIR/other-cmcofs$LIBEXT","cmcofs.o");
cppr("cmcofs.o","cmcofs.cpp");

lud("cmcofs_app","cmcofs_app.o $LIBDIR/other-cmcofs$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT $LIBDIR/common-BlockDeviceFile$LIBEXT");
cppr("cmcofs_app.o","main.cpp");

require "$RELTOP/makstep3.PL";

