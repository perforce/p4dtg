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

import com.atlassian.jira.rest.client.api.domain.User;
import com.perforce.p4dtg.plugin.jira.config.Configuration;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Handles determining Assignee value.
 * This is necessary because jira on premises uses the "name"
 * field, which is used to login and typically matches the perforce user.
 * 
 * jira in the cloud doesn't have a name (it is null) so we need to use
 * somthing else:
 * the email, full name, or a special emailshort (everything before the @)
 * this is controlled by a configuration option in jira-rest-config.xml
 * 
 * @author jbrown
 */
public class UserNameHelper {
	
	public static Configuration configuration;
	
    private static final Logger logger = Logger.getLogger(RequestHandler.class.getPackage().getName());
	
	public static void setConfiguration(Configuration configuration) {

		logger.log(Level.FINEST, "UserNameHelper userStyles: {0} ", configuration.getUserStyles());
		UserNameHelper.configuration = configuration;
	}
	
	/**
	 * Get the issue's field on type User (Assignee, Reporter)'s value.
	 * 
	 * This is usually the user "name" for jira on premises,
	 * But if that is null (like in jirz cloud) we look for other
	 * values.
	 * @param User
	 * @return String value for User
	 */
	public static String getUserValue(User user) {
		
		if (user == null) {
			return null;
		}
		
		// return value
		String value = null;
		
		String[] userStyles = configuration.getUserStyles();
		for (String style : userStyles) {
			switch (style) {
				case "name":
					value = user.getName();
					break;
				case "email":
					value = user.getEmailAddress();
					break;
				case "emailshort":
					value = user.getEmailAddress();
					if (value != null) {
						int atSign = value.indexOf("@");
						if (atSign > 1) {
							value = value.substring(0, atSign);
						}
					}
					break;
				case "displayname":
					value = user.getDisplayName();
					break;
			}
			if ( value != null) {
				break;
			}
		}
		return value;
	}

	// make sure all elements in assignedStyles are in stylesDefault.
	public static void validateConfig(String[] userStyles, String[] stylesDefault) 
		throws Exception {
		for (String style : userStyles) {
			boolean found = false;
			for (String string : stylesDefault) {
				if ( string.equals(style)) {
					found = true;
					break;
				}
			}
			if (!found) {
				String msg = "User value '" + style + "' not in valid values " + stylesDefault;
				Exception ex = new Exception(msg);
				throw ex;
			}
			
		}
	}
}
