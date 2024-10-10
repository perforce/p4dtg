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
package com.perforce.p4dtg.plugin.jira.tcp.response;

/**
 * The interface for implementing a response.
 */
public interface IResponse {

    String STRINGS = "STRINGS";
    String STRING = "STRING";

    String NAME = "NAME";
    String VALUE = "VALUE";

    String FIELD = "FIELD";
    String FIELDS = "FIELDS";

    String ERROR = "ERROR";
    String MESSAGE = "MESSAGE";
    String CONTINUE = "CONTINUE";

    String DESC = "DESC";
    String DESCS = "DESCS";

    String ACCESS = "ACCESS";
    int ACCESS_RW = 0;
    int ACCESS_RO = 1;
    int ACCESS_MOD_DATE = 2;
    int ACCESS_MOD_USER = 3;
    int ACCESS_DEFECT_ID = 4;

    String TYPE = "TYPE";
    String TYPE_WORD = "WORD";
    String TYPE_TEXT = "TEXT";
    String TYPE_DATE = "DATE";
    String TYPE_LINE = "LINE";
    String TYPE_FIX = "FIX";
    String TYPE_SELECT = "SELECT";

    /**
     * Returns the string representation of the response in XML form.
     *
     * @return the XML string
     * @see java.lang.Object#toString()
     */
    String toString();
}
