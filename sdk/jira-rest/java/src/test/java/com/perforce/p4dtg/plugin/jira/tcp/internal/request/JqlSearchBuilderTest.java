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

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Test JqlSearchBuilder for handling of projId and projects and if both set.
 *
 * @author jfbrown
 */
public class JqlSearchBuilderTest {

    public JqlSearchBuilderTest() {
    }

    /**
     * Test of build method, of class JqlSearchBuilder.
     */
    @Test
    public void testBuild() {
        System.out.println("build");
        JqlSearchBuilder instance ;

        String expResult = "";

        instance = new JqlSearchBuilder();
        instance.setProjId("xxx");
        instance.setOrderBy("order by updated");
        instance.setModDate("updated");
        instance.setDate("2018/ 1/ 1 12:12");

        String result = instance.build();
        System.out.println("Result = '" + result + "'");
        assertTrue("project xxx missing", result.indexOf("xxx") > 0);
        assertTrue("date not normalized", result.indexOf("2018/01/01") > 0 );


        instance = new JqlSearchBuilder();
        instance.setProjId("xxx");
        instance.setProjects(new String[] {"yyy","yyy2"});
        instance.setOrderBy("order by updated");
        instance.setModDate("updated");
        instance.setDate("2018/ 1/1 12:12");
        result = instance.build();
        System.out.println("Result = '" + result + "'");
        assertTrue("project missing", result.indexOf("xxx") > 0);


        instance = new JqlSearchBuilder();
        String p2 = "JBTWO";
        instance.setProjects(new String[] {"JBONE",p2});
        instance.setOrderBy("order by updated");
        instance.setModDate("updated");
        instance.setDate("2018/ 1/1 12:12");
        result = instance.build();
        System.out.println("Result = '" + result + "'");
        assertTrue("project yyy missing", result.indexOf(p2) > 0);

        instance = new JqlSearchBuilder();
        instance.setOrderBy("order by updated");
        instance.setModDate("updated");
        instance.setDate("2018/ 1/1 12:12");
        result = instance.build();
        System.out.println("Result = '" + result + "'");
        assertFalse("project field found", result.indexOf("project") > 0);

        instance = new JqlSearchBuilder();
        instance.setProjId(null);
        instance.setProjects(new String[0]);
        instance.setOrderBy("order by updated");
        instance.setModDate("updated");
        instance.setDate("2018/ 2/2 12:12");
        result = instance.build();
        System.out.println("Result = '" + result + "'");
        assertFalse("project field found", result.indexOf("project") > 0);
    }


}
