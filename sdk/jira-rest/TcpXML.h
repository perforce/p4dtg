/*
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

#ifndef TCPXML_HEADER
#define TCPXML_HEADER
/*
 * TcpXML.h is communication class which is used to pass XML
 * requests over a TCP connection and return the results
 */

struct DTGField;
class TiXmlNode;

class TcpXML {
	protected:
	    int sfd; 		// socket file descriptor

	    void process_strings( TiXmlNode *n );
	    void process_fields( TiXmlNode *n );
	    void process_descs( TiXmlNode *n );
	    void process_error( TiXmlNode *n );
	    void parse( const char *xml );
	    void clear();

	public:
	    struct DTGStrList *strings;
	    struct DTGField *fields;
	    struct DTGFieldDesc *descs;
	    struct DTGError *error;

	public:
	    TcpXML();
	    virtual ~TcpXML();

	    int open( const char *server, const char *url, const char *user, const char *pass );
	    int close_connection();
	    int ping();

	    int send( const char *req, struct DTGField *args, int elements=0 );

	    int opened() { return (sfd >= 0) ? 1 : 0; };
};

#endif
