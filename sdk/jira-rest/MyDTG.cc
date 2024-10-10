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
#include <ctype.h>
#include <sys/stat.h>

#include "MyDTG.h"
#include "MyDTS.h"
extern "C" {
#include <dtg-utils.h>
}
#include <dtg-str.h>
#include <DTG-platforms.h>

const char *MyDTG::MyDTGMagic = "MyDTGClass";

MyDTG::MyDTG( const char *server, const char *user, const char *pass,
	    const struct DTGField *attrs, struct DTGError *error  )
{
	magic = MyDTGMagic;

	cur_message = NULL;
	cur_message_level = 10; // None
	server_version = NULL;
	proj_list = NULL;

	/* Can new issues be created in Jira? */
	allow_creation = 0;
	const struct DTGField *f;
	for( f = attrs; f; f = f->next )
	    if( f->name && !strcmp( f->name, "allow_creation" ) )
	        if( f->value && ( *f->value == 'y' || *f->value == 'Y' ) )
	            allow_creation = 1;

	if( server && !strcmp( server, "*server*") &&
	    user && !strcmp( user, "*userid*") &&
	    pass && !strcmp( pass, "*passwd*") )
	{
	    testing = 1;
	    clear_DTGError( error );
	    return;
	}
	testing = 0;

	char *err = NULL;

	/* XXX GENERIC and VERSION are for any monitor of process support */
	/* that might be available for connections to the dts server */

	/* http://HOST:PORT/ is the default              */
    /* SERVER -> http://SERVER:8080/                 */
    /* SERVER:PORT -> http://SERVER:PORT/            */
    /* SERVER:PORT/ -> http://SERVER:PORT/           */
	/*                                               */
    /* http://... -> uses what is there, no changes  */
    /* https://... -> uses what is there, no changes */


	char *use_server = NULL;
	if( !strncasecmp( server, "http://", 7 ) ||
	    !strncasecmp( server, "https://", 8 ) )
	    // Assume full server desc
	    use_server = mk_string( server );
	else if( strchr( server, ':' ) )
	    // Assume HOST:PORT
	    if( strchr( server, '/' ) )
	        // Assume HOST:PORT/URL
	        use_server =
		  mk_string( "http://", server );
	    else
	        // Assume just HOST:PORT
	        use_server =
		    mk_string( "http://", server,
				"/" );
	else
	    // Assume just HOST
	    use_server =
		mk_string( "http://", server,
			":8080/" );
	dts = new MyDTS( use_server, user, pass, attrs, "JiraREST", "1.0", err );
	delete[] use_server;

	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = dts->is_valid();
	    delete[] err;
	}
	else
	    clear_DTGError( error );
}

MyDTG::~MyDTG()
{
	if( cur_message )
	    delete[] cur_message;
	if( server_version )
	    delete[] server_version;
	if( !testing )
	    delete dts;
	if( proj_list )
	    delete_DTGStrList( proj_list );
}

struct DTGDate *MyDTG::extract_date( const char *date_string )
{
	/* XXX Change to support the format used by the dts */
        /*          2006/05/12 12:07{:12}*/
	/* example: YYYY/MM/DD HH:MM{:SS} */
	/*          0123456789012345 678 */
	int len;
	if( !date_string || (len = strlen( date_string )) < 15 )
	    return NULL;
	struct DTGDate *date = new_DTGDate(0,0,0,0,0,0);
	date->month = atoi( &date_string[5] );
	date->day = atoi( &date_string[8] );
	date->year = atoi( &date_string[0] );
	date->hour = atoi( &date_string[11] );
	date->minute = atoi( &date_string[14] );
	if( len > 17 )
	    date->second = atoi( &date_string[17] );
	else
	    date->second = 0;
	return date;
}

char *MyDTG::format_date( struct DTGDate *date )
{
	if( !date )
	    return NULL;

	/* XXX Change to support the format used by the dts */
	/* return value will be deallocated using free() */
	char *date_string = (char *)malloc( sizeof(char)*32 );
	sprintf( date_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		date->year, date->month, date->day,
		date->hour, date->minute, date->second );
	return date_string;
}

