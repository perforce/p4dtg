MIGRATION FROM P4DTI TO P4DTG FOR BUGZILLA USERS
--------------------------------------------------------------------------------
This document tells you how to migrate your P4DTI/Bugzilla integration to 
P4DTG, the Perforce Defect Tracking Gateway.

Upgrading from P4DTI
--------------------

Before converting, back up your Bugzilla, Perforce, MySQL and P4DTI data.

To convert to P4DTG, you must be running version 2.4.2 of P4DTI.  If you aren't
running version 2.4.2, you must upgrade or verify that the "p4dti_fixes" table
in the Bugzilla database matches the current release.  See the appendix for
further information on upgrading.

To convert from P4DTI to P4DTG:

1.  Upgrade to Bugzilla 3.2.x, 3.4.x, 3.6.x or 4.0.x

    Refer to the Bugzilla documentation for details.


2.  *optional*  Enable P4Web URLs inline on Bugzilla bug pages.

    Create the "cf_p4fixes" Bugzilla custom field in Bugzilla.

    Refer to the Bugzilla documentation for details.  Likely steps:

      a. Log into Bugzilla with the administration account.
      b. Click on the "Administration" link off of the home page.
      c. Click on the "Custom Fields" link.
      d. Click on "Add a new custom field".
      e. In the "Name" field, enter "cf_p4fixes".
      f. In the "Type" dropdown, select "Large Text Box" or "Free Text".
      g. In the "Description" field, type "Perforce fix information".
      h. Click the "Create" button

    Apply the P4DTG Bugzilla patch to show P4Web URLs for fixes on bugs:

    In a shell window, change directories to the Bugzilla root directory,
    then invoke the patch script as follows:
    
      [/bugzilla] patch -b -p1 < /p4dtg/doc/bugz3mysql5/patch.bugzilla

    The patch program (see References) displays output resembling the following:

      patching file show_bug.cgi
      patching file template/en/default/bug/field.html.tmpl

     Note that you will need to select the patch file that matches your Bugzilla
     version.

    The URL to your P4Web instance is stored in the "show_bug.cgi" file.
    You must edit it to insert the correct data.  If the URL ever changes,
    you must update this file.  The URL is not stored in the
    database, so as soon as you change it in the source file, the change
    is active for all bugs.

    If you must use a non-default name for the custom field, you must
    edit "field.html.tmpl" accordingly.
    

3.  To copy P4 fix information from the P4DTI database table to the new custom
    field, run the "dti_to_dtg.pl" script.  Run this from the root directory of
    your Bugzilla installation.

4.  Edit the Perforce jobspec as follows:

    Run "p4 jobspec" as the super user and add the following fields,
    adjusting the leading number as appropriate.

        106 DTG_FIXES   text 0  optional
        107 DTG_DTISSUE word 32 optional
        108 DTG_ERROR   text 0  optional
	109 DTG_MAPID   word 32 optional

    The (optional) DTG_MAPID exists to support segmented replication.  See the
    P4DTG user's guide for details.

    Change all existing P4DTI fields that are specified as
    "required" or "always" to "optional".  

5.  To copy the P4DTI bug ID field to the new DTG identifier,
    run the "mk_dtgissue.pl" script.

    It gets its server connection details from your environment.  Set those
    up prior to running the script (P4PORT/P4USER/etc).

    The script makes use of the P4-Perl Perforce API.  It is a required
    dependency.  If you don't already have it installed, you can get it
    via Perforce.com->Downloads->"Related Software"->"Perforce Perl API".

6.  Install the Perforce server jobs trigger.

    Here is an example of what this looks like in the server triggers table:

      Triggers:
        jc form-in job "/p4/triggers/changeid.pl %user% %formfile%"

    You'll need to edit the trigger script, configuring it as necessary.

7.  Set the Perforce job counter to match the Bugzilla bug number.

    On the Perforce server, you can set the starting number for new
    job names.  If you want new jobs to use numbers in a sequence higher
    than your bug numbers, such as "bug1234" going to "job1235", then
    do this:

       p4 counter -f job 1235

    By default, P4DTG does not create jobs with names identical to the
    Bugzilla bug.  This is to support users with multiple defect trackers,
    as there may be bug name conflicts between them.  By allowing the Perforce
    server to assign its own numeric names to bugs, collisions are prevented.
    
    If you wish to have jobs created with the same name as the bug, then
    you may do so by configuring the "changeid.pl" script.


Configuring P4DTG after converting from P4DTI
---------------------------------------------

Beyond entering the appropriate connection details in the plugin, you should
change the "Start replication from" field under "Gateway Mappings" to a date
and time just after you stopped the old replicator.

Follow the instructions in README.txt for configuring the rest of P4DTG. The
*Optional* step above requires mapping the DTG "List of Change Numbers" field to
the "Perforce fix information" field in Bugzilla.

--------------------------------------------------------------------------------
Appendix

Upgrading P4DTI
---------------

Before upgrading to P4DTI version 2.4.2, remove the P4DTI patches to Bugzilla
(if you're installing the new Bugzilla on top of your old version.  Alternately,
put the new Bugzilla in a different directory and skip this step.)

Refer to the P4DTI documentation for specifics, but the upgrade process consists
of the following steps:

1. Stop P4DTI and any cron jobs that restart it.

2. Invoke the patch script, for example:

      [/bugzilla-3] patch -p1 -R < /p4dti-bugzilla-2.4.2/bugzilla-3.0-patch

The script output looks like this:

      patching file Bugzilla/Bug.pm
      patching file Bugzilla/Config/Core.pm
      patching file Bugzilla/Config.pm
      patching file template/en/default/admin/params/core.html.tmpl
      Hunk #1 succeeded at 108 (offset 1 line).


References
----------

Perforce Defect Tracking Integration Administrator's Guide
http://public.perforce.com/public/perforce/p4dti/release/2.4.2/ag/index.html

Reference for the 'patch' utility.  See 'external links' for downloads.
http://en.wikipedia.org/wiki/Patch_%28Unix%29
