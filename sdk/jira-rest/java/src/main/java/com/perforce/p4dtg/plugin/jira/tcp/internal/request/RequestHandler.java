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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import com.atlassian.jira.rest.client.api.RestClientException;
import com.atlassian.jira.rest.client.api.SearchRestClient;
import com.atlassian.jira.rest.client.api.domain.BasicIssue;
import com.atlassian.jira.rest.client.api.domain.BasicProject;
import com.atlassian.jira.rest.client.api.domain.Comment;
import com.atlassian.jira.rest.client.api.domain.Issue;
import com.atlassian.jira.rest.client.api.domain.Project;
import com.atlassian.jira.rest.client.api.domain.SearchResult;
import com.atlassian.jira.rest.client.api.domain.ServerInfo;
import com.atlassian.jira.rest.client.api.domain.Transition;
import com.atlassian.jira.rest.client.api.domain.User;
import com.atlassian.jira.rest.client.api.domain.input.ComplexIssueInputFieldValue;
import com.atlassian.jira.rest.client.api.domain.input.FieldInput;
import com.atlassian.jira.rest.client.api.domain.input.IssueInput;
import com.atlassian.jira.rest.client.api.domain.input.TransitionInput;
import com.atlassian.jira.rest.client.internal.ServerVersionConstants;
import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.TimeCommand;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.config.Configuration;
import com.perforce.p4dtg.plugin.jira.rest.client.RestClientManager;
import com.perforce.p4dtg.plugin.jira.rest.internal.search.JqlSearcher;
import com.perforce.p4dtg.plugin.jira.rest.search.SearchService;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.CustomFieldsResponseBuilder;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.DefectFieldsResponseBuilder;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.DescriptionResponse;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.ErrorResponse;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.FieldResponse;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.StatusResolutionFieldsResponseBuilder;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.StringResponse;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;
import java.util.logging.Handler;
import org.joda.time.DateTime;

/**
 * Manage the requests for the creation, modification and deletion of remote
 * JIRA objects on the remote JIRA server. It facilitates the sequence of remote
 * method calls to various JIRA REST services to fulfill specific requests.
 */
public class RequestHandler extends AbstractRequestHandler {

    private static final Logger logger = Logger.getLogger(RequestHandler.class.getPackage().getName());
    private static final boolean DUMP_DEBUG = Boolean.valueOf(System.getProperty(Constants.DUMP_DEBUG_PROPERTY));

    private static boolean QUERY_LEGACY = false;
    private static final Map<String, String> IGNORE_PROJECTS = new HashMap<>();

    private RestClientManager restClientManager;
    private SearchService searchService;
    private IssueFieldsMapper issueFieldsMapper;

    private String jiraServerUrl;
    private String jiraUsername;
    private String jiraPassword;

    private int queryBatchSize = Constants.JQL_BATCH_SIZE;
    private String defectBatch;

    private String segmentFilter;
    private String projectList;

    private String configFile;
    private Configuration configuration;

    private ServerInfo serverInfo = null;

    public void setLogLevel(Level level) {
        logger.setLevel(level);
        Handler[] handlers = logger.getHandlers();
        for (Handler handler : handlers) {
            handler.setLevel(level);
        }
    }

    public void setJiraServerUrl(String jiraServerUrl) {
        this.jiraServerUrl = jiraServerUrl;
    }

    public void setJiraUsername(String jiraUsername) {
        this.jiraUsername = jiraUsername;
    }

    public void setJiraPassword(String jiraPassword) {
        this.jiraPassword = jiraPassword;
    }

    public void setDefectBatch(String defectBatch) {
        this.defectBatch = defectBatch;
    }

    public void setConfigFile(String configFile) {
        this.configFile = configFile;
    }

    /**
     * Default constructor.
     */
    public RequestHandler() {
        if (DUMP_DEBUG) {
            logger.info("Setting logging level to FINEST");
            logger.setLevel(Level.FINEST);
            Handler[] handlers = logger.getHandlers();
            for (Handler handler : handlers) {
                handler.setLevel(Level.FINEST);
            }
        }
    }

    /**
     * Initialize the REST client and configuration. This must be done before
     * using any of the useful methods in this request handler.
     *
     * @throws Exception
     */
    public void initialize() throws Exception {
        if (this.jiraServerUrl == null) {
            throw new Exception("The jiraServerUrl is null.");
        }
        if (this.jiraUsername == null) {
            throw new Exception("The jiraUsername is null.");
        }
        if (this.jiraPassword == null) {
            throw new Exception("The jiraPassword is null.");
        }
        initRestClientManager();
        initConfig();
        initDefectBatch();
        this.issueFieldsMapper = new IssueFieldsMapper(this.restClientManager, this.configuration);
        this.searchService = new JqlSearcher(this.restClientManager);

        QUERY_LEGACY = "2014.1".equals(configuration.getJiraHandling(Configuration.QUERY_STYLE));
        if (QUERY_LEGACY) {
            logger.info("Using QueryStyle 2014.1");
        }

        String ignore = configuration.getJiraHandling(Configuration.IGNORE_PROJECTS);
        if (ignore != null && ignore.length() > 0) {
            String[] projs = ignore.split(",");
            // rebuild the list for display.
            StringBuilder sb = new StringBuilder();
            for (String proj : projs) {
                if (proj.length() < 1) {
                    continue;
                }
                IGNORE_PROJECTS.put(proj, proj);
                if (sb.length() > 0) {
                    sb.append(",");
                }
                sb.append(proj);
            }
            logger.log(Level.INFO, "Ignored Projects: {0}", sb);
        }
        if (IGNORE_PROJECTS.size() < 1) {
            logger.log(Level.INFO, "Ignored Projects: none");
        }
    }

    private void initConfig() throws Exception {
        // Create the config parser
        configuration = new Configuration(configFile);
        // Parse the config.xml
        configuration.parse();
        // Log the parsed configuration info
        logger.log(Level.INFO, "Configuration loaded from {0}", configuration.getXmlFile());
        logger.finer(configuration.toString());

    }

    private void initDefectBatch() {
        // Get the query batch size
        if (defectBatch != null) {
            try {
                int size = Integer.parseInt(defectBatch);
                if (size > 0) {
                    queryBatchSize = size;
                }
            } catch (NumberFormatException e) {
                logger.log(Level.WARNING, "defect Batch value ''{0}'' invalid: {1}", new Object[]{defectBatch, e.getMessage()});
            }
        }
        logger.log(Level.INFO, "JIRA query batch size is set to: {0}", queryBatchSize);
    }

