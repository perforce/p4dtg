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

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.atlassian.jira.rest.client.api.OptionalIterable;
import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.domain.BasicComponent;

import com.atlassian.jira.rest.client.api.domain.Field;
import com.atlassian.jira.rest.client.api.domain.FieldType;
import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.IssueType;
import com.atlassian.jira.rest.client.api.domain.Priority;
import com.atlassian.jira.rest.client.api.domain.Project;
import com.atlassian.jira.rest.client.api.domain.Resolution;
import com.atlassian.jira.rest.client.api.domain.Status;
import com.atlassian.jira.rest.client.api.domain.Transition;
import com.atlassian.jira.rest.client.api.domain.Version;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.config.Configuration;
import com.perforce.p4dtg.plugin.jira.config.CustomField;
import com.perforce.p4dtg.plugin.jira.rest.client.RestClientManager;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.ErrorResponse;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;

/**
 * This helper class provides convenient methods for the request handler.
 */
public class IssueFieldsMapper {

    private static final Logger logger = Logger.getLogger(IssueFieldsMapper.class.getPackage().getName());

    private RestClientManager restClientManager;
    private Configuration configuration;
    private TransitionStatusMatcher transitionStatusMatcher = new TransitionStatusMatcher(configuration);
    
    /**
     * Constructor to create a new issue fields mapper.
     *
     * @param restClientManger
     *            the rest client manager
     * @param configuration
     *            the configuration
     */
    public IssueFieldsMapper(RestClientManager restClientManger, Configuration configuration) {
        this.restClientManager = restClientManger;
        this.configuration = configuration;
        this.transitionStatusMatcher = new TransitionStatusMatcher(configuration);
    }

    /**
     * Gets the issue types map.
     *
     * @param projId
     *            the proj id
     * @return the issue types map
     */
    public Map<String, String> getIssueTypesMap(String projId) {
        Map<String, String> map = new LinkedHashMap<String, String>();
        if (projId != null && !projId.equalsIgnoreCase("*All*")) {
            // Get a list of available issue types for a specific project
            Project project = restClientManager.getProjectClient().getProject(projId).claim();
            if (project != null) {
            	OptionalIterable<IssueType> issueTypes = project.getIssueTypes();
            	if (issueTypes != null) {
            		for (IssueType type : issueTypes) {
            			if (type != null) {
                            // Name is unique according to JIRA.
                            map.put(type.getName(), type.getId().toString());
            			}
            		}
            	}
            }
        } else {
            // Get a complete list of available issue types from the server
        	Iterable<IssueType> issueTypes = restClientManager.getExtendedMetadataClient().getIssueTypes().claim();
        	if (issueTypes != null) {
        		for (IssueType type : issueTypes) {
        			if (type != null) {
                        // Name is unique according to JIRA.
                        map.put(type.getName(), type.getId().toString());
        			}
        		}
        	}
        }
        return map;
    }

    /**
     * Gets the priorities map.
     *
     * @return the priorities map
     */
    public Map<String, String> getPrioritiesMap() {
        Map<String, String> map = new LinkedHashMap<String, String>();
    	Iterable<Priority> priorities = restClientManager.getExtendedMetadataClient().getPriorities().claim();
        if (priorities != null) {
            for (Priority priority : priorities) {
                if (priority != null) {
                    // Name is unique according to JIRA.
                    map.put(priority.getName(), priority.getId().toString());
                }
            }
        }
        return map;
    }

    /**
     * Gets the resolutions map.
     *
     * @return the resolutions map
     */
    public Map<String, String> getResolutionsMap() {
        Map<String, String> map = new LinkedHashMap<String, String>();
        Iterable<Resolution> resolutions = restClientManager.getExtendedMetadataClient().getResolutions().claim();
        if (resolutions != null) {
            for (Resolution resolution : resolutions) {
                if (resolution != null) {
                    // Name is unique according to JIRA.
                    map.put(resolution.getName(), Utils.getIdFromUri(resolution.getSelf()));
                }
            }
        }
        return map;
    }

    /**
     * Gets the statuses map.
     *
     * @return the statuses map
     */
    public Map<String, String> getStatusesMap() {
        Map<String, String> map = new LinkedHashMap<String, String>();
        Iterable<Status> statuses = restClientManager.getExtendedMetadataClient().getStatuses().claim();
        if (statuses != null) {
            for (Status status : statuses) {
                if (status != null) {
                    // Name is unique according to JIRA.
                    map.put(status.getName(), Utils.getIdFromUri(status.getSelf()));
                }
            }
        }
        return map;
    }

