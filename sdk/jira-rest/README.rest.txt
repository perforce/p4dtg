README for Perforce Defect Tracking Gateway (P4DTG) Atlassian JIRA-REST Plug-in

This document is intended for new users and for users who want an overview of
the JIRA plug-in. For details about using P4DTG, see the Perforce Defect
Tracking Gateway User's Guide.

IMPORTANT NOTE: The plug-in described in this document is a plug-in for the
P4DTG system, it is not a plug-in to be installed into your JIRA installation.
Do not copy the .jar, .dll, or .so files out of the P4DTG directory into your
JIRA installation otherwise you will probably corrupt your JIRA database and
delete critical files necessary for the JIRA server.

--------------------------------------------------------------------------------
Overview

This plug-in mirrors fields between JIRA issues and Perforce jobs. Users can
create new Perforce jobs and mirror them as new issues in JIRA. Users can also
edit certain fields in a Perforce job and have their changes mirrored in JIRA.

System Requirements
-------------------

   Architecture: x64

   Operating systems:

      Windows 7 and later
      Linux 2.6+

   Software versions:

      Perforce server 2018.1+
      JRE 1.8.0+
      JIRA servers v7.1.x+, Jira Cloud.   
      
   *** Special Note ***

      * The JIRA server's character encoding is set to UTF-8.  If your Helix
        Core Server is not unicode enabled then you must pick a character
        set for the Helix Core Server in the Perforce Source's attributes.
      * Jira Cloud's REST api can change often.  Contact Perforce Support
        if problems are encountered.

--------------------------------------------------------------------------------
Configuring the JIRA server

The DTG JIRA plugin has been developed with the JIRA server version 6.x.x. JIRA
ships with the REST plugin which enables remote access through the RESTful API.

JIRA User Permissions
----------------------------
By default, the JIRA installation creates three user groups "jira-users",
"jira-developers", and "jira-administrators". Each of these user group has it's
own set of permissions.

The P4DTG JIRA plug-in requires an administrative JIRA user for configuration. 
The simplest way to achieve this is to assign this user to the "jira-administrators" 
group. 

At run time, the JIRA user must be able to read and update jira issues and
add comments.


JIRA "Perforce Job Integration" plug-in
----------------------------
The JIRA server may have the "Perforce Job Integration" plug-in using FishEye
and/or Crucible. Please turn the "Perforce Job Integration" off. There might be
unforeseen problems occurring if both the "Perforce Job Integration" and the DTG
JIRA replication are running at the same time.


--------------------------------------------------------------------------------
Configuring the JIRA plug-in

Before starting, please consult the Defect Tracking Gateway Guide's section 
"Plan your replication."

Configure any needed JIRA custom fields in jira-rest-config.xml.  You can
make a copy of this, alter as needed, then enter your file name in the JIRA
source's attributes (see below).

To configure the plug-in, launch the P4DTG configuration tool and perform
the following steps:

1. Click the "Defect Tracking Sources" tab.

2. Click "New".

3. From the "Type" dropdown list, choose "JIRA-REST".

4. Specify the login account details for your JIRA server in the "Server
   connection details" fields. The 'Server' field should be the JIRA server's
   web services URL. For example,

        Server: http://jiraserver:8080/
        User Name: jirauser
        Password: jirapasswd
  
  SSL is supported.  For troubleshooting connection problems, see
  https://community.perforce.com/s/article/3578
  
  For JIRA cloud, use your JIRA account's email and api token.