    private void initRestClientManager() throws Exception {
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        restClientManager = new RestClientManager(this.jiraServerUrl, this.jiraUsername, this.jiraPassword);

        if (restClientManager == null) {
            throw new Exception("Error occurred while connecting to the JIRA server: "
                    + "rest client manager initialization error.");
        }

        // Make sure the JIRA server version is 5 or greater.
        if (logger.isLoggable(Level.FINER)) {
            qMsg = "initRestClientManager: getServerInfo";
            logStart(qMsg, timer);
        }
        if (restClientManager.getExtendedMetadataClient().getServerInfo().claim()
                .getBuildNumber() < ServerVersionConstants.BN_JIRA_5) {
            throw new Exception("This DTG JIRA plugin only support JIRA server version 5 or greater.");
        } else {

            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
        }
    }

    /**
     * @return server's version
     * @throws
     * com.perforce.p4dtg.plugin.jira.tcp.internal.request.RequestException
     * @see com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#getId()
     */
    @Override
    public String getId() throws RequestException {
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        if (serverInfo == null) {
            if (logger.isLoggable(Level.FINER)) {
                qMsg = "getId;  getServerInfo()";
                logStart(qMsg, timer);
            }
            serverInfo = restClientManager.getExtendedMetadataClient().getServerInfo().claim();

            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
        }
        if (serverInfo != null) {
            return serverInfo.getVersion();
        }
        return "";
    }

