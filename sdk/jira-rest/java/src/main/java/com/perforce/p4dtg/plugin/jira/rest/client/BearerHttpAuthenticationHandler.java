/**
 * Copyright (c) 2024-2024 Perforce Software. All rights reserved.
 */
package com.perforce.p4dtg.plugin.jira.rest.client;

import com.atlassian.httpclient.api.Request;
import com.atlassian.jira.rest.client.api.AuthenticationHandler;

public class BearerHttpAuthenticationHandler implements AuthenticationHandler {

	private static final String BEARER_HEADER = "Authorization";
	private final String token;

	public BearerHttpAuthenticationHandler(String token) {
		this.token = token;
	}

	public void configure(Request.Builder builder) {
		builder.setHeader("Authorization", "Bearer " + this.token);
	}
}