struct DTGAttribute *MyDTG::list_attrs()
{
	DTGAttribute *in_field = new_DTGAttribute(
	        "config",
	        "Workflow and field definitions",
	        "Use this attribute to specify the XML definition file for the "
	        "workflow and custom fields for the selected project(s). This "
	        "file must be located in the 'config' subdirectory of the P4DTG "
	        "installation. By default, 'jira-rest-config.xml' will be used. "
	        "See the README.txt in the 'doc/jira' directory for information "
	        "on this important required definition file.",
	        "jira-rest-config.xml",
	        0 );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
	        "wait_time",
	        "Wait time",
	        "Specifies the number of seconds that the replication engine "
	        "waits after detecting a connection error before it retries "
	        "an operation. Use -1 to default to the General Wait Duration "
	        "specified for the replication map.",
	        "10",
	        0 ) );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
	        "tcp_port",
	        "TCP Port",
	        "This is the TCP port number of the DTG-JIRA proxy server will "
	        "be listening on. The TCP port number should not be used by "
	        "another service on the system. Must be between 49152 & 65535.",
	        "51666",
	        0 ) );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
	        "java_opts",
	        "Java options",
	        "Use this attribute to specify additional Java options for "
	        "starting the JIRA plug-in. Only update this attribute with "
	        "the help of Perforce technical support.",
	        "-Xms128m -Xmx512m",
	        0 ) );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
	        "allow_creation",
	        "Allow JIRA issue creation",
	        "Enable creating of new JIRA issue through replication. "
	        "Default is to disallow such creation. "
	        "Specify either 'y' or 'n'.",
	        "n",
	        0 ) );

	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
	       "defect_batch",
	       "Batch size for issues",
	       "This attribute affects the memory used by the Java process "
	       "by limiting the number of issues returned per query of the "
	       "JIRA server. A small number limits the memory but increases "
	       "processing time when there are a large number of issues to "
	       "be processed. A larger number improves performance but "
	       "consumes a much larger amount of memory. Default is 100. ",
	        "100",
	       0 ) );

	return in_field;
}

static int is_number( const char *str )
{
	if( !str )
	    return 0;

	if( *str == '.' )
	    return 0;

	// a negative number?
	if( strlen( str ) > 1 )
	{
	    if( *str == '-' && !isdigit( *(str + 1 )) )
	        return 0;
	    else
	        str += 1;
	}

	for( const char *i = str; *i; i++ )
	    if( !isdigit( *i ) )
	        return 0;

	return 1;
}

char *MyDTG::validate_attr( const struct DTGField *attr )
{
	/* XXX Change to support the validation of plugin specific attributes */
	/* XXX Sample validation code */

	if( !attr->name )
	    return strdup( "Unknown attribute: <>" );

	if( !strcmp( attr->name, "config" ) )
	    if( attr->value && *attr->value )
	    {
	        char *tmp = mk_string( "config", DIRSEPARATOR, attr->value );
	        struct stat buf;
	        char *msg = NULL;
	        if( stat( tmp, &buf ) )
	            msg = strdup("Configuration file: File is not accessible");
	        delete[] tmp;
	        return msg;
	    }
	    else
	        return strdup( "Configuration file: Must be specified" );
	if( !strcmp( attr->name, "allow_creation" ) )
	    if( attr->value &&
		( *attr->value == 'y' || *attr->value == 'n' ) &&
		!attr->value[1] )
	        return NULL;
	    else
	        return strdup( 
			"Enable issue creation: Must be either 'y' or 'n'");
	if( !strcmp( attr->name, "defect_batch" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) == 0 )
	            return strdup( "Batch size for issues: Must not be zero." );
	        else if( atoi( attr->value ) < 0 )
	            return strdup("Batch size for issues: Must not be negative.");
	        else
	            return NULL;
	    else
	        return strdup( "Batch size for issues: Must be a positive integer." );
	if( !strcmp( attr->name, "wait_time" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) == 0 )
	            return strdup( "Wait time: Must not be zero." );
	        else if( atoi( attr->value ) < -1 )
	            return strdup("Wait time: Must not be less than -1.");
	        else if( atoi( attr->value ) > 600 )
	            return strdup( "Wait time: Must be less than 10 minutes.");
	        else
	            return NULL;
	    else
	        return strdup( "Wait time: Must be a positive integer,"
	                       " or -1." );
	if( !strcmp( attr->name, "tcp_port" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) > 65535 )
	            return strdup( "TCP port: Must be between 49152 & 65535.");
	        else if( atoi( attr->value ) < 49152 )
	            return strdup( "TCP port: Must be between 49152 & 65535.");
	        else
	            return NULL;
	    else
	        return strdup( 
		 "TCP port: Must be a positive integer between 49152 & 65535.");
	if( !strcmp( attr->name, "java_opts" ) )
	    if( attr->value )
	        return NULL;
            else
                return strdup( "Java options: Only update this with the "
            		"help of Perforce technical support." );

	char *tmp = (char *)malloc( 20 + strlen( attr->name) );
	sprintf( tmp, "Unknown attribute: %s", attr->name );
	return tmp;
}

