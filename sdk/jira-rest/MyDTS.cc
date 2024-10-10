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
#include <signal.h>

#ifndef OS_NT
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <ctype.h>
#include <sys/stat.h>
#include "MyDTS.h"
extern "C" {
#include "dtg-utils.h"
}
#include "dtg-str.h"
#include "TcpXML.h"

struct DTGField *get_field( struct DTGField *fields, const char *id )
{
	while( fields && strcmp( fields->name, id ) )
	    fields = fields->next;
	return fields;
}

const char *find_value( struct DTGField *fields, const char *id )
{
	while( fields && strcmp( fields->value, id ) )
	    fields = fields->next;
	return fields ? fields->name : id;
}

const char *find_field( struct DTGField *fields, const char *id )
{
	fields = get_field( fields, id );
	return fields ? fields->value : id;
}

MyDTS::MyDTS( const char *server,
		const char *user,
		const char *pass,
		const struct DTGField *attrs,
		const char *prog_name,
		const char *prog_ver,
		char *&err)
{
	valid = 0;
	dtID = NULL;
	projID = NULL;
	defectName = NULL;
	sent_ref_fields = 0;
	tcp = NULL;
	tcp_port = NULL;
	tcp_server = NULL;
	java_opts = NULL;
	defect_batch = 0;
	if( server )
	    my_server = cp_string( server );
	else
	    my_server = NULL;
	if( user )
	    my_user = cp_string( user );
	else
	    my_user = NULL;
	if( pass )
	    my_pass = cp_string( pass );
	else
	    my_pass = NULL;
	if( prog_name )
	    my_prog_name = cp_string( prog_name );
	else
	    my_prog_name = cp_string( "TCPDTS-Unnamed" );
	if( prog_ver )
	    my_prog_ver = cp_string( prog_ver );
	else
	    my_prog_ver = cp_string( "TCPDTS-Unversioned" );

	DTGField *f;

	// Configuration File for Workflow and Custom Fields
	f = get_field( (DTGField*)attrs, "config" );
	if( f )
	    my_config = cp_string( f->value );
	else
	    my_config = cp_string( "jira-rest-config.xml" );

	// wait time
	f = get_field( (DTGField*)attrs, "wait_time" );
	if( f )
	    wait_time = atoi( f->value );
	else
	    wait_time = 10;

	// Batch size for issues
	f = get_field( (DTGField*)attrs, "defect_batch" );
	if( f )
	    defect_batch = atoi( f->value );
	else
	    defect_batch = 100;

	// TCP port
	f = get_field( (DTGField*)attrs, "tcp_port" );
	if( f )
		tcp_port = cp_string( f->value );
	else
		tcp_port = cp_string( "51666" );

	// Java options
	f = get_field( (DTGField*)attrs, "java_opts" );
	if( f )
		java_opts = cp_string( f->value );
	else
		java_opts = cp_string( "-Xms128m -Xmx512m" );

	f = NULL;

	int port_num = atoi( tcp_port ) - 1;
	char port_string[32];

	char *jira_properties = NULL;
	struct stat buf;
	do {
	    port_num++;
	    sprintf( port_string, "%d", port_num );
	    delete[] jira_properties;
	    jira_properties = mk_string( "jira/jira-rest-", port_string, ".properties" );
	} while( !stat( jira_properties, &buf ) && port_num < 65535 );

	// writes Jira information to properties file

	FILE *file = fopen( jira_properties, "w+" );
	if( !file )
	{
	    err = mk_string( "Unable to create Java properties file" );
	    return;
	}

	delete[] tcp_port;
	tcp_port = cp_string( port_string );
	tcp_server = mk_string( "localhost:", port_string );

	fprintf( file, "tcp_port=%s\n", tcp_port );
	fprintf( file, "config_file=config/%s\n", my_config );
	fprintf( file, "defect_batch=%d\n", defect_batch );
	if( my_server )
	{
		fprintf( file, "server=%s\n", my_server);
	}
	fclose( file );

	char *java_options = mk_string( java_opts,
	                                " -Djava.awt.headless=true",
	                                " -Djava.util.logging.config.file=jira/logging-rest.properties",
	                                " -Djavadts.TCP_PORT=", port_string );
	
	char *java_command = mk_string( "java ", java_options, " -jar jira/jira-rest.jar" );
	
	if( java_options )
	    delete[] java_options;

	char *command = NULL;

#ifdef OS_NT
	command = mk_string( "cmd.exe /c START /B ", java_command, " ", jira_properties);
#else
	command = mk_string( java_command, " ", jira_properties, " &");
#endif

	if( java_command )
	    delete[] java_command;

	if( jira_properties )
	    delete[] jira_properties;

	// spawn Java TCP server process
	int status = system( command );

	if( command )
	    delete[] command;


	if( status == -1 )
	{
	    err = mk_string( "Unable to spawn Java proxy" );
	    return;
	}

	// wait 8 seconds
#ifdef OS_NT
	Sleep( 8000 );
#else
	sleep(8);
#endif

	// Create TCP Connection object
	tcp = new TcpXML();

	// Try connecting
	valid = 0;
	valid = connected( err );
	if( err )
	    return;
}

