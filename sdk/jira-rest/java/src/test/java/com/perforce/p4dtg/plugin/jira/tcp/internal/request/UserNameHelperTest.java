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

/*
 * Tests for UserNameHelper
 */

package com.perforce.p4dtg.plugin.jira.tcp.internal.request;

import com.atlassian.jira.rest.client.api.domain.User;
import static com.atlassian.jira.rest.client.api.domain.User.S48_48;
import com.perforce.p4dtg.plugin.jira.config.Configuration;
import com.perforce.p4dtg.plugin.jira.config.ConfigurationTestHelper;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 *
 * @author jbrown
 */
public class UserNameHelperTest {

	public UserNameHelperTest() {
	}

	@BeforeClass
	public static void setUpClass() {
	}

	@AfterClass
	public static void tearDownClass() {
	}

	// required for User instantiation:  Map for avatars in User instantiation.
	Map<String, URI> avatars = new HashMap<>();

	@Before
	public void setUp() {
		URI uri = null;
		try {
			uri = new URI("http://avatar");
		} catch (URISyntaxException ex) {
			Logger.getLogger(UserNameHelperTest.class.getName()).log(Level.SEVERE, null, ex);
		}
		avatars.put(S48_48, uri);
	}

	@After
	public void tearDown() {
	}

	/**
	 * Util for making a com.atlassian.jira.rest.client.api.domain.User: public User(URI self, String name,
	 * String displayName, String accountId, String emailAddress, boolean active,
	 *
	 * @Nullable ExpandableProperty<String> groups, Map<String, URI> avatarUris, @Nullable String timezone) {
	 */
	public User makeUser(String name, String email) throws URISyntaxException {
		URI uri = new URI("http://localhost/" + name);
		User user = new User(uri, name, "full name", "accountId", email, true, null, avatars, null);
		return user;
	}

	/**
	 * Using JiraCloud xml: jira on prem should find name, cloud should find email.
	 */
	@Test
	public void testGetUserHelper1() throws Exception {
		System.out.println("testGetUserHelper2:  name test for jira on prem, email test for jira cloud");

		// default: "name,email,emailshort,displayname"
		Configuration config = ConfigurationTestHelper.getConfiguration("JiraCloud.xml");
		UserNameHelper.setConfiguration(config);

		User user = makeUser("name", "name@perforce.com");
		String str = UserNameHelper.getUserValue(user);
		assertTrue("name".equals(str));

		// jira cloud test - it does not have a name.
		user = makeUser(null, "name@perforce.com");
		str = UserNameHelper.getUserValue(user);
		assertTrue("Expected 'name@perforce.com', got '" + str + "'", "name@perforce.com".equals(str));

	}

	/**
	 * Using alternate order xml to test for emailShort.
	 */
	@Test
	public void testGetUserHelper2() throws Exception {
		System.out.println("testGetUserHelper2:  emailShort test for jira cloud");
		// name,emailshort,email,displayname
		Configuration config = ConfigurationTestHelper.getConfiguration("UserStylesGood.xml");
		UserNameHelper.setConfiguration(config);

		// jira cloud
		User user = makeUser(null, "name@perforce.com");
		String str = UserNameHelper.getUserValue(user);
		assertTrue("expected 'name', got=" + str, "name".equals(str));

	}
}