5. (Optional) To configure attributes, click "Edit attributes..." and specify
   options as follows.

        Workflow and field definitions:  enter the name of the configuration
        file.  The file must be in the config directory.

        Wait time: Specifies the number of seconds that the replication engine
        waits after detecting a connection error before it retries an operation.
        Use -1 to default to the General Wait Duration specified for the
        replication map.

        TCP Port: This is the TCP port number of the DTG-JIRA proxy server will
        be listening on. This port changes dynamically by DTG if it sees an
        instance already running or evidence of an instance running.
        Note: only change this value with the help of Perforce support.

        Java options: Use this attribute to specify additional Java options for
        starting the JIRA plugin.
        Note: only change this value with the help of Perforce support.

        Allow JIRA issue creation: Enable creating of new JIRA issue through
        replication. Default is to disallow such creation. Specify either 'y'
        or 'n'.  For this to work you must map all JIRA required fields. 
        Creating Jira Issues is not supported with Jira Cloud.

        Batch size for issues: This attribute affects the memory used by the
        Java process by limiting the number of issues returned per query of the
        JIRA server. A small number limits the memory but increases processing
        time when there are a large number of issues to be processed. A larger
        number improves performance but consumes a much larger amount of memory.
        Default is 100.
        Note: only change this value with the help of Perforce support.

6. To verify your configuration, click "Check connection and retrieve fields".
   If your entries are correct, the configuration tool displays "Valid
   connection to server".

7. Finally, configure your Perforce Source and mappings as described in the
   Defect Tracking Gateway Guide.
   
8. The JIRA plug-in adds the following fields to the list of defect tracker
   fields that is displayed in the Gateway Mappings dialog:

     Status/Resolution: combines the Status and Resolutions fields so that they
     can be mirrored with the Perforce Status field. See the jobspec.txt file
     for an example jobspec which would allow for mirroring with a JIRA server
     (v4.1.x - v5.0.x). If the possible resolutions have been altered, update
     the possible status fields accordingly.
     
     Summary: contains the Issue subject.
     To display this information in the corresponding Perforce job, map this
     field to a TEXT field in the Perforce job specification.

     Description: contains the Issue body.
     To display this information in the corresponding Perforce job, map this
     field to a TEXT field in the Perforce job specification.

     Comments: the aggregate of the JIRA "Comments" field.
     To display this information in the corresponding Perforce job, map this
     field to a TEXT field in the Perforce job specification. 

     Fix: to append fix information to the JIRA issue "Comments" field,
     map this field to the Perforce "Fix Details" field. You can specify details
     to be appended, such as the change number or user who made the fix.


Jira Plug-in Configuration file
-------------------------------
Consult the Defect Tracking Gateway Guide for details.   The distribution
contains a sample jira-rest-config.xml file.

Configuring custom workflows
----------------------------

Although mirroring Status/Resolution is *NOT* advised for JIRA, very simple 
workflows may be mirrored.   The config file must have a workflow element
even if you do not mirror Status/Resolution.

First, login to the JIRA server as the JIRA administrator and 
locate the active screens.

1. You must login to the JIRA server as a JIRA admin user.
2. Navigate to view screens.
3. For each screen that includes resolutions, note the workflow name
   and its associated resolution transition name(s) that is in parentheses.

Second, locate the active workflows and associated transitions.

1. Open a separate Web browser window or tab.
2. Navigate to view active workflows.
3. Use the Operation that allows you to view the workflow steps.
4. View each active workflow step screen in text mode as you will 
   copy the Step Name, Linked Status, and transitions into 
   the jira-rest-config.xml as outlined below.

Third, update the configuration XML file (default, 'config/jira-rest-config.xml').

1. Open the configuration XML file and enter each custom workflow from the
   screens as XML <Workflow>...</Workflow> elements inside the
   <Workflows>...</Workflows> outer XML element.

   Inside each workflow there are step elements. Each step element contains
   transitions. The step elements require values for the step name and
   linkedStatus attributes. The transition elements require values for the name
   and destinationStep attributes.

2. For each custom workflow create <ResolutionTransition>...
   </ResolutionTransition> XML elements inside the <ResolutionTransitions>...
   <ResolutionTransitions> outer XML element.

   The 'name' attribute value is the display value from the 'View Screens' page
   under the 'Workflow' heading.

