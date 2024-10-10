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
#include "MyDTS.h"
extern "C" {
#include "dtg-utils.h"
}
#include "dtg-str.h"

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
	    my_prog_name = cp_string( "MyDTS-Unnamed" );
	if( prog_ver )
	    my_prog_ver = cp_string( prog_ver );
	else
	    my_prog_ver = cp_string( "MyDTS-Unversioned" );

	proj_list = NULL;

	DTGField *f;

	// Enable UNICODE?
	f = get_field( (DTGField*)attrs, "unicode" );
	if( f )
	    utf8 = 1;
	else
	    utf8 = 0;

	// wait time
	f = get_field( (DTGField*)attrs, "wait_time" );
	if( f )
	    wait_time = atoi( f->value );
	else
	    wait_time = 10;

	// Try connecting
	valid = connected( err );
}

MyDTS::~MyDTS()
{
	if( my_server )
	    delete[] my_server;
	if( my_user )
	    delete[] my_user;
	if( my_pass )
	    delete[] my_pass;
	delete[] my_prog_name;
	delete[] my_prog_ver;

	// Close connections
}

int 
MyDTS::valid_project( const char *proj )
{
	if( !proj || !proj_list )
	    return 0;
	for( struct DTGStrList *p = proj_list; p; p = p->next )
	    if( !strcmp( p->value, proj ) )
	        return 1;
	return 0;
}

void 
MyDTS::connect_to_project( const char *proj, char *&err )
{
	// XXX Add any DTS specific processing that needs to be done
}

int
MyDTS::connected( char *&err)
{
	// Check to see if connection is still valid
	if( 1 )
	    return (valid = 1); // already connected
	valid = 0;

	// setup connection to server
	// return on error

	return (valid = 1);
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

	return ver;
}

char *MyDTS::get_server_date( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char *date = NULL;
	
	// Retrieve date from server 
	// Return a string which will be passed to extract_date to decode

	return date;
}

struct DTGStrList *MyDTS::list_projects( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	struct DTGStrList *list = NULL;

	// XXX Retrieve list of projects from server
	const char *plist[] = { "proj1", "proj2", "proj3", NULL };

	for( int i = 0; plist[i]; i++ )
	    list = append_DTGStrList( list, plist[i] );

	return list;
}

struct DTGFieldDesc *MyDTS::get_field_desc( const char *proj, char *&err )
{
	struct DTGFieldDesc *list = NULL;
	return list;
}

struct DTGStrList *
MyDTS::list_jobs( int max_rows, 
		struct DTGDate *since, const char *mod_date_field,
		const char *exclude_user, const char *mod_by_field,
		char *&err )
{
	if( !connected( err ) )
	    return NULL;

	DTGStrList *list = NULL;

	// XXX Retrieve list of defects from server
	const char *plist[] = { "1", "2", "3", NULL };

	for( int i = 0; plist[i]; i++ )
	    list = append_DTGStrList( list, plist[i] );

	return list;
}

struct DTGField *MyDTS::get_defect( const char *defect, 
				struct DTGStrList *ref_fields, char *&err )
{
	// XXX Retrieve specified defect and return the field values
	// XXX if ref_fields is defined, then restrict retrieval to these fields
	return NULL;
}

char *MyDTS::save_defect( const char *defect, struct DTGField *fields, char *&err )
{
	// XXX Save the specified defect and return a copy of the resulting defect name
	// XXX Handle new defects accordingly
	return NULL;
}

