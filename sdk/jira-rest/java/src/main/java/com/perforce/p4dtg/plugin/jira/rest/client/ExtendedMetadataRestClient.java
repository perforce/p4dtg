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

import com.atlassian.jira.rest.client.api.MetadataRestClient;
import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.domain.IssueType;
import com.atlassian.jira.rest.client.api.domain.Priority;
import com.atlassian.jira.rest.client.api.domain.Resolution;
import com.atlassian.jira.rest.client.api.domain.Status;
import io.atlassian.util.concurrent.Promise;

/**
 * Extending the MetadataRestClient interface to serve extended JIRA metadata
 * information like statuses and extended resolutions.
 * 
 * @see com.atlassian.jira.rest.client.api.MetadataRestClient
 */
public interface ExtendedMetadataRestClient extends MetadataRestClient {

	/**
	 * Retrieves from the server complete list of available issue statuses.
	 *
	 * @return list of available issue statuses for this JIRA instance
	 * @throws RestClientException in case of problems (connectivity, malformed messages, etc.)
	 */
	Promise<Iterable<Status>> getStatuses();

	/**
	 * Retrieves from the server complete information about selected issue type by name.
	 *
	 * @param type unique name of the issue type resource
	 * @return complete information about issue type resource
	 * @throws RestClientException in case of problems (connectivity, malformed messages, etc.)
	 */
	Promise<IssueType> getIssueType(String type);

	/**
	 * Retrieves from the server complete information about selected status by name.
	 *
	 * @param status unique name of the status resource
	 * @return complete information about the selected status
	 * @throws RestClientException in case of problems (connectivity, malformed messages, etc.)
	 */
	Promise<Status> getStatus(String status);

	/**
	 * Retrieves from the server complete information about selected priority by name.
	 *
	 * @param priority unique name of the priority resource
	 * @return complete information about the selected priority
	 * @throws RestClientException in case of problems (connectivity, malformed messages, etc.)
	 */
	Promise<Priority> getPriority(String priority);

	/**
	 * Retrieves from the server complete information about selected resolution by name.
	 *
	 * @param resolution unique name of the resolution resource
	 * @return complete information about the selected resolution
	 * @throws RestClientException in case of problems (connectivity, malformed messages, etc.)
	 */
	Promise<Resolution> getResolution(String resolution);
}