int MyDTG::accept_utf8()
{
	return 1;
}

int MyDTG::server_offline( struct DTGError *error )
{
	return dts->server_offline( error );
}

struct DTGDate *MyDTG::get_server_date( struct DTGError *error )
{
	if( testing )
	{
	    clear_DTGError( error );
	    return new_DTGDate( 2006, 5, 6, 12, 34, 56 );
	}

	char *err = NULL;
	char *tmp = dts->get_server_date( err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = dts->is_valid();
	    delete[] err;
	    return NULL;
	}
	clear_DTGError( error );
	struct DTGDate *server_date = extract_date( tmp );
	delete[] tmp;
	return server_date;
}

const char *MyDTG::get_name( struct DTGError *error )
{
	clear_DTGError( error );

	/* XXX Change to short name of defect tracking system */
	return "JIRA-REST";
}

const char *mod_version = "Rev. jirarest " ID_VER;

const char *MyDTG::get_module_version( struct DTGError *error )
{
	clear_DTGError( error );
	return mod_version;
}

const char *MyDTG::get_server_version( struct DTGError *error )
{
	if( testing )
	{
	    clear_DTGError( error );
	    return "JIRA REST Connector 2.0";
	}

	char *err = NULL;
	clear_DTGError( error );
	if( server_version )
	    return server_version;

	server_version = dts->get_server_version( err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = dts->is_valid();
	    delete[] err;
	}
	return server_version;
}

char *MyDTG::get_server_warnings( struct DTGError *error )
{
	clear_DTGError( error );
	if( testing )
	    return strdup( "No warnings for GENERIC x.y" );

	return NULL;
}

int MyDTG::get_message( struct DTGError *error )
{
	clear_DTGError( error );
	if( testing )
	{
	    set_DTGError( error, "Test message" );
	    return 0;
	}

	if( cur_message )
	{
	    set_DTGError( error, cur_message );
	    delete[] cur_message;
	    cur_message = NULL;
	    int level = cur_message_level;
	    cur_message_level = 10;
	    return level;
	}
	return 10; // No message
}

struct DTGStrList *MyDTG::list_projects( struct DTGError *error )
{
	struct DTGStrList *list;
	clear_DTGError( error );
	if( testing )
	{
	    list = new_DTGStrList( "*project*" );
	    return list;
	}

	char *err = NULL;
	list = dts->list_projects( err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = dts->is_valid();
	    delete[] err;
	}
	delete_DTGStrList( proj_list );
	proj_list = NULL;
	if( list )
	{
	    proj_list = copy_DTGStrList( list );
	    struct DTGStrList *all = new_DTGStrList( "*All*" );
	    all->next = list;
	    list = all;
	}
	return list;
}

MyDTGProj *MyDTG::get_project( const char *project, struct DTGError *error )
{
	if( !project || !*project )
	{
	    set_DTGError( error, "Undefined project" );
	    return NULL;
	}

	if( testing )
	{
	    if( strcmp( project, "*project*" ) )
	    {
	        set_DTGError( error, "Unknown project" );
	        return NULL;
	    }

	    MyDTGProj *proj = new MyDTGProj( this, project, error );
	    if( error->message )
	    {
	        delete proj;
	        proj = NULL;
	    }
	    return proj;
	}

	MyDTGProj *proj = new MyDTGProj( this, project, error );
	if( error->message )
	{
	    delete proj;
	    proj = NULL;
	}
	return proj;
}
