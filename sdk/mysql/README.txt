This is the README documentation for the Perforce Defect Tracking Gateway
(P4DTG) MySQL 5 plugin. This document is intended for new users and for users
who want an overview of the plugin. It tells you how to create and populate
the required MySQL database and start replication.

For details about using P4DTG, see the Perforce Defect Tracking Gateway User's
Guide.

--------------------------------------------------------------------------------
The MySQL 5 Plugin for P4DTG

This plugin mirrors fields between Perforce jobs and a MySQL server.  Jobs can
be created and modified on either side.  The plugin also associates fixes for
those jobs with changelists.

System Requirements
-------------------

   Architecture: x86

   Operating systems:

      Windows XP, 2003, Vista, 2008, 7
      GNU/Linux 26
      FreeBSD 6

   Software versions:

      Perforce server 2006.2+
      MySQL 5.0+

Note on MySQL Client: 

  This plug-in for DTG uses the MySQL client library. The source for
  this plug-in is available from ftp.perforce.com in the following directory:

  perforce/RELEASE/linux26x86.  

  This unsupported source distribution is licensed under the BSD license 
  of July 22, 1999.  

  To obtain the complete, supported P4DTG SDK for building new plug-ins,
  email partners@perforce.com.

--------------------------------------------------------------------------------
Configuring the MySQL 5 Plug-in

To configure the plugin, perform the following steps:

1. Initialize the MySQL database.

   Read the instructions in p4dtg/doc/mysql5/import.pl. 
   First, create the database manually. To create a script containing the DDL
   statements that create the required tables in the MySQL server, run import.pl
   against the Perforce server that you want to replicate. Finally, to  
   to create the necessary tables, run the script created by import.pl.

   Note that the 'new' job in the 'jobs' table is required  by the plugin.

2. Launch the DTG Configuration Tool. On the "Defect Tracking Sources" tab, 
   click "New". The tool displays the defect tracking source dialog.

4. From the "Type" dropdown list, choose MySQL 5.

5. In the "Server connection details" fields, specify the login account details for
   your MySQL server.  The MySQL account must have SELECT, INSERT and UPDATE
   privileges on the host where P4DTG is running.

6.  To modify plug-in attributes, click "Edit attributes...".

   The optional attribute fields are:

   *   MySQL database name

       The name of the jobs database in the MySQL server.  The default is
       "p4jobs".  

   *   Unicode server

       Specifies whether the MySQL database uses/supports Unicode (y/n).  
       Set to "y" if your Perforce server runs in Unicode mode.

   *   Wait time

       Number of seconds to wait on MySQL errors before retry.  
       To configure the plugin to quit replication when an error occurs, 
       set to -1.

   *   Check privileges

       Specifies whether the plugin checks for sufficient access rights to the MySQL
       user before connecting.   

7. Click "Check connection and retrieve fields". If your entries are valid, 
   the configuration tool displays "Valid connection to server". If the
   configuration tool displays an error, examine the settings you entered for 
   errors or invalid values.

IMPORTANT:

After you've configured MySQL as a defect tracking source, configure your
Perforce Source and mappings as described in the P4DTG User Guide and online
help. You must map the job, modifiedBy, modifiedDate fields (the fields that
record the user who last modified the job and the time it was last
updated).  After defining these mappings and starting replication,
do not change these mappings. If you must remap these fields, you must reinitialize 
the MySQL database as described above.

Finally, start replication, specifying the "sync from start date" option to
ensure that all existing Perforce jobs are replicated in the MySQL database
and that the Perforce jobs DTG_DTISSUE fields are populated, to link the
jobs with corresponding database entries.

--------------------------------------------------------------------------------
Disabling Perforce Job Creation

You might want to ensure that all defects originate only in your defect tracker
and never in Perforce. To prevent Perforce users from originating jobs, the 
P4DTG Bugzilla integration includes a Perforce trigger. 

To install the Perforce server trigger, follow the instructions at the top of
the trigger file: p4dtg/doc/bugz3mysql5/changeid.pl.  The trigger implements
the following functionality:

  * Prevent Perforce users from creating jobs.

  * Prevent editing of read-only fields in Perforce. You can define one-way
    mappings of read-only fields mapped from MySQL to Perforce. This trigger
    prevents users from editing these fields. If users edit one of these
    fields, any data that they enter is overwritten with the field value from
    MySQL during replication.

  * Make the "Job" field match the MySQL bug number. Normally, jobs in
    Perforce are named using "jobNNNNNN", where NNNNNN is an incrementing
    number.  If you're creating jobs from MySQL, this trigger duplicates 
    the MySQL-assigned job name in Perforce.
