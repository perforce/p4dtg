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
import java.util.List;
import java.util.Map;

import com.perforce.p4dtg.plugin.jira.config.Configuration;
import com.perforce.p4dtg.plugin.jira.config.Step;
import com.perforce.p4dtg.plugin.jira.config.Transition;
import com.perforce.p4dtg.plugin.jira.config.Workflow;

/**
 * Helper class for looking up possible transitions for changing issue statuses.
 */
public class TransitionStatusMatcher {

    private Configuration configuration;

    public TransitionStatusMatcher(Configuration configuration) {
    	this.configuration = configuration;
    }
	
    /**
     * Lookup the possible actions for transitioning from one status to another.
     *
     * @param fromStatus
     *            the from status
     * @param toStatus
     *            the to status
     * @return the actions for the target status
     */
    public Map<String, String> lookupActions(String fromStatus, String toStatus) {
        if (fromStatus != null && toStatus != null) {
        	if (configuration != null && configuration.getWorkflows() != null) {
		        Map<String, String> actionsMap = new HashMap<String, String>();
		        List<Workflow> workflows = configuration.getWorkflows();
		        if (workflows != null) {
		            for (Workflow workflow : workflows) {
		                if (workflow != null) {
		                	lookupSteps(fromStatus, toStatus, workflow.getSteps(), actionsMap);
		                }
		            }
		        }
		        return actionsMap;
        	}
        }
        return null;
    }

    /**
     * Lookup the steps.
     *
     * @param fromStatus
     *            the from status
     * @param toStatus
     *            the to status
     * @param steps
     *            the steps
     * @param actionsMap
     *            the actions map
     */
    private void lookupSteps(String fromStatus, String toStatus, List<Step> steps, Map<String, String> actionsMap) {
        if (fromStatus != null && toStatus != null && steps != null && actionsMap != null) {
            for (Step step : steps) {
                if (step != null) {
                    String linkedStatus = step.getLinkedStatus();
                    List<Transition> transitions = step.getTransitions();
                    if (linkedStatus != null && transitions != null) {
                        if (linkedStatus.trim().equalsIgnoreCase(fromStatus)) {
                        	lookupTransitions(fromStatus, toStatus, transitions, actionsMap);
                        }
                    }
                }
            }
        }
    }

    /**
     * Lookup the transitions.
     *
     * @param fromStatus
     *            the from status
     * @param toStatus
     *            the to status
     * @return the actions for the target
     */
    private void lookupTransitions(String fromStatus, String toStatus, List<Transition> transitions, Map<String, String> actionsMap) {
        if (fromStatus != null && toStatus != null && transitions != null && actionsMap != null) {
            for (Transition transition : transitions) {
                if (transition != null) {
                    String tn = transition.getName();
                    String dsn = transition.getDestinationStep();
                    if (tn != null && dsn != null) {
                        if (matchSteps(dsn, toStatus)) {
                            actionsMap.put(tn, tn);
                        }
                    }
                }
            }
        }
    }

    /**
     * Match the steps.
     *
     * @param stepName
     *            the step name
     * @param linkedStatus
     *            the linked status
     * @return true, if successful match
     */
    private boolean matchSteps(String stepName, String linkedStatus) {
        if (stepName != null && linkedStatus != null) {
            if (configuration != null) {
                List<Workflow> workflows = configuration.getWorkflows();
                if (workflows != null) {
                    for (Workflow workflow : workflows) {
                        if (workflow != null) {
                            List<Step> steps = workflow.getSteps();
                        	if (matchLinkedStatus(stepName, linkedStatus, steps)) {
                        		return true;
                        	}
                        }
                    }
                }
            }
        }
        return false;
    }

    /**
     * Match the linked status.
     *
     * @param stepName
     *            the step name
     * @param linkedStatus
     *            the linked status
     * @param steps
     *            the list of steps
     * @return true, if successful match
     */
    private boolean matchLinkedStatus(String stepName, String linkedStatus, List<Step> steps) {
        if (stepName != null && linkedStatus != null && steps != null) {
            for (Step step : steps) {
                if (step != null) {
                    String sn = new String(step.getName());
                    String ls = new String(step.getLinkedStatus());
                    if (sn != null && ls != null) {
                        if (sn.trim().equalsIgnoreCase(stepName)) {
                            if (ls.trim().equalsIgnoreCase(linkedStatus)) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
}
