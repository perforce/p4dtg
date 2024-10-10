#! /usr/bin/perl

# Copyright 2009 Perforce Software.  All rights reserved.
#
# This file is part of Perforce - the FAST SCM System.

################################################################################
# Script to:
#
# 1.  Create an SQL schema matching a given Perforce server's jobspec.
# 2.  Create MySQL triggers on that database to aid in replication.
# 3.  Create a sample one-to-one mapping between all job fields in the P4
#     instance and the MySQL database.
#
# It outputs to STDOUT, so you should redirect it to a file.  E.g.:
#
#    [] ./import.pl > jobs.sql
#
# Here are some testing hints:
#
# 1. Start the MySQL daemon with the "--binlog-ignore-db database_name" flag so
#    that if you drop/create the database repeatedly in the course of testing,
#    that the binary log files the server creates don't fill up your hard drive.
#
# 2. Create a database to replicate into.  "p4jobs" is the default.
#
#    mysqladmin --user=admin_user create database_name
#
# 3. Populate it with data from this script:
#
#    ./import.pl > jobs.sql 
#    mysql --user=admin_user --database=database_name --batch < jobs.sql
#
#    When replaying the data, there should be no errors printed.  For example,
#    if you see the following, you probably didn't configure this script to
#    use the correct modification date field in your jobspec.
#    
#    ERROR 1054 (42S22) at line 843934: Unknown column 'ModDate' in 'NEW'
#
#    This error means your P4 server hasn't been configured for DTG yet:
#
#    ERROR 1054 (42S22) at line 189: Unknown column 'dtg_dtissue' in 'NEW'
#
#    Once the database is initialized with tables, you can start DTG replication
#    with the "sync from start date" option in order to send all the P4
#    jobs to the MySQL database, and to update the P4 jobs with DTG_DTISSUE
#    fields.  Since this alters the P4 side of things, you could do it on a
#    copy of your repository to get a feel for the behavior.
#
# 4. Run the interactive MySQL prompt and examine your data.
#
#    mysql --user=admin_user --database=database_name
#    mysql> SHOW TABLES;
#    mysql> SELECT * FROM select_values;
#    mysql> SELECT * FROM typemap;
#    mysql> SELECT description FROM jobs ORDER BY reporteddate DESC LIMIT 1;
#
# 5. Delete the database if you need to restart from scratch:
#
#    mysqladmin --user=admin_user   drop database_name
#
# This script requires P4Perl, available here:
#
#    http://perforce.com/perforce/loadsupp.html#api

# blank values default to what's in your environment.
$p4port  = "";
$p4user  = "";
$p4pass  = "";

# jobspec "modified by"/"modified date" fields you selected in the config tool.
#$moddate = "ModDate";
#$modby   = "ModBy";

$moddate = "modifieddate";
$modby   = "modifiedby";


my $fetchJobs = 0;
if( $ARGV[ 0 ] =~ /-jobs/i ) { $fetchJobs = 1; }

################################################################################
# Connect to the server.

my $version = '$Change: 2609833 $';
$version =~ /(\d+)/;
$version = $1;

use P4;
my $p4 = new P4;

$p4->SetPort     ( $p4port );
$p4->SetUser     ( $p4user );
$p4->SetPassword ( $p4pass );
$p4->SetProg     ( "P4DTG-MySQL-import.pl" );
$p4->SetVersion  ( $version );
$p4->Connect     (         ) or die( "Failed to connect to Perforce Server.\n");

################################################################################
# Process the jobspec.

$jobspec = $p4->FetchJobSpec();

# Mapping between jobspec type and SQL type.
# When the jobspec says line/word/select, it can be an arbitrarily long field as
# far as the Perforce server is concerned, so we can't just use VARCHAR(N).
#
# If types are added here that should have SQL indexes, don't forget to update
# the table creation logic.
%h = ( "date"   => "DATETIME" ,
       "select" => "TEXT"     ,
       "line"   => "TEXT"     ,
       "text"   => "LONGTEXT" ,
       "word"   => "TEXT"
    );

$seltable = "CREATE TABLE select_values ( field TEXT, val TEXT ) ;\n";
# holds all the SQL inserts for select fields.
$selects;

=comment  This is a sample jobspec:

Fields:
        101 Job word 32 required
        102 Status select 10 required
        103 User word 32 required
        104 Date date 20 always
        105 Description text 0 required
        106 DTG_FIXES text 0 optional
        107 DTG_DTISSUE word 32 optional
        108 DTG_ERROR text 0 optional
        109 DTG_MAPID word 32 optional
        110 ModDate date 20 always
        111 ModBy word 32 always