    @Override
    public StringResponse createDefect(Element request) throws RequestException {
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";

        String projId = getFieldValue(request, PROJID);
        if (projId == null) {
            throw new RequestException(new ErrorResponse(
                    "Missing PROJID in createDefect", "0"));
        }
        if (projId.equalsIgnoreCase(Constants.DTG_PROJECT_ALL)) {
            throw new RequestException(new ErrorResponse(
                    "Invalid PROJID in newDefect", "0"));
        }
        Project project = null;
        try {

            if (logger.isLoggable(Level.FINER)) {
                qMsg = "createDefect:  getProject " + projId;
                logStart(qMsg, timer);
            }
            project = restClientManager.getProjectClient().getProject(projId).claim();
            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }

        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while retrieving project: " + projId + " :"
                    + e.toString(), "0"));
        }
        if (project == null) {
            throw new RequestException(new ErrorResponse(
                    "Defect requested for unknown project: " + projId, "0"));
        }

        Map<String, String[]> defectFields = getDefectFields(request);
        // Remove *Project* field from map
        if (defectFields.containsKey(Constants.DTG_PROJECT)) {
            defectFields.remove(Constants.DTG_PROJECT);
        }

        IssueInputFieldsBuilder iifBuilder = new IssueInputFieldsBuilder(
                projId, defectFields, this.issueFieldsMapper, this.restClientManager);

        BasicIssue basicIssue = null;
        try {
            IssueInput issueInput = iifBuilder.build();

            if (logger.isLoggable(Level.FINER)) {
                qMsg = "createDefect:  createIssue";
                logStart(qMsg, timer);
            }
            basicIssue = restClientManager.getExtendedIssueClient().createIssue(issueInput).claim();
            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while creating defect: " + e.toString(), "0"));
        }

        if (basicIssue == null) {
            throw new RequestException(new ErrorResponse(
                    "Unable to create defect", "0"));
        }

        if (logger.isLoggable(Level.FINER)) {
            qMsg = "createDefect:  getIssue";
            logStart(qMsg, timer);
        }
        Issue issue = restClientManager.getExtendedIssueClient().getIssue(basicIssue.getKey()).claim();
        if (logger.isLoggable(Level.FINER)) {
            logStop(qMsg, timer);
        }

        // Update the status of the newly created issue, if it is different
        // Extract only the status and resolution fields
        Map<String, String[]> statusResolutionFields = issueFieldsMapper.getStatusResolutionMap(defectFields);
        // Check if the update status/resolution differs from the issue
        if (!issueFieldsMapper.isDifferentStatusResolution(issue, statusResolutionFields)) {
            // Prevent updating of the status/resolution
            if (defectFields.containsKey(Constants.ISSUE_FIELD_STATUS)) {
                defectFields.remove(Constants.ISSUE_FIELD_STATUS);
            }
        }
        // Update the issue status
        issue = updateIssueStatus(issue, defectFields);

        return new StringResponse(issue.getKey());
    }

    /**
     * Gets the defect.
     *
     * @param request the request
     * @return the defect
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#getDefect(org.w3c.dom.Element)
     */
    @Override
    public FieldResponse[] getDefect(Element request) throws RequestException {
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        String projId = request.getAttribute(PROJID);
        if (Utils.isEmpty(projId)) {
            throw new RequestException(new ErrorResponse(
                    "Missing PROJID in getDefect", "0"));
        }
        String defectId = request.getAttribute(DEFECT);
        if (Utils.isEmpty(defectId)) {
            throw new RequestException(new ErrorResponse(
                    "Missing DEFECT in getDefect", "0"));
        }

        Issue issue = null;
        try {
            if (logger.isLoggable(Level.FINER)) {
                qMsg = "getDefect: getIssue " + defectId;
                logStart(qMsg, timer);
            }

            issue = restClientManager.getExtendedIssueClient().getIssue(defectId).claim();
            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while retrieving defect: " + defectId
                    + " :" + e.toString(), "0"));
        }
        if (issue == null) {
            throw new RequestException(new ErrorResponse("Defect: "
                    + defectId + " not found", "0"));
        }

        DefectFieldsMapBuilder dfmBuilder = new DefectFieldsMapBuilder(issue,
                this.issueFieldsMapper, this.configuration);
        Map<String, String[]> fieldValueMap = dfmBuilder.build();

        DefectFieldsResponseBuilder dfrBuilder = new DefectFieldsResponseBuilder();
        dfrBuilder.setFieldValueMap(fieldValueMap);

        FieldResponse[] fieldResponses = dfrBuilder.build();

        List<FieldResponse> fields = new ArrayList<>();
        fields.addAll(Arrays.asList(fieldResponses));

        // Add a special *Project* field with value projectId
        fields.add(new FieldResponse(Constants.DTG_PROJECT, issue.getProject().getKey()));

        return fields.toArray(new FieldResponse[fields.size()]);
    }

    /**
     * Gets the project.
     *
     * @param request the request
     * @return the project
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#getProject(org.w3c.dom.Element)
     */
    @Override
    public StringResponse getProject(Element request) throws RequestException {
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        String name = request.getAttribute(PROJECT);
        if (name == null) {
            throw new RequestException(new ErrorResponse(
                    "Missing PROJECT in getProject", "0"));
        }
        if (Constants.DTG_PROJECT_ALL.equalsIgnoreCase(name)) {
            return new StringResponse(name);
        }
        try {
            Project project = getProjectCached(name);
            if (project == null) {
                throw new RequestException(new ErrorResponse(
                        "Unknown project requested: " + name, "0"));
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while retrieving projec: " + name + " :"
                    + e.toString(), "0"));
        }
        return new StringResponse(name);
    }

    /**
     * Gets the field value.
     *
     * @param element the request
     * @param field the field
     * @return the field value
     */
    private String getFieldValue(Element element, String field) {
        if (element != null && field != null) {
            NodeList nl = element.getElementsByTagName(FIELD);
            if (nl != null && nl.getLength() > 0) {
                for (int i = 0; i < nl.getLength(); i++) {
                    Element e = (Element) nl.item(i);
                    if (e != null) {
                        if (e.getAttribute(FIELD_NAME) != null
                                && e.getAttribute(FIELD_NAME).equalsIgnoreCase(field)) {
                            return e.getAttribute(FIELD_VALUE);
                        }
                    }
                }
            }
        }
        return null;
    }

    /**
     * Gets the defect fields.
     *
     * @param element the element
     * @return the defect fields
     */
    public Map<String, String[]> getDefectFields(Element element) {
        if (element != null) {
            Map<String, String[]> nameValueMap = new HashMap<>();
            NodeList nl = element.getElementsByTagName(FIELD);
            if (nl != null && nl.getLength() > 0) {
                for (int i = 0; i < nl.getLength(); i++) {
                    Element e = (Element) nl.item(i);
                    if (e != null) {
                        String name = e.getAttribute(FIELD_NAME);
                        String value = e.getAttribute(FIELD_VALUE);
                        if (name != null && value != null) {
                            nameValueMap.put(name, new String[]{value});
                        }
                    }
                }
            }
            return nameValueMap;
        }
        return null;
    }

    /**
     * List defects. This is called from the p4dtg-repl engine and returns a
     * list of issues updated.
     *
     * @param request the request 
     * @return the list of issues updated
     * @throws RequestException
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#listDefects(org.w3c.dom.Element)
     */
    @Override
    public StringResponse listDefects(Element request) throws RequestException {
        String projId = request.getAttribute(PROJID); // one project or *All* (
        String date = request.getAttribute("DATE"); // updated after
        String max = request.getAttribute("MAX");  // max issues in a batch
        String modBy = request.getAttribute("MODBY"); // always "*Not Supported*"
        String modDate = request.getAttribute("MODDATE"); // updated by: "Updated"
        String user = request.getAttribute("USER");

        if (projId == null) {
            throw new RequestException(new ErrorResponse("Missing PROJID in listDefects", "0"));
        }
        // Return error if the segment filter contains "Status/Resolution"
        if (segmentFilter != null) {
            if (segmentFilter.contains("(Status/Resolution=")) {
                throw new RequestException(
                        new ErrorResponse("Segmentation on Status/Resolution field is not supported", "0"));
            }
        }
        // Normalize date with space in its parts (i.e. DATE="2014/ 3/ 6 11:39: 3")
        if (date != null) {
            try {
                date = Utils.formatDate(Utils.parseDate(date, Constants.DATE_PATTERN), Constants.DATE_PATTERN);
            } catch (ParseException e) {
                throw new RequestException(
                        new ErrorResponse("Invalid date", "0"));
            }
        }

        Map<String, String> allIssueKeys = new HashMap<>();

        List<String> projectIds = getProjectList(projId, QUERY_LEGACY);

        if (QUERY_LEGACY || projectIds.size() == 1) {
            for (String projectId : projectIds) {
                if (projectId != null) {
                    Map<String, String> issueKeys = queryDefects(projectId, null,
                            date, max, modDate);
                    if (issueKeys != null) {
                        allIssueKeys.putAll(issueKeys);
                    }
                }
            }
        } else {
            // New for 2018.1 
            Map<String, String> issueKeys = queryDefects(null, projectIds,
                    date, max, modDate);
            if (issueKeys != null) {
                allIssueKeys.putAll(issueKeys);
            }
        }

        return new StringResponse(allIssueKeys.keySet().toArray(new String[allIssueKeys.size()]));
    }

    /**
     * Gets the project list.
     *
     * If projId is not *All* return projId.
     *
     * Otherwise (when *All*), and IS segmented on project, return the list in
     * the segment.
     *
     * Else (also when *All*) and NOT segmented on project, return an empty
     * list.
     *
     * JqlSearchBuilder relies on the returned value.
     *
     * @param projId the proj id.
     * @return List of project ids.
     * @throws RequestException the request exception
     */
    private List<String> getProjectList(String projId, boolean legacy) throws RequestException {
        if (projId == null) {
            throw new RequestException(new ErrorResponse("Missing PROJID in listDefects", "0"));
        }

        List<String> projectIds = new ArrayList<>();
        if (projId.equalsIgnoreCase(Constants.DTG_PROJECT_ALL)) {
            // projId is *All*  but could be segmented on projects (projectList)
            if (projectList != null && !projectList.equalsIgnoreCase(Constants.DTG_PROJECT_ALL)) {
                // segmented on project, so check each project in the list for 
                // access
                String[] projects = projectList.split(Constants.DTG_PROJECT_SEPARATOR);
                for (String project : projects) {
                    if (project != null && hasProjectAccess(project)) {
                        projectIds.add(project);
                    }
                }
                // make sure the list is NOT empty - should never be if the 
                // p4dtg user's issue access permissions are properly set up.
                if (projectIds.isEmpty()) {
                    throw new RequestException(
                            new ErrorResponse("The P4DTG User does not have access to any of the"
                                    + " projects in the segment; must have one.", "0"));
                }
            } else {
                // not segmented on project, so default to all projects if LEGACY.
                if (legacy) {
                    try {
                        projectIds = getAllProjectsCached();
                    } catch (RestClientException e) {
                        logger.log(Level.SEVERE, e.toString(), e);
                        throw new RequestException(
                                new ErrorResponse("Error occurred while retrieving all projects: " + e.toString(), "0"));
                    }
                }
            }
        } else {
            projectIds.add(projId);
        }

        return projectIds;
    }

    /**
     * Query defects.  This retrieves all issues as defined in the
     * call parameters
     *
     * Either projId or projectIds(list of projects) must be provided.
     * 
     * TODO:  modBy and user can be removed as they a no-op for this
     * plugin.
     *
     * @param projId the project id (if not null, then projectIds is ignored.
     * @param date the date such as yyyy/mm/dd hh:mm
     * @param max the max number of issues to retrieve at a time.
     * @param modBy the mod by field name - for JIRA, "*not supported*
     * @param modDate the mod date field name - usually "Updated"
     * @return the map
     * @throws RequestException the request exception
     */
    protected Map<String, String> queryDefects(String projId, List<String> projectIds,
            String date, String max,
            String modDate) throws RequestException {

        TimeCommand timer = new TimeCommand();
        Project project = null;
        if (projId != null) {
            try {
                project = getProjectCached(projId);
            } catch (RestClientException e) {
                logger.log(Level.SEVERE, e.toString(), e);
                throw new RequestException(new ErrorResponse(
                        "Error occurred while retrieving project: "
                        + projId + ": " + e.toString(), "0"));
            }
            if (project == null) {
                throw new RequestException(new ErrorResponse("Unknown project: "
                        + projId, "0"));
            }
        }
        
        int limit = 0;
        if (max != null) {
            try {
                limit = Integer.parseInt(max);
            } catch (NumberFormatException e) {
                logger.log(Level.WARNING, "Exception parsing max issues limit '" + max + "'", e);
            }
        }

        if (limit <= 0) {
            limit = 200;
        }

        Map<String, String> issueKeys = new HashMap<>();
        int maxBatchResults = (queryBatchSize > limit ? limit : queryBatchSize);
        String query = null;
        JqlSearchBuilder jqlBuilder = new JqlSearchBuilder();
        try {
            jqlBuilder.setProjId(projId);
            if (projectIds != null) {
                jqlBuilder.setProjects(projectIds.toArray(new String[0]));
            }
            jqlBuilder.setDate(date);
            jqlBuilder.setModDate(modDate);
            jqlBuilder.setUserName(null);
            jqlBuilder.setSegmentFilter(segmentFilter);
            jqlBuilder.setOrderBy("ORDER BY key ASC");
            query = jqlBuilder.build();

            logger.log(Level.INFO, "Jira query with batch size ({0}): {1}", new Object[]{queryBatchSize, query});

            // Minus potential memory consuming fields.
            Set<String> fields = new HashSet<>(Arrays.asList(new String[]{"-description", "-comment"}));

            Map<String, Integer> issues = new HashMap<String, Integer>();
            SearchRestClient searchClient = restClientManager.getSearchClient();
            int cnt = maxBatchResults;
            int cntSoFar = 0;
            int loopCnt = 0;

            /**
             * put returned issues into the issueKeys Map to weed out
             * duplications. Duplicates can happen if the pagesize (batchCnt)
             * overlaps on the last page: see the REST api's javadoc. note: old
             * implementation always ran >= 2 queries if issue(s) found, fixed.
             */
            StringBuilder ignoredIssues = new StringBuilder();
            String qMsg;
            while (cnt == maxBatchResults) {
                loopCnt++;
                qMsg = "queryDefects:  Query " + loopCnt + " for project " + 
                        (projId !=null ? projId : "multiple projects") 
                        + " since " + date;
                if (logger.isLoggable(Level.FINER)) {
                    // optionally log the start-of-query time.
                    logStart(qMsg, timer);
                } else {
                    timer.start();
                }
                SearchResult searchResult = searchClient.searchJql(query, maxBatchResults, cntSoFar, fields).claim();
                if (logger.isLoggable(Level.INFO)) {
                    // always log the result time
                    logStop(qMsg, timer, Integer.toString(searchResult.getTotal()) + " issue(s)");
                }
                cnt = 0;
                for (Issue issue : searchResult.getIssues()) {
                    ++cnt;
                    ++cntSoFar;
                    String issueProject = issue.getProject().getKey();
                    // check if issueProject is in ignoreProjects.
                    if (configuration.isIgnoredProject(issueProject)) {
                        if (logger.isLoggable(Level.FINE)) {
                            ignoredIssues.append(issue.getKey()).append(" ");
                        }
                        continue;
                    }
                    issueKeys.put(issue.getKey(), issue.getKey());
                }
            }
            if (logger.isLoggable(Level.FINE)) {
                if (ignoredIssues.length() < 1) {
                    ignoredIssues.append("none.");
                }
                logger.log(Level.FINE, "Ignored issues: {0}", ignoredIssues);
            }

        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while retrieving defects from project: "
                    + projId + ", query = '" + query + "': " + e.toString(), "0"));
        }

        return issueKeys;
    }

    /**
     * List fields.
     *
     * @param request the request
     * @return the description response[]
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#listFields(org.w3c.dom.Element)
     */
    @Override
    public DescriptionResponse[] listFields(Element request) throws RequestException {
        String projId = request.getAttribute(PROJID);
        if (projId == null) {
            throw new RequestException(new ErrorResponse("Missing PROJID in listFields", "0"));
        }

        Map<String, String> issueTypesMap = issueFieldsMapper.getIssueTypesMap(projId);
        Map<String, String> prioritiesMap = issueFieldsMapper.getPrioritiesMap();
        Map<String, String> resolutionsMap = issueFieldsMapper.getResolutionsMap();
        Map<String, String> statusesMap = issueFieldsMapper.getStatusesMap();
        Map<String, String> customFieldsMap = issueFieldsMapper.getCustomFieldsMap();

        List<DescriptionResponse> descs = new LinkedList<>();
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_KEY,
                IResponse.TYPE_WORD, IResponse.ACCESS_DEFECT_ID, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_REPORTER,
                IResponse.TYPE_WORD, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_ASSIGNEE,
                IResponse.TYPE_WORD, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_SUMMARY,
                IResponse.TYPE_LINE, IResponse.ACCESS_RW, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_DESCRIPTION,
                IResponse.TYPE_TEXT, IResponse.ACCESS_RW, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_ENVIRONMENT,
                IResponse.TYPE_TEXT, IResponse.ACCESS_RW, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_COMMENTS,
                IResponse.TYPE_TEXT, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_DUEDATE,
                IResponse.TYPE_DATE, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_UPDATED,
                IResponse.TYPE_DATE, IResponse.ACCESS_MOD_DATE, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_ISSUETYPE,
                IResponse.TYPE_SELECT, IResponse.ACCESS_RW, issueTypesMap
                        .keySet().toArray(new String[issueTypesMap.keySet().size()])));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_PRIORITY,
                IResponse.TYPE_SELECT, IResponse.ACCESS_RW, prioritiesMap
                        .keySet().toArray(new String[prioritiesMap.keySet().size()])));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_RESOLUTION,
                IResponse.TYPE_SELECT, IResponse.ACCESS_RO, resolutionsMap
                        .keySet().toArray(new String[resolutionsMap.keySet().size()])));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_STATUS,
                IResponse.TYPE_SELECT, IResponse.ACCESS_RO, statusesMap
                        .keySet().toArray(new String[statusesMap.keySet().size()])));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_AFFECTSVERSIONS,
                IResponse.TYPE_LINE, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_FIXVERSIONS,
                IResponse.TYPE_LINE, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.ISSUE_FIELD_COMPONENTS,
                IResponse.TYPE_LINE, IResponse.ACCESS_RO, null));
        descs.add(new DescriptionResponse(Constants.DTG_FIELD_FIX,
                IResponse.TYPE_FIX, IResponse.ACCESS_RW, null));
        // Build Status/Resolution fields response
        StatusResolutionFieldsResponseBuilder srfBuilder = new StatusResolutionFieldsResponseBuilder(this.configuration);
        srfBuilder.setStatusesMap(statusesMap);
        srfBuilder.setResolutionsMap(resolutionsMap);
        DescriptionResponse srf = srfBuilder.build();
        if (srf != null) {
            descs.add(srf);
        }
        // Build custom fields
        CustomFieldsResponseBuilder cfBuilder = new CustomFieldsResponseBuilder(this.configuration);
        cfBuilder.setCustomFieldsMap(customFieldsMap);
        List<DescriptionResponse> cfs = cfBuilder.build();
        if (cfs != null) {
            descs.addAll(cfs);
        }
        return descs.toArray(new DescriptionResponse[descs.size()]);
    }

    /**
     * List projects.
     *
     * @param request the request for p4dtg-repl or p4dtg-config
     * @return the string response
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#listProjects(org.w3c.dom.Element)
     */
    @Override
    public StringResponse listProjects(Element request) throws RequestException {
        List<String> projectKeys = new ArrayList<>();
        try {
            projectKeys = getAllProjectsCached();
            if (projectKeys.size() < 1) {
                throw new RestClientException("No projects found:  check jira permissions for jira user.", null);
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while getting project list: " + e.toString(), "0"));
        }
        return new StringResponse(projectKeys.toArray(new String[projectKeys.size()]));
    }

    /**
     * Gets the segment filters.
     *
     * @param request the request
     * @return the segment filters
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#getSegmentFilters(org.w3c.dom.Element)
     */
    @Override
    public StringResponse getSegmentFilters(Element request)
            throws RequestException {
        String projId = request.getAttribute(PROJID);
        projectList = request.getAttribute(PROJECT_LIST);
        segmentFilter = request.getAttribute(SEGMENT_FILTER);

        SegmentFilterTranslator translator = new SegmentFilterTranslator();
        translator.setCustomFieldsMap(issueFieldsMapper.getCustomFieldsMap());
        translator.setIssueTypesMap(issueFieldsMapper.getIssueTypesMap(projId));
        translator.setPrioritiesMap(issueFieldsMapper.getPrioritiesMap());
        translator.setResolutionsMap(issueFieldsMapper.getResolutionsMap());
        translator.setStatusesMap(issueFieldsMapper.getStatusesMap());
        translator.setSegmentFilter(segmentFilter);
        // This will be used in the listDefects() method
        segmentFilter = translator.translate();
        if (logger.isLoggable(Level.FINER)) {
            logger.log(Level.FINER, SEGMENT_FILTER + ": {0}", segmentFilter);
            logger.log(Level.FINER, PROJECT_LIST + ": {0}", projectList);
        }
        return new StringResponse("OK");
    }

    /**
     * Login.
     *
     * @param request the request
     * @return the string response
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#login(org.w3c.dom.Element)
     */
    @Override
    public StringResponse login(Element request) throws RequestException {
        this.jiraServerUrl = request.getAttribute(JIRA_URL);
        if (Utils.isEmpty(this.jiraServerUrl)) {
            throw new RequestException(new ErrorResponse(
                    "Missing JIRA_URL in login", "0"));
        }
        this.jiraUsername = request.getAttribute(JIRA_USER);
        if (Utils.isEmpty(this.jiraUsername)) {
            throw new RequestException(new ErrorResponse(
                    "Missing JIRA_USER in login", "0"));
        }
        this.jiraPassword = request.getAttribute(JIRA_PASSWORD);
        if (Utils.isEmpty(this.jiraPassword)) {
            throw new RequestException(new ErrorResponse(
                    "Missing JIRA_PASSWORD in login", "0"));
        }
        try {
            initialize();
        } catch (Exception e) {
            logger.log(Level.SEVERE, "Error occurred while logging into the JIRA server.", e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while logging into the JIRA server. "
                    + "Please make sure the JIRA server URL, "
                    + "username and password are correct. "
                    + e.getMessage(), "0"));
        }
        return new StringResponse(getId());
    }

    /**
     * Connect.
     *
     * @param request the request
     * @return the string response
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.BaseRequestHandler#connect(org.w3c.dom.Element)
     */
    @Override
    public StringResponse connect(Element request) throws RequestException {
        return new StringResponse("connected");
    }

    /**
     * Ping.
     *
     * @param request the request
     * @return the string response
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#ping(org.w3c.dom.Element)
     */
    @Override
    public StringResponse ping(Element request) {
        return new StringResponse("PONG");
    }

    /**
     * Gets the server date.
     *
     * @param request the request
     * @return the server date
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#getServerDate(org.w3c.dom.Element)
     */
    @Override
    public StringResponse getServerDate(Element request) throws RequestException {
        DateFormat format = new SimpleDateFormat(DATE_PATTERN);
        Date serverDate = null;
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        try {
            if (logger.isLoggable(Level.FINER)) {
                qMsg = "getServerDate:  getServerInfo";
                logStart(qMsg, timer);
            }
            ServerInfo servInfo = restClientManager.getExtendedMetadataClient().getServerInfo().claim();
            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
            if (servInfo != null) {
                DateTime serverTime = servInfo.getServerTime();
                if (serverTime != null) {
                    serverDate = serverTime.toDate();
                } else {
                    throw new RestClientException("server time not included in server info.  See KB.", null);
                }
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while getting the JIRA server date time: " + e.toString(), "0"));
        }
        return new StringResponse(format.format(serverDate));
    }

    /**
     * New defect.
     *
     * @param request the request
     * @return the field response[]
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#newDefect(org.w3c.dom.Element)
     */
    @Override
    public FieldResponse[] newDefect(Element request) throws RequestException {
        String projId = request.getAttribute(PROJID);
        if (Utils.isEmpty(projId)) {
            throw new RequestException(new ErrorResponse("Missing PROJID in newDefect", "0"));
        }
        if (projId.equalsIgnoreCase(Constants.DTG_PROJECT_ALL)) {
            throw new RequestException(new ErrorResponse("Invalid PROJID in newDefect", "0"));
        }
        Project project = null;
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        try {
            if (logger.isLoggable(Level.FINER)) {
                qMsg = "newDefect: get project " + projId;
                logStart(qMsg, timer);
            }
            project = restClientManager.getProjectClient().getProject(projId).claim();
            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while retrieving project: "
                    + projId + " :" + e.toString(), "0"));
        }
        if (project == null) {
            throw new RequestException(new ErrorResponse("Unknown project: " + projId, "0"));
        }

        Map<String, String[]> defectFields = getDefectFields(request);
        // Remove *Project* field from map
        if (defectFields.containsKey(Constants.DTG_PROJECT)) {
            defectFields.remove(Constants.DTG_PROJECT);
        }

        DefaultDefectFieldsMapBuilder ddfmBuilder = new DefaultDefectFieldsMapBuilder(this.restClientManager);
        defectFields = ddfmBuilder.build();

        DefectFieldsResponseBuilder dfrBuilder = new DefectFieldsResponseBuilder();
        dfrBuilder.setFieldValueMap(defectFields);

        FieldResponse[] fieldResponses = dfrBuilder.build();

        List<FieldResponse> fields = new ArrayList<>();
        fields.addAll(Arrays.asList(fieldResponses));

        // Add a special *Project* field with value projectId
        fields.add(new FieldResponse(Constants.DTG_PROJECT, projId));

        return fields.toArray(new FieldResponse[fields.size()]);
    }

    /**
     * Save defect.
     *
     * @param request the request
     * @return the string response
     * @throws RequestException the request exception
     * @see
     * com.perforce.p4dtg.plugin.jira.tcp.IRequestHandler#saveDefect(org.w3c.dom.Element)
     */
    @Override
    public StringResponse saveDefect(Element request) throws RequestException {
        String projId = getFieldValue(request, PROJID);
        if (projId == null) {
            throw new RequestException(new ErrorResponse("Missing PROJID in saveDefect", "0"));
        }
        String defectName = getFieldValue(request, DEFECTID);
        if (defectName == null) {
            throw new RequestException(new ErrorResponse("Missing DEFECT in saveDefect", "0"));
        }

        Issue issue = null;
        TimeCommand timer = new TimeCommand();
        String qMsg = ".";
        try {
            if (logger.isLoggable(Level.FINER)) {
                qMsg = "saveDefect: get issue " + defectName;
                logStart(qMsg, timer);
            }
            issue = restClientManager.getExtendedIssueClient().getIssue(defectName).claim();
            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while retrieving defect: "
                    + defectName + " :" + e.toString(), "0"));
        }
        if (issue == null) {
            throw new RequestException(new ErrorResponse(
                    "Defect: " + defectName + " not found", "0"));
        }

        try {
            Map<String, String[]> defectFields = getDefectFields(request);
            if (defectFields != null) {
                // Remove *Project* field from map
                defectFields.remove(Constants.DTG_PROJECT);
                // Update issue
                issue = updateIssue(issue, defectFields);
            }
        } catch (RestClientException e) {
            logger.log(Level.SEVERE, e.toString(), e);
            throw new RequestException(new ErrorResponse(
                    "Error occurred while saving defect: "
                    + defectName + " :" + e.toString(), "0"));
        }

        return new StringResponse(issue.getKey());
    }

    /**
     * Updates the issue with the specified fields and transitions the issue
     * with the target status and resolution.
     */
    private Issue updateIssue(Issue issue, Map<String, String[]> defectFields) throws RequestException {
        if (defectFields != null) {
            Transition transition = null;
            FieldInput resolutionFieldInput = null;
            Comment comment = null;
            List<FieldInput> fieldInputs = new ArrayList<>();
            TimeCommand timer = new TimeCommand();
            String qMsg = ".";
            // Get the status and resolution
            String[] status = defectFields.remove(Constants.ISSUE_FIELD_STATUS);
            if (!Utils.isEmpty(status)) {
                transition = issueFieldsMapper.getTransitionForTargetStatus(issue, status[0]);
                if (transition != null) {
                    String[] resolution = defectFields.remove(Constants.ISSUE_FIELD_RESOLUTION);
                    if (!Utils.isEmpty(resolution)) {
                        resolutionFieldInput = new FieldInput(
                                Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_RESOLUTION),
                                ComplexIssueInputFieldValue.with("name", resolution[0]));
                    }
                }
            }
            // Get the fix as a comment
            String[] fix = defectFields.remove(Constants.DTG_FIELD_FIX);
            if (!Utils.isEmpty(fix)) {
                comment = Comment.valueOf(fix[0]);
            }
            // Translate defect fields to JIRA issue fields
            DefectFieldsTranslator dfTranslator = new DefectFieldsTranslator(defectFields,
                    this.issueFieldsMapper, this.restClientManager);
            Map<String, String[]> issueFields = dfTranslator.translate();

            for (Entry<String, String[]> entry : issueFields.entrySet()) {
                if (!Utils.isEmpty(entry.getKey())) {
                    //
                    // TODO: if transition, check to make sure only allowable fields in this transition are included
                    //
                    // TODO: if normal update, check editable fields
                    // 
                    if (isSelectInputField(entry.getKey())) {
                        // Don't set select to "" (more than likely to be an error)
                        if (!Utils.isEmpty(entry.getValue())) {
                            // If custom field, use 'name' instead of 'id'
                            if (entry.getKey().startsWith(Constants.CUSTOM_FIELD_ID_PREFIX)) {
                                fieldInputs.add(new FieldInput(entry.getKey(),
                                        ComplexIssueInputFieldValue.with("value", entry.getValue()[0])));
                            } else {
                                fieldInputs.add(new FieldInput(entry.getKey(),
                                        ComplexIssueInputFieldValue.with("id", entry.getValue()[0])));
                            }
                        }
                    } else {
                        fieldInputs.add(new FieldInput(entry.getKey(), entry.getValue()[0]));
                    }
                }
            }
            // Create issue input with specified fields
            IssueInput issueInput = IssueInput.createWithFields(fieldInputs.toArray(
                    new FieldInput[fieldInputs.size()]));
            try {
                // Perform transition on the issue
                if (transition != null) {
                    TransitionInput transitionInput = (resolutionFieldInput != null
                            ? new TransitionInput(transition.getId(), Arrays.asList(resolutionFieldInput), comment)
                            : new TransitionInput(transition.getId(), Collections.<FieldInput>emptyList(), comment));
                    if (logger.isLoggable(Level.FINER)) {
                        qMsg = "updateIssue: transition";
                        logStart(qMsg, timer);
                    }
                    restClientManager.getExtendedIssueClient().transition(issue, transitionInput).claim();
                    if (logger.isLoggable(Level.FINER)) {
                        logStop(qMsg, timer);
                    }
                } else if (comment != null) {
                    if (logger.isLoggable(Level.FINER)) {
                        qMsg = "updateIssue: add comment";
                        logStart(qMsg, timer);
                    }
                    restClientManager.getExtendedIssueClient().addComment(issue.getCommentsUri(), comment).claim();
                    if (logger.isLoggable(Level.FINER)) {
                        logStop(qMsg, timer);
                    }
                }
                // Update the other fields on the issue        
                if (logger.isLoggable(Level.FINER)) {
                    qMsg = "updateIssue: update";
                    logStart(qMsg, timer);
                }
                restClientManager.getExtendedIssueClient().updateIssue(issue.getKey(), issueInput).claim();
                if (logger.isLoggable(Level.FINER)) {
                    logStop(qMsg, timer);
                }

                // Get the updated issue        
                if (logger.isLoggable(Level.FINER)) {
                    qMsg = "updateIssue: re-retrieve issue";
                    logStart(qMsg, timer);
                }
                issue = restClientManager.getExtendedIssueClient().getIssue(issue.getKey()).claim();
                if (logger.isLoggable(Level.FINER)) {
                    logStop(qMsg, timer);
                }

            } catch (RestClientException e) {
                logger.log(Level.SEVERE, e.toString(), e);
                throw new RequestException(new ErrorResponse(
                        "Error occurred while updating defect: " + e.toString(), "0"));
            }
        }
        return issue;
    }

    /**
     * Transitions the issue with the target status and resolution.
     */
    private Issue updateIssueStatus(Issue issue, Map<String, String[]> defectFields) throws RequestException {
        if (defectFields != null) {
            Transition transition = null;
            FieldInput resolutionFieldInput = null;
            Comment comment = null;
            TimeCommand timer = new TimeCommand();
            String qMsg = ".";
            // Get the status and resolution
            String[] status = defectFields.remove(Constants.ISSUE_FIELD_STATUS);
            if (!Utils.isEmpty(status)) {
                transition = issueFieldsMapper.getTransitionForTargetStatus(issue, status[0]);
                if (transition != null) {
                    String[] resolution = defectFields.remove(Constants.ISSUE_FIELD_RESOLUTION);
                    if (!Utils.isEmpty(resolution)) {
                        resolutionFieldInput = new FieldInput(Constants.ISSUE_FIELDS.get(
                                Constants.ISSUE_FIELD_RESOLUTION),
                                ComplexIssueInputFieldValue.with("name", resolution[0]));
                    }
                }
            }
            // Get the fix field as a comment
            String[] fix = defectFields.remove(Constants.DTG_FIELD_FIX);
            if (!Utils.isEmpty(fix)) {
                comment = Comment.valueOf(fix[0]);
            }
            try {
                // Perform transition on the issue
                if (transition != null) {
                    TransitionInput transitionInput = (resolutionFieldInput != null
                            ? new TransitionInput(transition.getId(), Arrays.asList(resolutionFieldInput), comment)
                            : new TransitionInput(transition.getId(), Collections.<FieldInput>emptyList(), comment));
                    if (logger.isLoggable(Level.FINER)) {
                        qMsg = "updateIssueStatus: transition issue";
                        logStart(qMsg, timer);
                    }
                    restClientManager.getExtendedIssueClient().transition(issue, transitionInput).claim();
                    if (logger.isLoggable(Level.FINER)) {
                        logStop(qMsg, timer);
                    }

                }
                // Get the updated issue        
                if (logger.isLoggable(Level.FINER)) {
                    qMsg = "updateIssueStatus: re-retrieve issue";
                    logStart(qMsg, timer);
                }
                issue = restClientManager.getExtendedIssueClient().getIssue(issue.getKey()).claim();
                if (logger.isLoggable(Level.FINER)) {
                    logStop(qMsg, timer);
                }

            } catch (RestClientException e) {
                logger.log(Level.SEVERE, e.toString(), e);
                throw new RequestException(new ErrorResponse(
                        "Error occurred while updating defect status: " + e.toString(), "0"));
            }
        }
        return issue;
    }

    /**
     * Check if the field is a select field.
     */
    private boolean isSelectInputField(String fieldId) {
        if (fieldId != null) {
            // JIRA system select fields
            if (fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_AFFECTSVERSIONS))
                    || fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_COMPONENTS))
                    || fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_FIXVERSIONS))
                    || fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_ISSUETYPE))
                    || fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_PRIORITY))
                    || fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_RESOLUTION))
                    || fieldId.equals(Constants.ISSUE_FIELDS.get(Constants.ISSUE_FIELD_STATUS))) {

                return true;
            }
            // Handle custom select type fields
            String type = this.issueFieldsMapper.getCustomFieldTypeById(fieldId);
            if (type != null) {
                if (type.equalsIgnoreCase(IResponse.TYPE_SELECT)) {
                    return true;
                }
            }
        }
        return false;
    }

    private static void logStart(String qMsg, TimeCommand timer) {
        logger.log(logger.getLevel(), "START {0}", qMsg);
        timer.start();
    }

    private static void logStop(String qMsg, TimeCommand timer) {
        logger.log(logger.getLevel(), "DONE  {0} {1} ", new String[]{qMsg, timer.toString()});
    }

    private static void logStop(String qMsg, TimeCommand timer, String other) {
        if (other == null) {
            other = "";
        }
        logger.log(Level.INFO, "DONE  {0} {1} {2}", new String[]{qMsg, timer.toString(), other});
    }
    /**
     * cache for Projects: avoid repeated calls to getProject(). Assumes JIRA
     * Projects are not deleted -- if you do, restart p4dtg mapping. As of aug
     * 2018, we only check for existence of the project and don't care if it
     * changes.
     */
    private static final Map<String, Project> PROJECT_CACHE = new HashMap<>();

    /**
     * Check for project existence.
     *
     * @param projId
     * @return Project if found, null if not found.
     */
    private Project getProjectCached(String projId) {
        synchronized (PROJECT_CACHE) {
            Project proj = PROJECT_CACHE.get(projId);
            if (proj == null) {
                TimeCommand timer = new TimeCommand();

                String qMsg = "getProjectCached:  getProject " + projId;
                if (logger.isLoggable(Level.FINER)) {
                    logStart(qMsg, timer);
                }
                proj = restClientManager.getProjectClient().getProject(projId).claim();
                if (logger.isLoggable(Level.FINER)) {
                    logStop(qMsg, timer);
                }
                if (proj != null) {
                    PROJECT_CACHE.put(projId, proj);
                }
            }
            return proj;
        }
    }

    /**
     * *
     * cache for Users: avoid repeated calls to getUser(). Assumes p4dtg USER
     * does not change without restarting mapping.
     */
    private static final Map<String, User> USER_CACHE = new HashMap<>();

    /**
     * check if the user exists in Jira
     *
     * @param userName
     * @return true if userName exists, otherwise false.
     */
    public boolean isUserExists(String userName) {
        User u = getUserCached(userName);
        return u != null;
    }

    private User getUserCached(String userName) {
        User user = USER_CACHE.get(userName);
        if (null == null) {
            TimeCommand timer = new TimeCommand();

            String qMsg = "getUserCached:  user " + userName;
            if (logger.isLoggable(Level.FINER)) {
                logStart(qMsg, timer);
            }
            user = restClientManager.getUserClient().getUser(userName).claim();

            if (logger.isLoggable(Level.FINER)) {
                logStop(qMsg, timer);
            }
            if (user != null) {
                USER_CACHE.put(userName, user);
            }
        }
        return user;
    }
    /**
     * *
     * cache for All Projects: avoid repeated calls to getAllProjects().
     */
    private static final List<String> ALL_PROJECT_KEYS = new ArrayList<>();
    private static int GET_PROJECT_COUNT = 0;
    private static final Integer LOCK_GET_PROJECT_COUNT = 0;

    /**
     * get List of all project keys.
     *
     * @return List of all project keys known to the user.
     */
    public List<String> getAllProjectsCached() {
        synchronized (LOCK_GET_PROJECT_COUNT) {
            GET_PROJECT_COUNT += 1;
            if (GET_PROJECT_COUNT >= 30) {
                // periodically refresh the cache.  30 is arbitrary.
                GET_PROJECT_COUNT = 0;
                ALL_PROJECT_KEYS.clear();
            }
            if (ALL_PROJECT_KEYS.size() < 1) {
                String qMsg = ".";
                TimeCommand timer = new TimeCommand();
                if (logger.isLoggable(Level.FINER)) {
                    qMsg = "getAllProjectsCached: get all projects";
                    logStart(qMsg, timer);
                }
                Iterable<BasicProject> projects = restClientManager.getProjectClient().getAllProjects().claim();
                if (logger.isLoggable(Level.FINER)) {
                    logStop(qMsg, timer);
                }
                if (projects != null) {
                    for (BasicProject project : projects) {
                        ALL_PROJECT_KEYS.add(project.getKey());
                    }
                }
                if (ALL_PROJECT_KEYS.size() < 1) {
                    throw new RestClientException("No projects found:  check jira permissions for jira user.", null);
                }
            }
        }
        return ALL_PROJECT_KEYS;
    }

    private int getCountAllProjects() {
        return getAllProjectsCached().size();
    }
    /**
     * cache for Projects: avoid repeated calls to getProject(). Assumes JIRA
     * Projects are not deleted -- if you do, restart p4dtg mapping. As of aug
     * 2018, we only check for existence of the project and don't care if it
     * changes.
     */
    private static final Map<String, Integer> PROJECT_ACCESS_CACHE = new HashMap<>();

    private String projMessage = ".";
    private final TimeCommand projTimer = new TimeCommand();
    private static final int BAD_RESULT_CODE = 201;

    /**
     * check that the jiraUser has access to issues in the project. Note that an
     * admin user can successfully call getProjectClient().getProject(project)
     * but a search jql with "project = NoAccess" could throw an exception
     * with error code 400 if user can't access the issues. Will also throw a
     * 400 for a non existent project.
     *
     * @param projId
     * @return true if query for project doesn't return an exception.
     */
    protected synchronized boolean hasProjectAccess(String projId) {

        Integer accessCode = PROJECT_ACCESS_CACHE.get(projId);

        if (accessCode != null) {
            return accessCode < BAD_RESULT_CODE;
        }
        try {
            // proj = restClientManager.getProjectClient().getProject(project).claim();  
            if (logger.isLoggable(Level.FINER)) {
                projMessage = "hasProjectAccess for " + projId;
                logStart(projMessage, projTimer);
            }
            SearchResult result = restClientManager.getSearchClient().searchJql(
                    "project = \"" + projId + "\" and updated < '2006/1/1'", 1, 0, null).claim();
            accessCode = 0;
			
        } catch (RestClientException ex) {
			logger.log(Level.INFO,"RestClientException in hasProjectAccess(\"" + projId + "\"): " + ex.getMessage(), ex);
            accessCode = Utils.getErrorStatus(ex);
            logger.log(Level.WARNING, "User {0} denied access to project {1} (code {2}):  not "
                    + "replicating.  Check that your P4DTG user can access issues in this project",
                    new String[]{this.jiraUsername, projId, accessCode.toString()});
        } finally {
            if (logger.isLoggable(Level.FINER)) {
                logStop(projMessage, projTimer);
            }
        }
        PROJECT_ACCESS_CACHE.put(projId, accessCode);
        return accessCode < BAD_RESULT_CODE;
    }

}
