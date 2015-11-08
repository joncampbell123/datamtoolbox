
$has_src=0;

# generate "make all" conditions
if ($TARGET =~ /\.a$/) {
	print MAK "all:: $LIBDIR/$TARGET $OTHERTARGETS\n";
}
else {
	print MAK "all:: $TARGET $OTHERTARGETS\n";
}
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
foreach $i (%SRCFILES) {
	if ($i ne "") {
		$has_src=1;
		if ($i =~ /\.*$/) {
			$j = $i;
			$j =~ s/\..*$/\.o/;
			print MAK "\trm -f $j\n";
		}
	}
}
if ($MORECLEAN ne "") {
	print MAK "\trm -f $MORECLEAN\n";
}
print MAK "\n";

# generate "make distclean" conditions
print MAK "distclean:: clean\n";
foreach $i (%DIRS) {
	if ($i ne "") {
		print MAK "\tmake -i -C $i distclean\n";
	}
}
if ($has_src == 1 and $TARGET ne "") {
	if ($TARGET =~ /\.a$/) {
		print MAK "\trm -f $LIBDIR/$TARGET\n";
	}
	else {
		print MAK "\trm -f $TARGET\n";
	}
}
print MAK "\trm -f Makefile\n";
print MAK "\n";

$_OBJS = "";
$_LIBS = "";
foreach $i (%LIBS) {
	if ($i ne "") {
		print MAK "$LIBDIR/$i$LIBEXT:\n\t";
		$_LIBS .= "$LIBDIR/$i$LIBEXT ";
		if (	$i eq "common-SomeFileStdio" or
			$i eq "common-types" or
			$i eq "common-BlockDeviceFile") {
			print MAK "make -C $RELTOP/common";
		}
		elsif (	$i eq "apple-common-macintosh") {
			print MAK "make -C $RELTOP/Apple/common";
		}
		elsif (	$i eq "apple-image-diskcopy") {
			print MAK "make -C $RELTOP/Apple/Image-DiskCopy";
		}
		elsif (	$i eq "microsoft-msdosexe") {
			print MAK "make -C $RELTOP/Microsoft/MSDOS-EXE";
		}
		else {
			print MAK "echo Unknown library $i!";
		}

		print MAK "\n\n";
	}
}

if ($has_src == 1) {
	foreach $i (%SRCFILES) {
		if ($i ne "") {
			if ($i =~ /\.*$/) {
				$j = $i;
				$j =~ s/\..*$/\.o/;
				print MAK "$j: $i\n";
				if ($i =~ /\.c/) {
					print MAK "\tgcc $MORECFLAGS -c -o $j $i\n\n";
				}
				else {
					print MAK "\tg++ $MORECFLAGS -c -o $j $i\n\n";
				}
			}
		}
	}

	if ($TARGET =~ /\.a$/) {
		print MAK "$LIBDIR/$TARGET:";
	}
	else {
		print MAK "$TARGET:";
	}

	if ($MOREOBJS ne "") {
		$_OBJS = " $MOREOBJS";
	}
	else {
		$_OBJS = "";
	}

	foreach $i (%SRCFILES) {
		if ($i ne "") {
			if ($i =~ /\.*$/) {
				$j = $i;
				$j =~ s/\..*$/\.o/;
				print MAK " $j";
				$_OBJS .= " $j";
			}
		}
	}

	foreach $i (%LIBS) {
		if ($i ne "") {
			print MAK " $LIBDIR/$i$LIBEXT";
		}
	}
	print MAK "\n";

	if ($TARGET =~ /\.a$/) {
		print MAK "\trm -f $LIBDIR/$TARGET\n";
		print MAK "\tar r $LIBDIR/$TARGET $_OBJS\n";
	}
	else {
		print MAK "\tg++ -o $TARGET$_OBJS $_LIBS\n\n";
	}
}

return 1;