MyDTS::~MyDTS()
{
	if( dtID )
	    delete[] dtID;
	if( projID )
	    delete[] projID;
	if( defectName )
	    delete[] defectName;
	if( my_server )
	    delete[] my_server;
	if( my_user )
	    delete[] my_user;
	if( my_pass )
	    delete[] my_pass;
	delete[] my_prog_name;
	delete[] my_prog_ver;
	if( my_config )
	    delete[] my_config;
	if ( tcp_port )
	    delete[] tcp_port;
	if ( tcp_server )
	    delete[] tcp_server;
	if ( java_opts )
	   delete[] java_opts;

	// Close connections
	delete tcp;
}

void
MyDTS::connect_to_project( const char *proj, char *&err )
{
	if( !connected( err ) )
	    return;
	if( projID )
	{
	    delete[] projID;
	    projID = NULL;
	}
	sent_ref_fields = 0;

	// Connect to project
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "DTID", dtID ) );
	args = append_DTGField( args, new_DTGField( "PROJECT", proj ) );
	if( !tcp->send( "GET_PROJECT", args ) )
	{
	    err = mk_string( "Unable to connect to project: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return;
	}
	if( !tcp->strings || !tcp->strings->value || !*tcp->strings->value )
	{
	    if( tcp->error->message )
	        err = mk_string( tcp->error->message );
	    else
	        err = mk_string( "Connect to project request failed" );
	    delete_DTGField( args );
	    args = NULL;
	    return;
	}
	delete_DTGField( args );
	args = NULL;
	projID = mk_string( tcp->strings->value );
	return;
}

int
MyDTS::connected( char *&err)
{
	// Check to see if connection is still valid
	if( valid )
	{
	    valid = !tcp->ping();
	    if( valid )
 	        return valid;
 	}
	valid = 0;
	if( !tcp->opened() )
	{
	    if( !tcp->open( tcp_server, my_server, my_user, my_pass ) )
	        if( tcp->error->message )
	            err = mk_string( "Unable to connect to the JIRA plugin: ",
				tcp->error->message );
	        else
	            err = mk_string( "Unable to connect to the JIRA plugin. "
	                             "Please make sure the Java (JRE/JDK) is "
	                             "installed; the JIRA server is up and "
	                             "properly configured to accept REST API "
	                             "connections; the JIRA server URL, username "
	                             "and password are correct; the JIRA user "
	                             "has admin permissions; the JIRA plugin "
	                             "jira-rest-config.xml file is valid. Please "
	                             "also check the JIRA plugin log file for "
	                             "error details." );
	    else if( !tcp->strings || !tcp->strings->value ||
			!*tcp->strings->value )
	        err = mk_string( "Connection did not return dtID" );
	    else
	    {
	        dtID = mk_string( tcp->strings->value );
	        valid = 1;
	    }
	}
	else if( !tcp->ping() )
	{
	    if( dtID )
	    {
	        delete[] dtID;
	        dtID = NULL;
	    }
	}
	else
	    valid = 1;

	return valid;
}

int
MyDTS::server_offline( struct DTGError *error )
{
	char *err = NULL;
	int ret = 0; // assume success

	if( !connected( err ) )
	{
	    set_DTGError( error, err );
	    ret = wait_time;
	}

	if( err )
	    delete[] err;

	return ret;
}

