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

import java.text.ParseException;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;

import com.atlassian.jira.rest.client.api.domain.Field;
import com.atlassian.jira.rest.client.api.domain.IssueType;
import com.atlassian.jira.rest.client.api.domain.Priority;
import com.atlassian.jira.rest.client.api.domain.Resolution;
import com.atlassian.jira.rest.client.api.domain.Status;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.rest.client.RestClientManager;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;

/**
 * Translator for translating defect fields to JIRA issue fields.
 */
public class DefectFieldsTranslator {

    private static final Logger logger = Logger.getLogger(DefectFieldsTranslator.class.getPackage().getName());
	
    private RestClientManager restClientManager;
    private IssueFieldsMapper issueFieldsMapper;
    private Map<String, String[]> defectFields;

    public DefectFieldsTranslator(Map<String, String[]> defectFields, IssueFieldsMapper fieldsMapper,
    		RestClientManager restClientManager) {
    	this.defectFields = defectFields;
    	this.issueFieldsMapper = fieldsMapper;
    	this.restClientManager = restClientManager;
    }

    /**
     * Translates defect fields to JIRA issue fields.
     *
     * @return the translated issue fields map
     */
    public Map<String, String[]> translate() {
        Map<String, String[]> issueFields = new HashMap<String, String[]>();
    	if (this.defectFields != null && this.issueFieldsMapper != null && restClientManager != null) {
	       	String dueDate = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_DUEDATE);
	       	if (dueDate != null) {
	            try {
	                Date date = Utils.parseDate(dueDate, Constants.DATE_PATTERN);
	                if (date != null) {
	                	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_DUEDATE),
	                			new String[] {Utils.formatDate(date, Constants.DUE_DATE_PATTERN)});
	                }
	            } catch (ParseException pe) {
	                logger.warning(pe.getMessage());
	            }
	       	}
	       	String updated = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_UPDATED);
	       	if (updated != null) {
	       		try {
	                Date date = Utils.parseDate(updated, Constants.DATE_PATTERN);
	                if (date != null) {
	                	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_UPDATED),
	                			new String[] {Utils.formatDate(date, Constants.DUE_DATE_PATTERN)});
	                }
	            } catch (ParseException pe) {
	                logger.warning(pe.getMessage());
	            }
	       	}
	       	String fix = Utils.getMapValue(this.defectFields, Constants.DTG_FIELD_FIX);
	       	if (fix != null) {
	       		issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_COMMENTS),
	       				new String[] {fix});
	       	}
	       	String summary = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_SUMMARY);
	       	if (summary != null) {
	       		issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_SUMMARY),
	       				new String[] {summary});
	       	}
	       	String reporter = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_REPORTER);
	       	if (reporter != null) {
	       		issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_REPORTER),
	       				new String[] {reporter});
	       	}
	       	String assignee = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_ASSIGNEE);
	       	if (assignee != null) {
	       		issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_ASSIGNEE),
	       				new String[] {assignee});
	       	}
	       	String description = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_DESCRIPTION);
	       	if (description != null) {
	       		issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_DESCRIPTION),
	       				new String[] {description});
	       	}
	       	IssueType issueType = restClientManager.getExtendedMetadataClient().getIssueType(
	       			Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_ISSUETYPE)).claim();
	       	if (issueType != null) {
	        	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_ISSUETYPE),
	        			new String[] {issueType.getId().toString()});
	       	}
	       	Priority priority = restClientManager.getExtendedMetadataClient().getPriority(
	       			Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_PRIORITY)).claim();
	       	if (priority != null) {
	        	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_PRIORITY),
	        			new String[] {priority.getId().toString()});
	       	}
	       	Status status = restClientManager.getExtendedMetadataClient().getStatus(
	       			Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_STATUS)).claim();
	       	if (status != null) {
	        	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_STATUS),
	        			new String[] {Utils.getIdFromUri(status.getSelf())});
	       	}
	       	Resolution resolution = restClientManager.getExtendedMetadataClient().getResolution(
	       			Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_RESOLUTION)).claim();
	       	if (resolution != null) {
	        	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_RESOLUTION),
	        			new String[] {Utils.getIdFromUri(resolution.getSelf())});
	       	}
	       	String fixVersions = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_FIXVERSIONS);
	       	if (fixVersions != null) {
	       		String[] versions = fixVersions.split(Constants.MULTI_SELECT_SEPARATOR);
	       		if (versions != null) {
	            	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_FIXVERSIONS), versions);
	       		}
	       	}
	       	String affectsVersions = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_AFFECTSVERSIONS);
	       	if (affectsVersions != null) {
	       		String[] versions = affectsVersions.split(Constants.MULTI_SELECT_SEPARATOR);
	       		if (versions != null) {
	            	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_AFFECTSVERSIONS), versions);
	       		}
	       	}
	       	String components = Utils.getMapValue(this.defectFields, Constants.ISSUE_FIELD_COMPONENTS);
	       	if (components != null) {
	       		String[] comps = components.split(Constants.MULTI_SELECT_SEPARATOR);
	       		if (comps != null) {
	            	issueFields.put(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_COMPONENTS), comps);
	       		}
	       	}
	       	// Look for custom fields in the remaining map entries
	        for (Map.Entry<String, String[]> entry : this.defectFields.entrySet()) {
	            String key = entry.getKey();
	            String[] values = entry.getValue();
	            if (key != null && values != null && values.length > 0 && values[0] != null) {
	                String value = values[0];
	                
	                // Check if it is a custom field
	                Field field = this.issueFieldsMapper.getCustomFieldByName(key);
	                if (field != null) {
	                	// Get the field id
	                	key = field.getId();
	                    // Check the custom field type
	                    String type = this.issueFieldsMapper.getCustomFieldTypeByName(field.getName());
	                    if (type != null) {
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
	                    }
	                    // Get field id from field name
	                    if (Constants.ISSUE_FIELDS.containsKey(key)) {
	                        key = Constants.ISSUE_FIELDS.get(key);
	                    }
	                    // Add the field
	                    issueFields.put(key, new String[] {value});
	                }
	            }
	        }
    	}
    	return issueFields;
    }
}
