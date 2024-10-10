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

import java.util.HashMap;
import java.util.Map;

import com.atlassian.jira.rest.client.api.domain.Field;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.rest.client.RestClientManager;

/**
 * Builder for building the default defect fields map.
 */
public class DefaultDefectFieldsMapBuilder {

    private RestClientManager restClientManager;

    /**
     * Constructor to create a new default fields map builder.
     *
     * @param restClientManger
     *            the rest client manager
     */
    protected DefaultDefectFieldsMapBuilder(RestClientManager restClientManger) {
        this.restClientManager = restClientManger;
    }
    
    /**
     * Builds the default defect fields map.
     *
     * @return the fields map
     */
    /**
     * Builds the default defect fields.
     *
     * @return the map
     */
    public Map<String, String[]> build() {
        Map<String, String[]> fieldValueMap = new HashMap<String, String[]>();
        if (this.restClientManager != null) {
	        Iterable<Field> fields = this.restClientManager.getExtendedMetadataClient().getFields().claim();
	        if (fields != null) {
	        	for (Field field : fields) {
	        		if (Constants.ISSUE_FIELDS.containsValue(field.getId())) {
	                    fieldValueMap.put(field.getName(), new String[] {});
	        		} else if (field.getId().startsWith(Constants.CUSTOM_FIELD_ID_PREFIX)) {
	        			fieldValueMap.put(field.getName(), new String[] {});
	        		}
	        	}
	        }
        }
        return fieldValueMap;
    }
}
