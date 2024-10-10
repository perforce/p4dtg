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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;

/**
 * Builder for building the field response array.
 */
public class DefectFieldsResponseBuilder {

	private Map<String, String[]> fieldValueMap;
    
    /**
     * Builds the field response array.
     *
     * @return the field response array
     */
    public FieldResponse[] build() {
        List<FieldResponse> fields = new ArrayList<FieldResponse>(fieldValueMap.size());
        if (fieldValueMap != null) {
	        for (String key : fieldValueMap.keySet()) {
	            String[] values = fieldValueMap.get(key);
	            StringBuilder sb = new StringBuilder();
	            if (!Utils.isEmpty(values)) {
	                for (int i = 0; i < values.length; i++) {
	                    if (i > 0) {
	                        if (key.equalsIgnoreCase(Constants.ISSUE_FIELD_AFFECTSVERSIONS) ||
	                        		key.equalsIgnoreCase(Constants.ISSUE_FIELD_FIXVERSIONS) ||
	                        		key.equalsIgnoreCase(Constants.ISSUE_FIELD_COMPONENTS)) {
	                            sb.append(Constants.MULTI_SELECT_SEPARATOR);
	                        }
	                    }
	                    sb.append(values[i]);
	                }
	            }
	            fields.add(new FieldResponse(key, sb.toString()));
	        }
        }
        return fields.toArray(new FieldResponse[fields.size()]);
    }

    public void setFieldValueMap(Map<String, String[]> fieldValueMap) {
		this.fieldValueMap = fieldValueMap;
	}
}
