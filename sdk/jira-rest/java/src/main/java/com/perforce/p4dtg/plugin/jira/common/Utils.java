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
package com.perforce.p4dtg.plugin.jira.common;

import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.domain.util.ErrorCollection;
import java.net.URI;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Collection;
import java.util.Date;
import java.util.Map;

/**
 * Common utilities class with generally useful methods.
 */
public class Utils {

    /**
     * Parses the date string with the specified pattern into a date.
     *
     * @param date the date
     * @param pattern the pattern
     * @return the date
     * @throws ParseException the parse exception
     */
    public static Date parseDate(String date, String pattern) throws ParseException {
        Date d = null;
        if (date != null) {
            DateFormat df = new SimpleDateFormat(pattern);
            d = df.parse(date);
        }
        return d;
    }

    /**
     * Parses the date string with the specified pattern into a calendar.
     *
     * @param date the date
     * @param pattern the pattern
     * @return the calendar
     * @throws ParseException the parse exception
     */
    public static Calendar parseCalendar(String date, String pattern)
            throws ParseException {
        Calendar cal = null;
        if (date != null) {
            Date d = parseDate(date, pattern);
            if (d != null) {
                cal = Calendar.getInstance();
                cal.setTime(d);
            }
        }
        return cal;
    }

    /**
     * Formats the date with the specified pattern into a date string.
     *
     * @param date the date
     * @param pattern the pattern
     * @return the date string
     */
    public static String formatDate(Date date, String pattern) {
        String d = null;
        if (date != null) {
            DateFormat df = new SimpleDateFormat(pattern);
            d = df.format(date);
        }
        return d;
    }

    /**
     * Returns the first value from the map matching the key.
     *
     * @param map the map
     * @param key the key
     * @return the value string
     */
    public static String getMapValue(Map<String, String[]> map, String key) {
        if (map != null && key != null) {
            String[] values = map.get(key);
            if (values != null) {
                if (values.length > 0) {
                    return values[0];
                }
            }
        }
        return null;
    }

    /**
     * Returns the ID part (last part) of the URI.
     *
     * @param uri the URI
     * @return the ID string
     */
    public static String getIdFromUri(URI uri) {
        if (uri != null) {
            String path = uri.getPath();
            String id = path.substring(path.lastIndexOf('/') + 1);
            return id;
        }
        return null;
    }

    /**
     * Checks if the value is empty.
     *
     * @param value the value
     * @return true, if the value is empty
     */
    public static boolean isEmpty(String value) {
        if (value == null || value.trim().length() == 0) {
            return true;
        }
        return false;
    }

    /**
     * Checks if the first element of the array is empty.
     *
     * @param values the array of values
     * @return true, if value is empty
     */
    public static boolean isEmpty(String[] values) {
        if (values != null) {
            if (values.length > 0) {
                return isEmpty(values[0]);
            }
        }
        return true;
    }

    /**
     * Return an empty string if the value is null, otherwise return the value.
     *
     * @param value the value
     * @return the value string, if value is null return empty string
     */
    public static String guardNull(String value) {
        if (value == null) {
            return "";
        }
        return value;
    }

    /**
     * look for error code in the RestClientException.
     *
     * @param exc
     * @return error code, like 400 or 403 or 404. 0 if none found.
     */
    public static int getErrorStatus(RestClientException exc) {
        Collection<ErrorCollection> errs = exc.getErrorCollections();
        for (ErrorCollection err : errs) {
            return err.getStatus();
        }
        return 0;
    }
}
