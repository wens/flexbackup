#!/usr/bin/perl -w
#
# flexbackup.cgi
# cgi frontend for the flexbackup tape
# archival program
# Copyright 2002, "Linux Systems Engineers / PC Doctor SE, Inc"
# Released under GPL 2.0 By Andrew McRory <amacc@linuxsys.com>
# Written by Brad Andrews <brad@...>
# version 0.9a
# Wed Oct 9 2002
######################################################################

use strict;
use CGI;
use CGI::Carp "fatalsToBrowser";
use POSIX;
use NDBM_File;

my( $CGI ) = new CGI;
my( $home_directory ) = "/home/flexbackupweb";
my( $base );
my( $script ) = $CGI -> script_name;
my( $directory );
my( $tape_id );
my( %dir_hash );
my( $passed_file );
my( $tape_device ) = "/dev/st0";
my( $non_rw_tape_device ) = "/dev/nst0";
my( $flexbackup_conf ) = "/etc/flexbackup.conf";


if ( $ARGV[ 0 ] eq 'blind' ) {
# this is so you can run a backup from cron with
# the advanced database features, just run
# 'flexbackup.cgi blind'
    &run_backup;
    exit 0;
}

# This is the dispatch table to determine program state
my( %dispatch_table ) = (
			 'file'             => \&extract_file,
			 'tape_id'          => \&pre_db_crawler,
			 'Backup'           => \&run_backup,
			 'Change Schedule'  => \&change_schedule,
			 'edit_config_page' => \&edit_config_page,
			 'filesystems'      => \&change_conf,
			 'add_filesystem'   => \&change_conf,
			 'Change Options'   => \&change_conf,
			);
my( $param_test ) = 0;
foreach my $keys ( keys( %dispatch_table ) ) {
    if ( $CGI -> param( "$keys" ) ) {
	$param_test++;
	$dispatch_table{ "$keys" } -> ( $CGI -> param( "$keys" ) );
    }
}
if ( $param_test < 1) {
    &browse_archives;
}

sub pre_db_crawler {
    $tape_id = $_[ 0 ];
    &db_crawler;
}

sub browse_archives {
# Main screen and backup initiator
    &html_header;
    my( %temp_archive );
    opendir( DBDIR , "/var/lib/flexbackup" );
    my( @archives_list ) = readdir( DBDIR );
    print qq{<h3>Select a backup to browse</h3>};
    foreach ( @archives_list ) {
	if ( m/^([\d\.]+\.master)\.db$/ ) {
	    tie( %temp_archive,  "NDBM_File", "/var/lib/flexbackup/$1", O_RDONLY , 0600 ) || die "can't tie $1, $!";
	    print qq{<blockquote>};
	    foreach ( sort( keys( %temp_archive ) ) ) {
		print qq{Friendly name: <a href=$script?tape_id=$_>} , $temp_archive{ $_ } , qq{</a><br>\n};
	    }
	    print qq{</blockquote>};
	    untie( %temp_archive );
	}
    }
    print qq{<h3>Or...</h3>Enter a name to backup to or leave blank for date format.<br>},
    $CGI -> start_form( -action => $script ),
    $CGI -> textfield( -name => "friendly_name" ),
    $CGI -> submit( -name => "Backup" ),
    $CGI -> endform,
    qq{<h3>Or...</h3>click <a href=$script?edit_config_page=1>here</a> to edit configuration for backups};
}

