#!/usr/bin/perl

$size=`cd $ARGV[0];ls -sh flexbackup-$ARGV[1].tar.gz`;
$size =~ s/^\s+//;
chomp($size);

open(LSM,'>flexbackup.lsm');
open(TEMP,'flexbackup.lsm.template');
while (<TEMP>) {
    s/^(\s+)SIZE/$1$size/;
    print LSM;
}
