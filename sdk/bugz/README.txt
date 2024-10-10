This is the README documentation for the Perforce Defect Tracking Gateway
(P4DTG) Bugzilla plugin. This document is intended for new users and for users
who want an overview of the plugin.

If you are currently using P4DTI, see README.p4dti for details about migrating
from P4DTI to P4DTG. If you're a P4DTG SDK user, see README.src. For details
about using P4DTG, see the Perforce Defect Tracking Gateway User's Guide.

--------------------------------------------------------------------------------
The Bugzilla Plugin for P4DTG

This plugin mirrors fields between Bugzilla bugs and Perforce jobs. Although
bugs can be created only in Bugzilla, users can edit certain fields in the
corresponding Perforce job and have their changes mirrored in Bugzilla. 
The plugin also associates fixes for those jobs with changelists and enables 
them to be displayed in Bugzilla as links pointing to a P4Web instance, enabling 
users to view change descriptions, diffs and other data.

Note on MySQL Client: 
  This plug-in for DTG uses the MySQL client library. The source for
  this plug-in is available from ftp.perforce.com in the following directory:

  perforce/RELEASE/linux26x86

--------------------------------------------------------------------------------
Configuring the Bugzilla plugin

To configure the plugin using the P4DTG configuration tool:

1. Click the "Defect Tracking Sources" tab.

2. Click "New".

3. From the "Type" dropdown list, choose the Bugzilla/MySQL option.

4. In the "Server connection details" fields, specify the login account details for
   your MySQL server.  The MySQL account must have SELECT, INSERT and UPDATE
   privileges on the host P4DTG is running from and for the Bugzilla database.

5. To modify optional attributes, click "Edit attributes...".

   The optional attributes are as follows:

   "Bugzilla username"

       If your Bugzilla user name is different from your MySQL user name, enter
       it here.  Account names are typically formatted like an email address.

   "Fixes custom field"

       The custom field where fix information is stored.  
       Important: If you change the default name of the custom
       field, you must modify the patched Bugzilla code to reflect the
       difference.

   "MySQL database name"

       The name of the Bugzilla database in the MySQL server.

   "Unicode server"

       Specifies whether the Bugzilla database uses and supports Unicode.
       Starting with Bugzilla release 2.22, UTF-8 is the default encoding for
       Bugzilla databases. To check your database, see the instructions for the "utf8"
       field:

         http://www.bugzilla.org/docs/3.0/html/parameters.html

   "Wait time"

       Number of seconds to wait on MySQL errors before retry. To configure 
       replication to quit when an error occurs, specify -1.

   "Check privileges"

       Specifies whether the plug-in checks the user's access rights to the MySQL
       user before connecting.  

   "Change offset"

        Configures a delay to ensure that heavily-loaded servers do not skip
        records.

        Heavy defect tracker activity can cause a delay between the time that 
        Bugzilla records in the ModifiedDate field and the time that the issue 
        is actually committed. This discrepancy might cause P4DTG Replication 
        Engine to skip issues. To minimize this possibility, set "Change offset"
        to 30 seconds. To prevent Bugzilla defect data from overriding Perforce
        Job data, minimize the number of mirror mappings when using this 
        setting.

6. Click "Check connection and retrieve fields". If your entries are correct, 
   the configuration tool displays "Valid connection to server".

After you've configured Bugzilla as a defect tracking source, configure your
Perforce Source and mappings as described in the P4DTG User Guide and online
help.

The Bugzilla plugin adds the following fields to the list of defect tracker
fields that are displayed in the Gateway Mappings dialog:

- Status/Resolution: combines the Status and Resolutions fields so
  that they can be mirrored with the Perforce Status field. See the jobspec.txt
  file for an example jobspec which would allow for mirroring with a standard
  Bugzilla (3.2.x - 4.2.x) installation. If the possible resolutions have been
  altered for your site, update the possible status fields accordingly. The
  sample jobspec requires a Perforce Server 2008.1 or later. For earlier 
  servers, specify change 'Closed-Fixed' on the Perforce jobspec to 'closed'
  and alter the mirror mapping accordingly.

  If you have defined additional 'closed' status values for your site ('closed'
  status values are those which require a Resolution value), then list these 
  fields in the 'Added closed states' attribute for the Bugzilla DataSource 
  using the Configuration tool. You will need to expand the list of possible 
  Perforce status values to include these states. Note: All new values must
  match the case as defined in Bugzilla.

- Product/Component: combines the Product and Component fields so
  that they can be used for segmentation. For any field used in segmentation,
  the bug should never change such that it changes out of an existing segment.
  Bugs can migrate into a segment; but never out.

- Fixes: to append fix information to the Bugzilla bug "Comments" field, map 
  the Perforce "Fix Details" field to this field. You can specify details to
  be appended, such as the changelist number or user who made the fix, etc.

- Description: the aggregate of the Bugzilla "Comments" field. To display this
  information in the Job, map this field to a TEXT field in the Perforce job 
  specification.

--------------------------------------------------------------------------------
Optional Configuration Options

- To display a list of fixes associated with a bug:

   Create the "cf_p4fixes" Bugzilla custom field in Bugzilla
   as follows:

      1. Log into Bugzilla using the administration account.
      2. On the home page, click the "Administration" link.
      3. Click the "Custom Fields" link.
      4. Click "Add a new custom field".
      5. In the "Name" field, type "cf_p4fixes".
      6. In the "Type" dropdown, select "Large Text Box" or "Free Text".
      7. In the "Description" field, type "Perforce fix information".
      8. Click "Create".

   This field is displayed in the Gateway Mappings dialog as "Perforce fix
   information". To display a list of fixes associated with the bug, map 
   the "List of Change Numbers" field from Perforce to this field.

   For more details, refer to the Bugzilla documentation.

- To enable display of P4Web links to bug fixes, apply the P4DTG Bugzilla patch
  as follows:

      1. In a shell window, change directories to the Bugzilla root directory.
      2. Invoke the patch program (see References) as follows:

            [/bugzilla] patch -b -p1 < /p4dtg/doc/bugz3mysql5/patch.bugzilla

         The patch program displays output resembling the following:

             patching file show_bug.cgi
             patching file template/en/default/bug/field.html.tmpl

         Specify the patch file that corresponds to your Bugzilla version.

      3. Edit the "show_bug.cgi" file to ensure that the URL to your P4Web
         instance is correct. If the URL ever changes, update this file. The URL
         is not stored in the database. As you change the URL in the
         source file, the new URL is in effect for all bugs.

      4. To override the default name "cf_p4fixes" for the custom field, edit
         "field.html.tmpl", changing all instances of "cf_p4fixes" to be the
		 name of your custom field.

- To install the Perforce server trigger, follow the instructions at the top of
  the trigger file: p4dtg/doc/bugz3mysql5/changeid.pl.  The trigger implements
  the following functionality:

    - Prevent Perforce users from creating jobs. The Bugzilla plug-in does not
      support originating jobs in Perforce.

    - Prevent editing of read-only fields in Perforce. You can define one-way
      mappings of read-only fields in Bugzilla to Perforce, and this trigger
      prevents users from editing these fields. If users edit one of these
      fields, any data they enter is overwritten with the field value in
      Bugzilla during replication.

    - Make the "Job" field match the Bugzilla bug number. Normally, jobs in
      Perforce are named using "jobNNNNNN", where NNNNNN is an ever-increasing
      number.

    P4DTG's configuration tool prevents the mapping of Bugzilla workflow-
    related fields from Perforce to Bugzilla. For example, the "Status" and
    "Resolution" fields are read-only. This approach prevents replication of
    these fields from Perforce to Bugzilla. Use the "Status/Resolution" field
    to update these fields.

References
----------

Reference for the 'patch' utility.  See 'external links' for downloads.
http://en.wikipedia.org/wiki/Patch_%28Unix%29
