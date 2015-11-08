#!/usr/bin/perl

$RELTOP = "../..";

require "$RELTOP/makstep1.PL";

$OTHERTARGETS = "$LIBDIR/macromedia-flashswf-cmfswfunzip.lib$LIBEXT $LIBDIR/macromedia-flashswf-cmfswfzip.lib$LIBEXT $LIBDIR/macromedia-flashswf-cmfswftags.lib$LIBEXT swfzip swfunzip swfresex";
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

arr("$LIBDIR/macromedia-flashswf-cmfswftags.lib$LIBEXT","cmfswftags.o");
cppr("cmfswftags.o","cmfswftags.cpp");

arr("$LIBDIR/macromedia-flashswf-cmfswfunzip.lib$LIBEXT","cmfswfunzip.o");
cppr("cmfswfunzip.o","cmfswfunzip.cpp");

arr("$LIBDIR/macromedia-flashswf-cmfswfzip.lib$LIBEXT","cmfswfzip.o");
cppr("cmfswfzip.o","cmfswfzip.cpp");

lud("swfzip","swfzip.o $LIBDIR/macromedia-flashswf-cmfswfzip.lib$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT $LIBDIR/common-zlib$LIBEXT");
cppr("swfzip.o","swfzip.cpp");

lud("swfunzip","swfunzip.o $LIBDIR/macromedia-flashswf-cmfswfunzip.lib$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT $LIBDIR/common-zlib$LIBEXT");
cppr("swfunzip.o","swfunzip.cpp");

lud("swftags","swftags.o $LIBDIR/macromedia-flashswf-cmfswftags.lib$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT $LIBDIR/common-zlib$LIBEXT");
cppr("swftags.o","swftags.cpp");

lud("swfresex","swfresex.o $LIBDIR/macromedia-flashswf-cmfswftags.lib$LIBEXT $LIBDIR/common-SomeFileStdio$LIBEXT $LIBDIR/common-types$LIBEXT");
cppr("swfresex.o","swfresex.cpp");

require "$RELTOP/makstep3.PL";

