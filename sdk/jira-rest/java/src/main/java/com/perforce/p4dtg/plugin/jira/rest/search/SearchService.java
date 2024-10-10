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
package com.perforce.p4dtg.plugin.jira.rest.search;

import java.util.Set;

import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.domain.Issue;

public interface SearchService {

	/**
	 * Return issues matching the JQL search query.<p>
	 * 
	 * Note the restricted JQL characters must be properly escaped.
	 * 
	 * @see <a href="https://confluence.atlassian.com/display/JIRA/Advanced+Searching+Functions#AdvancedSearchingFunctions-characters">JQL Reserved Characters</a>
	 *
	 * @param jql        JQL search query.
	 * @return issues    matching the given JQL search query and parameters.
	 * @throws RestClientException
	 *                   rest client exception.
	 */
	Iterable<Issue> searchIssues(String jql);

	/**
	 * Return issues matching the JQL search query. The first startAt issues
	 * will be skipped and the number of returning issues will be limited by
	 * maxResults.<p>
	 * 
	 * Note the restricted JQL characters must be properly escaped.
	 * 
	 * @see <a href="https://confluence.atlassian.com/display/JIRA/Advanced+Searching+Functions#AdvancedSearchingFunctions-characters">JQL Reserved Characters</a>
	 *
	 * @param jql        JQL search query.
	 * @param maxResults maximum results for this JQL search query. If null, the
	 *                   configured maxResults in the JIRA server is used
	 *                   (default 50).
	 * @return issues    matching the given JQL search query and parameters.
	 * @throws RestClientException
	 *                   rest client exception.
	 */
	Iterable<Issue> searchIssues(String jql, Integer maxResults);

	/**
	 * Return issues matching the JQL search query. The first startAt issues
	 * will be skipped and the number of returning issues will be limited by
	 * maxResults.<p>
	 * 
	 * Note the restricted JQL characters must be properly escaped.
	 * 
	 * @see <a href="https://confluence.atlassian.com/display/JIRA/Advanced+Searching+Functions#AdvancedSearchingFunctions-characters">JQL Reserved Characters</a>
	 *
	 * @param jql        JQL search query.
	 * @param maxResults maximum results for this JQL search query. If null, the
	 *                   configured maxResults in the JIRA server is used
	 *                   (default 50).
	 * @param startAt    starting index (zero based) defining how many issues
	 *                   should be skipped in the results. If null, the default
	 *                   startAt is used (0).
	 * @return issues    matching the given JQL search query and parameters.
	 * @throws RestClientException
	 *                   rest client exception.
	 */
	Iterable<Issue> searchIssues(String jql, Integer maxResults, Integer startAt);

	/**
	 * Return issues matching the JQL search query. The first startAt issues
	 * will be skipped and the number of returning issues will be limited by
	 * maxResults. The returning issue fields can also be specified.<p>
	 * 
	 * Note the restricted JQL characters must be properly escaped.
	 * 
	 * @see <a href="https://confluence.atlassian.com/display/JIRA/Advanced+Searching+Functions#AdvancedSearchingFunctions-characters">JQL Reserved Characters</a>
	 *
	 * @param jql        JQL search query.
	 * @param maxResults maximum results for this JQL search query. If null, the
	 *                   configured maxResults in the JIRA server is used
	 *                   (default 50).
	 * @param startAt    starting index (zero based) defining how many issues
	 *                   should be skipped in the results. If null, the default
	 *                   startAt is used (0).
	 * @param fields     set of fields which should be retrieved. Specify *all
	 *                   for all fields or *navigable (default if null).
	 * @return issues    matching the given JQL search query and parameters.
	 * @throws RestClientException
	 *                   rest client exception.
	 */
	Iterable<Issue> searchIssues(String jql, Integer maxResults, Integer startAt, Set<String> fields);
}
