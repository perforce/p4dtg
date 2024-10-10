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

#include "MyDTG.h"
#include "MyDTS.h"
extern "C" {
#include <dtg-utils.h>
}

const char *MyDTG::MyDTGMagic = "MyDTGClass";

MyDTG::MyDTG( const char *server, const char *user, const char *pass,
	    const struct DTGField *attrs, struct DTGError *error  )
{
	cur_message = NULL;
	cur_message_level = 10; // None
	server_version = NULL;
	magic = MyDTGMagic;
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
	dts = new MyDTS( server, user, pass, attrs, "GENERIC", "VERSION", err );

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
	/* XXX Change to return the list of allowed attributes for the plugin */
	/* XXX Sample attribute code */
	DTGAttribute *in_field = new_DTGAttribute(
	        "color", 
	        "Color",
	        "This attribute controls the color of the sun. Please be "
	        "careful about changing it to odd colors as it may start a "
	        "panic. The default is Yellow and should never be Purple.", 
	        "Yellow", 
	        0 );
	in_field->next = new_DTGAttribute(
	        "explode", 
	        "Explode",
	        "This attribute controls whether the sun goes super-nova at "
	        "the end of the year. Once it has been set and the time passes,"
	        " it cannot be undone. This is a required field with no "
	        "defaults, you must choose.",
	        NULL, 
	        1 );
	in_field->next->next = new_DTGAttribute(
	        "flare", 
	        "Flare",
	        "This attribute controls whether the frequency for which the "
	        "sun emits solar flares. The value is the number of hours "
	        "between flares. This is an optional field with an empty "
	        "default value.",
	        "", 
	        0 );
	in_field->next->next->next = new_DTGAttribute(
	        "unicode", 
	        "Unicode server",
	        "Use this attribute to specify that server handles UTF-8. "
	        "By default, the plug-in will only use ASCII. "
	        "Specify either 'y' or 'n'.",
	        "n", 
	        0 );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
	        "wait_time",
	        "Wait time",
	        "This is the number of seconds to wait before retrying after"
	        " a connection error has occurred.  A value of -1 means"
	        " that the replicator quits on error.",
	        "10",
	        0 ) );

	return in_field;
}

/* XXX SAMPLE VALIDATION FUNCTION */
#include <ctype.h>

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

	if( !strcmp( attr->name, "color" ) )
	    if( attr->value && strcmp( attr->value, "Purple" ) )
	        return NULL; // valid if not Purple
	    else
	        return strdup( "Color: Purple is for prose" );
	if( !strcmp( attr->name, "explode" ) )
	    if( attr->value && 
		( !strcasecmp( attr->value, "true" ) ||
		  !strcasecmp( attr->value, "false" ) ) )
	        return NULL;
	    else
	        return strdup( "Explode: True or false question" );
	if( !strcmp( attr->name, "flare" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) > 0 )
	            return NULL;
	        else
	            return strdup( "Flare: Must be a positive integer." );
	    else
	        return strdup( "Flare: Must be a number" );
	if( !strcmp( attr->name, "unicode" ) )
	    if( attr->value &&
		( *attr->value == 'y' || *attr->value == 'n' ) && 
		!attr->value[1] )
	        return NULL;
	    else
	        return strdup( "Unicode server: Must be either 'y' or 'n'");
	if( !strcmp( attr->name, "wait_time" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) == 0 )
	            return strdup( "Wait time: Must not be zero." );
	        else if( atoi( attr->value ) < -1 )
	            return strdup("Wait time: Must not be less than -1.");
	        else if( atoi( attr->value ) > 600 )
	            return strdup( "Wait time:  Must be less than 10 minutes.");
	        else
	            return NULL;
	    else
	        return strdup( "Wait time: Must be a positive integer,"
	                       " or -1." );

	char *tmp = (char *)malloc( 20 + strlen( attr->name) );
	sprintf( tmp, "Unknown attribute: %s", attr->name );
	return tmp;
}

int MyDTG::accept_utf8()
{
	return dts->utf8_ok();
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
	return "GENERIC"; 
}

const char *MyDTG::get_module_version( struct DTGError *error )
{
	clear_DTGError( error );

	/* XXX Change to name of defect tracking system */
	return "Version 1.0 GENERIC Plugin for p4dtg";
}

const char *MyDTG::get_server_version( struct DTGError *error )
{
	if( testing )
	{
	    clear_DTGError( error );
	    return "GENERIC x.y";
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

	if( !dts->valid_project( project ) )
	{
	    set_DTGError( error, "Unknown project");
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
