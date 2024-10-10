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

import java.text.CharacterIterator;
import java.text.StringCharacterIterator;

/**
 * Helper class providing methods for handling the XML response.
 */
public class ResponseHelper {

    /**
     * Escape special characters for XML data.
     * <p>
     * 
     * <table border='1' cellpadding='0' cellspacing='0'>
     * <tr>
     * <th>Character</th>
     * <th>Encoding</th>
     * </tr>
     * <tr>
     * <td><</td>
     * <td>&lt;</td>
     * </tr>
     * <tr>
     * <td>></td>
     * <td>&gt;</td>
     * </tr>
     * <tr>
     * <td>&</td>
     * <td>&amp;</td>
     * </tr>
     * <tr>
     * <td>"</td>
     * <td>&quot;</td>
     * </tr>
     * <tr>
     * <td>'</td>
     * <td>&#039;</td>
     * </tr>
     * </table>
     */
    public static String escapeXML(String text) {
        if (text == null) {
            return null;
        }
        StringBuilder result = new StringBuilder();
        StringCharacterIterator iterator = new StringCharacterIterator(text);
        char character = iterator.current();
        while (character != CharacterIterator.DONE) {
            if (character == '<') {
                result.append("&lt;");
            } else if (character == '>') {
                result.append("&gt;");
            } else if (character == '\"') {
                result.append("&quot;");
            } else if (character == '\'') {
                result.append("&#039;");
            } else if (character == '&') {
                result.append("&amp;");
            } else {
                // Add it as is
                result.append(character);
            }
            character = iterator.next();
        }
        return result.toString();
    }
}
