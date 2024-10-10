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
package com.perforce.p4dtg.plugin.jira.logging;

import java.io.IOException;
import java.util.logging.Filter;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.XMLFormatter;

/**
 * This class extends java.util.logging.FileHandler to handle custom log file
 * names.
 */
public class FileHandler extends java.util.logging.FileHandler {

    private static String port = System.getProperty("javadts.TCP_PORT", "0");
    private static String pattern = "%h/java%u.log";
    private static String encoding = System.getProperty("file.encoding");
    private static int limit = 0;
    private static int count = 1;
    private static boolean append = false;
    private static Formatter formatter = new XMLFormatter();
    private static Filter filter = null;
    private static Level level = Level.ALL;

    static {
        try {
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.pattern") != null) {
                pattern = LogManager.getLogManager()
                        .getProperty("java.util.logging.FileHandler.pattern")
                        .replace("${javadts.TCP_PORT}", port);
            }
            if (LogManager.getLogManager().getProperty("encoding") != null) {
                encoding = LogManager.getLogManager().getProperty(
                        "java.util.logging.FileHandler.encoding");
            }
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.limit") != null) {
                limit = Integer.parseInt(LogManager.getLogManager()
                        .getProperty("java.util.logging.FileHandler.limit"));
            }
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.count") != null) {
                count = Integer.parseInt(LogManager.getLogManager()
                        .getProperty("java.util.logging.FileHandler.count"));
            }
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.append") != null) {
                append = Boolean.parseBoolean(LogManager.getLogManager()
                        .getProperty("java.util.logging.FileHandler.append"));
            }
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.formatter") != null) {
                Class<?> clazz = ClassLoader.getSystemClassLoader().loadClass(
                        LogManager.getLogManager().getProperty(
                                "java.util.logging.FileHandler.formatter"));
                formatter = (Formatter) clazz.newInstance();
            }
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.filter") != null) {
                Class<?> clazz = ClassLoader.getSystemClassLoader().loadClass(
                        LogManager.getLogManager().getProperty(
                                "java.util.logging.FileHandler.filter"));
                filter = (Filter) clazz.newInstance();
            }
            if (LogManager.getLogManager().getProperty(
                    "java.util.logging.FileHandler.level") != null) {
                level = Level.parse(LogManager.getLogManager().getProperty(
                        "java.util.logging.FileHandler.level"));
            }
        } catch (Exception ignore) {
        }
    }

    /**
     * Instantiates a new file handler.
     *
     * @throws SecurityException
     *             the security exception
     * @throws IOException
     *             Signals that an I/O exception has occurred.
     */
    public FileHandler() throws SecurityException, IOException {
        super(pattern, limit, count, append);
        this.setEncoding(encoding);
        this.setFormatter(formatter);
        this.setLevel(level);
        this.setFilter(filter);
    }
}
