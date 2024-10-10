#! /usr/bin/perl

################################################################################
##
## Script to assist migrating from a Bugzilla P4DTI installation to one that
## uses P4DTG.
##
## What this does specifically is to go through all of the existing jobs in the
## Perforce server and copy the existing "P4DTI-issue-id" field into a new
## "DTG_DTISSUE" field.  This allows the replicator to know how to map all of
## the existing jobs to bugs.

use P4;
my $p4 = new P4;

$p4->SetClient( $clientname );
$p4->SetPort( $p4port );
$p4->SetPassword( $p4password );
$p4->Connect() or die( "Failed to connect to Perforce Server.\n" );

print "\nThis is the \"p4 info\" for the server you're about to use:\n\n";

my @info = $p4->RunInfo();
foreach ( @info )
{
    foreach my $k ( keys %$_ )
    {
        my $v = $$_{ $k };
        print"\t$k:  $v\n";
    }
}

if( $ARGV[ 0 ] !~ /batchmode/i )
{
    print "\n\nAbout to edit all jobs, copying the P4DTI issue ID to the\n";
    print "corresponding P4DTG field name.\n";
    print "\nProceed?  (yes/no)  ";

    $prompt = <STDIN>;
    chomp( $prompt );
    if( $prompt eq "no" || $prompt ne "yes" )
    {
	print "\nAborting job field update.\n";
	exit;
    }
    print "\n";
}

################################################################################
# Check that the jobspec has been updated for use with DTG.

$jobspec = $p4->FetchJobSpec();

my $hasDTISSUE = 0;
foreach( @$jobspec{ "Fields" } )
{
    @f = @$_;
    for( $i = 0; $i < @f; $i++ )
    {
        @_ = split( /\s+/, $f[ $i ] );
        my $name = lc( $_[ 1 ] );
	if( $name eq "dtg_dtissue" )
	{ $hasDTISSUE = 1; } # make sure the field we need exists in the jobspec
    }
}

if( !$hasDTISSUE )
{
    print "The P4 server's jobspec does not contain the \"DTG_DTISSUE\" field.\n";
    print "It is required to proceed.  See the readme file for details.\n";
    exit 1;
}

print "\n";

################################################################################
# Fetch all the jobs from the server.

@alljobs = $p4->RunJobs();
my $count = 0;
foreach my $j ( @alljobs )
{
    my $P4DTI_issue_id = $$j{ "P4DTI-issue-id" };
    my $job = $$j{ "Job" };
    if( length( $P4DTI_issue_id ) == 0 )
    {
	print "Skipping job \"$job\":  missing P4DTI-issue-id field.\n";
	next;
    }
    if( !exists( $$j{ "DTG_DTISSUE" } ) )
    {
	$$j{ "DTG_DTISSUE" } = $P4DTI_issue_id;
	$count += 1;
    }

    $p4->SaveJob( $j );
}

################################################################################
# Done.  Close server connection.

print "\nErrors:  \n\n" . $p4->Errors() . "\n" if $p4->Errors();
$p4->Disconnect();

print "\nDone.  Updated \"$count\" jobs.\n";

exit;
