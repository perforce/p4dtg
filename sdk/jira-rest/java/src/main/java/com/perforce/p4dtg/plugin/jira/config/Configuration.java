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
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.rest.client.P4AsynchronousHttpClientFactory;
import com.perforce.p4dtg.plugin.jira.tcp.internal.request.UserNameHelper;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathFactory;

/**
 * Parser for extracting the workflows and custom fields from the config XML file.
 */
public class Configuration {

	private static final Logger logger = Logger.getLogger(Configuration.class.getPackage().getName());

	private static final String LINE_SEPARATOR = System.getProperty("line.separator");

	private static final String CONFIG_XML_FILE = System.getProperty(Constants.CONFIG_XML_FILE_PROPERTY, Constants.DEFAULT_CONFIG_XML_FILE);
	private List<CustomField> customFields = null;
	private List<Workflow> workflows = null;
	private String xmlFile = null;
	private Document dom = null;
	private Map<String, Map<String, String>> workflowMap = null;

	private final Map<String, String> jiraHandling = new HashMap<>();

	public static final String IGNORE_PROJECTS = "IgnoreProjects";
	public static final String QUERY_STYLE = "QueryStyle";
	public static final String USER_NAME_STYLE = "UserStyle";
	public static final String USER_NAME_STYLE_DEFAULT = "name,email,emailshort,displayname";
	public static final String SOCKET_TIMEOUT_SECONDS = "SocketTimeoutSeconds";
	public static final String REQUEST_TIMEOUT_SECONDS = "RequestTimeoutSeconds";
	public static final String CONNECTION_TIMEOUT_SECONDS = "ConnectionTimeoutSeconds";
	private final String[] jiraHandlingElements = {IGNORE_PROJECTS, QUERY_STYLE, USER_NAME_STYLE,
		SOCKET_TIMEOUT_SECONDS, REQUEST_TIMEOUT_SECONDS, CONNECTION_TIMEOUT_SECONDS};

	/**
	 * Gets the workflow map.
	 *
	 * @return the workflow map
	 */
	public Map<String, Map<String, String>> getWorkflowMap() {
		return workflowMap;
	}

	/**
	 * Sets the workflow map.
	 *
	 * @param workflowMap the workflow map
	 */
	public void setWorkflowMap(Map<String, Map<String, String>> workflowMap) {
		this.workflowMap = workflowMap;
	}

	/**
	 * Gets the custom fields.
	 *
	 * @return the custom fields
	 */
	public List<CustomField> getCustomFields() {
		return customFields;
	}

	/**
	 * Sets the custom fields.
	 *
	 * @param customFields the new custom fields
	 */
	public void setCustomFields(List<CustomField> customFields) {
		this.customFields = customFields;
	}

	/**
	 * Gets the workflows.
	 *
	 * @return the workflows
	 */
	public List<Workflow> getWorkflows() {
		return workflows;
	}

	/**
	 * Sets the workflows.
	 *
	 * @param workflows the new workflows
	 */
	public void setWorkflows(List<Workflow> workflows) {
		this.workflows = workflows;
	}

	/**
	 * Gets the xml file.
	 *
	 * @return the xml file
	 */
	public String getXmlFile() {
		return xmlFile;
	}

	/**
	 * Sets the xml file.
	 *
	 * @param xmlFile the new xml file
	 */
	public void setXmlFile(String xmlFile) {
		this.xmlFile = xmlFile;
	}

	public void setJiraHandling(String key, String value) {
		if (key == null) {
			throw new IllegalArgumentException("Configuration.setJiraHandling key must not be null.");
		}
		jiraHandling.put(key, value);
	}

	public String getJiraHandling(String key) {
		return jiraHandling.get(key);
	}

	/**
	 * Instantiates a new config XML parser.
	 *
	 * @param configFile the config file
	 * @throws Exception the exception
	 */
	public Configuration(String configFile) throws Exception {
		if (configFile != null) {
			this.xmlFile = configFile;
		} else {
			this.xmlFile = CONFIG_XML_FILE;
			logger.log(Level.INFO, "The JIRA config file is not specified. The default jira-rest-config.xml file will be used.");
		}

		File file = new File(xmlFile);
		if (!file.exists()) {
			throw new Exception("The JIRA config file " + configFile + " does not exist.");
		}

		customFields = new ArrayList<>();
		workflows = new ArrayList<>();
		workflowMap = new HashMap<>();
	}

