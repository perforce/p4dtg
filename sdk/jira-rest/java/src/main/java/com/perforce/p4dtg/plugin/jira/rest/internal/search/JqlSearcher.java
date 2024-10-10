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
package com.perforce.p4dtg.plugin.jira.rest.internal.search;

import java.net.URISyntaxException;
import java.util.Set;

import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.SearchResult;
import com.perforce.p4dtg.plugin.jira.rest.client.RestClientManager;
import com.perforce.p4dtg.plugin.jira.rest.search.SearchService;

public class JqlSearcher implements SearchService {

	private RestClientManager clientManager = null;
	
	/**
	 * Constructor for instantiating a new JqlSearcher.
	 *
	 * @param clientManager the REST client manager
	 * @throws URISyntaxException the URI syntax exception
	 */
	public JqlSearcher(RestClientManager clientManager) {
		this.clientManager = clientManager;
	}
	
	/**
	 * @see com.perforce.p4dtg.plugin.jira.rest.search.SearchService#searchIssues(java.lang.String)
	 */
	public Iterable<Issue> searchIssues(String jql) {
		return searchIssues(jql, null);
	}

	/**
	 * @see com.perforce.p4dtg.plugin.jira.rest.search.SearchService#searchIssues(java.lang.String, java.lang.Integer)
	 */
	public Iterable<Issue> searchIssues(String jql, Integer maxResults) {
		return searchIssues(jql, maxResults, null, null);
	}

	/**
	 * @see com.perforce.p4dtg.plugin.jira.rest.search.SearchService#searchIssues(java.lang.String, java.lang.Integer, java.lang.Integer)
	 */
	public Iterable<Issue> searchIssues(String jql, Integer maxResults, Integer startAt) {
		SearchResult searchResult = clientManager.getSearchClient().searchJql(jql, maxResults, startAt, null).claim();
		return searchResult.getIssues();
	}

	/**
	 * @see com.perforce.p4dtg.plugin.jira.rest.search.SearchService#searchIssues(java.lang.String, java.lang.Integer, java.lang.Integer, java.util.Set)
	 */
	public Iterable<Issue> searchIssues(String jql, Integer maxResults, Integer startAt, Set<String> fields) {
		SearchResult searchResult = clientManager.getSearchClient().searchJql(jql, maxResults, startAt, fields).claim();
		return searchResult.getIssues();
	}
}
