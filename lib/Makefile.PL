#!/usr/bin/perl

# create Makefile
open(MAK,">Makefile") or die("Cannot create Makefile");

# directories we target
%DIRS = ();

# generate "make all" conditions
print MAK "all::\n";
foreach $i (%DIRS) {
	if ($i ne "") {
		print MAK "\tmake -C $i\n";
	}
}
print MAK "\n";

# generate "make clean" conditions
print MAK "clean::\n";
foreach $i (%DIRS) {
	if ($i ne "") {
		print MAK "\tmake -C $i clean\n";
	}
}
print MAK "\n";

# generate "make distclean" conditions
print MAK "distclean::\n";
foreach $i (%DIRS) {
	if ($i ne "") {
		print MAK "\tmake -i -C $i distclean\n";
	}
}
print MAK "\trm -f Makefile\n";
print MAK "\trm -f *.a *.lib\n";
print MAK "\n";

# done
close(MAK);

# generate makefiles in subdirectories
foreach $i (%DIRS) {
	print "Creating makefile in $i\n";
	chdir $i or die("Cannot chdir to $i");
	exec "perl", "Makefile.PL" or die("Cannot exec Makefile.PL in $i");
	chdir ".." or die("Cannot return from $i");
}

