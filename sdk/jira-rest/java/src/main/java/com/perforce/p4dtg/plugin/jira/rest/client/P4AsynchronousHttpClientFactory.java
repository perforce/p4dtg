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

import com.atlassian.event.api.EventPublisher;
import com.atlassian.httpclient.apache.httpcomponents.DefaultHttpClientFactory;
import com.atlassian.httpclient.api.HttpClient;
import com.atlassian.httpclient.api.factory.HttpClientOptions;
import com.atlassian.jira.rest.client.api.AuthenticationHandler;
import com.atlassian.jira.rest.client.internal.async.AsynchronousHttpClientFactory;
import com.atlassian.jira.rest.client.internal.async.AtlassianHttpClientDecorator;
import com.atlassian.jira.rest.client.internal.async.DisposableHttpClient;
import com.atlassian.sal.api.ApplicationProperties;
import com.atlassian.sal.api.UrlMode;
import com.atlassian.sal.api.executor.ThreadLocalContextManager;
import java.io.File;
import java.net.URI;
import java.nio.file.Path;
import java.util.Date;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.annotation.Nonnull;

/**
 * Creates clients for Jira Rest Client This custom class is to allow setting
 * timeouts for Jira Rest calls.
 *
 * @author jbrown
 */
public class P4AsynchronousHttpClientFactory extends AsynchronousHttpClientFactory {

	private static Integer socketTimeout = null;
	private static Integer requestTimeout = null;
	private static Integer connectionTimeout = null;
	
	private static final Logger logger = Logger.getLogger(P4AsynchronousHttpClientFactory.class.getPackage().getName());

	// the timeouts are defined in the jira config file (eg jira-rest-config.xml) and 
	// updated here when the config file is loaded => configuration.parse().
	
	public static void setSocketTimeout(int socketTimeout) {
		P4AsynchronousHttpClientFactory.socketTimeout = socketTimeout;
        logger.log(Level.INFO, "Setting Socket Timeout to {0} seconds.", socketTimeout);
	}

	public static void setRequestTimeout(int requestTimeout) {
		P4AsynchronousHttpClientFactory.requestTimeout = requestTimeout;
        logger.log(Level.INFO, "Setting Request Timeout to {0} seconds.", requestTimeout);
	}

	public static void setConnectionTimeout(int connectionTimeout) {
		P4AsynchronousHttpClientFactory.connectionTimeout = connectionTimeout;
        logger.log(Level.INFO, "Setting Connection Timeout to {0} seconds.", connectionTimeout);
	}

	/**
	 * Creates a client for Jira access, possibly with timeouts other than the default.
	 * @param serverUri
	 * @param authenticationHandler
	 * @return
	 */
	@SuppressWarnings("unchecked")
	@Override
	public DisposableHttpClient createClient(final URI serverUri, final AuthenticationHandler authenticationHandler) {

		final HttpClientOptions options = new HttpClientOptions();
		if (socketTimeout != null) {
			options.setSocketTimeout(socketTimeout, TimeUnit.SECONDS);
		}
		if (requestTimeout != null) {
			options.setRequestTimeout(requestTimeout, TimeUnit.SECONDS);
		}
		if (connectionTimeout != null) {
			options.setConnectionTimeout(connectionTimeout, TimeUnit.SECONDS);
		}

		final DefaultHttpClientFactory defaultHttpClientFactory = new DefaultHttpClientFactory(new NoOpEventPublisher(),
				new RestClientApplicationProperties(serverUri),
				new ThreadLocalContextManager() {
			@Override
			public Object getThreadLocalContext() {
				return null;
			}

			@Override
			public void setThreadLocalContext(Object context) {
			}

			@Override
			public void clearThreadLocalContext() {
			}
		});

		final HttpClient httpClient = defaultHttpClientFactory.create(options);

		return new AtlassianHttpClientDecorator(httpClient, authenticationHandler) {
			@Override
			public void destroy() throws Exception {
				defaultHttpClientFactory.dispose(httpClient);
			}
		};
	}

	private static class NoOpEventPublisher implements EventPublisher {

		@Override
		public void publish(Object o) {
		}

		@Override
		public void register(Object o) {
		}

		@Override
		public void unregister(Object o) {
		}

		@Override
		public void unregisterAll() {
		}
	}

	/**
	 * These properties are used to present JRJC as a User-Agent during http
	 * requests.
	 */
	@SuppressWarnings("deprecation")
	private static class RestClientApplicationProperties implements ApplicationProperties {

		@Override
		public String getApplicationFileEncoding() {
			return "UTF8";
		}

		private final String baseUrl;

		private RestClientApplicationProperties(URI jiraURI) {
			this.baseUrl = jiraURI.getPath();
		}

		@Override
		public String getBaseUrl() {
			return baseUrl;
		}

		/**
		 * We'll always have an absolute URL as a client.
		 */
		@Nonnull
		@Override
		public String getBaseUrl(UrlMode urlMode) {
			return baseUrl;
		}

		@Nonnull
		@Override
		public String getDisplayName() {
			return "P4DTG P4 Atlassian JIRA Rest Java Client";
		}

		@Nonnull
		@Override
		public String getPlatformId() {
			return ApplicationProperties.PLATFORM_JIRA;
		}

		@Nonnull
		@Override
		public String getVersion() {
			return "P4 2021.2";
		}

		@Nonnull
		@Override
		public Date getBuildDate() {
			return new Date(2022, 4, 1);
		}

		@Nonnull
		@Override
		public String getBuildNumber() {
			return String.valueOf(0);
		}

		@Override
		public File getHomeDirectory() {
			return new File(".");
		}

		@Override
		public String getPropertyValue(final String s) {
			throw new UnsupportedOperationException("Not implemented");
		}

		@Override
		public Optional<Path> getLocalHomeDirectory() {
			return Optional.empty();
		}

		@Override
		public Optional<Path> getSharedHomeDirectory() {
			return Optional.empty();
		}
		
	}

}