	/**
	 * Parses the config XML file.
	 *
	 * @throws Exception the exception
	 */
	public void parse() throws Exception {
		// Parse the XML file and get the dom object
		parseXmlFile();
		// Get the elements and create objects
		parseDocument();
		// Make sure there are workflow
		if (workflows == null || workflows.isEmpty()) {
			throw new Exception("There are no workflows parsed from the JIRA config file.");
		}
		// Validate config file content
		validate();

		// Build resolution status map
		buildResolutionStatusMap();

		UserNameHelper.setConfiguration(this);

	}

	/**
	 * Validates the workflows.
	 */
	private void validate() throws Exception {
		if (workflows != null) {
			Map<String, Map<String, String>> transitionMap = new HashMap<>();
			for (Workflow workflow : workflows) {
				if (workflow != null) {
					List<Step> steps = workflow.getSteps();
					validateSteps(steps, transitionMap);
				}
			}
		}

		// validate then assign userStylea value
		String[] stylesDefault = USER_NAME_STYLE_DEFAULT.toLowerCase().split(",");

		String stylesString = this.getJiraHandling(USER_NAME_STYLE);
		if (stylesString == null || stylesString.length() < 1) {
			userStyles = stylesDefault;
		} else {
			userStyles = stylesString.split(",");

			try {
				UserNameHelper.validateConfig(userStyles, stylesDefault);
			} catch (Exception e) {
				throw e;
			}

		}

	}

	/**
	 * Validates the workflow steps.
	 */
	private void validateSteps(List<Step> steps, Map<String, Map<String, String>> transitionMap) throws Exception {
		if (steps != null) {
			for (Step step : steps) {
				if (step != null) {
					List<Transition> transitions = step.getTransitions();
					validateTransitions(transitions, transitionMap);
				}
			}
		}
	}

	/**
	 * Validates the step transitions.
	 */
	private void validateTransitions(List<Transition> transitions, Map<String, Map<String, String>> transitionMap) throws Exception {
		if (transitions != null) {
			for (Transition transition : transitions) {
				if (transition != null) {
					String name = transition.getName();
					String destStep = transition.getDestinationStep();
					if (!isEmpty(name) && !isEmpty(destStep)) {
						Map<String, String> destStepMap = transitionMap.get(name);
						if (destStepMap == null) {
							destStepMap = new HashMap<>();
						}
						destStepMap.put(destStep, destStep);
						transitionMap.put(name, destStepMap);
						if (destStepMap.keySet().size() > 1) {
							StringBuilder sb = new StringBuilder();
							for (String key : destStepMap.keySet()) {
								if (key != null) {
									sb.append(" '").append(key).append("'");
								}
							}
							throw new Exception(
								"JIRA config file error: Transition '"
								+ name
								+ "' have different destination steps:"
								+ sb.toString());
						}
					}
				}
			}
		}
	}

	/**
	 * Builds the resolution status map.
	 */
	private void buildResolutionStatusMap() {
		Map<String, String> statusMap = new HashMap<>();
		if (workflows != null) {
			for (Workflow workflow : workflows) {
				if (workflow != null) {
					List<ResolutionTransition> resolutionTransitions = workflow.getResolutionTransitions();
					buildResolutionTransitionStatusMap(workflow, resolutionTransitions, statusMap);
				}
			}
		}
		workflowMap.put("resolutionStatus", statusMap);
	}

	/**
	 * Builds the resolution transition status map.
	 */
	private void buildResolutionTransitionStatusMap(Workflow workflow, List<ResolutionTransition> resolutionTransitions, Map<String, String> statusMap) {
		if (resolutionTransitions != null) {
			for (ResolutionTransition resolutionTransition : resolutionTransitions) {
				String transitionName = resolutionTransition.getName();
				if (transitionName != null) {
					String stepName = getStepForTransitionName(workflow, transitionName);
					if (stepName != null) {
						String status = getStatusForStep(workflow, stepName);
						if (status != null) {
							statusMap.put(status, status);
						}
					}
				}
			}
		}
	}

