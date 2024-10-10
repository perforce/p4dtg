#! /usr/bin/env perl

################################################################################
## Perforce server trigger for use in conjunction with P4DTG job replication.
##
## This script can optionally:
##
##  1.  Prevent creation of new jobs in Perforce by anyone other than the
##      replication user.
##  2   Prevent modification of read-only fields of jobs in Perforce by anyone
##      other than the replication user.
##  3.  Create newly mirrored jobs using the same name as that in the defect
##      tracker.
##
## To install, add a line to your triggers table like the following:
##
##    jobId form-in job "/p4root/triggers/changeid.pl %user% %formfile%" 
##
## or if you're on Windows, you need to prepend the Perl interpreter like this:
##
##    jobId form-in job "c:\path\to\perl.exe changeid.pl %user% %formfile%" 
##
## Also, don't forget to make the file executable, change the path to the Perl
## interpreter, and set the configuration variables below.
##
## To configure, read and modify the following lines up to the comment that
## reads "END OF CONFIGURATION BLOCK".  You may also need to modify the
## definition of which fields constitute a new job based on your jobspec.  This
## is in the allowed_job() function.
##

## use strict;
## use warnings;
use File::Basename;

my $freeJobCreation = 0; # anyone can create jobs from Perforce. this trumps
                         # the $enforceROFL option.
my $enforceROFL     = 1; # enforce the read-only status of certain fields
my $sameName        = 1; # use the same job name as that in the defect tracker
my $jobPrefix       = "";# text to prepend to the name of new jobs, for use
                         # with $sameName.

# Perforce user that is allowed to create new jobs and edit read-only fields.
# This is the same user that you configured P4DTG to use.
my $p4authuser = "p4dtguser";

# Server connection details.  Needed for "p4 job -o". 
# Assumes the long-lived ticket is in the p4tickets file.
my $p4 = "p4 -p port -u $p4authuser ";

# print debugging information to STDOUT.
my $debug = 0;

# List of job fields that are writable by anyone from within Perforce.
# The ROFL (read-only field list) is much more amusing, but usually more verbose,
# so we use the inverse, the WFL (writable field list.)  Note that the case of
# the field names must be the same as is in your jobspec.
# Typically, these are just the job fields copied or mirrored to Bugzilla.

my %wfl = (
    "BZ_STATUSRESOLUTION" => 1, "BZ_ASSIGNEDTO" => 1,
    "BZ_URL" => 1, "BZ_SEVERITY" => 1, "Summary" => 1,
    "BZ_PRIORITY" => 1, "ReportedBy" => 1,
    "BZ_VERSION" => 1
    );

# runtime absolute path of this script.
my $scriptname = abs_path( $0 );
my $shortname = basename ($scriptname);
my $logfile;

if ($ENV{LOGS}) {
    $logfile = "$ENV{LOGS}/$shortname.log";
} else {
    $logfile = "$scriptname.log";
}

## END OF CONFIGURATION BLOCK
################################################################################
## 

# turn off output buffering.
$| = 1;

use Carp                  ;
use strict                ;
use POSIX  qw( strftime ) ;
use Cwd    'abs_path'     ;