    /**
     * Gets the custom fields map.
     *
     * @return the custom fields map
     */
    public Map<String, String> getCustomFieldsMap() {
        Map<String, String> map = new LinkedHashMap<String, String>();
    	Iterable<Field> fields = restClientManager.getExtendedMetadataClient().getFields().claim();
        if (fields != null) {
            for (Field field : fields) {
                if (field != null) {
                	if (field.getFieldType() == FieldType.CUSTOM) {
	                    // Name is NOT unique, so use id as key.
	                    map.put(field.getId(), field.getName());
                	}
                }
            }
        }
        return map;
    }

    /**
     * Gets the components map.
     *
     * @param projectKey
     *            the project key
     * @return the components map
     */
    public Map<String, String> getComponentsMap(String projectKey) {
        Map<String, String> map = new LinkedHashMap<String, String>();
        if (projectKey != null) {
	    	Project project = restClientManager.getProjectClient().getProject(projectKey).claim();
	    	if (project != null) {
	    		Iterable<BasicComponent> components = project.getComponents();
	            if (components != null) {
	                for (BasicComponent component : components) {
	                    if (component != null) {
	                        // Name is unique according to JIRA.
	                        map.put(component.getName(), component.getId().toString());
	                    }
	                }
	            }
	    	}
        }
        return map;
    }

    /**
     * Gets the versions map.
     *
     * @param projectKey
     *            the project key
     * @return the versions map
     */
    public Map<String, String> getVersionsMap(String projectKey) {
        Map<String, String> map = new LinkedHashMap<String, String>();
        if (projectKey != null) {
	    	Project project = restClientManager.getProjectClient().getProject(projectKey).claim();
	    	if (project != null) {
	            Iterable<Version> versions = project.getVersions();
	            if (versions != null) {
	                for (Version version : versions) {
	                    if (version != null) {
	                        // Name is unique according to JIRA.
	                        map.put(version.getName(), version.getId().toString());
	                    }
	                }
	            }
	    	}
        }
        return map;
    }

    /**
     * Gets the actions map.
     *
     * @param issueKey
     *            the issue key
     * @return the actions map
     */
    public Map<String, String> getActionsMap(String issueKey) {
        Map<String, String> map = new HashMap<String, String>();
        if (issueKey != null) {
        	Issue issue = restClientManager.getExtendedIssueClient().getIssue(issueKey).claim();
        	if (issue != null) {
        		Iterable<Transition> transitions = restClientManager.getExtendedIssueClient().getTransitions(issue).claim();
	            if (transitions != null) {
	                for (Transition transitioin : transitions) {
	                    if (transitioin != null) {
	                        // Name is unique according to JIRA.
	                        map.put(transitioin.getName(), String.valueOf(transitioin.getId()));
	                    }
	                }
	            }
        	}
        }
        return map;
    }

    /**
     * Extract the update status and resolution fields.
     * 
     * @param defectFields the update defect fields
     * @return map the status and resolution fields
     */
	public Map<String, String[]> getStatusResolutionMap(Map<String, String[]> defectFields) {
		Map<String, String[]> statusResolutionFields = new HashMap<String, String[]>();
		if (defectFields != null) {
			if (defectFields.containsKey(Constants.ISSUE_FIELD_STATUS)) {
				statusResolutionFields.put(Constants.ISSUE_FIELD_STATUS,
						defectFields.get(Constants.ISSUE_FIELD_STATUS));
				if (defectFields.containsKey(Constants.ISSUE_FIELD_RESOLUTION)) {
					statusResolutionFields.put(Constants.ISSUE_FIELD_RESOLUTION,
							defectFields.get(Constants.ISSUE_FIELD_RESOLUTION));
				}
			}
		}
		return statusResolutionFields;
	}