3. The following are examples of custom workflow XML elements for the workflow
   named 'jira'.

   <Config>
     ...
     ...
     <Workflows>
       <Workflow name="jira">

         <ResolutionTransitions>
           <ResolutionTransition name="Resolve Issue" />
           <ResolutionTransition name="Close Issue" />
         </ResolutionTransitions>

         <Step name="Open" linkedStatus="Open">
           <Transition name="Start Progress" destinationStep="In Progress" />
           <Transition name="Resolve Issue" destinationStep="Resolved" />
           <Transition name="Close Issue" destinationStep="Closed" />
         </Step>

         <Step name="In Progress" linkedStatus="In Progress">
           <Transition name="Stop Progress" destinationStep="Open" />
           <Transition name="Resolve Issue" destinationStep="Resolved" />
           <Transition name="Close Issue" destinationStep="Closed" />
         </Step>

         <Step name="Resolved" linkedStatus="Resolved">
           <Transition name="Close Issue" destinationStep="Closed" />
           <Transition name="Reopen Issue" destinationStep="Reopened" />
         </Step>

         <Step name="Reopened" linkedStatus="Reopened">
           <Transition name="Resolve Issue" destinationStep="Resolved" />
           <Transition name="Close Issue" destinationStep="Closed" />
           <Transition name="Start Progress" destinationStep="In Progress" />
         </Step>

         <Step name="Closed" linkedStatus="Closed">
           <Transition name="Reopen Issue" destinationStep="Reopened" />
         </Step>

       </Workflow>

     </Workflows>
   </Config>

*** Important Notes ***

Custom fields
-------------

1. Custom fields should have unique names. The custom field name is assumed to
   be unique and it is used to do reverse lookup of the custom field id in JIRA.

2. If custom fields exist in the JIRA server but are not defined in the
   jira-rest-config.xml file, the 'type' and 'access' attributes will be set to
   the following default values:

   type="LINE" access="RO"

Workflows (steps and transitions)
---------------------------------

1. Transitions should have unique "destinationStep" values in jira-rest-config.xml.
   This means each transition should be unique. The unique transitions can be
   used multiple times inside the steps of the workflows.

   *** The following configuration is right. Notice both "My Reopen Issue"
       transitions point to the same 'destinationStep' value "My Reopened".

   <Workflows>
       <Workflow name="Flow100">
         <Step name="MyStep100" linkedStatus="MyClosed100">
           <Transition name="My Reopen Issue" destinationStep="My Reopened" />
         </Step>
       </Workflow>
       <Workflow name="Flow200">
         <Step name="MyStep200" linkedStatus="MyClosed200">
           <Transition name="My Reopen Issue" destinationStep="My Reopened" />
         </Step>
       </Workflow>
   </Workflows>

   *** The following configuration is wrong. Notice both "My Reopen Issue"
       transitions point to different 'destinationStep' values "My Reopened"
       and "My Open"

   <Workflows>
       <Workflow name="Flow100">
         <Step name="MyStep100" linkedStatus="MyClosed100">
           <Transition name="My Reopen Issue" destinationStep="My Reopened" />
         </Step>
       </Workflow>
       <Workflow name="Flow200">
         <Step name="MyStep200" linkedStatus="MyClosed200">
           <Transition name="My Reopen Issue" destinationStep="My Open" />
         </Step>
       </Workflow>
   </Workflows>

2. Delete the default JIRA workflow in jira-rest-config.xml if you are not using it.

3. The JIRA administrator needs to add (if missing) a 'Post Function' to clear
   (set field value to 'None') the 'Resolution' field for transitions that move
   an issue from a resolved state back to an unresolved state. If not in place 
   the user may see the following error displayed in the Perforce job's 
   'DTG_ERROR' field during replication:

   "[UsageError]Error in job specification.  Missing required field 'Status'."

Handling multi-select fields
----------------------------

DTG does not support multi-select fields. Therefore, JIRA multi-select fields
can only be copy as read-only text fields in DTG. The following are default
JIRA multi-select fields.

1. Component/s
2. Affects Version/s
3. Fix Version/s

Alter the Perforce jobspec to support 'Components', 'Affects Versions' and
'Fix Versions'. Copying read-only from JIRA to Perforce text fields. Please
see 'jobspec.txt' file for examples.

The workaround to mirror these fields is to setup single-select JIRA custom
fields, configure them in 'jira-rest-config.xml' and map them to single-select
Perforce fields.
