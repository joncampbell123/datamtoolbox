#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/demoscene-futurecrew-exedir$LIBEXT fcxdir_app";
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

arr("$LIBDIR/demoscene-futurecrew-exedir$LIBEXT","fcxdir.o s3m_correct.o");
cppr("fcxdir.o","fcxdir.cpp");
cppr("s3m_correct.o","s3m_correct.cpp");

lud("fcxdir_app","fcxdir_app.o $LIBDIR/demoscene-futurecrew-exedir$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT");
cppr("fcxdir_app.o","main.cpp");

require "$RELTOP/makstep3.PL";

