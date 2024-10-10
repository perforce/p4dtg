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
package com.perforce.p4dtg.plugin.jira.rest.client;

import java.net.URI;

import com.atlassian.jira.rest.client.api.IssueRestClient;
import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.input.IssueInput;
import io.atlassian.util.concurrent.Promise;


/**
 * Extending the IssueRestClient interface to handle updating of issue resources
 * (fields) like summary, description, etc.
 * @see com.atlassian.jira.rest.client.api.MetadataRestClient
 */
public interface ExtendedIssueRestClient extends IssueRestClient {

	/**
	 * Performs selected transition on selected issue.
	 *
	 * @param issueUri  URI of selected issue. Usually obtained by calling <code>Issue.getSelf()</code>
	 * @param issueInput data for this issue (fields modified, the description, etc.)
	 * @throws RestClientException in case of problems (connectivity, malformed messages, invalid argument, etc.)
	 */
	Promise<Void> update(URI issueUri, IssueInput issueInput);

	/**
	 * Performs selected transition on selected issue.
	 *
	 * @param issue           selected issue
	 * @param issueInput data for this issue (fields modified, the description, etc.)
	 * @throws RestClientException in case of problems (connectivity, malformed messages, invalid argument, etc.)
	 */
	Promise<Void> update(Issue issue, IssueInput issueInput);
}
