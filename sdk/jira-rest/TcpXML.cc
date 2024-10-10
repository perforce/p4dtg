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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern "C" {
#include "dtg-utils.h"
}
#include <dtg-str.h>

#include "tinyxml.h"
#include "tinystr.h"
#include "TcpXML.h"

#ifndef OS_NT
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <io.h>
#endif

int open_socket( const char *host, int port )
{
# ifdef OS_NT
        int err;
        WSADATA data;
	WORD version = MAKEWORD( 1, 1 );

	if( err = WSAStartup( version, &data ) )
	{
	    fprintf( stderr, "WSAStartup failure\n" );
	    return -1;
	}
# endif
	struct hostent *h = gethostbyname( host );
	if( !h )
	    return -1;

	int fd = socket( AF_INET, SOCK_STREAM, 0 );
	if( fd < 0 )
	    return -1;

	struct sockaddr_in target;
        target.sin_family = AF_INET;
        target.sin_port = htons(port);
        target.sin_addr = *((struct in_addr *)h->h_addr);
#ifdef OS_NT
	memset( &(target.sin_zero), '\0', 8);
#else
        bzero(&(target.sin_zero), 8);
#endif

	if( connect( fd, (struct sockaddr *)&target,
			sizeof(struct sockaddr) ) == -1 )
	    return -1;
	return fd;
}

/* Message format: LENGTH_OF_MESSAGE_IN_BYTESmessage */

int send_string( int fd, const char *str )
{
	size_t len = strlen( str );
	char num[32]; 	   	
	sprintf( num, "%ld", strlen( str ) );
	int i = send( fd, num, strlen( num ), 0 );
	i = send( fd, str, len, 0 );
	return i;
}

int recv_string( int fd, char *&str )
{
	char buffer[8096+1];
	long end = 8096;
	int done = 0;

	long len = end;
	str = new char[len+1];
	long cnt;

	// clean up the buffer
	memset(buffer, 0, end);

	if( (cnt = recv( fd, buffer, end, 0 )) == -1 )
	    return 0;
	size_t msg = atol( buffer );
	char num[32];
	sprintf( num, "%ld", msg );

	msg = msg - cnt + strlen( num );
	int i;
	long j = 0;
	for( i = strlen( num ); i < cnt; i++,j++ )
	    str[j] = buffer[i];
	str[j] = '\0';

	while( msg > 0 )
	{
	    // clean up the buffer
	    memset(buffer, 0, end);

	    if( (cnt = recv( fd, buffer, end, 0 )) == -1 )
	        return 0;
	    if( (j + cnt) >= len )
	    {
	        // expand input buffer
	        len += end;
	        char *tmp = new char[len + 1];
	        tmp[0] = '\0';
	        strcat( tmp, str );
	        delete[] str;
	        str = tmp;
	    }
	    for( int i = 0; i < cnt; i++,j++ )
	        str[j] = buffer[i];
	    str[j] = '\0';
	    msg -= cnt;
	}
	return 1;
}

