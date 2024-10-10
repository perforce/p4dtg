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
package com.perforce.p4dtg.plugin.jira.tcp.server;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.Charset;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.xml.sax.SAXException;

import com.perforce.p4dtg.plugin.jira.common.Constants;
import com.perforce.p4dtg.plugin.jira.common.Utils;
import com.perforce.p4dtg.plugin.jira.tcp.internal.request.RequestException;
import com.perforce.p4dtg.plugin.jira.tcp.internal.request.RequestHandler;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.DescriptionResponse;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.ErrorResponse;
import com.perforce.p4dtg.plugin.jira.tcp.internal.response.FieldResponse;
import com.perforce.p4dtg.plugin.jira.tcp.request.IRequestHandler;
import com.perforce.p4dtg.plugin.jira.tcp.response.IResponse;

/**
 * TCP socket server processes XML requests from DTG and relays calls to the
 * JIRA server via the JIRA REST API Java client.
 */
public class TcpSocketServer {

    private static final Logger logger = Logger.getLogger(TcpSocketServer.class.getPackage().getName());
    private static final boolean DUMP_TRAFFIC = new Boolean(System.getProperty(Constants.DUMP_TRAFFIC_PROPERTY));
    private final Charset charset = Charset.forName("UTF-8");
    private static int socketTimeout = Constants.SOCKET_TIMEOUT;
    // Retrieve socket timeout value from Java system properties
    static {
        String socketTimeoutValue = System.getProperty(Constants.SOCKET_TIMEOUT_PROPERTY);
        if (!Utils.isEmpty(socketTimeoutValue)) {
            try {
                int timeout = Integer.parseInt(socketTimeoutValue);
                if (timeout > 0) {
                    socketTimeout = timeout;
                }
            } catch (NumberFormatException e) {
                logger.log(Level.WARNING, e.getMessage());
            }
        }
    }

    /**
     * Request types corresponding to root XML element name.
     */
    public static enum Request {
        SHUTDOWN,
        CONNECT,
        LOGIN,
        PING,
        LIST_PROJECTS,
        GET_PROJECT,
        GET_SERVER_VERSION,
        GET_SERVER_DATE,
        LIST_FIELDS,
        LIST_DEFECTS,
        CREATE_DEFECT,
        NEW_DEFECT,
        SEGMENT_FILTERS,
        REFERENCED_FIELDS,
        SAVE_DEFECT,
        GET_DEFECT
    }

    private DocumentBuilderFactory factory;
    private IRequestHandler handler;

    private boolean shutdown = false;

    /**
     * Constructor to create a new TCP XML socket server with a request handler.
     *
     * @param handler
     *            the handler
     */
    public TcpSocketServer(IRequestHandler handler) {
        this.factory = DocumentBuilderFactory.newInstance();
        this.handler = handler;
    }

    /**
     * Creates the XML document parser.
     *
     * @return the document builder
     */
    private DocumentBuilder createParser() {
        DocumentBuilder parser = null;
        try {
            parser = this.factory.newDocumentBuilder();
        } catch (ParserConfigurationException e) {
            logger.log(Level.SEVERE, "Error creating new document parser.", e);
        }
        return parser;
    }

    /**
     * Gets the request.
     *
     * @param stream
     *            the stream
     * @return the request
     */
    private Document getRequest(InputStream stream) {
        DataInputStream is = new DataInputStream(stream);
        Document request = null;
        DocumentBuilder parser = createParser();
        if (parser != null) {
            try {
                StringBuilder readLength = new StringBuilder();
                int read = stream.read();
                while (read != '<' && read != -1) {
                    readLength.append((char) read);
                    read = stream.read();
                }
                if (readLength.length() > 0) {
                    int expectedLength = Integer.parseInt(readLength.toString());
                    int length = expectedLength;
                    byte[] byteRequest = new byte[length];
                    byteRequest[0] = '<';
                    length--;
                    is.readFully(byteRequest, 1, length);
                    int totalRead = byteRequest.length;
                    if (totalRead != expectedLength) {
                        logger.severe("Expected message of size: " + expectedLength
                        		+ " but received: " + totalRead);
                    }
                    if (DUMP_TRAFFIC) {
                    	String reqTag = new String(byteRequest);
                    	// Don't dump the password out to the debug log.
                    	if (reqTag.startsWith("<LOGIN JIRA_URL=")) {
                    		reqTag = reqTag.replaceAll("JIRA_PASSWORD=\"(.*?)\" />",
                    				"JIRA_PASSWORD=\"*****\" />");
                    	}
                        logger.info("Request: " + reqTag);
                    }
                    ByteArrayInputStream byteStream = new ByteArrayInputStream(byteRequest);
                    request = parser.parse(byteStream);
                }
            } catch (SAXException e) {
                logger.log(Level.SEVERE, "XML parser exception reading request.", e);
            } catch (IOException e) {
                logger.log(Level.SEVERE, "I/O exception reading from stream.", e);
            } catch (NumberFormatException e) {
                logger.log(Level.SEVERE, "Number format exception parsing message length.", e);
            }
        }
        return request;
    }

