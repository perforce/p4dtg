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
package com.perforce.p4dtg.plugin.jira.config;

/**
 * Base class for the configuration information.
 */
public abstract class Base {

    protected static final String LINE_SEPARATOR = System.getProperty("line.separator", "\n");

    protected String name;

    /**
     * Instantiates a new base.
     */
    protected Base() {
    }

    /**
     * Instantiates a new base.
     *
     * @param name
     *            the name
     * @throws Exception
     *             the exception
     */
    protected Base(String name) throws Exception {
        this.name = name;
    }

    /**
     * Gets the name.
     *
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * Sets the name.
     *
     * @param name
     *            the new name
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("name: ").append(name == null ? "" : name).append(LINE_SEPARATOR);
        return sb.toString();
    }

    /**
     * Checks if the string is empty.
     *
     * @param value
     *            the value
     * @return true, if is empty
     */
    protected static boolean isEmpty(String value) {
        if (value == null || value.trim().length() == 0) {
            return true;
        }
        return false;
    }
}
