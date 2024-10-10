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
 * Default implementation of the description response.
 */
public class DescriptionResponse implements IResponse {

    private String name;
    private String type;
    private int access;
    private StringResponse values;

    /**
     * Instantiates a new description response.
     */
    public DescriptionResponse() {
        this(null, null, ACCESS_RW, null);
    }

    /**
     * Instantiates a new description response.
     *
     * @param name
     *            the name
     * @param type
     *            the type
     * @param access
     *            the access
     * @param values
     *            the values
     */
    public DescriptionResponse(String name, String type, int access,
            String[] values) {
        this.name = name;
        this.type = type;
        this.access = access;
        this.values = new StringResponse(values);
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
     * Gets the type.
     *
     * @return the type
     */
    public String getType() {
        return this.type;
    }

    /**
     * Sets the type.
     *
     * @param type
     *            the type to set
     */
    public void setType(String type) {
        this.type = type;
    }

    /**
     * Gets the access.
     *
     * @return the access
     */
    public int getAccess() {
        return this.access;
    }

    /**
     * Sets the access.
     *
     * @param access
     *            the access to set
     */
    public void setAccess(int access) {
        this.access = access;
    }

    /**
     * Gets the values.
     *
     * @return the values
     */
    public StringResponse getValues() {
        return this.values;
    }

    /**
     * Sets the values.
     *
     * @param values
     *            the values to set
     */
    public void setValues(StringResponse values) {
        this.values = values;
    }

    /**
     * Returns the string representation of the description response in XML form.
     *
     * @return the XML string
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuilder xml = new StringBuilder();
        xml.append('<');
        xml.append(DESC);

        if (this.name != null) {
            xml.append(' ');
            xml.append(NAME);
            xml.append('=');
            xml.append('"');
            xml.append(ResponseHelper.escapeXML(this.name));
            xml.append('"');
        }

        xml.append(' ');
        xml.append(ACCESS);
        xml.append('=');
        xml.append('"');
        xml.append(this.access);
        xml.append('"');

        if (this.type != null) {
            xml.append(' ');
            xml.append(TYPE);
            xml.append('=');
            xml.append('"');
            xml.append(this.type);
            xml.append('"');
        }
        xml.append('>');

        if (this.values != null && !this.values.isEmpty()) {
            xml.append(this.values.toString());
        }

        xml.append("</");
        xml.append(DESC);
        xml.append('>');

        return xml.toString();
    }
}
