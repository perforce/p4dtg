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
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.logging.Logger;

import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.IssueField;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.config.Configuration;
import com.perforce.p4dtg.plugin.jira.config.CustomField;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;
import org.codehaus.jettison.json.JSONArray;

/**
 * Builder for building the defect custom fields map.
 */
public class DefectCustomFieldsMapBuilder {

	static Logger logger = Logger.getLogger(DefectCustomFieldsMapBuilder.class.getPackage().getName());

	private Configuration configuration;
	private Issue issue;
	private IssueFieldsMapper issueFieldsMapper;

	public DefectCustomFieldsMapBuilder(Issue issue, IssueFieldsMapper issueFieldsMapper, Configuration configuration) {
		this.issue = issue;
		this.issueFieldsMapper = issueFieldsMapper;
		this.configuration = configuration;
	}

	/**
	 * Builds the defect custom fields.
	 *
	 * @return the defect custom fields map
	 */
	public Map<String, String[]> build() {
		Map<String, String[]> fieldValueMap = new HashMap<String, String[]>();
		if (this.issue != null && this.configuration != null && this.issueFieldsMapper != null) {
			// Set the custom field empty select values
			Map<String, String[]> customFieldSelectValues = buildEmptySelectOptions();
			if (customFieldSelectValues != null) {
				fieldValueMap.putAll(customFieldSelectValues);
			}
			Iterable<IssueField> fields = this.issue.getFields();
			if (fields != null) {
				for (IssueField field : fields) {
					if (field != null && field.getId() != null && field.getName() != null && field.getValue() != null) {
						// 
						// job104586.  Support any field in IssueField. was just 
						// customer fields: if (field.getId().startsWith(Constants.CUSTOM_FIELD_ID_PREFIX)) {
						if (true) {
							String type = this.issueFieldsMapper.getCustomFieldTypeByName(field.getName());
							String name = field.getName();
							if (type != null) {
								String value = "";
								// Handle select type as JSONObject
								// Note: make sure the custom fields are properly defined in the config xml file.
								if (type.equalsIgnoreCase(IResponse.TYPE_SELECT)) {
									if (field.getValue() instanceof JSONObject) {
										try {
											value = ((JSONObject) field.getValue()).getString("value");
										} catch (JSONException e) {
											logger.warning("Error getting the field " + field.getName() + " value: " + e.getLocalizedMessage());
										}
									}
								} else {

									if (field.getValue() instanceof String) {
										value = (String) field.getValue();

									} else if (field.getValue() instanceof JSONArray) {
										// Support for Label, job088895
										JSONArray values = (JSONArray) field.getValue();
										try {
											value = values.join(Constants.MULTI_SELECT_SEPARATOR);
										} catch (JSONException ex) {
											logger.warning("Error getting the field " + field.getName()
													+ "  value: " + ex.getLocalizedMessage());
										}

									} else if (field.getValue() instanceof Integer) {
										// job104586. support Integer Fields like "Time Spent". 
										value = ((Integer) field.getValue()).toString();
									
									} else if (field.getValue() instanceof Double) {
										// support Number Fields
										value = ((Double) field.getValue()).toString();
									}
								}
								// Handle data type DATE
								if (type.equalsIgnoreCase(IResponse.TYPE_DATE)) {
									Date date = null;
									// Parse with date time pattern
									try {
										date = Utils.parseDate(value,
												Constants.CUSTOM_FIELD_DATE_TIME_PATTERN);
									} catch (ParseException ignore) {
										// ignore
									}
									if (date == null) {
										// Parse with date pattern
										try {
											date = Utils.parseDate(value,
													Constants.CUSTOM_FIELD_DATE_PATTERN);
										} catch (ParseException ignore) {
											// ignore
										}
									}
									if (date != null) {
										value = Utils.formatDate(date, Constants.DATE_PATTERN);
									} else {
										logger.warning("Error parsing the date: " + value);
									}
								}
								fieldValueMap.put(name, new String[]{value});
							}
						}
					}
				}
			}
		}
		return fieldValueMap;
	}

	/**
	 * Builds the custom field empty select options map.
	 *
	 * @return the empty select options map
	 */
	private Map<String, String[]> buildEmptySelectOptions() {
		Map<String, String[]> customFieldValues = new LinkedHashMap<String, String[]>();
		if (this.configuration != null) {
			if (this.configuration.getCustomFields() != null) {
				for (CustomField cf : this.configuration.getCustomFields()) {
					if (cf != null) {
						if (cf.getName() != null) {
							if (cf.getType() != null) {
								if (cf.getType().trim().equalsIgnoreCase("SELECT")) {
									customFieldValues.put(cf.getName(),
											new String[]{Constants.EMPTY_SELECT_OPTION});
								}
							}
						}
					}
				}
			}
		}
		return customFieldValues;
	}
}
