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

import java.util.List;

/**
 * Represents a step in a user defined workflow.
 */
public class Step extends Base {

    private String linkedStatus;
    private List<Transition> transitions;

    /**
     * Instantiates a new step.
     *
     * @param name
     *            the name
     * @param access
     *            the access
     * @param type
     *            the type
     * @param transitions
     *            the transitions
     * @throws Exception
     */
    public Step(String name, String linkedStatus, List<Transition> transitions)
            throws Exception {
        super(name);
        this.linkedStatus = linkedStatus;
        this.transitions = transitions;
    }

    /**
     * Gets the linked status.
     *
     * @return the linked status
     */
    public String getLinkedStatus() {
        return linkedStatus;
    }

    /**
     * Sets the linked status.
     *
     * @param linked
     *            status the new linked status
     */
    public void setLinkedStatus(String linkedStatus) {
        this.linkedStatus = linkedStatus;
    }

    /**
     * Gets the transitions.
     *
     * @return the transitions
     */
    public List<Transition> getTransitions() {
        return transitions;
    }

    /**
     * Sets the transitions.
     *
     * @param transitions
     *            the new transitions
     */
    public void setTransitions(List<Transition> transitions) {
        this.transitions = transitions;
    }

    /**
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("linkedStatus: ").append(linkedStatus == null ? "" : linkedStatus).append(LINE_SEPARATOR);
        if (transitions != null) {
            for (Transition transition : transitions) {
                if (transition != null) {
                    sb.append("--- transition: ---").append(LINE_SEPARATOR);
                    sb.append(transition.toString());
                    sb.append("-------------------").append(LINE_SEPARATOR);
                }
            }
        }
        return sb.toString();
    }
}