	/**
	 * Gets the step for transition name.
	 *
	 * @param workflow the workflow
	 * @param transitionName the transition name
	 * @return the step for transition name
	 */
	protected String getStepForTransitionName(Workflow workflow, String transitionName) {
		if (transitionName != null) {
			if (workflow != null) {
				List<Step> steps = workflow.getSteps();
				if (steps != null) {
					for (Step step : steps) {
						if (step != null) {
							List<Transition> transitions = step.getTransitions();
							if (transitions != null) {
								for (Transition transition : transitions) {
									if (transition != null) {
										String name = transition.getName();
										if (name != null) {
											if (name.equalsIgnoreCase(transitionName)) {
												return transition.getDestinationStep();
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return null;
	}

	/**
	 * Gets the status for step.
	 *
	 * @param workflow the workflow
	 * @param stepName the step name
	 * @return the status for step
	 */
	protected String getStatusForStep(Workflow workflow, String stepName) {
		if (workflow != null && stepName != null) {
			List<Step> steps = workflow.getSteps();
			if (steps != null) {
				for (Step step : steps) {
					if (step != null) {
						String name = step.getName();
						if (name != null) {
							if (name.equalsIgnoreCase(stepName)) {
								return step.getLinkedStatus();
							}
						}
					}
				}
			}
		}
		return null;
	}

	/**
	 * Parses the config xml file.
	 */
	private void parseXmlFile() throws Exception {
		// Get the factory
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		try {
			// Using factory get an instance of document builder
			DocumentBuilder db = dbf.newDocumentBuilder();
			// Parse using builder to get DOM representation of the XML file
			dom = db.parse(xmlFile);
		} catch (ParserConfigurationException e) {
			logger.log(Level.SEVERE, "ParserConfigurationException parsing the JIRA REST config XML file.", e);
			throw new Exception("ParserConfigurationException occurred while parsing the JIRA REST config file.", e);
		} catch (SAXException e) {
			logger.log(Level.SEVERE, "SAXException parsing the JIRA REST config XML file.", e);
			throw new Exception("SAXException occurred while parsing the JIRA REST config file.", e);
		} catch (IOException e) {
			logger.log(Level.SEVERE, "OException parsing the JIRA REST config XML file.", e);
			throw new Exception("IOException occurred while parsing the JIRA REST config file.", e);
		}
	}

	/**
	 * Parses the top level elements - e.g., custom fields and workflows from the XML document.
	 */
	private void parseDocument() throws Exception {
		// Get the root elememt
		Element element = dom.getDocumentElement();
		// Get a nodelist of <CustomField> elements
		NodeList nl = element.getElementsByTagName("CustomField");
		if (nl != null && nl.getLength() > 0) {
			for (int i = 0; i < nl.getLength(); i++) {
				// Get the CustomField element
				Element e = (Element) nl.item(i);
				// Get the CustomField object
				CustomField cf = getCustomField(e);
				// Add it to list
				customFields.add(cf);
			}
		}

		// Get a nodelist of <Workflow> elements
		NodeList nlist = element.getElementsByTagName("Workflow");
		if (nlist != null && nlist.getLength() > 0) {
			for (int i = 0; i < nlist.getLength(); i++) {
				// Get the Workflow element
				Element e = (Element) nlist.item(i);
				// Get the Workflow object
				Workflow wf = getWorkflow(e);
				// Add it to list
				workflows.add(wf);
			}
		}

		// JiraHandling elements
		XPath xPath = XPathFactory.newInstance().newXPath();
		for (String lookFor : jiraHandlingElements) {
			String find = "/Config/JiraHandling/" + lookFor + "/text()";
			Object val = (Object) xPath.evaluate(find, dom);
			if (val != null && val instanceof String) {
				String textVal = (String) val;
				textVal = textVal.replaceAll("\\s+", "");
				switch (lookFor) {
					case SOCKET_TIMEOUT_SECONDS:
					case REQUEST_TIMEOUT_SECONDS:
					case CONNECTION_TIMEOUT_SECONDS:
						boolean isValid = setTimeout(textVal, lookFor);
						if (!isValid) {
							break;
						}
					default:
						// add valid values
						jiraHandling.put(lookFor, textVal);
				}
			}
		}
	}

	/**
	 * validate and set the timeout for future Jira connections.
	 *
	 * @param textVal (not null) the value in the config file, trimmed, represents Integer.
	 * @param lookFor the timeout setting to set (e.g. SOCKET_TIMEOUT_SECONDS)
	 * @return 0 if textVal is < 1 or not an integer
	 */
	private boolean setTimeout(String textVal, String lookFor) {
		if (textVal.length() < 1) {
			return false;
		}
		int timeOut;
		try {
			timeOut = Integer.parseInt(textVal);
		} catch (NumberFormatException e) {
			logger.log(Level.INFO, "The config file value for JiraHandling/{0} has non integer: \"{1}\"; Using Jira Default.", new Object[]{lookFor, textVal});
			return false;
		}
		if ((timeOut < 1)) {
			logger.log(Level.INFO, "The config file value for JiraHandling/{0} must be > 0.  Value: {1}; Using Jira Default.", new Object[]{lookFor, textVal});
			return false;
		}
		switch (lookFor) {
			case SOCKET_TIMEOUT_SECONDS:
				P4AsynchronousHttpClientFactory.setSocketTimeout(timeOut);
				break;
			case REQUEST_TIMEOUT_SECONDS:
				P4AsynchronousHttpClientFactory.setRequestTimeout(timeOut);
				break;
			case CONNECTION_TIMEOUT_SECONDS:
				P4AsynchronousHttpClientFactory.setConnectionTimeout(timeOut);
				break;
		}
		return true;
	}

	private Map<String, String> ignoredProjects;

	/**
	 * check if project is the ignored list
	 *
	 * @param projId Project to check
	 * @return true if project is in the ignored project list.
	 */
	public boolean isIgnoredProject(String projId) {
		if (ignoredProjects == null) {
			ignoredProjects = new HashMap<>();
			String projList = this.getJiraHandling(IGNORE_PROJECTS);
			if (projList == null) {
				return false;
			}
			String[] projects = projList.split(",");
			for (String project : projects) {
				if (project.length() > 0) {
					ignoredProjects.put(project, project);
				}
			}
		}
		return ignoredProjects.containsKey(projId);
	}

	/**
	 * what to do with user field. On prem uses name Cloud and name=null, so other options are
	 * 'email','fullname','emailshort'.
	 */
	private String[] userStyles;

	public String[] getUserStyles() {
		return userStyles;
	}

	/**
	 * Gets the custom field.
	 */
	private CustomField getCustomField(Element element) throws Exception {
		List<Option> options = new ArrayList<>();
		// For each <CustomField> element get the values
		String name = getTextValue(element, "name");
		String access = getTextValue(element, "access");
		String type = getTextValue(element, "type");
		// Get the options
		NodeList nl = element.getElementsByTagName("Option");
		if (nl != null && nl.getLength() > 0) {
			for (int i = 0; i < nl.getLength(); i++) {
				// Get the element
				Element e = (Element) nl.item(i);
				// Get the Option object
				Option o = getOption(e);
				// add it to list
				options.add(o);
			}
		}
		if (options.isEmpty()) {
			options = null;
		}
		// Create a new CustomField with the values read from the XML nodes
		CustomField cf = new CustomField(name, access, type, options);
		return cf;
	}

	/**
	 * Gets the option.
	 */
	private Option getOption(Element element) throws Exception {
		// For each <Option> element get the values
		String value = getTextValue(element, "value");
		// Create a new Option with the values read from the XML nodes
		Option opt = new Option(value, value);
		return opt;
	}

	/**
	 * Gets the workflow.
	 */
	private Workflow getWorkflow(Element element) throws Exception {
		List<Step> steps = new ArrayList<>();
		List<ResolutionTransition> resolutionTransitions = new ArrayList<>();
		// For each <Workflow> element get the values
		String name = getTextValue(element, "name");
		// Get a nodelist of <Step> elements
		NodeList nl = element.getElementsByTagName("Step");
		if (nl != null && nl.getLength() > 0) {
			for (int i = 0; i < nl.getLength(); i++) {
				// Get the element
				Element e = (Element) nl.item(i);
				// Get the Step object
				Step s = getStep(e);
				// add it to list
				steps.add(s);
			}
		}
		// Get a nodelist of <ResolutionTransition> elements
		nl = element.getElementsByTagName("ResolutionTransition");
		if (nl != null && nl.getLength() > 0) {
			for (int i = 0; i < nl.getLength(); i++) {
				// Get the element
				Element e = (Element) nl.item(i);
				// Get the ResolutionTransition object
				ResolutionTransition rt = getResolutionTransition(e);
				// add it to list
				resolutionTransitions.add(rt);
			}
		}
		// Create a new Workflow with the value read from the XML nodes
		Workflow wf = new Workflow(name, steps, resolutionTransitions);
		return wf;
	}

	/**
	 * Gets the resolution transition.
	 */
	private ResolutionTransition getResolutionTransition(Element element)
		throws Exception {
		// For the <ResolutionTransition> element get the values
		String name = getTextValue(element, "name");
		// Create a new ResolutionTransition with the values read from the XML
		// nodes
		ResolutionTransition rt = new ResolutionTransition(name);
		return rt;
	}

	/**
	 * Gets the step.
	 */
	private Step getStep(Element element) throws Exception {
		List<Transition> transitioins = new ArrayList<>();
		// For each <Step> element get the values
		String name = getTextValue(element, "name");
		String linkedStatus = getTextValue(element, "linkedStatus");
		// Get a nodelist of <Transition> elements
		NodeList nl = element.getElementsByTagName("Transition");
		if (nl != null && nl.getLength() > 0) {
			for (int i = 0; i < nl.getLength(); i++) {
				// Get the element
				Element e = (Element) nl.item(i);
				// Get the Transition object
				Transition t = getTransition(e);
				// add it to list
				transitioins.add(t);
			}
		}
		// Create a new Step with the values read from the XML nodes
		Step s = new Step(name, linkedStatus, transitioins);
		return s;
	}

	/**
	 * Gets the transition.
	 */
	private Transition getTransition(Element element) throws Exception {
		// For the <Transition> element get the values
		String name = getTextValue(element, "name");
		String destinationStep = getTextValue(element, "destinationStep");
		// Create a new Transition with the values read from the XML nodes
		Transition t = new Transition(name, destinationStep);
		return t;
	}

	/**
	 * Gets the text value.
	 */
	private String getTextValue(Element element, String tag) {
		// Get value from the attribute
		String textVal = element.getAttribute(tag);
		// If there is no attribute value, then get it from the nested element
		if (isEmpty(textVal)) {
			NodeList nl = element.getElementsByTagName(tag);
			if (nl != null && nl.getLength() > 0) {
				Element e = (Element) nl.item(0);
				textVal = e.getFirstChild().getNodeValue();
			}
		}
		// Remove extra whitespace in the value
		if (textVal != null) {
			textVal = textVal.trim();
			textVal = textVal.replaceAll("\\s+", " ");
		}
		return (textVal == null ? "" : textVal);
	}

	/**
	 * Check if the string is empty.
	 */
	private boolean isEmpty(String value) {
		return value == null || value.trim().length() == 0;
	}

	/**
	 * @return String
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("******** User Defined Configurations ********").append(LINE_SEPARATOR);
		sb.append("========== Custom fields ==========").append(LINE_SEPARATOR);
		if (customFields != null) {
			for (CustomField customField : customFields) {
				if (customField != null) {
					sb.append("--- customField:").append(customField.toString()).append(LINE_SEPARATOR);
				}
			}
		}
		sb.append("========= Workflows =========").append(LINE_SEPARATOR);
		if (workflows != null) {
			for (Workflow workflow : workflows) {
				if (workflow != null) {
					sb.append("--- workflow: ---").append(LINE_SEPARATOR);
					sb.append(workflow.toString());
					sb.append("---------------").append(LINE_SEPARATOR);
				}
			}
		}
		sb.append("=========== JiraHandling ==========").append(LINE_SEPARATOR);

		for (String lookFor : jiraHandlingElements) {
			String value = jiraHandling.get(lookFor);
			sb.append(lookFor).append(" = '").append(value != null ? value : "").append("'").append(LINE_SEPARATOR);
		}
		sb.append("***********************************").append(LINE_SEPARATOR);
		return sb.toString();
	}
}
