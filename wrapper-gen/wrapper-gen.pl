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
    DIR      => 'DIR',
    FILE     => 'FILE',
);
my @orig_ptr;
my @orig_init;
my %func_i;
my @funcs;
my $line;
my $vaforward=0;
my (@vat,@van);
my $ret_type;
my $func_name;
my (@argt,@argn);
my $in_proto = 0;

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
    my $chaincall_arg=join(', ',@nonvar_args);
    my $orig_func_name="__real_${func_name}";
    my $orig_func="$ret_type (*$orig_func_name)($func_arg);";
    push @orig_ptr,$orig_func;
    push @orig_init,"$orig_func_name = dlsym(RTLD_NEXT, \"$func_name\");";
    $orig_func_name="__real_v${func_name}" if ($vaforward);
    unless (exists $func_i{$func_name}) {
        my $funcbody='';
        if ($argt[-1] ne '...') {
            $funcbody=<<"            CCODE";
$ret_type $func_name($func_arg) {
    __real_fprintf(stderr,"called '$func_name'\\n");
    return($orig_func_name($chaincall_arg));
}
            CCODE
        } elsif ($vaforward) {
            $funcbody=<<"            CCODE";
$ret_type $func_name($func_arg) {
    va_list argp;
    $ret_type retval;
    va_start(argp,$argn[-2]);
    __real_fprintf(stderr,"called '$func_name'\\n");
    retval=$orig_func_name($chaincall_arg, argp);
    va_end(argp);
    return(retval);
}
            CCODE
        } else {
            $funcbody=<<"            CCODE";
$ret_type $func_name($func_arg) {
    va_list argp;
    $ret_type retval;
    __real_fprintf(stderr,"called '$func_name'\\n");
            CCODE
            for (my $i=0;$i<=$#vat;$i++) {
                $funcbody.="    $vat[$i] $van[$i];\n";
            }
            $funcbody.=<<"            CCODE";
    va_start(argp,$argn[-2]);
    va_end(argp);
    return(retval);
}
            CCODE
        }
        push @funcs,$funcbody;
        $func_i{$func_name}=$#funcs;
    }
}

my $lineno = 0;
while ($line = <$in_fh>) {
    $lineno++;
    $_=$line;
    chomp;
    if (/^\/\/vaforward/) {
        $vaforward=1;
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
            print STDERR "$func_name: andiff $andiff\n";
            push @argn,(('') x $andiff);
        }
        function_process();
        $vaforward=0;
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
print $out_fh join("\n",@orig_ptr) . "\n";
print $out_fh <<"    CCODE";
void __attribute__ ((constructor)) wrapper_init(void) {
    CCODE
print $out_fh "    " . join("\n    ",@orig_init) . "\n";
print $out_fh <<"    CCODE";
}
    CCODE
foreach my $func (@funcs) {
    print $out_fh "$func\n";
}
