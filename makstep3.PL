
# done
close(MAK);

# generate makefiles in subdirectories
foreach $i (%DIRS) {
	if ($i ne "") {
		print "Creating makefile in $i\n";
		chdir $i or die("Cannot chdir to $i");
		system("perl Makefile.PL");
		chdir ".." or die("Cannot return from $i");
	}
}

return 1;