Values:
        Status open/suspended/closed

Presets:
        Status open
        User $user
        Date $now
        Description $blank

=cut

foreach( @$jobspec{ "Values" } )
{
    @f = @$_;
    for( $i = 0; $i < @f; $i++ )
    {
	# example:  Severity A/B/C
	@_ = split( /\s+/, $f[ $i ] );
	my $name = lc( $_[ 0 ] );
	@_ = split( /\//, "$_[ 1 ]" );
	my $options;

	for( $j = 0; $j < @_; $j++ )
	{
	    # need to lower case the values and escape single quotes.
	    $selects .= "INSERT INTO select_values VALUES ( '$name', ";
	    $selects .= "'$_[ $j ]' ) ;\n";
	}
    }
}
print "START TRANSACTION;\n";
print "\n$seltable\n$selects\n";

# SQL to create the jobs table.
$jobtable = "CREATE TABLE jobs ( \`_ModBy\` TEXT, \`_ModDate\` DATETIME, \`_job\` TEXT, ";

# Add the DTG "List of change numbers" and "Fix details", neither of which are
# in the jobspec, as it is data synthesized by DTG.

$jobtable .= "\`change_numbers\` TEXT, \`fix_details\` TEXT, ";

# Maps the p4 jobspec type to the SQL type.
$maptable = "CREATE TABLE typemap ( field TEXT, p4val TEXT, sqlval TEXT ) ;";

$typemaps;
foreach( @$jobspec{ "Fields" } )
{
    @f = @$_;
    for( $i = 0; $i < @f; $i++ )
    {
	# example:  108 Severity select 10 required
	@_ = split( /\s+/, $f[ $i ] );
	$name   = lc  $_[ 1 ]     ;
	$p4type = lc( $_[ 2 ] );
	$type   =  $h{ $p4type } ;

	$jobtable .= "\`$name\` $type ";
	# Add indexes to the table.  MySQL/InnoDB allow at most 767 bytes.
	if( $type =~ /TEXT|LONGTEXT/ )
	{ $jobtable .= ", INDEX(\`$name\`(767)) "; }

	if( $i != (@$_ - 1) )
	  { $jobtable .= ", "; }

	$typemaps .= "INSERT INTO typemap VALUES ( '$name', '$p4type', '$type' ) ;\n";
    }
}
$jobtable .= ") ;";

$typemaps .= "INSERT INTO typemap VALUES ( \'_ModBy\'  , 'word', 'TEXT'     ) ;\n";
$typemaps .= "INSERT INTO typemap VALUES ( \'_ModDate\', 'date', 'DATETIME' ) ;\n";
$typemaps .= "INSERT INTO typemap VALUES ( '_job', 'word', 'TEXT' ) ;\n";
$typemaps .= "INSERT INTO typemap VALUES ( \'change_numbers\', 'line', 'TEXT' );\n";
$typemaps .= "INSERT INTO typemap VALUES ( \'fix_details\', 'TEXT', 'TEXT' );\n";

print "\n$jobtable\n";
print "\n$maptable\n";
print "\n$typemaps\n";


################################################################################

print "\nINSERT INTO jobs ( job, _job ) VALUES ( 'new', 'new' ) ;\n";

# Triggers on UPDATE and INSERT to mirror data to non-replicated fields.
print qq[

DELIMITER //
CREATE TRIGGER jobs_update BEFORE UPDATE ON jobs FOR EACH ROW BEGIN

SET NEW.$moddate    = now();
SET NEW._ModDate    = now(); 
SET NEW._ModBy      = (SELECT SUBSTRING_INDEX( USER(), '\@', 1 ));
SET NEW.dtg_dtissue = new.job;

END//
DELIMITER ;

DELIMITER //
CREATE TRIGGER jobs_insert BEFORE INSERT ON jobs FOR EACH ROW BEGIN

SET new._ModDate    = new.$moddate; 
SET new._job        = new.job;
SET new._ModBy      = (SELECT SUBSTRING_INDEX( USER(), '\@', 1 ));
SET new.dtg_dtissue = new.job;

end//
DELIMITER ;
];

################################################################################
# Done.  Close server connection.

if( $p4->Errors() || $p4->Warnings() )
{
    print "ROLLBACK;\n";

    print "\nP4 Errors:  \n\n"   . $p4->Errors()   . "\n";
    print "\nP4 Warnings:  \n\n" . $p4->Warnings() . "\n";
    print "This is invalid SQL so you notice that something went wrong.\n";

    $p4->Disconnect();
    exit;
}
$p4->Disconnect();

print "COMMIT;\n";

exit;
