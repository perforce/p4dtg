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

const char *MyDTG::MyDTGMagic = "MyDTGClass";

MyDTG::MyDTG( const char *server, const char *user, const char *pass,
	    const struct DTGField *attrs, struct DTGError *error  )
{
	magic = MyDTGMagic;
	cur_message = NULL;
	cur_message_level = 10; // None
	server_version = NULL;
	charset = NULL;
	sub_missing = 0;
	gen_jobname = 0;
	mapid = NULL;

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

	/* Specify process and version for monitor */
	dts = new MyDTS( server, user, pass, attrs, "p4jobdt", "1.0", err );

	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = dts->is_valid();
	    delete[] err;
	}
	else
	    clear_DTGError( error );
	if( attrs )
	{
	    struct DTGStrList *msg = 
		new_DTGStrList( "Attributes passed into plug-in:" );
	    for( const struct DTGField *a = attrs; a; a = a->next )
	    {
	        char *tmp = mk_string( "\t", a->name, ": ", a->value );
	        msg = append_DTGStrList( msg, tmp );
	        delete[] tmp;

	        if( !strcmp( a->name, "charset" ) && strcmp( a->value, "none") )
	            charset = cp_string( a->value );
	        else if( !strcmp( a->name, "sub_missing" ) )
	            sub_missing = ( *a->value == 'y' ) ? 1 : 0;
	        else if( !strcmp( a->name, "gen_jobname" ) )
	            gen_jobname = ( *a->value == 'y' ) ? 1 : 0;
	        else if( !strcmp( a->name, "DTG-MapID" ) )
	            mapid = cp_string( a->value );
	    }
	    char *tmp = join_DTGStrList( msg, "\n" ); 
	    cur_message = cp_string( tmp );
	    free( tmp );
	    delete_DTGStrList ( msg );
	    cur_message_level = 2;
	}
}

MyDTG::~MyDTG()
{
	if( cur_message )
	    delete[] cur_message;
	if( server_version )
	    delete[] server_version;
	if( !testing )
	    delete dts;
	if( charset )
	    delete[] charset;
	if( mapid )
	    delete[] mapid;
}

int MyDTG::server_offline( struct DTGError *error )
{
	return dts->server_offline( error );
}

struct DTGDate *MyDTG::extract_date( const char *date_string )
{
	/* Supports the format used by the dts */
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

	/* Supports the format used by the dts */
	/* return value will be deallocated using free() */
	char *date_string = (char *)malloc( sizeof(char)*32 ); 
	sprintf( date_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		date->year, date->month, date->day,
		date->hour, date->minute, date->second );
	return date_string;
}

/* Used in help for valid values */
const char *CSETS = 
	"none, cp1251, cp949, eucjp, iso8859-1, iso8859-15, iso8859-5, "
	"koi8-r, macosroman, shiftjis, winansi";

/* Used to check value */
const char *CHECK_CSETS = 
	",none,cp1251,cp949,eucjp,iso8859-1,iso8859-15,iso8859-5,"
	"koi8-r,macosroman,shiftjis,winansi,";

struct DTGAttribute *MyDTG::list_attrs()
{
	DTGAttribute *in_field = new_DTGAttribute(
		"unicode", 
		"Unicode server",
		"Specifies whether the Perforce server is running in Unicode "
		"mode. Specify 'y' if the server is running in Unicode, 'n' "
		"if not. By default, the plug-in does not use Unicode mode.",
		"n", 
		0 );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
		"gen_jobname", 
		"Generate job ids",
		"Specify that P4DTG is to create a new job id based on the "
		"Defect Tracking id and MapID for all replicated issues. "
		"The default is to let the Perforce server generate the "
		"new job id. This behavior can also be implemented using a "
		"Job In trigger, see the doc/bugzilla directory for an "
		"example trigger.",
		"n", 
		0 ) );
	char *tmp = mk_string(
		"For Perforce servers that are not running in Unicode mode, "
		"specify the encoding to use in converting the text into UTF8. "
		"By default, 'none' is used and no conversion is performed. "
		"Use this attribute when you want to connect a non-Unicode "
		"Perforce server to a Defect Tracking system which requires "
		"UTF8. Valid values from 'p4 help charset' are:\n", CSETS );
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
		"charset", 
		"Non-Unicode server character set",
		tmp,
		"none", 
		0 ) );
	delete[] tmp;
	in_field = append_DTGAttribute( in_field, new_DTGAttribute(
		"sub_missing", 
		"Replace missing character set mappings",
		"When using 'Non-Unicode server character set', if a character "
		"is missing the translation map, it can either generate an "
		"error or be replaced with a '?' symbol. By default, an error "
		"will be generated. Set to 'y' to substitute missing "
		"characters.",
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
	        return strdup( "Unicode server: Must be either 'y' or 'n'");
    
	if( !strcmp( attr->name, "gen_jobname" ) )
	    if( attr->value &&
	        ( *attr->value == 'y' || *attr->value == 'n' ) && 
	        !attr->value[1] )
	        return NULL;
	    else
	        return strdup( "Generate job id: Must be either 'y' or 'n'");
    
	if( !strcmp( attr->name, "charset" ) )
	    if( attr->value && *attr->value )
	    {
	        char *tmp = mk_string( ",", attr->value, "," );
	        if( strstr( CHECK_CSETS, tmp ) )
	        {
	            delete[] tmp;
	            return NULL;
	        }
	        else
	        {
	            delete[] tmp;
	            return strdup( "Non-Unicode server character set: "
			       "See help for list of valid values");
	        }
	    }
	    else
	        return strdup( "Non-Unicode server character set: "
			       "See help for list of valid values");
    
	if( !strcmp( attr->name, "sub_missing" ) )
	    if( attr->value &&
	        ( *attr->value == 'y' || *attr->value == 'n' ) && 
	        !attr->value[1] )
	        return NULL;
	    else
	        return strdup( ": Must be either 'y' or 'n'");
    
	if( !strcmp( attr->name, "wait_time" ) )
	    if( attr->value && is_number( attr->value ) )
	        if( atoi( attr->value ) == 0 )
	            return strdup( "Wait time:  Must not be zero." );
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
	if( dts->utf8_ok() || charset )
	    return 1;
	else 
	    return 0;
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
	if( !tmp )
	{
	    set_DTGError( error, "No date returned" );
	    error->can_continue = dts->is_valid();
	    return NULL;
	}
	clear_DTGError( error );
	struct DTGDate *server_date = extract_date( tmp );
	delete[] tmp;
	if( !server_date )
	{
	    set_DTGError( error, "Unable to extract date" );
	    error->can_continue = dts->is_valid();
	}
	return server_date;
}

const char *MyDTG::get_name( struct DTGError *error )
{
	clear_DTGError( error );

	/* Short name of defect tracking system */
	return "Perforce Jobs";
}

const char *mod_version = "Rev. p4jobdt " ID_VER;

const char *MyDTG::get_module_version( struct DTGError *error )
{
	clear_DTGError( error );

	/* Name of defect tracking system */
	return mod_version;
}

const char *MyDTG::get_server_version( struct DTGError *error )
{
	if( testing )
	{
	    clear_DTGError( error );
	    return "P4 5.2";
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
	    return strdup( "No warnings for P4 5.2" );
	}

	clear_DTGError( error );
	const char *sv = get_server_version( error );
	if( dts->server_id )
	{
	    if( atoi( dts->server_id ) < 22 )
	        return strdup( "Unsupported server." );
	}
	else
	    return strdup( "Unable to determine server version." );
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