/***

	Instead of the defined response, the following error can be sent
		<ERROR CONTINUE="(0|1)" MESSAGE="messages" />

	Requests/Responses:
	-------------------
	connect() -> return randomString in STRINGS
		<CONNECT />

		<STRINGS> <STRING value="randomString" />

	shutdown() -> no return value
		<SHUTDOWN />

		<STRINGS> <STRING value="CLOSING" />

	login() -> return dID in STRINGS
		<LOGIN JIRA_URL="url" JIRA_USER="user" JIRA_PASSWORD="pass" />

		<STRINGS> <STRING value="dID" /> </STRINGS>

	get_server_version( dID ) -> return version in STRINGS
		<GET_SERVER_VERSION DID="dID" />

		<STRINGS> <STRING value="serverVersion" /> </STRINGS>

	get_server_date( dID ) -> return date string in STRINGS
		<GET_SERVER_DATE DID="dID" />

		<STRINGS> <STRING value="serverDate" /> </STRINGS>

	list_projects( dID ) -> return set of projects in STRINGS
		<LIST_PROJECTS DID="dID" />

		<STRINGS> <STRING value="dID" /> </STRINGS>

	get_project( dID, project ) -> return projID in STRINGS
		<GET_PROJECT DID="dID" PROJECT="projName" />

		<STRINGS> <STRING value="projID" /> </STRINGS>

	list_fields( projID ) -> return set of DESC
		<LIST_FIELDS PROJID="projID" />

		<DESCS> .... (see below) .... </DESCS>

	referenced_fields( projID, fields... ) -> return "OK" in STRINGS
		<REFERENCED_FIELDS PROJID="projID"
			1="field1"
			2="field2"
			...
		/>

		<STRINGS> <STRING value="OK" /> </STRINGS>

	list_defects( projID, moddate, date, modby, user, max ) ->
						return defects in STRINGS
		<LIST_DEFECTS PROJID="projID"
			MODDATE="fieldName"
			DATE="YYYY/MM/DD/HH/MM/SS"
			MODBY="fieldName"
			USER="userName"
			MAX="n entries, 0 = all"
		/>

		<STRING>
			<STRING value="defect1"
			<STRING value="defect2"
			...
		/>

	new_defect( projID ) -> set of FIELD
		<NEW_DEFECT PROJID="projID" />

		<FIELDS> .... (see below) .... </FIELDS>

	get_defect( projID, defect ) -> set of FIELD
		<GET_DEFECT PROJID="projID" DEFECT="defectName" />

		<FIELDS> .... (see below) .... </FIELDS>

	create_defect( defectID, fields ) -> return defect name in STRINGS
		<CREATE_DEFECT />
			<FIELD NAME="field1" VALUE="value1" />
			<FIELD NAME="field2" VALUE="value2" />
			...
		<CREATE_DEFECT />

		<STRINGS> <STRING value="defectName" /> </STRINGS>

	save_defect( defectID, fields ) -> return defect name in STRINGS
		<SAVE_DEFECT />
			<FIELD NAME="DEFECTID" VALUE="defectID" />
			<FIELD NAME="field1" VALUE="value1" />
			<FIELD NAME="field2" VALUE="value2" />
			...
		/>

		<STRINGS> <STRING value="defectName" /> </STRINGS>


Responses:
	<STRINGS>
		<STRING VALUE="val" />
		...
	</STRINGS>

	<FIELDS>
		<FIELD NAME="fieldN" VALUE="val1" />
		...
	</FIELDS>

	<DESCS>
		<DESC NAME="fieldN"
			TYPE="(word|line|text|date)"
			ACCESS="[0-4]"/>
		...
		<DESC NAME="field"
			TYPE="select"
			ACCESS="[0-4]">
		    <STRINGS
			<STRING VALUE="opt1" />
			...
		    </STRINGS>
		</DESC>
		...

	<ERROR CONTINUE="0|1", MESSAGE="message1" />

***/

void TcpXML::clear()
{
	delete_DTGStrList( strings );
	strings = NULL;
	delete_DTGField( fields );
	fields = NULL;
	delete_DTGFieldDesc( descs );
	descs = NULL;
	clear_DTGError( error );
}

TcpXML::~TcpXML()
{
	if( sfd >= 0 )
	{
	    this->send( "SHUTDOWN", NULL );
	    close_connection();
	}
	clear();
	delete_DTGError( error );
	error = NULL;
}

TcpXML::TcpXML()
{
	strings = NULL;
	fields = NULL;
	descs = NULL;
	error = new_DTGError( NULL );
	sfd = -1;
};

