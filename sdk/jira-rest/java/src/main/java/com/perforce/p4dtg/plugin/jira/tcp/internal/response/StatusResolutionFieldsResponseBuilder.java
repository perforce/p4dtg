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
package com.perforce.p4dtg.plugin.jira.tcp.internal.response;

import java.util.LinkedHashMap;
import java.util.Map;

import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.config.Configuration;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;

/**
 * Builder for building the status resolution fields response.
 */
public class StatusResolutionFieldsResponseBuilder {

    private Configuration configuration;
    private Map<String, String> statusesMap;
    private Map<String, String> resolutionsMap;
    
    public StatusResolutionFieldsResponseBuilder(Configuration configuration) {
    	this.configuration = configuration;
    }
    
    /**
     * Builds the status resolution fields response.
     *
     * @return the status resolution fields response
     */
    public DescriptionResponse build() {
        if (this.statusesMap != null) {
	        DescriptionResponse desc = null;
	        Map<String, String> statusResolutionMap = new LinkedHashMap<String, String>();
            for (Map.Entry<String, String> stutusEntry : this.statusesMap.entrySet()) {
                String statusName = stutusEntry.getKey();
                if (statusName != null) {
                    statusResolutionMap.put(statusName, statusName);
                }
            }
            if (this.resolutionsMap != null) {
	            for (Map.Entry<String, String> statusEntry : this.statusesMap.entrySet()) {
	                String statusName = statusEntry.getKey();
	                if (statusName != null) {
	                    if (exists(statusName)) {
                            for (Map.Entry<String, String> resolutionEntry : this.resolutionsMap.entrySet()) {
                                if (resolutionEntry != null && resolutionEntry.getKey() != null) {
                                    String statusResolution = statusName + "/" + resolutionEntry.getKey();
                                    if (statusResolutionMap.containsKey(statusName)) {
                                        statusResolutionMap.remove(statusName);
                                    }
                                    statusResolutionMap.put(statusResolution, statusResolution);
                                }
                            }
                        }
                    }
                }
            }
	        if (statusResolutionMap != null && !statusResolutionMap.isEmpty()) {
	            desc = new DescriptionResponse(Constants.DTG_FIELD_STATUS_RESOLUTION,
	                    IResponse.TYPE_SELECT, IResponse.ACCESS_RW,
	                    statusResolutionMap.keySet().toArray(
	                            new String[statusResolutionMap.keySet().size()]));
	        }
	        return desc;
        }
        return null;
    }

    /**
     * Checks if the status exists in the resolution status map.
     *
     * @param status
     *            the status
     * @return true, if it exists in the resolution status map
     */
    private boolean exists(String status) {
        if (status != null && configuration != null) {
            Map<String, Map<String, String>> workflowMap = configuration.getWorkflowMap();
            if (workflowMap != null) {
                Map<String, String> resolutionStatusMap = workflowMap.get("resolutionStatus");
                if (resolutionStatusMap != null) {
                    if (resolutionStatusMap.containsValue(status)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

	public void setStatusesMap(Map<String, String> statusesMap) {
		this.statusesMap = statusesMap;
	}

	public void setResolutionsMap(Map<String, String> resolutionsMap) {
		this.resolutionsMap = resolutionsMap;
	}
}
