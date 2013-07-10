#!/usr/bin/perl
use strict;
use warnings;
my $io_calls_template = 'io-calls.c';
my $io_wrapper = 'wrapper_func_auto.c';
open (my $in_fh,'<',$io_calls_template) or die "fukk: $io_calls_template";
my %type = (
    PATHNAME => 'char*',
    FD       => 'int',
    DIR      => 'DIR',
    FILE     => 'FILE',
);
my @orig_ptr;
my @orig_init;
my %func_i;
my @funcs;
my $vaforward=0;
my ($vac,@vat,@van);
my $ret_type;
my $func_name;
my (@argt,@argn);
my $in_proto = 0;

open(my $header_fh,'>','real_func_auto.h');
print $header_fh <<"CCODE";
#ifndef REAL_FUNC_AUTO_H
#define REAL_FUNC_AUTO_H
CCODE

sub function_process() {
    my @func_args;
    my @nonvar_args;
    die "no argt" if ($#argt<0);
    for (my $i=0; $i<=$#argt; $i++) {
        if ($argn[$i] eq '') {
            push @func_args,$argt[$i];
        } else {
            push @func_args,"$argt[$i] $argn[$i]";
            if ($argn[$i] =~ /\(\*([^\)]+)/) {
                push @nonvar_args,$1;
            } else {
                push @nonvar_args,$argn[$i];
            }
        }
    }
    my $func_arg=join(', ',@func_args);
    my $orig_func_name="__real_${func_name}";
    my $orig_func="$ret_type (*$orig_func_name)($func_arg);";
    print $header_fh "extern $orig_func\n";
    push @orig_ptr,$orig_func;
    push @orig_init,"$orig_func_name = dlsym(RTLD_NEXT, \"$func_name\");";
    push @orig_init,"if ($orig_func_name == NULL) { fprintf(stderr,\"cannot resolve $func_name\\n\");exit(1); };";
    my $chaincall_arg=join(', ',@nonvar_args);
    my $vafunc = 0;
    if ($argt[-1] eq '...') {
        $vafunc = 1;
        if ($vaforward) {
            $orig_func_name="__real_v${func_name}";
            $chaincall_arg.=", argp";
        } else {
            $chaincall_arg.=", " . join(", ",@van);
        }
    } else {
        $vaforward = 0;
    }
    my $void_ret = ($ret_type eq 'void');
    unless (exists $func_i{$func_name}) {
        my $funcbody='';
        $funcbody.="$ret_type $func_name($func_arg) {\n";
        $funcbody.="    $ret_type retval;\n"                                   unless ($void_ret);
        $funcbody.="    va_list argp;\n"                                       if     ($vafunc);
        if ($vafunc and (! $vaforward)) {
            for (my $i=0;$i<=$#vat;$i++) {
                $funcbody.="    $vat[$i] $van[$i] = 0;\n";
            }
            $funcbody.="    int va_count = $vac;\n";
        }
        $funcbody.="    va_start(argp,$argn[-2]);\n"                           if     ($vafunc);
        if ($vafunc and (! $vaforward)) {
            for (my $i=0;$i<=$#vat;$i++) {
                $funcbody.="    if (va_count>$i) $van[$i]=va_arg(argp,$vat[$i]);\n";
            }
        }
        $funcbody.="    __real_fprintf(stderr,\"called '$func_name'\\n\");\n";
        $funcbody.="    retval = $orig_func_name($chaincall_arg);\n"           unless ($void_ret);
        $funcbody.="    $orig_func_name($chaincall_arg);\n"                    if     ($void_ret);
        $funcbody.="    va_end(argp);\n"                                       if     ($vafunc);
        $funcbody.="    return(retval);\n"                                     unless ($void_ret);
        $funcbody.="}\n\n";
        push @funcs,$funcbody;
        $func_i{$func_name}=$#funcs;
    }
}

my $lineno = 0;
my $in_preamble = 0;
while (<$in_fh>) {
    $lineno++;
    chomp;
    if (/^\/\/begin_preamble/) {
        $in_preamble = 1;
        next;
    }
    if ($in_preamble) {
        if (/^\/\/end_preamble/) {
            $in_preamble = 0;
            next;
        }
        print $header_fh "$_\n";
        next;
    }
    if (/^\/\/vaforward/) {
        $vaforward=1;
        next;
    }
    if (/^\/\/vac:\s+(.+?)\s*$/) {
        $vac=$1;
        next;
    }
    if (/^\/\/vat:\s+(.+?)\s*$/) {
        push @vat,$1;
        next;
    }
    if (/^\/\/van:\s+(.+?)\s*$/) {
        push @van,$1;
        next;
    }
    if (/^\/\// or /^\s*$/) {
        push @orig_ptr,$_;
        push @orig_init,$_;
        push @funcs,$_;
        print $header_fh "$_\n";
        next;
    }
    if (/^#/) {
        push @orig_ptr,$_;
        next;
    }
    s/^\s*//;
    s/,?\s*$//;
    if ($_ eq ');') {
        my $andiff = $#argt-$#argn;
        if (($#argt>=0) and ($andiff > 0)) {
            push @argn,(('') x $andiff);
        }
        function_process();
        $vaforward=0;
        $vac=undef;
        @vat=();
        @van=();
        @argt=();
        @argn=();
        $func_name=undef;
        $in_proto=0;
    } elsif ($in_proto == 3) {
        push @argn,$_;
        $in_proto = 2;
    } elsif ($in_proto == 2) {
        push @argt,$_;
        $in_proto = 3 if ($_ ne '...');
    } elsif ($in_proto == 1) {
        if (s/\($//) {
            $func_name=$_;
            $in_proto=2;
        } else {
            printf STDERR "parse error: %3d %d '%s'\n",$lineno,$in_proto,$_;
            $in_proto=2;
        }
    } elsif ($in_proto == 0) {
        $ret_type=$_;
        $in_proto=1;
    }
}
print $header_fh <<"CCODE";
#endif
CCODE
close($header_fh);

my $out_fh;
open($out_fh,'>','real_func_auto.c');
print $out_fh <<"CCODE";
#include "real_func_auto.h"
#include <dlfcn.h>
#include <stdlib.h>

CCODE
print $out_fh join("\n",@orig_ptr) . "\n";
print $out_fh <<"CCODE";

void __attribute__ ((constructor(200))) __real_func_auto_init(void) {
CCODE

print $out_fh "    " . join("\n    ",@orig_init) . "\n";
print $out_fh <<"CCODE";
}
CCODE

open ($out_fh,'>',$io_wrapper) or die "fukk: $io_wrapper";
print $out_fh <<"CCODE";
#include "real_func_auto.h"
#include "stdlib.h"
#include <stdarg.h>
CCODE
foreach my $func (@funcs) {
    print $out_fh "$func\n";
}