    /**
     * Wrap response array.
     *
     * @param responses
     *            the responses
     * @param outerElmement
     *            the outer elmement
     * @return the i response
     */
    private IResponse wrapResponseArray(IResponse[] responses, String outerElmement) {
        IResponse response = null;
        if (responses != null && outerElmement != null) {
            final StringBuilder xml = new StringBuilder();
            xml.append('<');
            xml.append(outerElmement);
            xml.append('>');
            for (IResponse inner : responses) {
                if (inner != null) {
                    xml.append(inner.toString());
                }
            }
            xml.append("</");
            xml.append(outerElmement);
            xml.append('>');

            response = new IResponse() {
                public String toString() {
                    return xml.toString();
                }
            };
        }
        return response;
    }

    /**
     * Wrap field responses.
     *
     * @param responses
     *            the responses
     * @return the i response
     */
    private IResponse wrapFieldResponses(FieldResponse[] responses) {
        return wrapResponseArray(responses, IResponse.FIELDS);
    }

    /**
     * Wrap description responses.
     *
     * @param responses
     *            the responses
     * @return the i response
     */
    private IResponse wrapDescriptionResponses(DescriptionResponse[] responses) {
        return wrapResponseArray(responses, IResponse.DESCS);
    }

    /**
     * Gets request type.
     *
     * @param request
     *            the request
     * @return the request type
     */
    private Request getRequestType(Document request) {
        Request requestType = null;
        Element root = request.getDocumentElement();
        if (root != null) {
            String rootTag = root.getTagName();
            requestType = Request.valueOf(rootTag);
        }
        return requestType;
    }

    /**
     * Gets the response.
     *
     * @param request
     *            the request
     * @return the response
     */
    private String getResponse(Document request) {
        IResponse response = null;
        Element root = request.getDocumentElement();
        if (root != null) {
            String rootTag = root.getTagName();
            try {
                Request requestType = Request.valueOf(rootTag);
                try {
                    switch (requestType) {
                    case SHUTDOWN:
                        response = handler.shutdown(root);
                        break;
                    case CONNECT:
                        response = handler.connect(root);
                        break;
                    case LOGIN:
                        response = handler.login(root);
                        break;
                    case PING:
                        response = handler.ping(root);
                        break;
                    case LIST_PROJECTS:
                        response = handler.listProjects(root);
                        break;
                    case GET_PROJECT:
                        response = handler.getProject(root);
                        break;
                    case GET_SERVER_VERSION:
                        response = handler.getServerVersion(root);
                        break;
                    case GET_SERVER_DATE:
                        response = handler.getServerDate(root);
                        break;
                    case LIST_FIELDS:
                        response = wrapDescriptionResponses(handler.listFields(root));
                        break;
                    case LIST_DEFECTS:
                        response = handler.listDefects(root);
                        break;
                    case CREATE_DEFECT:
                        response = handler.createDefect(root);
                        break;
                    case NEW_DEFECT:
                        response = wrapFieldResponses(handler.newDefect(root));
                        break;
                    case SEGMENT_FILTERS:
                        response = handler.getSegmentFilters(root);
                    case REFERENCED_FIELDS:
                        response = handler.getReferencedFields(root);
                        break;
                    case SAVE_DEFECT:
                        response = handler.saveDefect(root);
                        break;
                    case GET_DEFECT:
                        response = wrapFieldResponses(handler.getDefect(root));
                        break;
                    default:
                        response = new ErrorResponse("Unhandled element name in request: " + rootTag, "0");
                        break;
                    }
                } catch (RequestException e) {
                    response = e.getResponse();
                } catch (Throwable e) {
                	response = new ErrorResponse(
                			"Error occurred while processing request: " + e.getLocalizedMessage(), "0");
                }
            } catch (IllegalArgumentException e) {
                response = new ErrorResponse("Unknown element name in request: " + rootTag, "0");
            }
        }
        return response != null ? response.toString() : null;
    }

    /**
     * Start the server listening.
     *
     * @param port
     *            the port
     * @throws Exception
     *             the exception
     */
    public void start(int port) throws Exception {
        start(null, port);
    }

