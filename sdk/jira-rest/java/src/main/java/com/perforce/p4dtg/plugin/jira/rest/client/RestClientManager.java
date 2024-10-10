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

import com.atlassian.jira.rest.client.api.AuthenticationHandler;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import javax.ws.rs.core.UriBuilder;

import com.atlassian.jira.rest.client.api.ComponentRestClient;
import com.atlassian.jira.rest.client.api.JiraRestClient;
import com.atlassian.jira.rest.client.api.ProjectRestClient;
import com.atlassian.jira.rest.client.api.ProjectRolesRestClient;
import com.atlassian.jira.rest.client.api.SearchRestClient;
import com.atlassian.jira.rest.client.api.SessionRestClient;
import com.atlassian.jira.rest.client.api.UserRestClient;
import com.atlassian.jira.rest.client.api.VersionRestClient;
import com.atlassian.jira.rest.client.auth.BasicHttpAuthenticationHandler;
import com.atlassian.jira.rest.client.internal.async.AsynchronousJiraRestClient;
import com.atlassian.jira.rest.client.internal.async.DisposableHttpClient;
import com.perforce.p4dtg.plugin.jira.rest.internal.client.AsynchronousExtendedIssueRestClient;
import com.perforce.p4dtg.plugin.jira.rest.internal.client.AsynchronousExtendedMetadataRestClient;

import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;

/**
 * This class is responsible for managing the life cycle of various JIRA rest
 * clients. Note that this manager must be initialized by calling the 'init()'
 * method in order for it to be useful.
 */
public class RestClientManager {

    private static final Logger logger = Logger.getLogger(RestClientManager.class.getPackage().getName());
    private static final String DEFAULT_URL_PATH = "/rest/api/latest";
    private JiraRestClient jiraRestClient = null;
    private SearchRestClient searchClient = null;
    private SessionRestClient sessionClient = null;
    private UserRestClient userClient = null;
    private ProjectRestClient projectClient = null;
    private ProjectRolesRestClient projectRolesRestClient = null;
    private ExtendedIssueRestClient extendedIssueClient = null;
    private ComponentRestClient componentClient = null;
    private VersionRestClient versionRestClient = null;
    private ExtendedMetadataRestClient extendedMetadataClient = null;
    private String serverUri = null;
    private String username = null;
    private String password = null;
    private String url_path = DEFAULT_URL_PATH;

    /**
     * Constructor for creating an rest client manager that initializes various
     * specialized rest clients.
     *
     * @param serverUri the JIRA server uri
     * @param username  the JIRA username
     * @param password  the JIRA password
     * @throws URISyntaxException the uRI syntax exception
     */
    public RestClientManager(String serverUri, String username, String password) throws URISyntaxException {
        this(serverUri, username, password, null, null);
    }

    /**
     * Constructor for creating an rest client manager that initializes various
     * specialized rest clients.<p>
     * <p>
     * Note: for connecting to a secure JIRA server that uses a self-signed SSL
     * certificate you must first add the self-signed certificate to your JVM
     * trust store prior to using this constructor.
     *
     * @param serverUri          the JIRA server uri
     * @param username           the JIRA username
     * @param password           the JIRA password
     * @param trustStorePath     the SSL trust store file path
     * @param trustStorePassword the SSL trust store password (null, if no
     *                           password required)
     * @throws URISyntaxException the uRI syntax exception
     */
    public RestClientManager(String serverUri, String username, String password, String trustStorePath,
                             String trustStorePassword) throws URISyntaxException {
        // Trust all certificates (self-signed) for HTTPS (SSL) connections.
        // Might not work, since JRJC uses Apache HttpAsyncClient 4.0.
        //disableCertificateValidation();

        // Set the SSL trust store file path
        if (trustStorePath != null) {
            System.setProperty("javax.net.ssl.trustStore", trustStorePath);
        }
        // This is not necessary since we're only reading the trust store.
        if (trustStorePassword != null) {
            System.setProperty("javax.net.ssl.trustStorePassword", trustStorePassword);
        }

        this.serverUri = serverUri;
        this.username = username;
        this.password = password;

        URI serverUriObject = new URI(serverUri);

        P4AsynchronousHttpClientFactory cf = new P4AsynchronousHttpClientFactory();

        DisposableHttpClient timeoutClient = cf.createClient(serverUriObject, getAuthHandler());

        this.jiraRestClient = new AsynchronousJiraRestClient(serverUriObject, timeoutClient);

        this.sessionClient = this.jiraRestClient.getSessionClient();
        this.userClient = this.jiraRestClient.getUserClient();
        this.projectClient = this.jiraRestClient.getProjectClient();
        this.componentClient = this.jiraRestClient.getComponentClient();
        this.searchClient = this.jiraRestClient.getSearchClient();
        this.versionRestClient = this.jiraRestClient.getVersionRestClient();
        this.projectRolesRestClient = this.jiraRestClient.getProjectRolesRestClient();

        URI baseUri = UriBuilder.fromUri(serverUri).path(url_path).build();

        DisposableHttpClient httpClient = new P4AsynchronousHttpClientFactory().createClient(serverUriObject, getAuthHandler());

        // Create the extended metadata client
        this.extendedMetadataClient = new AsynchronousExtendedMetadataRestClient(baseUri, httpClient);

        // Create the extended issue client
        this.extendedIssueClient = new AsynchronousExtendedIssueRestClient(baseUri, httpClient, this.sessionClient, this.extendedMetadataClient);
    }

