#!/usr/bin/perl
use strict;
use warnings;
my $io_calls_template   = shift @ARGV;
my $wrapper_func_manual = shift @ARGV;
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
my @autowrap;
my $need_khiter = 0;
my @autoerr;

push @orig_init,"//first resolve fprintf, so it can be used for log messages";
push @orig_init,"__real_fprintf = dlsym(RTLD_NEXT, \"fprintf\");";
push @orig_init,"if (__real_fprintf == NULL) { fprintf(stderr,\"cannot resolve fprintf\\n\");exit(1); };";

open(my $header_fh,'>','real_func_auto.h');
print $header_fh <<"CCODE";
#ifndef REAL_FUNC_AUTO_H
#define REAL_FUNC_AUTO_H
CCODE

sub function_process() {
    my @func_args;
    my @nonvar_args;
    my @nonvar_argt;
    die "no argt" if ($#argt<0);
    for (my $i=0; $i<=$#argt; $i++) {
        if ($argn[$i] eq '') {
            push @func_args,$argt[$i];
        } else {
            push @func_args,"$argt[$i] $argn[$i]";
            if ($argn[$i] =~ /^\((\*+)([^\)]+)/) {
                push @nonvar_args,$2;
                push @nonvar_argt,'function*';
            } else {
                push @nonvar_argt, $argt[$i];
                push @nonvar_args, $argn[$i];
            }
        }
    }
    my $func_arg=join(', ',@func_args);
    my $orig_func_name="__real_${func_name}";
    my $orig_func="$ret_type (*$orig_func_name)($func_arg);";
    print $header_fh "extern $orig_func\n";
    push @orig_ptr,$orig_func;
    push @orig_init,"$orig_func_name = dlsym(RTLD_NEXT, \"$func_name\");";
    push @orig_init,"if ($orig_func_name == NULL) { __real_fprintf(stderr,\"cannot resolve $func_name\\n\");exit(1); };";
    my $chaincall_arg=join(', ',@nonvar_args);
    my $vafunc = 0;
    my @all_argn = @nonvar_args;
    my @all_argt = @nonvar_argt;
    if ($argt[-1] eq '...') {
        $vafunc = 1;
        if ($vaforward) {
            $orig_func_name="__real_v${func_name}";
            $chaincall_arg.=", argp";
        } else {
            push @all_argn, @van;
            push @all_argt, @vat;
            $chaincall_arg.=", " . join(", ",@van);
        }
    } else {
        $vaforward = 0;
    }
    # construct debug print statement
    my @d_fstring;
    my @d_arglist;
    my @pathname_args;
    my @file_args;
    my @fd_args;
    my @dir_args;
    for (my $i=0;$i<=$#all_argn;$i++) {
        if ($all_argt[$i] =~ /PATHNAME$/) {
            push @d_fstring, "'%s'";
            push @d_arglist, $all_argn[$i];
            push @pathname_args,$all_argn[$i];
        } elsif ($all_argt[$i] =~ /FILE\*$/) {
            push @d_fstring,'%p';
            push @d_arglist, $all_argn[$i];
            push @file_args, $all_argn[$i];
        } elsif ($all_argt[$i] =~ /DIR\*$/) {
            push @d_fstring,'%p';
            push @d_arglist, $all_argn[$i];
            push @dir_args, $all_argn[$i];
        } elsif ($all_argt[$i] =~ /\*$/) {
            push @d_fstring,'%p';
            push @d_arglist, $all_argn[$i];
        } elsif ($all_argt[$i] =~ /FD$/) {
            push @d_fstring,'%d';
            push @d_arglist,'(int)' . $all_argn[$i];
            push @fd_args,$all_argn[$i];
        } elsif ($all_argt[$i] =~ /(?:off_t|off64_t|size_t|int)$/) {
            push @d_fstring,'%d';
            push @d_arglist,'(int)' . $all_argn[$i];
        } elsif ($all_argt[$i] =~ /long$/) {
            push @d_fstring,'%ld';
            push @d_arglist,$all_argn[$i];
        } elsif ($all_argt[$i] =~ /mode_t$/) {
            push @d_fstring,'%4o';
            push @d_arglist,'(int)' . $all_argn[$i];
        } else {
            push @d_fstring,"arg$i ($all_argt[$i] $all_argn[$i])";
        }
    }
    my $d_option = '"(' . join(', ',@d_fstring) . ')"';
    $d_option .= ', ' . join(', ',@d_arglist) if ($#d_arglist >= 0);

    my $void_ret = ($ret_type eq 'void');
    unless (exists $func_i{$func_name}) {
        my $funcbody='';
        $need_khiter |= (($#file_args >= 0) or ($#fd_args >= 0) or ($#dir_args >= 0));
        $funcbody.="$ret_type $func_name($func_arg) {\n";
        $funcbody.="    int need_to_wrap = 0;\n";
        $funcbody.="    khiter_t k;\n"                                         if     ($need_khiter);
        $funcbody.="    // khiter_t k;\n"                                      unless ($need_khiter);
        $funcbody.="    int old_errno;\n";
        $funcbody.="    $ret_type retval;\n"                                   unless ($void_ret);
        $funcbody.="    va_list argp;\n"                                       if     ($vafunc);
        if ($vafunc and (! $vaforward)) {
            for (my $i=0;$i<=$#vat;$i++) {
                $funcbody.="    $vat[$i] $van[$i] = 0;\n";
            }
            $funcbody.="    int va_count = $vac;\n";
        }
        for (my $i=0;$i<=$#pathname_args;$i++) {
            $funcbody.="    PATHNAME scr_$pathname_args[$i]=NULL;\n";
        }
        for (my $i=0;$i<=$#file_args;$i++) {
            $funcbody.="    h5fd_t * scr_$file_args[$i]=NULL;\n";
        }
        for (my $i=0;$i<=$#fd_args;$i++) {
            $funcbody.="    h5fd_t * scr_$fd_args[$i]=NULL;\n";
        }
        for (my $i=0;$i<=$#dir_args;$i++) {
            $funcbody.="    h5dd_t * scr_$dir_args[$i]=NULL;\n";
        }
        $funcbody.="    va_start(argp,$argn[-2]);\n"                           if     ($vafunc);
        if ($vafunc and (! $vaforward)) {
            for (my $i=0;$i<=$#vat;$i++) {
                $funcbody.="    if (va_count>$i) $van[$i]=va_arg(argp,$vat[$i]);\n";
            }
        }
        for (my $i=0;$i<=$#pathname_args;$i++) {
            $funcbody.="    need_to_wrap|=((scr_$pathname_args[$i]=path_below_scratch($pathname_args[$i]))!=NULL);\n";
        }
        for (my $i=0;$i<=$#file_args;$i++) {
            $funcbody.="    k=kh_get(WFILE,wrapper_files,(PTR2INT)$file_args[$i]);\n";
            $funcbody.="    if (k!=kh_end(wrapper_files)) {\n";
            $funcbody.="        need_to_wrap|=1;\n";
            $funcbody.="        scr_$file_args[$i]=kh_value(wrapper_files,k);\n";
            $funcbody.="    };\n";
        }
        for (my $i=0;$i<=$#fd_args;$i++) {
            $funcbody.="    k=kh_get(WFD,wrapper_fds,(int)$fd_args[$i]);\n";
            $funcbody.="    if (k!=kh_end(wrapper_fds)) {\n";
            $funcbody.="        need_to_wrap|=1;\n";
            $funcbody.="        scr_$fd_args[$i]=kh_value(wrapper_fds,k);\n";
            $funcbody.="    };\n";
        }
        for (my $i=0;$i<=$#dir_args;$i++) {
            $funcbody.="    k=kh_get(WDIR,wrapper_dirs,(PTR2INT)$dir_args[$i]);\n";
            $funcbody.="    need_to_wrap|=(k!=kh_end(wrapper_dirs));\n";
            $funcbody.="    if (k!=kh_end(wrapper_dirs)) {\n";
            $funcbody.="        need_to_wrap|=1;\n";
            $funcbody.="        scr_$dir_args[$i]=kh_value(wrapper_dirs,k);\n";
            $funcbody.="    };\n";
        }
        $funcbody.="    LOG_DBG(\"called \"$d_option);\n";
        $funcbody.="    if (need_to_wrap) {\n";
        if ($#autowrap >= 0) {
            $funcbody.="        LOG_INFO(\"autowrap\"$d_option);\n";
            foreach (@autowrap) {
                $funcbody.="        $_\n";
            }
        } else {
            $funcbody.="        LOG_ERR(\"wrapping_needed\"$d_option);\n";
            $funcbody.="        retval = $orig_func_name($chaincall_arg);\n"       unless ($void_ret);
            $funcbody.="        $orig_func_name($chaincall_arg);\n"                if     ($void_ret);
        }
        $funcbody.="    } else {\n";
        $funcbody.="        retval = $orig_func_name($chaincall_arg);\n"       unless ($void_ret);
        $funcbody.="        $orig_func_name($chaincall_arg);\n"                if     ($void_ret);
        $funcbody.="    }\n";
        $funcbody.="    va_end(argp);\n"                                       if     ($vafunc);
        for (my $i=0;$i<=$#pathname_args;$i++) {
            $funcbody.="    free(scr_$pathname_args[$i]);\n";
        }
        $funcbody.="    return(retval);\n"                                     unless ($void_ret);
        $funcbody.="errlabel:\n";
        $funcbody.="    old_errno=errno;\n";
        $funcbody.="    va_end(argp);\n"                                       if     ($vafunc);
        foreach (@autoerr) {
            $funcbody.="    $_\n";
        }
        for (my $i=0;$i<=$#pathname_args;$i++) {
            $funcbody.="    free(scr_$pathname_args[$i]);\n";
        }
        $funcbody.="    errno=old_errno;\n";
        $funcbody.="    return(retval);\n"                                     unless ($void_ret);
        $funcbody.="}\n\n";
        push @funcs,$funcbody;
        $func_i{$func_name}=$#funcs;
    }
}

open(my $wfm_fh,'-|','ctags','-x','--c-kinds=f',$wrapper_func_manual) or die "error calling ctags on '$wrapper_func_manual'";
while (<$wfm_fh>) {
    chomp;
    $func_i{$1}=-1 if (/^(\S+)\s+/);
}
close($wfm_fh);

my $lineno = 0;
my $in_preamble = 0;
open (my $in_fh,'<',$io_calls_template) or die "fukk: $io_calls_template";
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
    if (/^\/\/autowrap:\s+(.+?)\s*$/) {
        push @autowrap,$1;
        next;
    }
    if (/^\/\/autoerr:\s+(.+?)\s*$/) {
        push @autoerr,$1;
        next;
    }
    if (/^\/\/need_khiter\s*$/) {
        $need_khiter = 1;
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
        @autowrap =();
        @autoerr  =();
        $func_name=undef;
        $in_proto=0;
        $need_khiter=0;
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

open ($out_fh,'>','wrapper_func_auto.c') or die "fukk: 'wrapper_func_auto.c'";
print $out_fh <<"CCODE";
#include "real_func_auto.h"
#include "wrapper_func.h"
#include "logger.h"
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
CCODE
foreach my $func (@funcs) {
    print $out_fh "$func\n";
}
