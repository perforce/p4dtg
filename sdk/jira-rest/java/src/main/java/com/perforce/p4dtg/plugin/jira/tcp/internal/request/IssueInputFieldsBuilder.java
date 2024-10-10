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
package com.perforce.p4dtg.plugin.jira.tcp.internal.request;

import static com.atlassian.jira.rest.client.api.domain.EntityHelper.findEntityByName;

import java.text.ParseException;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.logging.Logger;

import org.joda.time.DateTime;

import com.atlassian.jira.rest.client.api.GetCreateIssueMetadataOptionsBuilder;
import com.atlassian.jira.rest.client.api.domain.BasicPriority;
import com.atlassian.jira.rest.client.api.domain.CimIssueType;
import com.atlassian.jira.rest.client.api.domain.CimProject;
import com.atlassian.jira.rest.client.api.domain.Field;
import com.atlassian.jira.rest.client.api.domain.IssueFieldId;
import com.atlassian.jira.rest.client.api.domain.IssueType;
import com.atlassian.jira.rest.client.api.domain.Priority;
import com.atlassian.jira.rest.client.api.domain.input.ComplexIssueInputFieldValue;
import com.atlassian.jira.rest.client.api.domain.input.FieldInput;
import com.atlassian.jira.rest.client.api.domain.input.IssueInput;
import com.atlassian.jira.rest.client.api.domain.input.IssueInputBuilder;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.rest.client.RestClientManager;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;

/**
 * Builder for building the issue input fields.
 */
public class IssueInputFieldsBuilder {

    private static final Logger logger = Logger.getLogger(IssueInputFieldsBuilder.class.getPackage().getName());

    private RestClientManager restClientManager;
    private IssueFieldsMapper issueFieldsMapper;
    private Map<String, String[]> defectFields;
    private IssueInputBuilder issueInputBuilder;

    public IssueInputFieldsBuilder (String projectKey, Map<String, String[]> defectFields,
    		IssueFieldsMapper issueFieldsMapper, RestClientManager restClientManager) {
    	this.defectFields = defectFields;
    	this.issueFieldsMapper = issueFieldsMapper;
    	this.restClientManager = restClientManager;

    	// Create default issue input builder
    	this.issueInputBuilder = createDefaultIssueInputBuilder(projectKey);
    }

