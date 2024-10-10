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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

/**
 * Extracts resources from testfiles subdirectory into a temp file
 * and this parses the Configuration file.
 * The temp file is needed because the Configuration class expects
 *   a file (it can't load a resource.)
 * @author jbrown
 */
public class ConfigurationTestHelper {

    private static final Map<String, Configuration> CONFIGS = new HashMap<>();
    private static final Map<String, String> CONFIG_FILES = new HashMap<>();

    /***
     * get the Configuration
     * @param configName the config file name in testcases/<configName>
     * @return instance of Configuration
     * @throws IOException
     * @throws Exception
     */
    public static Configuration getConfiguration(String configName) throws IOException, Exception {
        Configuration config = CONFIGS.get(configName);
        if (config == null) {
            getConfigurationFile(configName);
            config = CONFIGS.get(configName);
        }
        return config;
    }

    public static String getConfigurationFile(String configName) throws IOException, Exception {
        String fName = CONFIG_FILES.get(configName);
        if ( fName == null ) {
            fName = getTempFile(configName);
            Configuration config = new Configuration(fName);
            config.parse();
            CONFIGS.put(configName, config);
            CONFIG_FILES.put(configName, fName);
        }
        return fName;
    }
    /**
     * *
     * extracts resource into temp file
     *
     * @param resourceName
     * @return temp file name.
     * @throws IOException
     */
    private static String getTempFile(String resourceName) throws IOException {
        String location = "testfiles/" + resourceName;
        OutputStream out = null;
        String tempFileName;
        try (InputStream fileStream = ConfigurationTestHelper.class.getResourceAsStream(location)) {
            if (fileStream == null) {
                throw new IllegalArgumentException("Can't find test resource '" + resourceName + "'");
            }
            File tempFile = File.createTempFile("config-", resourceName);
            tempFileName = tempFile.getCanonicalPath();
            tempFile.deleteOnExit();
            out = new FileOutputStream(tempFile);
            // Write the file to the temp file
            byte[] buffer = new byte[1024];
            int len = fileStream.read(buffer);
            while (len != -1) {
                out.write(buffer, 0, len);
                len = fileStream.read(buffer);
            }
        } finally {
            try {
                if (out != null) {
                    out.close();
                }
            } catch (IOException e) {
                throw e;
            }
        }
        return tempFileName;
    }

}
