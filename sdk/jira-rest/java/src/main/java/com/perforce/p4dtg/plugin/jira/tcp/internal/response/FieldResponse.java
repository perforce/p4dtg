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

import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;

/**
 * Default implementation of the field response.
 */
public class FieldResponse implements IResponse {

    private String name;
    private String value;

    /**
     * Instantiates a new field response.
     */
    public FieldResponse() {
        this(null, null);
    }

    /**
     * Instantiates a new field response.
     *
     * @param name
     *            the name
     * @param value
     *            the value
     */
    public FieldResponse(String name, String value) {
        this.name = name;
        this.value = value;
    }

    /**
     * Gets the name.
     *
     * @return the name
     */
    public String getName() {
        return this.name;
    }

    /**
     * Sets the name.
     *
     * @param name
     *            the name to set
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * Gets the value.
     *
     * @return the value
     */
    public String getValue() {
        return this.value;
    }

    /**
     * Sets the value.
     *
     * @param value
     *            the value to set
     */
    public void setValue(String value) {
        this.value = value;
    }

    /**
     * Returns the string representation of the field response in XML form.
     *
     * @return the XML string
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuilder xml = new StringBuilder();
        xml.append('<');
        xml.append(FIELD);

        if (this.name != null) {
            xml.append(' ');
            xml.append(NAME);
            xml.append('=');
            xml.append('"');
            xml.append(ResponseHelper.escapeXML(this.name));
            xml.append('"');
        }

        if (this.value != null) {
            xml.append(' ');
            xml.append(VALUE);
            xml.append('=');
            xml.append('"');
            xml.append(ResponseHelper.escapeXML(this.value));
            xml.append('"');
        }

        xml.append(" />");
        return xml.toString();
    }
}