# takes the %formfile% data from is_new_job() and parses it into a hash.
sub read_form_var
{
    my @ar   = @_ ;
    my %hash      ;
    my $lino = 0  ;

    while( @ar )
    {
	$_ = shift @ar;
	$lino += 1;
	if( $debug ) { print "starting.\n"; }

	if( /^#/ || /^$/ )
	{
	    if( $debug ) { print "skipping comment/empty line:  \"$_\".\n"; }
	    next;
	}

	if( $debug ) { print "after comment:  \"$_\"\n"; }

	# "key:  word" or, "key:\n\tword".
	if( /^(\w+):\s*(.*)/ )
	{
	    if( $debug ) { print "found \"$1\" and \"$2\".\n"; }
	    my $k = $1;
	    $hash{ $k } = $2 ;
	    my $x = shift @ar;
	    if( $debug ) { print "found next \"$x\".\n"; }

	    $lino += 1;

	    while( $x =~ /^\s+(.*)/ )
	    {
		$lino += 1;
		$hash{ $k } .= $1;

		if( $debug ) { print "also found \"$1\"\n\n"; }

		$x = shift @ar;
		last if ! length $x;
	    }

	    if( ( length $x ) > 0 )
	    {
		unshift @ar, $x;
		if( $debug ) { print "pushing \"$x\" back.\n"; }
	    }
	}
	if( $debug ) { print "out:  \"$_\".\n"; }

    }
    if( $debug ) { print "out of read_form:  $lino.\n\n\n"; }

    # if the field is owned by DTG, trim any trailing whitespace
    # to accommodate non-conforming clients.  the modification to the
    # data here just affects the comparison, not saving.
    foreach my $k (keys %hash)
    {
	my $v = $hash{ $k };
	if( $k =~ /^DTG_\w+/ && $v =~ /\s+$/ )
	{
	    # we don't substitute leading whitespace since that's already
	    # trimmed by the parse.
	    $hash{ $k } =~ s/\s+$//g;
	    if( $debug )
	    {
		print "\nTrimmed whitespace in DTG field - '$k'.\n";
		print "'$v' became '$hash{$k}'\n"
	    }
	}
    }
    return %hash;
}

# run a p4 command and return the results.
sub P4()
{
    my $cmd = "@_";
    my $p4cmd = "$p4 $cmd 2>&1";
    my $result = `$p4cmd`;
    if( $result =~ /^Perforce client error:/ ||
	$result =~ /^Perforce password \(P4PASSWD\) invalid or unset\./ )
    {
	my $errmsg = "\n\n$scriptname:  Possible script configuration error.\n";
	$errmsg .= "Found Perforce error text in command result.  Command was:\n";
	$errmsg .= "\"$p4cmd\"\nResulting text is:\n\n$result\n\n";
	print LOG $errmsg;
	print $errmsg;
	close LOG;
	exit 1;
    }
    return $result;
}


# determines if a job is allowed or not.  this is based on whether or not
# the job is new, or if it doesn't touch read-only fields.
sub allowed_job()
{
    # the job as edited by the user.
    my %edjob = @_;

    my $jobname = $edjob{ "Job" };
    # check for metacharacters to prevent shell expansion.
    if( $jobname =~ /([\$\`\;\(])/g )
    { return 2; }

    # fetch the named job from the server so we can compare its
    # fields against that which the user is submitting.
    my $job = &P4( "job -o $jobname" );

    # the job of the same name as retrieved from the server.
    my %value = &read_form_var( split( /\n/, $job ) );

    if( $value{ "Job" } eq "new" )
    {
	if( $debug ) { print "Rejected as new by name.\n"; }
	return 1;
    }

    if( $debug )
    {
	foreach ( keys %edjob ) { print "edjob:  $_:  \"" . $edjob{ $_ } . "\"\n"; }
	foreach ( keys %value ) { print "value:  $_:  \"" . $value{$_}   . "\"\n"; }
    }

    # definition of a new job.  may need to be changed if jobspec differs.
    if( $value{ "Status" } =~ /unconfirmed/i &&
	$value{ "Summary" } =~ /\s*<enter description here>\s*/  ||
	$value{ "Description" } =~ /\s*<enter description here>\s*/ )
	# && $value{ "BZ_CREATIONDATE" } eq "1969/12/31 16:00:00" )
    {
	if( $debug )
	{
	    print "Rejected as new by stat/desc/date:  " .
		$value{ "Status" }. ", " .
		$value{ "Description" } . "\n";
	}
	return 1;
    }

    foreach my $k ( keys %edjob )
    {
	# some clients insert a pair of double quotes when they really
	# mean "empty field".  ignore the change if that's the case.
	if( !defined $value{ $k } && $edjob{ $k } eq '""' ) 
	{
	    next;
	}
	if( $wfl{ $k } != 1 && $edjob{ $k } ne $value{ $k } )
	{
	    if( $debug )
	    {
		print "\n\nnew by wfl:  $k:\n\"" . $edjob{ $k }  . "\"\n\"" .
		    $value{ $k } . "\"\n";
		print "l ed:  "   . length $edjob{ $k };
		print "\nl va:  " . length $value{ $k };
	    }
	    return $k;
	}
    }

    return 0;
}

sub date_format()
{
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
    my @abbr = qw( Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec );
    $year = $year + 1900;

    return "$year-$abbr[$mon] $mday,  $hour:$min:$sec";
}

################################################################################
## MAIN

# from %user% in trigger table
my $user       = $ARGV[ 0 ];

# from %formfile% in trigger table
my $formfile   = $ARGV[ 1 ];

chomp( $user     );
chomp( $formfile );

my $now = &date_format();

if ( "$user" eq "" || "$formfile" eq "" )
{
    print "$scriptname: Error: Missing argument\n";
    exit 1;
}

open( LOG,">> $logfile" )
    or croak( "$scriptname:  Couldn't open log file \"$logfile\":  $!" );

# read the form from the temporary file on disk.
open( F, "< $formfile" )
    or croak( "$scriptname:  Couldn't open temporary file \"$formfile\": $!" );
my @f = <F>;
my %value = read_form_var( @f );
close F;

my $jobname = $value{ "Job" };

my $result = &allowed_job( %value );

if( $freeJobCreation == 1 && $result == 1 )
{
    close LOG;
    exit 0;
}

if( "$user" ne "$p4authuser" && $result )
{
    print "\n\n";
    print "$scriptname:\n\nError:\n\tYou are not authorized to ";

    if( $result == 1 )
    {
	print "create new jobs.\n";
	print LOG "$now:  user \"$user\" tried to create a new job named ";
	print LOG "\"$jobname\".\n";
    }
    elsif( $result == 2 )
    {
	print "create a job with the specified name (\"$jobname\").\n";
	print LOG "$now:  user \"$user\" tried to create a job name with ";
	print LOG "shell metacharacters (\"$jobname\").\n";
    }
    else
    {
	print "modify read-only fields in an existing job.\n\tYou modified ";
	print "the \"$result\" field.  The fields that you may\n\tmodify ";
	print "are:\n\n";
	foreach( keys %wfl ) { print "\t\t$_\n"; }

	print LOG "$now:  user \"$user\" tried to modify read-only field ";
	print LOG "\"$result\" for job \"$jobname\".\n";
    }

    print "\n";
    close LOG;
    exit 1;
}

my $dtName = $value{ "DTG_DTISSUE" };
if( $sameName == 1 && $value{ "Job" } =~ /new/i && length( $dtName ) > 0 )
{
    ## print "formfile:  $formfile\n";

    open( F, "> $formfile" ) or
croak( "$scriptname $now:  Couldn't open formfile \"$formfile\" for write.  $!"
);
    foreach my $line ( @f )
    {
        if( $line =~ /^Job:/ ) { $line = "Job: $jobPrefix$dtName\n"; }
        print F $line;
    }
    close F;
}

close LOG;

# exit, reporting to the Perforce server that the job edit can proceed.
exit 0;
