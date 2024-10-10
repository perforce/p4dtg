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

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;

/**
 * Builder for composing the JQL for querying issues from the JIRA server.
 */
public class JqlSearchBuilder {

    static Logger logger = Logger.getLogger(JqlSearchBuilder.class.getPackage().getName());

    private String projId = null;
    private String projects[] = null;
    private String date = null;
    private String modBy = null;
    private String modDate = null;
    private String userName = null;
    private String segmentFilter = null;
    private String orderBy = null;

    /**
     * Builds the JQL.
     *
     * @return the JQL string
     */
    public String build() {
        DateFormat formatter = new SimpleDateFormat(Constants.JQL_DATE_PATTERN);
        StringBuilder sb = new StringBuilder();
        // if projId set:  use that
        // else if projects set:  use that
        // else there is no  "project=xxx" in the query.
        if (projId != null || projects != null) {
            if (projId != null) {
                sb.append("project = \"").append(projId).append("\"");
            } else if (projects.length > 0) {
                boolean firstProject = true;
                sb.append("project in (");
                for (String project : projects) {
                    if (!firstProject) {
                        sb.append(",");
                    } else {
                        firstProject = false;
                    }
                    sb.append("\"").append(project).append("\"");
                }
                sb.append(")");
            }
        }

        if (modDate != null && date != null) {
            try {
                Date d = formatter.parse(date);
                addAnd(sb);
                sb.append(modDate.toLowerCase()).append(" > \"").append(formatter.format(d)).append("\"");
            } catch (ParseException e) {
                String msg = "JqlSearchBuilder:  Exception parsing date '" + date + "'";
                logger.log(Level.SEVERE, msg, e);
                throw new IllegalArgumentException();
            }
        }
        // we ignore mobBy field as jira does not have a "last modified by" field
        // out of the box.
        if (modBy != null && userName != null) {
            // see old revisions if you want to see the old logic.
        }

        if (!Utils.isEmpty(segmentFilter)) {
            sb.append(" ").append(segmentFilter);
        }

        if (!Utils.isEmpty(orderBy)) {
            sb.append(" ").append(orderBy);
        }

        return sb.toString();
    }

    private void addAnd(StringBuilder sb) {
        if (sb.length() > 0) {
            sb.append(" AND ");
        }
    }

    public void setProjId(String projId) {
        this.projId = projId;
    }

    public void setProjects(String[] projects) {
        this.projects = projects;
    }

    public void setDate(String date) {
        this.date = date;
    }

    public void setModBy(String modBy) {
        this.modBy = modBy;
    }

    public void setModDate(String modDate) {
        this.modDate = modDate;
    }

    public void setUserName(String userName) {
        this.userName = userName;
    }

    public void setSegmentFilter(String segmentFilter) {
        this.segmentFilter = segmentFilter;
    }

    public void setOrderBy(String orderBy) {
        this.orderBy = orderBy;
    }
}
