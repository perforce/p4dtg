/**
*    P4DTG - Defect tracking integration tool.
*    Copyright (C) 2024 Perforce Software, Inc.
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
package com.perforce.p4dtg.plugin.jira.common;

import java.util.HashMap;

/**
 * This class provides common constants for the DTG JIRA plugin.
 */
public class Constants {
    // Property for socket timeout
    public static final String SOCKET_TIMEOUT_PROPERTY = "javadts.SOCKET_TIMEOUT";
    // Set to 30 seconds (in milliseconds). NOTE: it should be greater than 5
    // seconds, because the C++ side of the code waits 5 seconds before making
    // an attempt to connect to the Java socket server.
    public static final int SOCKET_TIMEOUT = 30000;

    // Default configurations
    public static final String DUMP_TRAFFIC_PROPERTY = "javadts.DUMP_TRAFFIC";
    public static final String DUMP_DEBUG_PROPERTY = "javadts.DUMP_DEBUG";
    public static final String CONFIG_XML_FILE_PROPERTY = "javadts.XML_FILE";
    public static final String DEFAULT_CONFIG_XML_FILE = "config/jira-rest-config.xml";

    /** Default JIRA issue values. */
    public static final String DEFAULT_ISSUE_SUMMARY = "New Issue";
    public static final int DEFAULT_ISSUE_DUEDATE = 7;

    /** Default JIRA issue fields. */
    public static final String ISSUE_FIELD_KEY = "Issue Key";
    public static final String ISSUE_FIELD_ISSUETYPE = "Issue Type";
    public static final String ISSUE_FIELD_SUMMARY = "Summary";
    public static final String ISSUE_FIELD_PRIORITY = "Priority";
    public static final String ISSUE_FIELD_DUEDATE = "Due Date";
    public static final String ISSUE_FIELD_COMPONENTS = "Component/s";
    public static final String ISSUE_FIELD_AFFECTSVERSIONS = "Affects Version/s";
    public static final String ISSUE_FIELD_FIXVERSIONS = "Fix Version/s";
    public static final String ISSUE_FIELD_ASSIGNEE = "Assignee";
    public static final String ISSUE_FIELD_REPORTER = "Reporter";
    public static final String ISSUE_FIELD_ENVIRONMENT = "Environment";
    public static final String ISSUE_FIELD_DESCRIPTION = "Description";
    public static final String ISSUE_FIELD_COMMENTS = "Comments";
    public static final String ISSUE_FIELD_STATUS = "Status";
    public static final String ISSUE_FIELD_RESOLUTION = "Resolution";
    public static final String ISSUE_FIELD_UPDATED = "Updated";
    public static final String ISSUE_FIELD_CREATED = "Created";

    /** Default JIRA custom field empty "None" select option */
    public static final String EMPTY_SELECT_OPTION = "<Empty>";

    /** Default JIRA issue fields map */
    public static final HashMap<String, String> ISSUE_FIELDS = new HashMap<String, String>();
    static {
        ISSUE_FIELDS.put(ISSUE_FIELD_KEY, "key");
        ISSUE_FIELDS.put(ISSUE_FIELD_ISSUETYPE, "issuetype");
        ISSUE_FIELDS.put(ISSUE_FIELD_SUMMARY, "summary");
        ISSUE_FIELDS.put(ISSUE_FIELD_PRIORITY, "priority");
        ISSUE_FIELDS.put(ISSUE_FIELD_DUEDATE, "dueDate");
        ISSUE_FIELDS.put(ISSUE_FIELD_COMPONENTS, "components");
        ISSUE_FIELDS.put(ISSUE_FIELD_AFFECTSVERSIONS, "versions");
        ISSUE_FIELDS.put(ISSUE_FIELD_FIXVERSIONS, "fixVersions");
        ISSUE_FIELDS.put(ISSUE_FIELD_ASSIGNEE, "assignee");
        ISSUE_FIELDS.put(ISSUE_FIELD_REPORTER, "reporter");
        ISSUE_FIELDS.put(ISSUE_FIELD_ENVIRONMENT, "environment");
        ISSUE_FIELDS.put(ISSUE_FIELD_DESCRIPTION, "description");
        ISSUE_FIELDS.put(ISSUE_FIELD_COMMENTS, "comments");
        ISSUE_FIELDS.put(ISSUE_FIELD_STATUS, "status");
        ISSUE_FIELDS.put(ISSUE_FIELD_RESOLUTION, "resolution");
        ISSUE_FIELDS.put(ISSUE_FIELD_UPDATED, "updated");
        ISSUE_FIELDS.put(ISSUE_FIELD_CREATED, "created");
        // Default JIRA custom field empty select option
        ISSUE_FIELDS.put(EMPTY_SELECT_OPTION, "");
    };

