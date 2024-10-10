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
package com.perforce.p4dtg.plugin.jira.rest.internal.client;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.net.URI;
import java.nio.charset.Charset;
import java.util.Collections;
import java.util.Map;

import org.codehaus.jettison.json.JSONException;

import com.atlassian.httpclient.api.EntityBuilder;
import com.atlassian.httpclient.api.HttpClient;
import com.atlassian.httpclient.api.ResponsePromise;
import com.atlassian.jira.rest.client.api.MetadataRestClient;
import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.SessionRestClient;
import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.input.IssueInput;
import com.atlassian.jira.rest.client.internal.async.AsynchronousIssueRestClient;
import com.atlassian.jira.rest.client.internal.json.gen.IssueInputJsonGenerator;
import com.atlassian.jira.rest.client.internal.json.gen.JsonGenerator;
import com.perforce.p4dtg.plugin.jira.rest.client.ExtendedIssueRestClient;
import io.atlassian.util.concurrent.Promise;

/**
 * Asynchronous implementation of ExtendedIssueRestClient.
 */
public class AsynchronousExtendedIssueRestClient extends AsynchronousIssueRestClient implements ExtendedIssueRestClient {

	private static final String JSON_CONTENT_TYPE = "application/json";

	private final HttpClient client;

	public AsynchronousExtendedIssueRestClient(final URI baseUri, final HttpClient client, final SessionRestClient sessionRestClient,
			final MetadataRestClient metadataRestClient) {
		super(baseUri, client, sessionRestClient, metadataRestClient);
		this.client = client;
	}

	/**
	 * @see com.perforce.p4dtg.plugin.jira.rest.client.ExtendedIssueRestClient#update(URI, IssueInput)
	 */
	@Override
	public Promise<Void> update(final URI issueUri, final IssueInput issueInput) {
		return put(issueUri, issueInput, new IssueInputJsonGenerator());
	}

	/**
	 * @see com.perforce.p4dtg.plugin.jira.rest.client.ExtendedIssueRestClient#update(Issue, IssueInput)
	 */
	@Override
	public Promise<Void> update(final Issue issue, final IssueInput issueInput) {
		return update(issue.getSelf(), issueInput);
	}

	/***  
	 * I have no idea why this was implemented here, as the current AbstractAsynchronousRestClient.put()
	 * code matches this code.
	 * Comment out now, as Parent class (AbstractAsynchronousRestClient).put() is final.  
	protected final <T> Promise<Void> put(final URI uri, final T entity, final JsonGenerator<T> jsonGenerator) {
		final ResponsePromise responsePromise = client.newRequest(uri)
				.setEntity(toEntity(jsonGenerator, entity))
				.put();
		return call(responsePromise);
	}
	* **/

	private <T> EntityBuilder toEntity(final JsonGenerator<T> generator, final T bean) {
		return new EntityBuilder() {

			@Override
			public Entity build() {
				return new Entity() {
					@Override
					public Map<String, String> getHeaders() {
						return Collections.singletonMap("Content-Type", JSON_CONTENT_TYPE);
					}

					@Override
					public InputStream getInputStream() {
						try {
							return new ByteArrayInputStream(generator.generate(bean).toString().getBytes(Charset
									.forName("UTF-8")));
						} catch (JSONException e) {
							throw new RestClientException(e);
						}
					}
				};
			}
		};
	}
}