sub db_crawler {
# open the db file and read in directories/files from it
    &html_header;
    my( %hash );
    tie( %hash , "NDBM_File" , $CGI -> param( 'tape_id' ) , O_RDONLY , 0600 ) || die "can't tie, $!";
    my( $i , $full_rootlink );
    my( %dir_hash, %file_hash );
    if ( $CGI -> param( 'directory' ) ) {
	$passed_file = $CGI -> param( 'directory' );
    }
    else {
	$passed_file = $hash{ "<base_directory>" };
    }
    my( @rootlink ) = split( m/\// , $passed_file );
    print qq{<table width=100% border=1><tr><td valign=top><h3>Files:</h3>};
    #$passed_file =~ s/(.*)\/$/$1/;
    foreach ( sort( keys( %hash ) ) ) {
	if ( $hash{ $_ } =~ m/^$passed_file([^\/]+\/)$/ ) {
	    $dir_hash{ $hash{ $_ } } = $1;
	}
	#elsif ( $hash{ $_ } =~ m/^$passed_file$/ ) {
	elsif ( $hash{ $_ } =~ m/^$passed_file$/ ) {
	    unless ( $_ eq "<base_directory>" ) {
		my( $tempfile ) = $_;
		$tempfile =~ s/.*\/(.*)/$1/;
		$file_hash{ $_ } = $tempfile;
	    }
	}
    }
    foreach( sort( keys( %file_hash ) ) ) {
	print qq{<a href="$script/} , $file_hash{ $_ } , qq{?tape_id=$tape_id&file=$_">} , $file_hash{ $_ } , qq{</a><br>\n};
    }
    print qq{<p>Click <a href=$script?tape_id=$tape_id&tarfile=$passed_file>here</a> to download directory as a .tar file</td><td valign=top><h3>Subdirectories of } , ( $CGI -> param( 'directory' ) || $passed_file ) , qq{:</h3>};
    foreach ( sort( keys( %dir_hash ) ) ) {
	print "[<a href=$script?tape_id=$tape_id&directory=$_>$dir_hash{ $_ }</a>]<br>\n";
    }
    print qq{</td></tr><tr><td colspan=2><h3>Current Directory:</h3>};
    for ( $i = 0 ; $i < ( scalar( @rootlink ) - 1 ) ; $i++ ) {
	$full_rootlink .= "/$rootlink[ $i ]/";
	$full_rootlink =~ s/\/\/*/\//;
	print qq{<a href=$script?tape_id=$tape_id&directory=$full_rootlink>} , $rootlink[ $i ] , qq{</a>/};
    }
    print pop( @rootlink ),
    qq{<br>\n<a href=$script>Return to archive selector</a></td></tr></table>};
}

sub run_backup {
# run the actual backup (calling flexbackup)
    &html_header;
    print qq{Your backup has begun. It may take up to several hourse to complete, at which time access will be available.<br>};
    my( $start_read , $base_directory , $tape_key , $file_num );
    my( %hash_file , %master_file );
    my( $friendly_name ) = $CGI -> param( 'friendly_name' ) || scalar( localtime() );

    $ENV{'PATH'} = "/usr/bin:/bin";
    $ENV{'BASH_ENV'} = undef;
    $< = $>;
    my( @old_key ) = `flexbackup -toc`;
    my( $old_key ) = '';
    foreach ( @old_key ) {
	if ( $_ =~ m/(\d{12}\.\d\d)/ ) {
	    $old_key = $1;
	}
    }
    if ( $old_key ne '' ) {
	print "$old_key<br>";
	opendir( KILL_DIRECTORY , "/var/lib/flexbackup" );
	my( @kill_files ) = readdir( KILL_DIRECTORY );
	close( KILL_DIRECTORY );
	foreach ( @kill_files ) {
	    m/(.*)/;
	    $_ = $1;
	    print "$_<br>";
	    unless ( index( $_ , $old_key ) == -1 ) {
		unlink( "/var/lib/flexbackup/$_" ) || die "can't unlink $_";
	    }
	}
	`mt -f $tape_device erase`;
	`flexbackup -rmindex $old_key`;
    }
    open( FILE , "flexbackup -fs all|" ) || die "Can't open flexbackup, $!";

    while ( <FILE> ) {
	if ( m/File number (\d)+, index key ([\d\.]+)/ ){
	    $tape_key = $2;
	    $file_num = $1;
	    tie( %master_file,
		 "NDBM_File",
		 "/var/lib/flexbackup/$tape_key.master",
		 O_RDWR|O_CREAT,
		 0660 )
		|| die "Can't tie, $!";
	    print "backing up $tape_key.$file_num";
	    $master_file{ "/var/lib/flexbackup/$tape_key.$file_num" } = "$friendly_name";
	    untie( %master_file );
	    tie( %hash_file,
		 "NDBM_File",
		 "/var/lib/flexbackup/$tape_key.$file_num",
		 O_RDWR|O_CREAT,
		 0660 )
		|| die "Can't tie, $!";
	}
	if ( m/Backup of\: (.*)\s*$/ ) {
	    $base_directory = $1;
	}
	if ( m/([\w\.].*) \-\- / ) {
	    my( $full_name ) = "$base_directory/$1";
	    my( $file_name ) = $1;
	    $full_name =~ m/(.*\/)(.*)/;
	    my( $directory_name ) = $1;
	    #$hash_file{ $full_name } = $directory_name;
	    print "$file_name stored in $directory_name<br>\n";
	    $hash_file{ $file_name } = $directory_name;
	}
    }
    close( FILE );

    $hash_file{ "<base_directory>" } = "$base_directory/";
    untie( %hash_file );
    exit( 0 );
}

sub extract_file {
# extract file from tape and send (single file restore)
    $ENV{'PATH'} = "/usr/bin:/bin";
    $ENV{'BASH_ENV'} = undef;
    $< = $>;
    $_[ 0 ] =~ m/(.*)/;
    chdir( "$home_directory" );
    `mt -t $tape_device rewind`;
    `mt -t $non_rw_tape_device fsf 1`;
    `buffer -m 3m -s 64k -u 100 -t -p 75 -B -i $tape_device | afio -i -y $1 -z -x -D /usr/bin/flexbackup -P gzip -Q -d -Q -q -Z -v -b 64k -`;
    print $CGI -> header( $CGI -> meta( { -http_equiv => 'Content-Type',
					  -content => 'application/x-Binary' } ) );
    open( SEND_FILE , "$home_directory/$1" );
    while ( <SEND_FILE> ) {
	print;
    }
    exit 0;
}

sub html_header {
    print $CGI -> header,
    $CGI -> start_html,"HTML interface to flexbackup<p>\n";
}

sub edit_config_page {
# mess with the flexbackup config file in etc
    &html_header;
    my( $type , $compress , $compr_level , $blksize , $fs_ref ) = &read_flexbackup_conf;
    my( @filesystems ) = @{ $fs_ref };
    my( $hour , $m_tens , $m_ones , $day , $half_day ) = &read_cron;
    print $CGI -> startform( -action => $script ),
    qq{<table border="1"><tr><td bgcolor="#FFEEEE">Scheduled backup time<p>},
    $CGI -> popup_menu( -name => 'schedule_hour',
			-values => [1..12],
			-default => $hour ),
    qq{<b>:</b>},
    $CGI -> popup_menu( -name => 'schedule_minutes_tens',
			-values => [0..5],
			-default => $m_tens ),
    $CGI -> popup_menu( -name => 'schedule_minutes_ones',
			-values => [0..9],
			-default => $m_ones ),
    $CGI -> popup_menu( -name => 'schedule_halfday',
			-values => [ "am" , "pm" ],
			-default => $half_day ),
    qq{</p>every<br>},
    $CGI -> popup_menu( -name => 'schedule_day_of_week',
			-values => [ "Sunday" , "Monday" , "Tuesday",
				     "Wednesday" , "Thursday" , "Friday",
				     "Saturday" , "Weekday" , "Day" ],
			-default => $day ),
    qq{<br>},
    $CGI -> submit( -name => 'Change Schedule' ),
    qq{</td></tr></table><p>},
    qq{<table border="1"><tr><td bgcolor="#EEFFEE">Filesystems to backup<br>\n},
    $CGI -> scrolling_list( -name => 'filesystems',
			    -values => @filesystems,
			    -size=> 5 ),
    $CGI -> textfield( -name => 'add_filesystem' ),
    qq{<br>},
    $CGI -> submit( -name => 'Remove Filesystem' ),
    $CGI -> submit( -name => 'Add Filesystem' ),
    qq{</td>},
    qq{</td></tr></table><p>},
    qq{<b>WARNING:</b> changing these options is not recommended},
    qq{<table border="1" cellpadding="3"><tr>},
    qq{<td bgcolor="#FFFFEE">Backup type<br>\n},
    $CGI -> popup_menu( -name =>'type',
			-values => [ "afio" , "dump" , "tar",
				     "cpio" , "zip" ],
			-default => $type ),
    qq{</td>},
    qq{<td bgcolor="#EEFFFF">Compress type<br>\n},
    $CGI -> popup_menu( -name => 'compress',
			-values => [ "false" , "gzip" , "bzip2",
				     "compress" , "hardware" ],
			-default => $compress ),
    qq{</td></tr>},
    qq{<tr>},
    qq{<td bgcolor="#EEEEEE">Block size<br>\n},
    $CGI -> textfield( -name => 'blksize',
		       -default => $blksize ),
    qq{</td>},
    qq{<td bgcolor="#FFEEFF">Compress level<br>\n},
    $CGI -> popup_menu( -name => 'compress',
			-values => [ 1..9 ],
			-default => $compr_level ),
    qq{</td></tr><tr><td colspan="2" align="center">},
    $CGI -> submit( -name => 'Change Options' ),
    qq{</td</tr></table>},
    $CGI -> endform;
}

sub change_schedule {
# edit flexbackup cron jobs
    my( $hour ) = $CGI -> param( 'schedule_hour' );
    my( $minutes ) = $CGI -> param( 'schedule_minutes_tens' ) . $CGI -> param( 'schedule_minutes_ones' );
    my( $ampm ) = $CGI -> param( 'schedule_halfday' ),
    my( $day  )=  $CGI -> param( 'schedule_day_of_week' );
    if ( $day eq 'Day' ) { $day = '*' };
    if ( $day eq 'Weekday' ) { $day = '1-5' };
    $hour += 12;
    if ( $ampm eq 'am' ) {
	$hour %= 12;
    }
    my( $cron_line ) = "$minutes $hour * * $day /var/www/cgi-bin/flexbackup.cgi ?Backup=Backup\n";
    open( CRON , "/var/spool/cron/root" ) || die "Can't open cron, $!";
    my( $found ) = 0;
    my( @cron_file );
    while ( <CRON> ) {
	unless ( m/^\#/ ) {
	    if ( m/flexbackup/ ) {
		$_ = $cron_line;
		$found++;
	    }
	}
	push( @cron_file , $_ );
    }
    if ( $found == 0 ) {
	push( @cron_file , $cron_line );
    }
    close( CRON );
    open( CRON , ">/var/spool/cron/root" ) || die "Can't open cron, $!";
    foreach ( @cron_file ) {
	print CRON $_;
    }
    close( CRON );
    &edit_config_page;
}

sub read_flexbackup_conf {
# read in config file for flexbackup
    my( $rtype , @rfilesystems , $rcompress , $rcompr_level , $rblksize );
    open( CONF_FILE , "$flexbackup_conf" ) || die "Can't open $flexbackup_conf, $!";
    while ( <CONF_FILE> ) {
	if ( $_ =~ m/^[^\#]*\$type\s*\=\s*[\'\"](.*)[\'\"]/ ) {
	    $rtype = $1;
	}
	elsif ( $_ =~ m/^[^\#]*\$filesystems\[\s*(\d+)\s*\]\s*\=\s*[\'\"](.*)[\'\"]/ ) {
	    $rfilesystems[ $1 ] = $2;
	}
	elsif ( $_ =~ m/^[^\#]*\$compress\s*\=\s*[\'\"](.*)[\'\"]/ ) {
	    $rcompress = $1;
	}
	elsif ( $_ =~ m/^[^\#]*\$compr_level\s*\=\s*[\'\"](.*)[\'\"]/ ) {
	    $rcompr_level = $1;
	}
	elsif ( $_ =~ m/^[^\#]*\$blksize\s*\=\s*[\'\"](.*)[\'\"]/ ) {
	    $rblksize = $1;
	}
    }
    close( CONF_FILE );
    return( $rtype , $rcompress , $rcompr_level , $rblksize , \@rfilesystems );
}

sub read_cron {
# read in cron to determine flexbackup's current schedule
    my( $fb_line , @fb_components , @return_vals );
    open( CRON , "/var/spool/cron/root" );
    while ( <CRON> ) {
	unless ( m/^#/ ) {
		 if ( index( $_ , 'flexbackup' ) != -1 ) {
		     $fb_line = $_;
		 }
	     }
    }
    @fb_components = split( m/\s+/ , $fb_line , 6 );
    $return_vals[ 0 ] = $fb_components[ 1 ] % 12;
    $return_vals[ 1 ] = int( $fb_components[ 0 ] / 10 );
    $return_vals[ 2 ] = $fb_components[ 0 ] % 10;
    if ( $fb_components[ 4 ]  eq '*' ) {
	$return_vals[ 3 ] = 'Day';
    }
    elsif ( $fb_components[ 4 ] eq '1-5' ) {
	$return_vals[ 3 ] = 'Weekday';
    }
    else {
	$return_vals[ 3 ] = $fb_components[ 4 ];
    }
    if ( $fb_components[ 0 ] > 11 ) {
	$return_vals[ 4 ] = 'pm';
    }
    else {
	$return_vals[ 4 ] = 'am';
    }
    return( @return_vals );
}

sub change_conf {
    if ( $CGI -> param( 'add_filesystem' ) ) {
	add_fs( $_[ 0 ] );
    }
    elsif ( $CGI -> param( 'filesystems' ) ) {
	remove_fs( $_[ 0 ] );
    }
}

sub add_fs {
# take on new directory for backup
    open( FB_CONF , "$flexbackup_conf" );
    my( $started , $fs_counter , @filesystems_list , @conf_file , $i );
    while ( <FB_CONF> ) {
	push( @conf_file , $_ );
    }
    for ( $i = 0 ; $i < scalar( @conf_file ) ; $i++ ) {
	if ( $conf_file[ $i ] =~ m/^[^\#]*\$filesystems\[\s*(\d+)\s*\]\s*\=\s*[\'\"](.*)[\'\"]/ ) {
	    $rfilesystems[ $1 ] = $2;
	}
    }
}