	/**
	 * Check if the update target status/resolution differs from the issue's
	 * current status and resolution.
	 * 
	 * @param issue the current issue
	 * @param statusResolutionFields the update target defect fields
	 * @return true if the update target status/resolution differs
	 */
	public boolean isDifferentStatusResolution(Issue issue, Map<String, String[]> statusResolutionFields) {
		if (issue != null && statusResolutionFields != null) {
			StringBuilder current = null;
			StringBuilder update = null;
			String status = Utils.getMapValue(statusResolutionFields, Constants.ISSUE_FIELD_STATUS);
			String resolution = Utils.getMapValue(statusResolutionFields, Constants.ISSUE_FIELD_RESOLUTION);
			if (!Utils.isEmpty(status)) {
				update = new StringBuilder();
				update.append(status);
				if (!Utils.isEmpty(resolution)) {
					update.append("/").append(resolution);
				}
				if (issue.getStatus() != null) {
					current = new StringBuilder();
					current.append(issue.getStatus().getName());
					if (issue.getResolution() != null) {
						current.append("/").append(issue.getResolution().getName());
					}
				}
			}
			if (current != null && update != null) {
				if (!current.toString().equalsIgnoreCase(update.toString())) {
					return true;
				}
			}
		}
		return false;
	}

    /**
     * Get transition for the specified target status.
     */
    public Transition getTransitionForTargetStatus(Issue issue, String targetStatus) throws RequestException {
        Transition transition = null;
        if (targetStatus != null) {
	        try {
	        	Status currentStatus = issue.getStatus();
	            Map<String, String> actions = this.transitionStatusMatcher.lookupActions(currentStatus.getName(), targetStatus);
	            if (actions == null || actions.isEmpty()) {
	                String message = "Error occurred while saving defect:"
	                		+ " no transition defined for current status to target status: "
	                        + " issue key ("
	                        + issue.getKey()
	                        + "), current status ("
	                        + currentStatus.getName()
	                        + "), target status (" + targetStatus + ")";
	                throw new RequestException(new ErrorResponse(message, "0"));
	            }
	            Iterable<Transition> transitions = restClientManager.getExtendedIssueClient().getTransitions(issue).claim();
	            if (transitions == null) {
	                String message = "Error occurred while saving defect:"
	                		+ " no transitions available for current status: "
	                        + " issue key ("
	                        + issue.getKey()
	                        + "), current status ("
	                        + currentStatus.getName()
	                        + "), target status (" + targetStatus + ")";
	                throw new RequestException(new ErrorResponse(message, "0"));
	            }
	            for (Transition t : transitions) {
	                if (actions.containsKey(t.getName())) {
	                    transition = t;
	                    break;
	                }
	            }
	            if (transition == null) {
	                String message = "Error occurred while saving defect:"
	                		+ " no matching transition found for current status: "
	                        + " issue key ("
	                        + issue.getKey()
	                        + "), current status ("
	                        + currentStatus.getName()
	                        + "), target status (" + targetStatus + ")";
	                throw new RequestException(new ErrorResponse(message, "0"));
	            }
	        } catch (RestClientException e) {
	            throw new RequestException(new ErrorResponse(
	            		"Error occurred while saving defect: "
	            				+ issue.getKey() + " :" + e.toString(), "0"));
	        }
        }
        return transition;
    }
	
	/**
     * Gets the custom field by name.
     *
     * @param name
     *            the name
     * @return the custom field by name
     */
    public Field getCustomFieldByName(String name) {
    	Field customField = null;
        if (name != null) {
        	Iterable<Field> fields = restClientManager.getExtendedMetadataClient().getFields().claim();
            if (fields != null) {
                for (Field field : fields) {
                    if (field != null) {
                    	if (field.getFieldType() == FieldType.CUSTOM) {
                    		if (field.getName().equals(name)) {
                    			return field;
                    		}
                    	}
                    }
                }
            }
        }          
        return customField;
    }

    /**
     * Gets the custom field type by id.
     *
     * @param id
     *            the id
     * @return the custom field type by id
     */
    public String getCustomFieldTypeById(String id) {
        String type = null;
        if (id != null && this.configuration != null && configuration.getCustomFields() != null) {
        	Map<String, String> cfsm = getCustomFieldsMap();
        	if (cfsm != null) {
        		String name = cfsm.get(id);
        		if (name != null) {
                    for (CustomField cf : configuration.getCustomFields()) {
                        if (cf != null && cf.getName() != null && cf.getType() != null) {
                            if (cf.getName().trim().equalsIgnoreCase(name)) {
                                if (cf.getType().trim().equalsIgnoreCase("WORD")) {
                                    type = IResponse.TYPE_WORD;
                                } else if (cf.getType().trim().equalsIgnoreCase("LINE")) {
                                    type = IResponse.TYPE_LINE;
                                } else if (cf.getType().trim().equalsIgnoreCase("TEXT")) {
                                    type = IResponse.TYPE_TEXT;
                                } else if (cf.getType().trim().equalsIgnoreCase("DATE")) {
                                    type = IResponse.TYPE_DATE;
                                } else if (cf.getType().trim().equalsIgnoreCase("SELECT")) {
                                    type = IResponse.TYPE_SELECT;
                                }
                            }
                        }
                    }
        		}
        	}
        }
        return type;
    }