    /**
     * Builds the issue input from the defect fields.
     *
     * @return the issue input
     */
    public IssueInput build() {
    	if (defectFields != null) {
	       	IssueType issueType = restClientManager.getExtendedMetadataClient().getIssueType(
	       			Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_ISSUETYPE)).claim();
	       	if (issueType != null) {
	       		issueInputBuilder.setIssueType(issueType);
	       	}
	       	Priority priority = restClientManager.getExtendedMetadataClient().getPriority(
	       			Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_PRIORITY)).claim();
	       	if (priority != null) {
	       		issueInputBuilder.setPriority(priority);
	       	}
	       	String fixVersions = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_FIXVERSIONS);
	       	if (fixVersions != null) {
	       		String[] versions = fixVersions.split(Constants.MULTI_SELECT_SEPARATOR);
	       		if (versions != null) {
	       			issueInputBuilder.setFixVersionsNames(Arrays.asList(versions));
	       		}
	       	}
	       	String affectsVersions = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_AFFECTSVERSIONS);
	       	if (affectsVersions != null) {
	       		String[] versions = affectsVersions.split(Constants.MULTI_SELECT_SEPARATOR);
	       		if (versions != null) {
	       			issueInputBuilder.setAffectedVersionsNames(Arrays.asList(versions));
	       		}
	       	}
	       	String components = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_COMPONENTS);
	       	if (components != null) {
	       		String[] comps = components.split(Constants.MULTI_SELECT_SEPARATOR);
	       		if (comps != null) {
	       			issueInputBuilder.setComponentsNames(Arrays.asList(comps));
	       		}
	       	}
	       	String summary = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_SUMMARY);
	       	if (summary != null) {
	       		issueInputBuilder.setSummary(summary);
	       	}
	       	String reporter = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_REPORTER);
	       	if (reporter != null) {
	       		issueInputBuilder.setReporterName(reporter);
	       	}
	       	String assignee = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_ASSIGNEE);
	       	if (assignee != null) {
	       		issueInputBuilder.setAssigneeName(assignee);
	       	}
	       	String description = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_DESCRIPTION);
	       	if (description != null) {
	       		issueInputBuilder.setDescription(description);
	       	}
	       	String environment = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_ENVIRONMENT);
	       	if (environment != null) {
	       		issueInputBuilder.setFieldValue(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_ENVIRONMENT), environment);
	       	}
	       	String dueDate = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_DUEDATE);
	       	if (dueDate != null) {
	            try {
	                Calendar cal = Utils.parseCalendar(dueDate, Constants.DUE_DATE_PATTERN);
	                if (cal != null) {
	               		issueInputBuilder.setDueDate(new DateTime(cal.getTimeInMillis()));
	                }
	            } catch (ParseException pe) {
	                logger.warning(pe.getMessage());
	            }
	       	}
	       	String updated = Utils.getMapValue(defectFields, Constants.ISSUE_FIELD_UPDATED);
	       	if (updated != null) {
	            try {
	                Calendar cal = Utils.parseCalendar(updated, Constants.DUE_DATE_PATTERN);
	                if (cal != null) {
	                	issueInputBuilder.setFieldValue(Constants.ISSUE_FIELDS.get(
	                			Constants.ISSUE_FIELD_UPDATED), new DateTime(cal.getTimeInMillis()));
	                }
	            } catch (ParseException pe) {
	                logger.warning(pe.getMessage());
	            }
	       	}
	       	// Look for custom fields in the remaining map entries
	        for (Map.Entry<String, String[]> entry : defectFields.entrySet()) {
	            String key = entry.getKey();
	            String[] values = entry.getValue();
	            if (key != null && values != null && values.length > 0 && values[0] != null) {
	                String value = values[0];
	                // Check if it is a custom field
	                Field field = this.issueFieldsMapper.getCustomFieldByName(key);
	                if (field != null && field.getId() != null && field.getName() != null) {
	                    // Check the custom field type
	                    String type = this.issueFieldsMapper.getCustomFieldTypeByName(field.getName());
	                    if (type != null) {
		                	// Get the field id
		                	String id = field.getId();
	                        // Handle the date format conversion
	                        if (type.equalsIgnoreCase(IResponse.TYPE_DATE)) {
	                            try {
	                                Date date = Utils.parseDate(value, Constants.DATE_PATTERN);
	                                if (date != null) {
	                                    value = Utils.formatDate(date, Constants.CUSTOM_FIELD_DATE_TIME_PATTERN);
	                                }
	                            } catch (ParseException pe) {
	                                logger.warning(pe.getMessage());
	                            }
	                        }
	                        // Handle "<Empty>" select option
	                        if (type.equalsIgnoreCase(IResponse.TYPE_SELECT)) {
	                            if (Constants.ISSUE_FIELDS.containsKey(value)) {
	                                value = Constants.ISSUE_FIELDS.get(value);
	                            }
	                        }
		                    // Get field id from field name
		                    if (Constants.ISSUE_FIELDS.containsKey(id)) {
		                        id = Constants.ISSUE_FIELDS.get(id);
		                    }
		                    // Set the field
		                    // Handle select type as complex input field
		                    // Assume custom field, so use 'name' value
		                    FieldInput fieldInput = null;
		                    if (type.equalsIgnoreCase(IResponse.TYPE_SELECT)) {
		                    	fieldInput = new FieldInput(id, ComplexIssueInputFieldValue.with("value", value));
		                    } else {
		                    	fieldInput = new FieldInput(id, value);
		                    }
		                    issueInputBuilder.setFieldInput(fieldInput);
	                    }
	                }
	            }
	        }
        }
    	return issueInputBuilder.build();
    }

    /**
     * Create a default issue input builder with specified project key.
     *
     * @param projectKey
     *            the project key
     * @return the default issue input builder
     */
    private IssueInputBuilder createDefaultIssueInputBuilder(String projectKey) {
    	// Grab the project
		final Iterable<CimProject> metadataProjects = restClientManager.getExtendedIssueClient()
				.getCreateIssueMetadata(new GetCreateIssueMetadataOptionsBuilder()
				.withProjectKeys(projectKey).withExpandedIssueTypesFields().build()).claim();
		final CimProject project = metadataProjects.iterator().next();

		// Use "Bug" as the issue type for Jira Cloud.
		// leave old value of "BUG" just in case someone's on-prem uses "BUG".
		CimIssueType issueType ;
		try {
			issueType = findEntityByName(project.getIssueTypes(), "Bug");
		} catch(NoSuchElementException e) {
			issueType = findEntityByName(project.getIssueTypes(), "BUG");
		}

		// Grab the first priority
		final Iterable<Object> allowedValuesForPriority = issueType.getField(
				IssueFieldId.PRIORITY_FIELD).getAllowedValues();

		final BasicPriority priority = (BasicPriority) allowedValuesForPriority.iterator().next();

		// Prepare IssueInput
		final IssueInputBuilder issueInputBuilder = new IssueInputBuilder(
				project, issueType, Constants.DEFAULT_ISSUE_SUMMARY)
				.setAssigneeName(restClientManager.getUsername())
				.setPriority(priority);

		return issueInputBuilder;
    }
}