    /**
     * Disable certificate validation.
     */
    @SuppressWarnings("unused")
    private static void disableCertificateValidation() {
        // Create a trust manager that does not validate certificate chains
        TrustManager[] trustAllCerts = new TrustManager[]{
                new X509TrustManager() {
                    public X509Certificate[] getAcceptedIssuers() {
                        return new X509Certificate[0];
                    }

                    @Override
                    public void checkClientTrusted(X509Certificate[] certs, String authType) {
                    }

                    @Override
                    public void checkServerTrusted(X509Certificate[] certs, String authType) {
                    }
                }
        };

        // Ignore differences between given hostname and certificate hostname
        HostnameVerifier hv = new HostnameVerifier() {
            @Override
            public boolean verify(String hostname, SSLSession session) {
                return true;
            }
        };

        // Install the all-trusting trust manager
        try {
            SSLContext sc = SSLContext.getInstance("SSL");
            sc.init(null, trustAllCerts, new SecureRandom());
            HttpsURLConnection.setDefaultSSLSocketFactory(sc.getSocketFactory());
            HttpsURLConnection.setDefaultHostnameVerifier(hv);
        } catch (KeyManagementException | NoSuchAlgorithmException e) {
            logger.log(Level.SEVERE, "Exception occurred while initializing SSL context.", e);
        }
    }

    private AuthenticationHandler getAuthHandler() {
        AuthenticationHandler authHandler;
        if ("".equals(this.username) || "*".equals(this.username)) {
            authHandler = new BearerHttpAuthenticationHandler(password);
        } else {
            authHandler = new BasicHttpAuthenticationHandler(username, password);
        }
        return authHandler;
    }

    /**
     * Dispose the JIRA rest client cleanly.
     *
     * @throws IOException
     */
    public void dispose() throws IOException {
        this.jiraRestClient.close();
    }

    /**
     * Gets the extended issue client.
     *
     * @return the extended issue client
     */
    public ExtendedIssueRestClient getExtendedIssueClient() {
        return extendedIssueClient;
    }

    /**
     * Sets the extended issue client.
     *
     * @param extendedIssueClient the new extended issue client
     */
    public void setExtendedIssueClient(ExtendedIssueRestClient extendedIssueClient) {
        this.extendedIssueClient = extendedIssueClient;
    }

    /**
     * Gets the session client.
     *
     * @return the session client
     */
    public SessionRestClient getSessionClient() {
        return sessionClient;
    }

    /**
     * Sets the session client.
     *
     * @param sessionClient the new session client
     */
    public void setSessionClient(SessionRestClient sessionClient) {
        this.sessionClient = sessionClient;
    }

    /**
     * Gets the user client.
     *
     * @return the user client
     */
    public UserRestClient getUserClient() {
        return userClient;
    }

    /**
     * Sets the user client.
     *
     * @param userClient the new user client
     */
    public void setUserClient(UserRestClient userClient) {
        this.userClient = userClient;
    }

    /**
     * Gets the project client.
     *
     * @return the project client
     */
    public ProjectRestClient getProjectClient() {
        return projectClient;
    }

    /**
     * Sets the project client.
     *
     * @param projectClient the new project client
     */
    public void setProjectClient(ProjectRestClient projectClient) {
        this.projectClient = projectClient;
    }

    /**
     * Gets the component client.
     *
     * @return the component client
     */
    public ComponentRestClient getComponentClient() {
        return componentClient;
    }

    /**
     * Sets the component client.
     *
     * @param componentClient the new component client
     */
    public void setComponentClient(ComponentRestClient componentClient) {
        this.componentClient = componentClient;
    }

    /**
     * Gets the extended metadata client.
     *
     * @return the extended metadata client
     */
    public ExtendedMetadataRestClient getExtendedMetadataClient() {
        return extendedMetadataClient;
    }

    /**
     * Sets the extended metadata client.
     *
     * @param extendedMetadataClient the new extended metadata client
     */
    public void setExtendedMetadataClient(ExtendedMetadataRestClient extendedMetadataClient) {
        this.extendedMetadataClient = extendedMetadataClient;
    }

    /**
     * Gets the search client.
     *
     * @return the search client
     */
    public SearchRestClient getSearchClient() {
        return searchClient;
    }

    /**
     * Sets the search client.
     *
     * @param searchClient the new search client
     */
    public void setSearchClient(SearchRestClient searchClient) {
        this.searchClient = searchClient;
    }

    /**
     * Gets the version rest client.
     *
     * @return the version rest client
     */
    public VersionRestClient getVersionRestClient() {
        return versionRestClient;
    }

    /**
     * Sets the version rest client.
     *
     * @param versionRestClient the new version rest client
     */
    public void setVersionRestClient(VersionRestClient versionRestClient) {
        this.versionRestClient = versionRestClient;
    }

    /**
     * Gets the project roles rest client.
     *
     * @return the project roles rest client
     */
    public ProjectRolesRestClient getProjectRolesRestClient() {
        return projectRolesRestClient;
    }

    /**
     * Sets the project roles rest client.
     *
     * @param projectRolesRestClient the new project roles rest client
     */
    public void setProjectRolesRestClient(ProjectRolesRestClient projectRolesRestClient) {
        this.projectRolesRestClient = projectRolesRestClient;
    }

    /**
     * Gets the server uri.
     *
     * @return the server uri
     */
    public String getServerUri() {
        return serverUri;
    }

    /**
     * Sets the server uri.
     *
     * @param serverUri the new server uri
     */
    public void setServerUri(String serverUri) {
        this.serverUri = serverUri;
    }

    /**
     * Gets the username.
     *
     * @return the username
     */
    public String getUsername() {
        return username;
    }

    /**
     * Sets the username.
     *
     * @param username the new username
     */
    public void setUsername(String username) {
        this.username = username;
    }

    /**
     * Gets the password.
     *
     * @return the password
     */
    public String getPassword() {
        return password;
    }

    /**
     * Sets the password.
     *
     * @param password the new password
     */
    public void setPassword(String password) {
        this.password = password;
    }
}
