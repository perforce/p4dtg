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

import java.io.IOException;
import java.util.List;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 *
 * @author jbrown
 */
public class ConfigurationTest {

    public ConfigurationTest() {
    }

    @BeforeClass
    public static void setUpClass() throws IOException {
    }

    @AfterClass
    public static void tearDownClass() {
    }

    @Before
    public void setUp() {
    }

    @After
    public void tearDown() {
    }

    /**
     * Test of getWorkflowMap method, of class Configuration.
     */
    @Test
    public void testGetWorkflowMap() {
    }

    /**
     * Test of setWorkflowMap method, of class Configuration.
     */
    @Test
    public void testSetWorkflowMap() {
    }

    /**
     * Test of getCustomFields method, of class Configuration.
     */
    @Test
    public void testGetCustomFields() {
    }

    /**
     * Test of setCustomFields method, of class Configuration.
     */
    @Test
    public void testSetCustomFields() {

    }

    /**
     * Test of getWorkflows method, of class Configuration.
     */
    @Test
    public void testGetWorkflows() {

    }

    /**
     * Test of setWorkflows method, of class Configuration.
     */
    @Test
    public void testSetWorkflows() {

    }

    /**
     * Test of getXmlFile method, of class Configuration.
     */
    @Test
    public void testGetXmlFile() {

    }

    /**
     * Test of setXmlFile method, of class Configuration.
     */
    @Test
    public void testSetXmlFile() {
    }

    /**
     * Test of setJiraHandling method, of class Configuration.
     */
    @Test
    public void testSetJiraHandling() {

    }

    /**
     * Test of getJiraHandling method, of class Configuration.
     */
    @Test
    public void testGetJiraHandling() {

    }

    /**
     * Test of parse method, of class Configuration.
     */
    @Test
    public void testParse() throws Exception {

    }

    /**
     * Test of getStepForTransitionName method, of class Configuration.
     */
    @Test
    public void testGetStepForTransitionName() {

    }

    /**
     * Test of getStatusForStep method, of class Configuration.
     */
    @Test
    public void testGetStatusForStep() {

    }

    /**
     * Test of isIgnoredProject method, of class Configuration.
     */
    @Test
    public void testIsIgnoredProject() throws Exception {
        System.out.println("\nisIgnoredProject list of ignore projects");

        Configuration config = ConfigurationTestHelper.getConfiguration("HasIgnoreFiles.xml");
        boolean tf = config.isIgnoredProject("aaa");
        assertTrue("aaa not ignored", tf);
        assertTrue("bbb not ignored", config.isIgnoredProject("bbb"));
        assertTrue("ccc not ignored", config.isIgnoredProject("ccc"));
        assertFalse("not existing if list = false", config.isIgnoredProject("notExistoInList"));

        System.out.println("\nisIgnoredProject empty ignores");

        config = ConfigurationTestHelper.getConfiguration("NoIgnoreFiles.xml");
        tf = config.isIgnoredProject("P4V");
        assertFalse("just checking for exceptions", tf);

    }

    @Test
    public void testQueryProject() throws Exception {

        System.out.println("\nisIgnoredProject QueryStyle != 2014.1");
        Configuration config = ConfigurationTestHelper.getConfiguration("HasIgnoreFiles.xml");
        String legacy = config.getJiraHandling(Configuration.QUERY_STYLE);
        assertFalse("expected QueryStyle=2014.1", "2014.1".equals(legacy));

        System.out.println("\nisIgnoredProject QueryStyle==2014.1");
        config = ConfigurationTestHelper.getConfiguration("NoIgnoreFiles.xml");
        legacy = config.getJiraHandling(Configuration.QUERY_STYLE);
        assertTrue("expected QueryStyle=2014.1", "2014.1".equals(legacy));
    }

    @Test
    public void testUserValuesGood() throws Exception {
        System.out.println("\nUserStylesvalues Found in config in proper order");
        Configuration config ;

		config = ConfigurationTestHelper.getConfiguration("UserStylesGood.xml");
		String[] styles = config.getUserStyles();

		// vals must be the same as in the test file (not in the default order
		// so that it's useful.
		String[] vals = "name,emailshort,email,displayname".split(",");
		assertArrayEquals(styles,vals);

	}


	@Test
    public void testUserValuesBad() throws Exception {
        System.out.println("\nUserStylesvalues Bad value in config.xml");
        Configuration config ;
		try {
			config = ConfigurationTestHelper.getConfiguration("UserStylesBad.xml");
			fail("Did not detect bad value for UserStyles");
		}
		catch( Exception e) {
			String msg = e.getMessage();
			if ( e instanceof IllegalArgumentException) {
				fail("Did not find: " + msg);
			} else {
				System.out.println("Caught bad value in config: " + e.getMessage());
			}
		}
	}
    @Test
    public void testCustomFields() throws Exception {
        System.out.println("\nCustom Fields");
        Configuration config = ConfigurationTestHelper.getConfiguration("CustomFields.xml");

        List<CustomField> customFields = config.getCustomFields();

        assertTrue("Expected 3 custom fields", customFields.size() >= 3);

        String[] flds = {"P4Summary", "QAResource", "Bongos"};
        for (String fld : flds) {
            CustomField found = null;
            for (int i = 0; i < customFields.size(); i++) {
                CustomField cf = customFields.get(i);
                if (cf.getName().equals(fld)) {
                    found = cf;
                    break;
                }
            }
            if (found == null) {
                fail("Field " + fld + " not found");
            }
            switch (fld) {
                case "P4Summary":
                    assertTrue (fld + " access ", "RW".equals(found.getAccess()));
                    assertTrue(fld + " type", "LINE".equals(found.getType()));
                    break;
                case "Bongos":
                    assertTrue (fld + " access ", "RO".equals(found.getAccess()));
                    assertTrue(fld + " type", "SELECT".equals(found.getType()));
                    assertTrue(fld + " options 3 ",found.getOptions().size() == 3);
                    break;
            }
        }
    }
}
