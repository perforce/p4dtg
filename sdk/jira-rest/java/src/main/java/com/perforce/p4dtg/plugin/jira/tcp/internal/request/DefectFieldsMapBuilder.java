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

import java.util.HashMap;
import java.util.Map;

import com.atlassian.jira.rest.client.api.domain.BasicComponent;
import com.atlassian.jira.rest.client.api.domain.Comment;
import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.IssueField;
import com.atlassian.jira.rest.client.api.domain.TimeTracking;
import com.atlassian.jira.rest.client.api.domain.Version;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.config.Configuration;

/**
 * Builder for building the defect fields map.
 */
public class DefectFieldsMapBuilder {

	private Configuration configuration;
	private Issue issue;
	private IssueFieldsMapper issueFieldsMapper;

	public DefectFieldsMapBuilder(Issue issue, IssueFieldsMapper issueFieldsMapper,
			Configuration configuration) {
		this.issue = issue;
		this.issueFieldsMapper = issueFieldsMapper;
		this.configuration = configuration;
	}

	/**
	 * Builds the defect fields map.
	 *
	 * @return the defect fields map
	 */
	public Map<String, String[]> build() {
		Map<String, String[]> fieldValueMap = new HashMap<String, String[]>();
		if (issue != null && configuration != null && issueFieldsMapper != null) {
			if (issue.getKey() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_KEY, new String[]{issue.getKey()});
			}
			if (issue.getReporter() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_REPORTER, new String[]{UserNameHelper.getUserValue(issue.getReporter())});
			}
			if (issue.getAssignee() != null) {
				// fieldValueMap.put(Constants.ISSUE_FIELD_ASSIGNEE, new String[]{issue.getAssignee().getName()});
				fieldValueMap.put(Constants.ISSUE_FIELD_ASSIGNEE, new String[]{UserNameHelper.getUserValue(issue.getAssignee())});

			}
			if (issue.getSummary() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_SUMMARY, new String[]{issue.getSummary()});
			}
			if (issue.getDescription() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_DESCRIPTION, new String[]{issue.getDescription()});
			}
			IssueField envField = issue.getFieldByName(Constants.ISSUE_FIELD_ENVIRONMENT);
			if (envField != null && envField.getValue() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_ENVIRONMENT, new String[]{envField.getValue().toString()});
			}
			Iterable<Comment> comments = issue.getComments();
			if (comments != null) {
				StringBuilder sb = new StringBuilder();
				int count = 0;
				for (Comment comment : comments) {
					if (count > 0) {
						sb.append(Constants.COMMENT_SEPARATOR);
					}
					sb.append(comment.getBody());
					count++;
				}
				fieldValueMap.put(Constants.ISSUE_FIELD_COMMENTS, new String[]{sb.toString()});
			}
			Iterable<Version> affectsVersions = issue.getAffectedVersions();
			if (affectsVersions != null) {
				StringBuilder sb = new StringBuilder();
				int count = 0;
				for (Version version : affectsVersions) {
					if (count > 0) {
						sb.append(Constants.MULTI_SELECT_SEPARATOR);
					}
					sb.append(version.getName());
					count++;
				}
				fieldValueMap.put(Constants.ISSUE_FIELD_AFFECTSVERSIONS, new String[]{sb.toString()});
			}
			Iterable<Version> fixVersions = issue.getFixVersions();
			if (fixVersions != null) {
				StringBuilder sb = new StringBuilder();
				int count = 0;
				for (Version version : fixVersions) {
					if (count > 0) {
						sb.append(Constants.MULTI_SELECT_SEPARATOR);
					}
					sb.append(version.getName());
					count++;
				}
				fieldValueMap.put(Constants.ISSUE_FIELD_FIXVERSIONS, new String[]{sb.toString()});
			}
			Iterable<BasicComponent> components = issue.getComponents();
			if (components != null) {
				StringBuilder sb = new StringBuilder();
				int count = 0;
				for (BasicComponent component : components) {
					if (count > 0) {
						sb.append(Constants.MULTI_SELECT_SEPARATOR);
					}
					sb.append(component.getName());
					count++;
				}
				fieldValueMap.put(Constants.ISSUE_FIELD_COMPONENTS, new String[]{sb.toString()});
			}
			if (issue.getDueDate() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_DUEDATE, new String[]{
					Utils.formatDate(issue.getDueDate().toDate(), Constants.DATE_PATTERN)});
			}
			if (issue.getUpdateDate() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_UPDATED, new String[]{
					Utils.formatDate(issue.getUpdateDate().toDate(), Constants.DATE_PATTERN)});
			}
			if (issue.getIssueType() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_ISSUETYPE, new String[]{issue.getIssueType().getName()});
			}
			if (issue.getPriority() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_PRIORITY, new String[]{issue.getPriority().getName()});
			}
			if (issue.getStatus() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_STATUS, new String[]{issue.getStatus().getName()});
			}
			if (issue.getResolution() != null) {
				fieldValueMap.put(Constants.ISSUE_FIELD_RESOLUTION, new String[]{issue.getResolution().getName()});
			}

			// Build custom fields map
			DefectCustomFieldsMapBuilder dcfmBuilder = new DefectCustomFieldsMapBuilder(
					this.issue, this.issueFieldsMapper, this.configuration);
			Map<String, String[]> customFieldValueMap = dcfmBuilder.build();
			if (customFieldValueMap != null) {
				fieldValueMap.putAll(customFieldValueMap);
			}
		}
		return fieldValueMap;
	}

}
