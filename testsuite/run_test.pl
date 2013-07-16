#!/usr/bin/perl
use strict;
use warnings;
use Time::HiRes;
use Getopt::Long;
my $wrapped = 0;
my $wrappednoauto = 0;
my $wrappeddbg = 0;
my $np_default = 1;
my $dir = shift @ARGV;
die "no dir" unless ((defined $dir) and (-d $dir));
$dir =~ s/\/+$//;
if ($dir=~/wrapped$/) {
    $wrapped=1;
}
if ($dir=~/wrappednoauto$/) {
    $wrappednoauto=1;
}
if ($dir=~/wrappeddbg$/) {
    $wrappeddbg=1;
}
if ($dir=~/\.np(\d+)\./) {
    $np_default = $1;
}
my $test_index=0;
my $time_fh;
my @wrap_ro_stack;
my %wrap_ro_pos;
my $np=$np_default;
my $espresso_dir="/home/glawe/build/espresso-sc-search";
if ($dir=~/\.nopara\./) {
    $espresso_dir="/home/glawe/build/espresso-sc-search-nopara";
    $np=$np_default=1;
}
if ($dir=~/\.noomp\./) {
    $espresso_dir="/home/glawe/build/espresso-sc-search-noomp";
}
my %code = (
    'pw' =>      "$espresso_dir/PW/pw.x",
    'pp' =>      "$espresso_dir/PP/pp.x",
    'dos' =>     "$espresso_dir/PP/dos.x",
    'projwfc' => "$espresso_dir/PP/projwfc.x",
);
sub test_step {
    my ($filebase)=@_;
    my ($starttime,$endtime);
    my @fb = split(/\./,$filebase);
    my $cmd = "mpirun -np $np ";
    if ($wrapped or $wrappeddbg) {
        if ($wrapped) {
            $cmd.="-x LD_PRELOAD=/home/glawe/build/hdf5fs-wrapper/h5fs-wrapper.so ";
        } else {
            $cmd.="-x LD_PRELOAD=/home/glawe/build/hdf5fs-wrapper/wrapper-debugwrap/h5fs-wrapper.so ";
        }
        if ($#wrap_ro_stack>=0) {
            $cmd .= "-x H5FS_RO=" . join(':',@wrap_ro_stack) . " ";
        }
    } elsif ($wrappednoauto) {
        $cmd.="-x LD_PRELOAD=/home/glawe/build/hdf5fs-wrapper/wrapper-noautowrap/h5fs-wrapper.so ";
    }
    $cmd.=$code{$fb[-1]};
    my $full_cmd = "$cmd < ${filebase}.IN 2>${filebase}.ERR >${filebase}.OUT";
    print STDERR "$full_cmd\n"; 
    $starttime=Time::HiRes::time();
    system($full_cmd);
    $endtime=Time::HiRes::time();
    printf $time_fh "%50s %15.2f\n",$filebase,($endtime-$starttime);

    my $md5_out;
    open ($md5_out,'>',"${filebase}.md5sum_size");
    if ($wrapped or $wrappeddbg) {
        my $new_h5="$filebase.h5";
        system('/home/glawe/build/hdf5fs-wrapper/h5fs-repack',$new_h5,sort(glob('scratch*.h5')),'base',@wrap_ro_stack);
        push @wrap_ro_stack,$new_h5;
        $wrap_ro_pos{$new_h5}=$#wrap_ro_stack;
        my $md5_fh;
        open($md5_fh,'-|','/home/glawe/build/hdf5fs-wrapper/h5fs-md5sum-size',@wrap_ro_stack);
        my @md5_list = <$md5_fh>;
        close($md5_fh);
        print $md5_out sort { lc($a) cmp lc($b) } @md5_list;
        unlink(glob 'scratch*.h5');
        mkdir("SCRATCH.${filebase}");
        chdir("SCRATCH.${filebase}");
        system("/home/glawe/build/hdf5fs-wrapper/h5fs-unpack",map { "../$_" } @wrap_ro_stack);
        chdir("..");
        if ($wrappeddbg) {
            my ($md5_in,$md5_out_dbg);
            open ($md5_out_dbg,'>',"${filebase}.dbg.md5sum_size");
            open($md5_in,'-|', 'find SCRATCH -type f | sort | xargs md5sum');
            while (<$md5_in>) {
                chomp;
                my ($sum,$filename) = split;
                my $size = (-s $filename);
                $filename=~s/^SCRATCH\///;
                printf $md5_out_dbg "%-40s %32s %20d\n",$filename,$sum,$size;
            }
            close($md5_in);
            close($md5_out_dbg);
            system('cp','-a','SCRATCH',"SCRATCH.dbg.${filebase}");
        }
    } else {
        my $md5_in;
        open($md5_in,'-|', 'find SCRATCH -type f | sort | xargs md5sum');
        while (<$md5_in>) {
            chomp;
            my ($sum,$filename) = split;
            my $size = (-s $filename);
            $filename=~s/^SCRATCH\///;
            printf $md5_out "%-40s %32s %20d\n",$filename,$sum,$size;
        }
        close($md5_in);
        system('cp','-a','SCRATCH',"SCRATCH.${filebase}");
    }
    close($md5_out);
    $test_index++;
}
my $time_file = $dir;
$time_file=~s/\/*$//;
$time_file.=".time";
open($time_fh,'>',$time_file) or die "error opening timefile '$time_file'";
my ($suite_starttime,$suite_endtime);
chdir "$dir";
system("clean_manifest");
$suite_starttime=Time::HiRes::time();

$np=$np_default;
test_step("scf.pw");
$np=1;
test_step("cube.ccharge.pp");
system("/home/glawe/build/bader-0.28/bader -p atom_index ccharge.cube");
test_step("localization_debug.vtk.pp");
$np=$np_default;
test_step("smearing.dos");
test_step("projwfc");
test_step("tetrahedra.nscf.pw");
test_step("tetrahedra.dos");

$suite_endtime=Time::HiRes::time();
printf $time_fh "%20s %15.2f\n","total",($suite_endtime-$suite_starttime);