int TcpXML::open( const char *my_server, const char *dts_url, const char *dts_user, const char *dts_pass )
{
	if( !my_server || !*my_server || !dts_url || !*dts_url || !dts_user || !*dts_user || !dts_pass || !*dts_pass )
	    return 0;
	if( sfd >= 0 )
	    return 0;

	char *url = strdup(dts_url);
	char *user = strdup(dts_user);
	char *pass = strdup(dts_pass);
	char *server = strdup(my_server);
	char *my_port;
	my_port = strchr( server, ':' );
	if( my_port )
	{
	    *my_port = '\0';
	    my_port++; // skip over ':'
	}
	else
	{
	    free(server);
	    return 0;
	}

	sfd = open_socket( server, atoi( my_port ) );
	free(server);

	this->send( "CONNECT", NULL );
	// Check response
	if( !strings || !strings->value || !*strings->value )
	{
	    // capture the CONNECT error message
	    char *connect_error = mk_string(error->message);
	    // Send shutdown
		this->send( "SHUTDOWN", NULL );
	    close_connection();
	    // Set the CONNECT error message (ignore the SHUTDOWN message)
	    set_DTGError( error, connect_error );
	    delete[] connect_error;
	    if( !error->message )
	        set_DTGError( error,
			      "Invalid or no response to CONNECT request" );
	    return 0;
	}

    // DTS login info
    struct DTGField *args = NULL;
    args = append_DTGField( args, new_DTGField( "JIRA_URL", url ) );
    args = append_DTGField( args, new_DTGField( "JIRA_USER", user ) );
    args = append_DTGField( args, new_DTGField( "JIRA_PASSWORD", pass ) );
    
	this->send( "LOGIN", args );
    delete_DTGField( args );
    args = NULL;
	free(url);
	free(user);
	free(pass);
    
    // Check response
	if( !strings || !strings->value || !*strings->value )
	{
	    // capture the LOGIN error message
	    char *login_error = mk_string(error->message);
	    // Send shutdown
		this->send( "SHUTDOWN", NULL );
	    close_connection();
	    // Set the LOGIN error message (ignore the SHUTDOWN message)
	    set_DTGError( error, login_error );
	    delete[] login_error;
	    if( !error->message )
	        set_DTGError( error,
			      "Invalid or no response to LOGIN request" );
	    return 0;
	}
	return 1;
}

int TcpXML::close_connection()
{
	if( sfd < 0 )
	    return 0;

# ifdef OS_NT
	closesocket( sfd );
        WSACleanup();
#else
	close( sfd );
# endif

	sfd = -1;
	return 1;
}

int TcpXML::ping()
{
	if( sfd < 0 )
	    return 0;

	// Test connection
	this->send( "PING", NULL );

	// Check response
	if( !strings || !strings->value ||
		!*strings->value || strcmp( strings->value, "PONG" ) )
	    return 0;
	return 1;
}

int TcpXML::send( const char *req, struct DTGField *args, int elements )
{
	if( sfd < 0 )
	    return 0;

	TiXmlElement elem( req );
	if( elements )
	    for( struct DTGField *a = args; a; a = a->next )
	    {
	        TiXmlElement *f = new TiXmlElement( "Field" );
	        f->SetAttribute( "NAME", a->name );
	        f->SetAttribute( "VALUE", a->value ? a->value : "" );
	        elem.LinkEndChild( f );
	    }
	else // use attributes
	    for( struct DTGField *a = args; a; a = a->next )
	    {
	        elem.SetAttribute( a->name, a->value ? a->value : "" );
	    }
	
	TiXmlOutStream out;
	out << elem;
	const char *txt = out.c_str();


	// Send Request
#ifdef DEBUG
	fprintf( stderr, "REQ:%s\n", txt );
#endif
	send_string( sfd, txt );

	// Process Response
	clear();
	char *str = NULL;
	recv_string( sfd, str );
#ifdef DEBUG
	fprintf( stderr, "RECV:%s\n", str );
#endif
	parse( str );

	if( str )
	    delete[] str;
	str = NULL;	

	if( error->message )
	    return 0;
	else
	    return 1;
}

