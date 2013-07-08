#!/usr/bin/perl
use strict;
use warnings;
my $io_calls_template = 'io-calls.c';
my $io_wrapper = 'io-wrapper.c';
open (my $in_fh,'<',$io_calls_template) or die "fukk: $io_calls_template";
open (my $out_fh,'>',$io_wrapper) or die "fukk: $io_wrapper";
my %type = (
    PATHNAME => 'char*',
    FD       => 'int',
    SDIRENT  => 'struct dirent',
    DIR      => 'DIR',
    FILE     => 'FILE',
);
my @orig_ptr;
my @orig_init;
my @funcs;
my $line;
while ($line = <$in_fh>) {
    $_=$line;
    chomp;
    if (/^\/\// or /^\s+$/) {
        push @orig_ptr,$_;
        push @orig_init,$_;
        push @funcs,$_;
        next;
    }
    if (/^#/) {
        push @orig_ptr,$_;
        next;
    }
    if (/^(.+)\s+(\S+)\((.*)\);\s*$/) {
        print "# $_\n";
        print "$2\n  $1\n  $3\n";
    }
}
print $out_fh join("\n",@orig_ptr) . "\n";
