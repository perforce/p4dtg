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

import com.perforce.p4dtg.plugin.jira.tcp.internal.response.ErrorResponse;

/**
 * Request exception and its response error message.
 */
public class RequestException extends Exception {

    private static final long serialVersionUID = 5896177158909833058L;

    private ErrorResponse response;

    /**
     * Create a request exception with an error response.
     * 
     * @param response
     *            the response
     */
    public RequestException(ErrorResponse response) {
        this.response = response;
    }

    /**
     * Get error response to send for this request exception.
     * 
     * @return error response
     */
    public ErrorResponse getResponse() {
        return this.response;
    }
}
