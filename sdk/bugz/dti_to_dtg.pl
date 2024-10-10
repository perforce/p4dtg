#! /usr/bin/perl

################################################################################
##
## Script to assist migrating from a Bugzilla P4DTI installation to one that
## uses P4DTG.
##
## What this does specifically is to copy all of the Perforce fix information
## that P4DTI stored in the Bugzilla database into the new "cf_p4fixes" custom
## field.  If you don't want to have P4Web URLs in the bug display or otherwise
## don't want to do this, then it's completely optional.
##
## To run, all you have to do is execute this script from your Bugzilla
## directory.  It uses the Bugzilla database access code, and shouldn't require
## any tweaking.  It assumes that you've already upgraded to Bugzilla 3 and
## created the new "cf_p4fixes" custom field.  You'll get an error if not.
## Errors you might see are:
##
##    DBD::mysql::db do failed: Unknown column 'cf_p4fixes' in 'field list'
##    DBD::mysql::st execute failed: Table 'bugs3.p4dti_fixes' doesn't exist
##
## The first means that you haven't created the new custom field, and the
## second means that you didn't have P4DTI installed on this database.

use Bugzilla;
use Bugzilla::DB;
my $dbh = Bugzilla->dbh;

my $sq = "SELECT changelist, bug_id FROM p4dti_fixes";
my $dtiFixes = $dbh->prepare( $sq );
$dtiFixes->execute();

my %h;
while ( my ( $c, $b ) = $dtiFixes->fetchrow_array() )
{
    $h{ $b } .= "$c ";
}
$dtiFixes->finish();

my @updates;
foreach $k ( keys %h )
{
    my $v = $h{ $k };
    my $qu = "UPDATE bugs SET cf_p4fixes = '$v' where bug_id = $k ;" ;
    push @updates, $qu;
}

if( $ARGV[ 0 ] !~ /batchmode/i )
{
    print "Ready to update '" . @updates . "' bug records.\n\n";
    print "Proceed?  (yes/no)  ";
    
    $prompt = <STDIN>;
    chomp( $prompt );
    if( $prompt =~ "^no" || $prompt !~ "^yes" )
    {
	print "\nAborting update of fixes.\n";
	exit;
    }
    print "\n";
}

foreach( @updates )
{
    $dbh->do( $_ );
}

$dbh->disconnect();

print "\nDone.\n";

exit 0;
