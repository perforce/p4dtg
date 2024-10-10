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

#include "MyDTG.h"
#include "MyDTS.h"
extern "C" {
#include <dtg-utils.h>
}
#include <dtg-str.h>

extern void print_DTGField( const struct DTGField *list );

const char *MyDTG::MyDTGMagic = "MyDTGClass";

struct DTGAttribute *MyDTG::list_attrs()
{

	DTGAttribute *in_field = new_DTGAttribute( 
		"jobs_db",
		"MySQL database name",
		"The destination MySQL database name, "
		"if different than \"p4jobs\".", 
		"p4jobs", 
		0 );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
		"unicode", 
		"Unicode server",
		"Use this attribute to specify that server handles UTF-8. "
		"By default, the plug-in will only use ASCII. "
		"Specify either 'y' or 'n'.",
		"n", 
		0 ) );
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
		"ck_privs",
		"Check privileges",
		"Check MySQL access rights before connecting.",
		"y",
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
	if( !attr->name )
	    return strdup( "Unknown attribute: <>" );

	if( !strcmp( attr->name, "unicode" ) )
	    if( attr->value &&
	        ( *attr->value == 'y' || *attr->value == 'n' ) && 
	        !attr->value[1] )
	        return NULL;
	    else
	        return strdup( "Unicode server: Must be either 'y' or 'n'." );

	if( !strcmp( attr->name, "ck_privs" ) )
	    if( attr->value &&
	        ( *attr->value == 'y' || *attr->value == 'n' ) && 
	        !attr->value[1] )
	        return NULL;
	    else
	        return strdup( "Check privileges: Must be either 'y' or 'n'." );
    
	if( !strcmp( attr->name, "wait_time" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) == 0 )
	            return strdup( "Wait time: Must not be zero." );
	        else if( atoi( attr->value ) < -1 )
	            return strdup( "Wait time: Must not be less than -1." );
	        else if( atoi( attr->value ) > 600 )
	            return strdup( "Wait time:  Must be less than 10 minutes.");
	        else
	            return NULL;
	    else
	        return strdup( "Wait time: Must be a positive integer,"
	                           " or -1." );

	if( !strcmp( attr->name, "jobs_db" ) )
	    return NULL; // OK

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

struct DTGField *get_attrs( char *user, const struct DTGField *attrs, 
				char *&err )
{
	struct DTGField *list = NULL;
	if( attrs )
	    list = copy_DTGField( attrs );
	else
	  list = new_DTGField( "sql_user", user ); // todo:  is this right?

	return list;
}

MyDTG::MyDTG( const char *server, const char *user, const char *pass,
	    const struct DTGField *attrs,
	    struct DTGError *error  )
{
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

	/* BUGZ-DTG and 1.0 are for any monitor of process support */
	/* that might be available for connections to the dts server */
	/* bugzilla actually does not support showing this information */
	char *my_user = cp_string( user );
	struct DTGField *my_attrs = get_attrs( my_user, attrs, err );
	dts = new MyDTS( server, my_user, pass, my_attrs, 
			"BUGZ-DTG", "1.0", err );
	delete[] my_user;
	delete_DTGField( my_attrs );

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
	if( server_version )
	    delete[] server_version;
	if( !testing )
	    delete dts;
}

struct DTGDate *MyDTG::extract_date( const char *date_string )
{
	/* XXX Change to support the format used by the dts */
        /*          2006-05-12 12:07:12 */
	/* example: YYYY/MM/DD HH:MM:SS */
	/*          0123456789012345678 */
	/*          2006-08-24 10:14:04 */
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
	sprintf( date_string, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
		date->year, date->month, date->day,
		date->hour, date->minute, date->second );
	return date_string;
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
	return "MySQL 5"; 
}

const char *mod_version =
  "Rev. Perforce Jobs to MySQL5 plugin " ID_VER;

const char *MyDTG::get_module_version( struct DTGError *error )
{
	clear_DTGError( error );

	/* XXX Change to name of defect tracking system */
	return mod_version;
}

const char *MyDTG::get_server_version( struct DTGError *error )
{
	if( testing )
	{
	    clear_DTGError( error );
	    return "Jobs to MySQL5 test .1";
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
	if( testing )
	{
	    clear_DTGError( error );
	    return strdup( "No warnings for Jobs to MySQL5 .1" );
	}

	clear_DTGError( error );
	return NULL;
}

struct DTGStrList *MyDTG::list_projects( struct DTGError *error )
{
	struct DTGStrList *list;
	if( testing )
	{
	    list = new_DTGStrList( "*project*" );
	    clear_DTGError( error );
	    return list;
	}

	char *err = NULL;
	clear_DTGError( error );
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
