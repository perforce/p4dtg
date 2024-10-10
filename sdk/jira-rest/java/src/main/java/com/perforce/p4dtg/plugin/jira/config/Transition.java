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

/**
 * Represents a transition in a workflow step.
 */
public class Transition extends Base {

    private String destinationStep;

    /**
     * Instantiates a new transition.
     *
     * @param name
     *            the name
     * @param destinationStep
     *            the destinationStep
     * @throws Exception
     */
    public Transition(String name, String destinationStep) throws Exception {
        super(name);
        this.destinationStep = destinationStep;
    }

    /**
     * Gets the destination step.
     *
     * @return the destination step
     */
    public String getDestinationStep() {
        return destinationStep;
    }

    /**
     * Sets the destination step.
     *
     * @param destinationStep
     *            the new destination step
     */
    public void setDestinationStep(String destinationStep) {
        this.destinationStep = destinationStep;
    }

    /**
     * @see java.lang.Object#toString()
     */
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("destinationStep: ").append(destinationStep == null ? "" : destinationStep).append(LINE_SEPARATOR);
        return sb.toString();
    }
}