    /**
     * Gets the custom field type by name.
     *
     * @param name
     *            the name
     * @return the custom field type by name
     */
    public String getCustomFieldTypeByName(String name) {
        String type = null;
        if (name != null && this.configuration != null && configuration.getCustomFields() != null) {
            for (CustomField cf : configuration.getCustomFields()) {
                if (cf != null && cf.getName() != null && cf.getType() != null) {
                    if (cf.getName().trim().equalsIgnoreCase(name)) {
                        if (cf.getType().trim().equalsIgnoreCase("WORD")) {
                            type = IResponse.TYPE_WORD;
                        } else if (cf.getType().trim().equalsIgnoreCase("LINE")) {
                            type = IResponse.TYPE_LINE;
                        } else if (cf.getType().trim().equalsIgnoreCase("TEXT")) {
                            type = IResponse.TYPE_TEXT;
                        } else if (cf.getType().trim().equalsIgnoreCase("DATE")) {
                            type = IResponse.TYPE_DATE;
                        } else if (cf.getType().trim().equalsIgnoreCase("SELECT")) {
                            type = IResponse.TYPE_SELECT;
                        }
                    }
                }
            }
        }
        return type;
    }

    /**
     * Gets the issue key number.
     *
     * @param issueKey
     *            the issue key
     * @return the issue key number
     */
    public int getIssueKeyNumber(String issueKey) {
        int issueKeyNumber = -1;
        if (issueKey != null) {
            String[] parts = issueKey.split("-");
            if (parts != null && parts.length > 1) {
                if (parts[1] != null) {
                    try {
                        issueKeyNumber = Integer.parseInt(parts[1]);
                    } catch (NumberFormatException e) {
                        logger.log(Level.SEVERE, "Exception parsing issue key number.", e);
                    }
                }
            }
        }
        return issueKeyNumber;
    }

    /**
     * Next issue key number low.
     *
     * @param projectKey
     *            the project key
     * @param issueKeyNumber
     *            the issue key number
     * @param upperLimit
     *            the upper limit
     * @return the int
     */
    public int nextIssueKeyNumberLow(String projectKey, int issueKeyNumber, int upperLimit) {
        int issueKeyNumberLow = -1;
        if (!Utils.isEmpty(projectKey) && issueKeyNumber > 0) {
            int nextIssueKeyNumber = issueKeyNumber;
            while (nextIssueKeyNumber <= upperLimit) {
                String issueKey = projectKey + "-" + nextIssueKeyNumber;
                Issue issue = restClientManager.getExtendedIssueClient().getIssue(issueKey).claim();
                if (issue != null) {
                    issueKeyNumberLow = getIssueKeyNumber(issue.getKey());
                    break;
                }
                nextIssueKeyNumber++;
            }
        }
        return issueKeyNumberLow;
    }

    /**
     * Next issue key number high.
     *
     * @param projectKey
     *            the project key
     * @param issueKeyNumber
     *            the issue key number
     * @param lowerLimit
     *            the lower limit
     * @return the int
     */
    public int nextIssueKeyNumberHigh(String projectKey, int issueKeyNumber, int lowerLimit) {
        int issueKeyNumberHigh = -1;
        if (!Utils.isEmpty(projectKey) && issueKeyNumber > 0) {
            int nextIssueKeyNumber = issueKeyNumber;
            while (nextIssueKeyNumber >= lowerLimit) {
                String issueKey = projectKey + "-" + nextIssueKeyNumber;
                Issue issue = restClientManager.getExtendedIssueClient().getIssue(issueKey).claim();
                if (issue != null) {
                    issueKeyNumberHigh = getIssueKeyNumber(issue.getKey());
                    break;
                }
                nextIssueKeyNumber--;
            }
        }
        return issueKeyNumberHigh;
    }
}
