#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/demoscene-realtech94res$LIBEXT rt94res_app";
$MORECLEAN = "*.o";

# directories we target
%DIRS = ();
%LIBS = ("common-SomeFileStdio","common-types");

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

arr("$LIBDIR/demoscene-realtech94res$LIBEXT","rt94res.o");
cppr("rt94res.o","rt94res.cpp");

lud("rt94res_app","rt94res_app.o $LIBDIR/demoscene-realtech94res$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT");
cppr("rt94res_app.o","main.cpp");

require "$RELTOP/makstep3.PL";

