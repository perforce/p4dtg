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
 * Represents a user defined workflow.
 */
public class Workflow extends Base {

    private List<Step> steps;
    private List<ResolutionTransition> resolutionTransitions;

    /**
     * Instantiates a new workflow.
     *
     * @param name
     *            the name
     * @param steps
     *            the steps
     * @param resolutionTransitions
     *            the resolution transitions
     * @throws Exception
     *             the exception
     */
    public Workflow(String name, List<Step> steps, List<ResolutionTransition> resolutionTransitions) throws Exception {
        super(name);
        this.resolutionTransitions = resolutionTransitions;
        this.steps = steps;
    }

    /**
     * Get the steps.
     *
     * @return the steps
     */
    public List<Step> getSteps() {
        return steps;
    }

    /**
     * Set the steps.
     *
     * @param steps
     *            the new steps
     */
    public void setSteps(List<Step> steps) {
        this.steps = steps;
    }

    /**
     * Gets the resolution transitions.
     *
     * @return the resolution transitions
     */
    public List<ResolutionTransition> getResolutionTransitions() {
        return resolutionTransitions;
    }

    /**
     * Sets the resolution transitions.
     *
     * @param resolutionTransitions
     *            the new resolution transitions
     */
    public void setResolutionTransitions(List<ResolutionTransition> resolutionTransitions) {
        this.resolutionTransitions = resolutionTransitions;
    }

    /**
     * To string.
     *
     * @return the string
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("name: ").append(name == null ? "" : name).append(LINE_SEPARATOR);
        if (steps != null) {
            for (Step step : steps) {
                if (step != null) {
                    sb.append("--- step: ---").append(LINE_SEPARATOR);
                    sb.append(step.toString());
                    sb.append("---------------").append(LINE_SEPARATOR);
                }
            }
        }
        if (resolutionTransitions != null) {
            for (ResolutionTransition resolutionTransition : resolutionTransitions) {
                if (resolutionTransition != null) {
                    sb.append("--- resolutionTransition: ---").append(LINE_SEPARATOR);
                    sb.append(resolutionTransition.toString());
                    sb.append("---------------").append(LINE_SEPARATOR);
                }
            }
        }
        return sb.toString();
    }

}