    /**
     * Handle the socket input and output streams.
     *
     * @param socket
     *            the socket
     * @throws Exception
     *             the exception
     */
    private void handle(Socket socket) throws Exception {
        InputStream incoming = null;
        OutputStream outgoing = null;
        try {
            incoming = socket.getInputStream();
            Document request = getRequest(incoming);
            while (request != null) {
                String response = getResponse(request);
                if (response == null) {
                    break;
                }
                outgoing = socket.getOutputStream();
                byte[] byteResponse = response.getBytes(charset);
                outgoing.write(Integer.toString(byteResponse.length).getBytes(charset));
                outgoing.write(byteResponse);
                outgoing.flush();
                if (DUMP_TRAFFIC) {
                    logger.info("Response length: " + byteResponse.length);
                    logger.info("Response: " + response);
                }
                Request requestType = getRequestType(request);
                if (requestType == Request.SHUTDOWN) {
                    shutdown = true;
                    break;
                }
                request = getRequest(incoming);
	            if (request == null) {
	                logger.severe("Unable to parse request.");
	                ErrorResponse er = new ErrorResponse("Unable to parse the request.", "0");
	                byteResponse = er.toString().getBytes(charset);
	                outgoing.write(Integer.toString(byteResponse.length).getBytes(charset));
	                outgoing.write(byteResponse);
	                outgoing.flush();
	            }
            }
        } catch (Throwable e) {
            logger.log(Level.SEVERE, "Problem occurred while handling request.", e);
            throw new Exception(e);
        } finally {
            if (outgoing != null) {
                try {
                    outgoing.flush();
                    outgoing.close();
                } catch (IOException ignore) {
                	// Ignore
                }
            }
            if (incoming != null) {
                try {
                    incoming.close();
                } catch (IOException ignore) {
                	// Ignore
                }
            }
        }
    }

    /**
     * Start the server listening.
     *
     * @param address
     *            the address
     * @param port
     *            the port
     * @throws Exception
     *             the exception
     */
    public void start(InetAddress address, int port) throws Exception {
    	ServerSocket socket = null;
        try {
            socket = new ServerSocket(port, 0, address);
            // Set socket timeout
            socket.setSoTimeout(socketTimeout);
            logger.log(Level.INFO, "Socket timeout is set to (milliseconds): "
                    + socketTimeout);
            Socket incoming = socket.accept();
            while (incoming != null) {
                try {
                    if (DUMP_TRAFFIC) {
                        logger.info("Starting connection.");
                    }
                    // Handle incoming connection
                    handle(incoming);
                    // Handle shutdown
                    if (shutdown) {
                        if (DUMP_TRAFFIC) {
                            logger.info("Shutdown request has been called.");
                        }
                        break;
                    }
                } catch (Throwable e) {
                    logger.log(Level.SEVERE, "Problem handling incoming connection.", e);
                    // Since it's a localhost one-to-one connection between DTG
                    // and the JIRA plugin we need to shutdown if we encounters
                    // a socket connection problem.
                    break;
                } finally {
                    if (DUMP_TRAFFIC) {
                        logger.info("Closing connection.");
                    }
                    incoming.close();
                }
                incoming = socket.accept();
            }
        } catch (Throwable e) {
            logger.log(Level.SEVERE, "Problem occurred while handling socket connection.", e);
            throw new Exception(e);
        } finally {
            if (socket != null) {
	            try {
	                socket.close();
	            } catch (IOException ignore) {
	            	// Ignore
	            }
            }
        }
    }

    /**
     * Create and start a TCP socket server with the properties created by the
     * DTG JIRA plugin interface.
     *
     * @param args
     *            the arguments
     */
    public static void main(String args[]) {
        if (args.length < 1) {
            logger.severe("Must enter the properties file as a parameter.");
            System.exit(1);
        }
        File propFile = null;
        FileInputStream fis = null;
        try {
            propFile = new File(args[0]);
            if (propFile == null || !propFile.exists()) {
                logger.severe("Properties file: " + args[0] + " does not exist.");
                System.exit(1);
            }
            // Delete properties file on JVM exit
            // Note, only works for normal termination of the JVM
            // It doesn't work with System.exit(int status) method call
            //propFile.deleteOnExit();
            fis = new FileInputStream(propFile);
            if (fis != null) {
                // Read properties from file.
                Properties properties = new Properties();
                properties.load(fis);
                String tcpPort = properties.getProperty("tcp_port");
                String configFile = properties.getProperty("config_file");
                String defectBatch = properties.getProperty("defect_batch");
                // This handler will be initialized during the "LOGIN" request.
                RequestHandler handler = new RequestHandler();
                handler.setConfigFile(configFile);
                handler.setDefectBatch(defectBatch);
                TcpSocketServer dts = new TcpSocketServer(handler);
                // Start the TCP socket server
                dts.start(Integer.parseInt(tcpPort));
            }
        } catch (Exception e) {
            logger.log(Level.SEVERE, "Exception occurred.", e);
        } catch (Error e) {
            logger.log(Level.SEVERE, "Error occurred.", e);
        } catch (Throwable t) {
            logger.log(Level.SEVERE, "Problem occurred.", t);
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (Throwable ignore) {
                	// Ignore
                }
            }
            if (propFile != null && propFile.exists()) {
            	try {
            		propFile.delete();
                } catch (Throwable ignore) {
                	// Ignore
            	}
            }
        }
        // Safe exit
        System.exit(0);
    }
}