void TcpXML::process_strings( TiXmlNode *n )
{
	for( TiXmlNode *s = n->FirstChild(); s; s = s->NextSibling() )
	{
	    if( !(s->Type() == TiXmlNode::ELEMENT ) ||
		strcasecmp( s->Value(), "STRING" ) )
	        continue;
	    TiXmlElement *e = s->ToElement();
	    if( !e->Attribute( "VALUE" ) )
	        continue;
	    strings = append_DTGStrList( strings, e->Attribute( "VALUE" ) );
	}
}

void TcpXML::process_fields( TiXmlNode *n )
{
	for( TiXmlNode *s = n->FirstChild(); s; s = s->NextSibling() )
	{
	    if( !(s->Type() == TiXmlNode::ELEMENT ) ||
		strcasecmp( s->Value(), "FIELD" ) )
	        continue;
	    TiXmlElement *e = s->ToElement();
	    if( !e->Attribute( "VALUE" ) || !e->Attribute( "NAME" ) )
	        continue;
	    fields = append_DTGField( fields,
		new_DTGField( e->Attribute( "NAME" ), e->Attribute( "VALUE" )));
	}
}

void TcpXML::process_descs( TiXmlNode *n )
{
	for( TiXmlNode *s = n->FirstChild(); s; s = s->NextSibling() )
	{
	    if( !(s->Type() == TiXmlNode::ELEMENT ) ||
		strcasecmp( s->Value(), "DESC" ) )
	        continue;

	    TiXmlElement *e = s->ToElement();
	    if( !e->Attribute( "NAME" ) ||
		!e->Attribute( "TYPE" ) ||
		!e->Attribute( "ACCESS" ) )
	        continue;
	    if( s->FirstChild() &&
		s->FirstChild()->Type() == TiXmlNode::ELEMENT &&
		!strcasecmp( s->FirstChild()->Value(), "STRINGS" ) )
	    {
	        struct DTGStrList *tmp = strings;
	        strings = NULL;
	        process_strings( s->FirstChild() );
	        descs = append_DTGFieldDesc( descs,
				new_DTGFieldDesc( e->Attribute( "NAME" ),
					e->Attribute( "TYPE" ),
					*e->Attribute( "ACCESS" ) - '0',
					strings ) );
	        strings = tmp;
	    }
	    else
	        descs = append_DTGFieldDesc( descs,
				new_DTGFieldDesc( e->Attribute( "NAME" ),
					e->Attribute( "TYPE" ),
					*e->Attribute( "ACCESS" ) - '0',
					NULL ) );
	}
}

void TcpXML::process_error( TiXmlNode *n )
{
	TiXmlElement *e = n->ToElement();
	if( e->Attribute( "MESSAGE" ) && e->Attribute( "CONTINUE" ) )
	{
	    set_DTGError( error, e->Attribute( "MESSAGE" ) );
	    if( *(e->Attribute( "CONTINUE" )) == '1' )
	        error->can_continue = 1;
	    else
	        error->can_continue = 0;
	}
	else
	    set_DTGError( error, "Error parsing ERROR element" );
}

void TcpXML::parse( const char *xml )
{
	char *in = mk_string( xml );
	TiXmlDocument doc;
	const char *res = doc.Parse( in, 0, TIXML_ENCODING_UTF8 );
	if( in )
	    delete[] in;
	in = NULL;
	TiXmlOutStream out;
	out << doc;
	const char *txt = out.c_str();

	for( TiXmlNode *n = doc.FirstChild(); n; n = n->NextSibling() )
	{
	    if( !(n->Type() == TiXmlNode::ELEMENT ) )
	        continue;
	    const char *val = n->Value();
	    if( !strcasecmp( val, "STRINGS" ) )
	        process_strings( n );
	    else if( !strcasecmp( val, "FIELDS" ) )
	        process_fields( n );
	    else if( !strcasecmp( val, "DESCS" ) )
	        process_descs( n );
	    else if( !strcasecmp( val, "ERROR" ) )
	        process_error( n );
	}
}