    /** DTG segment filter constructs **/
    public static final String EQUAL = "=";
    public static final String SINGLE_QUOTE = "'";
    public static final String DOUBLE_QUOTE = "\"";
    public static final String LEFT_SINGLE_QUOTE_EQUAL = "'=";
    public static final String LEFT_DOUBLE_QUOTE_EQUAL = "\"=";
    public static final String RIGHT_EQUAL_SINGLE_QUOTE = "='";
    public static final String RIGHT_EQUAL_DOUBLE_QUOTE = "=\"";
    public static final String LEFT_PAREN = "(";
    public static final String RIGHT_PAREN = ")";
    public static final String LEFT_PAREN_SINGLE_QUOTE = "('";
    public static final String LEFT_PAREN_DOUBLE_QUOTE = "(\"";
    public static final String SINGLE_QUOTE_RIGHT_PAREN = "')";
    public static final String DOUBLE_QUOTE_RIGHT_PAREN = "\")";

    /** DTG Project *All* (special value) */
    public static final String DTG_PROJECT_ALL = "*All*";

    /** DTG *Project* (special value) */
    public static final String DTG_PROJECT = "*Project*";

    /** DTG Project separator */
    public static final String DTG_PROJECT_SEPARATOR = ",";

    /** DTG Status/Resolution field name. */
    public static final String DTG_FIELD_STATUS_RESOLUTION = "Status/Resolution";

    /** DTG Fix field name */
    public static final String DTG_FIELD_FIX = "Fix";

    /** Date pattern. */
    public static final String DATE_PATTERN = "yyyy/MM/dd HH:mm:ss";

    /** Date pattern for JIRA server info. */
    // Ignoring the time zone (removed 'Z' at the end)
    // Important if DTG and JIRA server are on different time zones
    public static final String SERVER_INFO_DATE_PATTERN = "yyyy-MM-dd'T'HH:mm:ss.SSS";

    /** Date pattern for JIRA Query Language (JQL). */
    public static final String JQL_DATE_PATTERN = "yyyy/MM/dd HH:mm";

    /** JIRA query batch size */
    public static final int JQL_BATCH_SIZE = 100;
    
    /** max projects to include in a query **/
    public static final int JQL_MAX_PROJECTS = 10;

    /** Date pattern for JIRA due date. */
    public static final String DUE_DATE_PATTERN = "d/MMM/yy";

    /** Date pattern for JIRA custom fields. */
    public static final String CUSTOM_FIELD_DATE_PATTERN = "dd/MMM/yy";

    /** Date time pattern for JIRA custom fields. */
    public static final String CUSTOM_FIELD_DATE_TIME_PATTERN = "dd/MMM/yy h:mm a";

    /** JIRA custom field ID prefix. */
    public static final String CUSTOM_FIELD_ID_PREFIX = "customfield_";
    
    /** Separator for concatenating JIRA multi-select fields. */
    // Examples: versions and components
    public static final String MULTI_SELECT_SEPARATOR = ", ";

    /** Separator for concatenating JIRA comments. */
    public static final String COMMENT_SEPARATOR = "\n------\n";

    /** Empty String. */
    public static final String EMPTY_STRING = "";
}