char *MyDTS::get_server_version( char *&err )
{
	if( !connected( err ) )
	    return NULL;
	char *ver = NULL;

	// Retrieve version from server
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "DTID", dtID ) );
	if( !tcp->send( "GET_SERVER_VERSION", args ) )
	{
	    err = mk_string( "Unable to retrieve server version: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	delete_DTGField( args );
	args = NULL;
	if( !tcp->strings || !tcp->strings->value || !*tcp->strings->value )
	{
	    if( tcp->error->message )
	        err = mk_string( tcp->error->message );
	    else
	        err = mk_string( "Server version request failed" );
	    return NULL;
	}
	ver = mk_string( tcp->strings->value );
	return ver;
}

char *MyDTS::get_server_date( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char *date = NULL;

	// Retrieve date from server
	// Return a string which will be passed to extract_date to decode
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "DTID", dtID ) );
	if( !tcp->send( "GET_SERVER_DATE", args ) )
	{
	    err = mk_string( "Unable to retrieve server date: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	delete_DTGField( args );
	args = NULL;
	if( !tcp->strings || !tcp->strings->value || !*tcp->strings->value )
	{
	    if( tcp->error->message )
	        err = mk_string( tcp->error->message );
	    else
	        err = mk_string( "Server date request failed" );
	    return NULL;
	}
	date = mk_string( tcp->strings->value );
	return date;
}

struct DTGStrList *MyDTS::list_projects( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	// Retrieve list of projects from server
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "DTID", dtID ) );
	if( !tcp->send( "LIST_PROJECTS", args ) )
	{
	    err = mk_string( "Unable to retrieve list of projects: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	delete_DTGField( args );
	args = NULL;
	if( !tcp->strings || !tcp->strings->value || !*tcp->strings->value )
	{
	    if( tcp->error->message )
	        err = mk_string( tcp->error->message );
	    else
	        err = mk_string( "List projects request failed" );
	    return NULL;
	}

	struct DTGStrList *list = copy_DTGStrList( tcp->strings );
	return list;
}

struct DTGFieldDesc *MyDTS::get_field_desc( char *&err )
{
	if( !connected( err ) || !projID )
	    return NULL;

	// Retrieve list of fields from project
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	if( !tcp->send( "LIST_FIELDS", args ) )
	{
	    err = mk_string( "Unable to retrieve list of fields: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	if( !tcp->descs )
	{
	    if( tcp->error->message )
	        err = mk_string( tcp->error->message );
	    else
	        err = mk_string( "List fields request failed" );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	delete_DTGField( args );
	args = NULL;

	struct DTGFieldDesc *list = copy_DTGFieldDesc( tcp->descs );
	return list;
}

void
MyDTS::segment_filters( const char *projs, const char *filter, char *&err )
{
	if( !connected( err ) || !projID )
	    return;

	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	args = append_DTGField( args, new_DTGField( "SEGMENT_FILTER", filter ));
	args = append_DTGField( args, new_DTGField( "PROJECT_LIST", projs ) );
	if( !tcp->send( "SEGMENT_FILTERS", args ) )
	    err = mk_string( "Unable to set segment filters: ",
				tcp->error->message );
	delete_DTGField( args );
	args = NULL;
	return;
}

struct DTGStrList *
MyDTS::list_jobs( int max_rows,
		struct DTGDate *since, const char *mod_date_field,
		const char *exclude_user, const char *mod_by_field,
		char *&err )
{
	if( !connected( err ) )
	    return NULL;

	// Retrieve list of defects from server
	char date_string[32];
	sprintf( date_string, "%4d/%2d/%2d %2d:%2d:%2d",
		since->year, since->month, since->day,
		since->hour, since->minute, since->second );
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	args = append_DTGField( args, new_DTGField( "MODDATE", mod_date_field));
	args = append_DTGField( args, new_DTGField( "DATE", date_string ) );
	if( mod_by_field )
	    args = append_DTGField( args, new_DTGField( "MODBY", mod_by_field));
	else
	    args = append_DTGField( args, new_DTGField( "MODBY", "" ) );
	if( exclude_user )
	    args = append_DTGField( args, new_DTGField( "USER", exclude_user));
	else
	    args = append_DTGField( args, new_DTGField( "USER", "" ) );
	char tmp[32];
	snprintf( tmp, 30, "%d", max_rows );
	args = append_DTGField( args, new_DTGField( "MAX", tmp ) );

	if( !tcp->send( "LIST_DEFECTS", args ) )
	{
	    err = mk_string("Unable to process list defects request: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	delete_DTGField( args );
	args = NULL;

	DTGStrList *list = NULL;
	for( struct DTGStrList *s = tcp->strings; s; s = s->next )
	    list = append_DTGStrList( list, s->value );
	return list;
}

struct DTGField *MyDTS::get_defect( const char *defect,
				struct DTGStrList *ref_fields, char *&err )
{
	// Send ref_fields once per connection to a project
	// Setting of the ref_fields may not be done until right before this
	// method is called

	struct DTGField *args = NULL;
	if( ref_fields && !sent_ref_fields )
	{
	    args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	    int n = 0;
	    char num[32];
	    for( struct DTGStrList *i = ref_fields; i; n++, i = i->next )
	    {
	        sprintf( num, "F%d", n );
	        args = append_DTGField( args, new_DTGField( num, i->value ) );
	    }
	    if( !tcp->send( "REFERENCED_FIELDS", args ) )
	    {
	        err = mk_string("Unable to process referenced fields request: ",
				tcp->error->message );
	        delete_DTGField( args );
	        args = NULL;
	        return NULL;
	    }
	    delete_DTGField( args );
	    args = NULL;
	    if( !tcp->strings || !tcp->strings->value || !*tcp->strings->value
		|| strcasecmp( tcp->strings->value, "OK" ) )
	    {
	        if( tcp->error->message )
	            err = mk_string( tcp->error->message );
	        else
	            err = mk_string( "Referenced fields request failed" );
	        return NULL;
	    }
	    sent_ref_fields = 1;
	}

	// Retrieve specified defect and return the field values
	if( !strcasecmp( defect, "new" ) )
	{
	    args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	    if( !tcp->send( "NEW_DEFECT", args ) )
	    {
	        err = mk_string( "Unable to process new defect request: ",
				tcp->error->message );
	        delete_DTGField( args );
	        args = NULL;
	        return NULL;
	    }
	    delete_DTGField( args );
	    args = NULL;
	    if( !tcp->fields )
	    {
	        if( tcp->error->message )
	            err = mk_string( tcp->error->message );
	        else
	            err = mk_string( "New defect request failed" );
	        return NULL;
	    }
	    args = copy_DTGField( tcp->fields );
	}
	else
	{
	    args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	    args = append_DTGField( args, new_DTGField( "DEFECT", defect ) );
	    if( !tcp->send( "GET_DEFECT", args ) )
	    {
	        err = mk_string( "Unable to process get defect request: ",
				tcp->error->message );
	        delete_DTGField( args );
		args = NULL;
	        return NULL;
	    }
	    delete_DTGField( args );
	    args = NULL;
	    if( !tcp->fields )
	    {
	        if( tcp->error->message )
	            err = mk_string( tcp->error->message );
	        else
	            err = mk_string( "Get defect request failed" );
	        return NULL;
	    }
	    args = copy_DTGField( tcp->fields );
	}

	return args;
}

char *MyDTS::save_defect( const char *defect, struct DTGField *fields, char *&err )
{
	//  Save defect and return a copy of the resulting defect name
	char *new_name = NULL;
	struct DTGField *args = NULL;
	args = append_DTGField( args, new_DTGField( "PROJID", projID ) );
	const char *cmd;
	if( !strcasecmp( defect, "new" ) )
	    cmd = "CREATE_DEFECT";
	else
	{
	    cmd = "SAVE_DEFECT";
	    args = append_DTGField( args, new_DTGField( "DEFECTID", defect ));
	}
	for( struct DTGField *f = fields; f; f = f->next )
	    args = append_DTGField( args, new_DTGField( f->name, f->value ) );
	if( !tcp->send( cmd, args, 1 ) )
	{
	    err = mk_string( "Unable to process ", cmd, " request: ",
				tcp->error->message );
	    delete_DTGField( args );
	    args = NULL;
	    return NULL;
	}
	delete_DTGField( args );
	args = NULL;
	if( !tcp->strings || !tcp->strings->value || !*tcp->strings->value )
	{
	    if( tcp->error->message )
	        err = mk_string( tcp->error->message );
	    else
	        err = mk_string( "Defect name not returned" );
	    return NULL;
	}
	return mk_string( tcp->strings->value );
}

