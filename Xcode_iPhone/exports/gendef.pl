#!/usr/bin/perl
#
# Program to take a set of header files and generate DLL export definitions

while ( ($file = shift(@ARGV)) ) {
	if ( ! defined(open(FILE, $file)) ) {
		warn "Couldn't open $file: $!\n";
		next;
	}
	$printed_header = 0;
	$file =~ s,.*/,,;
	while (<FILE>) {
		if ( / DECLSPEC.*SDLCALL ([^\s\(]+)/ ) {
			print "\t_$1\n";
		}
	}
	close(FILE);
}
