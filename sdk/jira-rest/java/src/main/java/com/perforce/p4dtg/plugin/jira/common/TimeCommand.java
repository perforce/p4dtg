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

import java.util.Date;

/**
 * *
 * Simple timer. call start(), do stuff, call stop() or timer.toString();
 *
 * Not thread safe.
 *
 * @author jbrown
 */
public final class TimeCommand {

    private long start = 0;
    private long end = 0;

    /**
     * start the clock
     */
    public void start() {
        Date now = new Date();
        start = now.getTime();
        end = 0;
    }

    /**
     * stop the clock
     *
     * @return number of milliseconds between start and stop
     */
    public long stop() {
        Date now = new Date();
        end = now.getTime();
        return (end - start);
    }

    /**
     * Retrieve milliseconds
     *
     * @return number of milliseconds between start and stop
     */
    public long getMilliseconds() {
        if (end == 0) {
            stop();
        }
        return (end - start);
    }

    // diff time as string
    @Override
    public String toString() {
        if (end == 0) {
            stop();
        }
        return "Time[" + getMilliseconds() + " ms]";
    }
}
