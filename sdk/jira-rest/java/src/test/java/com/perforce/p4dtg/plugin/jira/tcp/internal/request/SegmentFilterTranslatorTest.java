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

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Map;
import java.util.TreeMap;

import static org.junit.Assert.*;

public class SegmentFilterTranslatorTest {

	private IssueFieldsMapper issueFieldsMapper;
	@Before
	public void setUp() throws Exception {
	}

	@After
	public void tearDown() throws Exception {
	}

	// for P4DTG-868
	@Test
	public void testTranslateQuote() {
		SegmentFilterTranslator translator = new SegmentFilterTranslator();

		// use TreeMap because order is important
		Map<String,String> tmap = new TreeMap<String,String>();

		tmap.put("Progress", "Progress") ;
		tmap.put("Work Progress", "Work Progress") ;
		translator.setCustomFieldsMap(tmap);

		translator.setIssueTypesMap(new TreeMap<String,String>());
		translator.setPrioritiesMap(new TreeMap<String,String>());
		translator.setResolutionsMap(new TreeMap<String,String>());
		translator.setStatusesMap(new TreeMap<String,String>());

		translator.setSegmentFilter("Work Progress='Yes' AND Progress='No'");

		String retval = translator.translate();

		assertEquals("\"Work Progress\"='Yes' AND \"Progress\"='No'", retval);
	}
}