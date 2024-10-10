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
#include <stdlib.h>
#include <string.h>
#include "MyDTS.h"
extern "C" {
#include "dtg-utils.h"
}
#include "dtg-str.h"

void print_DTGStrList( struct DTGStrList *list )
{
	struct DTGStrList *cur = list;
	while( cur )
	{
	    struct DTGStrList *next = cur->next;
	    if( cur->value )
	        printf( "cur->value:  \"%s\"\n", cur->value );
	    cur = next;
	}
}

void print_DTGField( const struct DTGField *list )
{
	struct DTGField *cur = (DTGField*)list;
	while( cur )
	{
	    struct DTGField *next = cur->next;
	    if( cur->value )
	        printf( "cur->name:  \"%s\"\ncur->value:  \"%s\"\n\n",
	                cur->name, cur->value );
	    cur = next;
	}
}

int count_DTGField( struct DTGField *list )
{
	struct DTGField *cur = list;
	int r = 0;
	while( cur )
	{
	    struct DTGField *next = cur->next;
	    r += 1;
	    cur = next;
	}
	return r;
}

void print_DTGFieldDesc( struct DTGFieldDesc *list )
{
	struct DTGFieldDesc *cur = list;
	while( cur )
	{
	    struct DTGFieldDesc *next = cur->next;
	    if( cur->type )
	        printf( "cur->name:  \"%s\"\ncur->type:  \"%s\"\n\n",
	                cur->name, cur->type );
	    if( strcmp( "select", cur->type ) == 0 )
	    {
	        print_DTGStrList( cur->select_values );
	        printf( "\n" );
	    }
	    cur = next;
	}
}

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

char* MyDTS::esc_field( const char* fld )
{
	if( !fld )
	    return NULL;

	unsigned long fldLen = strlen( fld );
	if( !fldLen )
	    return NULL;

	unsigned long bufSize = sizeof( char ) * fldLen * 2 + 1;
	char* buf = (char*) malloc( bufSize );

	mysql_real_escape_string( mysql, buf, fld, fldLen );

	return buf;
}

DTGField *MyDTS::single_row( const char *query, char *&err )
{
	DTGField *fields = NULL;
	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    if( !res )
	        return fields;;
	    unsigned int f = mysql_num_fields( res );
	    MYSQL_FIELD *cols = mysql_fetch_fields( res );
	    MYSQL_ROW row = mysql_fetch_row( res );
	    for( unsigned int i = 0; row && i < f; i++ )
	    {
	        if( row[i] == NULL )
	            continue;
	        fields = append_DTGField( fields,
	                                    new_DTGField( cols[i].name, row[i] )
	                                );
           }
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to retrieve data: ",
	                     mysql_error( mysql ) );
	return fields;
}

DTGStrList *
MyDTS::single_col( const char *query, char *&err )
{
	DTGStrList *values = NULL;

	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    if( !res )
	        return values;
	    MYSQL_ROW row;
	    while( ( row = mysql_fetch_row( res ) ) )
	        values = append_DTGStrList( values, row[0] );
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to retrieve data: ",
	                     mysql_error( mysql ) );

	return values;
}

int MyDTS::no_privs( char *&err )
{
	// These column/table names will never exist in the real schema, so if
	// we do have INSERT or UPDATE rights, then we won't change anything.

	const int acserr = 1142;

	const char *i = "INSERT INTO lajsglkjaal ( jASAS3a3a ) VALUES ( abcd )";
	if( mysql_query( mysql, i ) && mysql_errno( mysql ) == acserr )
	{
	    err = mk_string( "Check privileges: No 'INSERT' rights." );
	    return 1;
	}

	const char *u = "UPDATE laAHAAagw SET asAYWagsa = 1 WHERE alkWAj6j = 0";
	if( mysql_query( mysql, u ) && mysql_errno( mysql ) == acserr )
	{
	    err = mk_string( "Check privileges: No 'UPDATE' rights." );
	    return 1;
	}

	const char *s = "SELECT * FROM jlkasjfdlajsfdl";
	if( mysql_query( mysql, s ) && mysql_errno( mysql ) == acserr )
	{
	    err = mk_string( "Check privileges: No 'SELECT' rights." );
	    return 1;
	}
	return 0;
}

MyDTS::MyDTS( const char *server, 
		const char *user, 
		const char *pass,
		const struct DTGField *attrs,
		const char * /* prog_name */,
		const char * /* prog_ver */,
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

	mysql = NULL;
	if( err )
	{
	    valid = 0;
	    return;
	}

	// default MySQL port.
	my_port = 3306;

	// look for "host:port" format, otherwise use the default.  make sure
	// "server" only has one colon, and it's not at the beginning or end.
	char* i0 =  strchr( my_server, ':' );
	char* i1 = strrchr( my_server, ':' );

	if( i0                                            &&
	    i0 == i1                                      && // only one match
	    strlen( my_server ) != ( i0 - my_server + 1 ) && // not at end
	    i0 != my_server                                  // not at start
	  )
	    {
	        my_port = atoi( i0 + 1 );

	        char* s = my_server;
	        int end = i0 - s; // points to ':'.
	        my_server = new char[ end + 1 ];
	        my_server[ end ] = '\0';
	        strncpy( my_server, s, end );
	        delete[] s;
	    }

	DTGField *f;

	// privilege check 
	f = get_field( (DTGField*)attrs, "ck_privs" );
	if( f )
	    ck_privs = 0;
	else
	    ck_privs = 1;

	// Enable UNICODE?
	f = get_field( (DTGField*)attrs, "unicode" );
	if( f )
	    utf8 = 1;
	else
	    utf8 = 0;

	f = get_field( (DTGField*)attrs, "wait_time" );
	if( f )
	    wait_time = atoi( f->value );
	else
	    wait_time = 10;

	f = get_field( (DTGField*)attrs, "jobs_db" );
	if( f )
	    jobs_db = cp_string( f->value );
	else
	    jobs_db = mk_string( "p4jobs" );

	mysql = mysql_init( NULL );

	// Try connecting
	valid = 0;
	valid = connected( err );
	if( err )
	    return;

	if( ck_privs )
	    if( no_privs( err ) )
	        return;

	char *qjobs_db = esc_field( jobs_db );
	char *uq = mk_string( "USE `", qjobs_db, "`" );
	if( mysql_query( mysql, uq ) )
	{
	    err = mk_string( "Failed to USE database: \"", qjobs_db, "\" - ",
                              mysql_error( mysql ) );
	    free( qjobs_db );
	    delete[] uq;
	    return;
	}
	free( qjobs_db );
	delete[] uq;
}

MyDTS::~MyDTS()
{
	if( my_server )
	    delete[] my_server;
	if( my_user )
	    delete[] my_user;
	if( my_pass )
	    delete[] my_pass;

	delete[] jobs_db;

	// Close connections
	if( mysql )
	    mysql_close( mysql );
}

int 
MyDTS::valid_project( const char *proj )
{
	if( !proj )
	    return 0;
	if( !strcmp( "jobs", proj ) )
	    return 1;
	return 0;
}

void 
MyDTS::connect_to_project( const char *proj, char *&err )
{
	// XXX Add any DTS specific processing that needs to be done
	// Nothing to do for MySQL.
}

int
MyDTS::connected( char *&err)
{
	// Check to see if connection is still valid
	if( valid )
	{
	    valid = !mysql_ping( mysql );
	    if( valid )
 	        return valid;
 	}
	valid = 0;

	// If the connection is invalid, try a new connection to the server.

	// Check for unrecoverable mysql_init() error
	if( !mysql )
	    return valid;

	int arg = MYSQL_PROTOCOL_TCP;
	mysql_options( mysql, MYSQL_OPT_PROTOCOL, (const char *)(&arg) );

	if( utf8 )
	    mysql_options( mysql, MYSQL_SET_CHARSET_NAME, "utf8" );

	MYSQL *tmp = mysql_real_connect( mysql, my_server, my_user, my_pass, 
		jobs_db, my_port, NULL, 0 );
	if( !tmp )
	{
	    err = mk_string( "Connect Failure: ", mysql_error( mysql ) );
	    return valid;
	}

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

	// Retrieve version from server 
	return (char*) cp_string( mysql_get_server_info( mysql ) );
}

char *MyDTS::get_server_date( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char *date = NULL;
	
	// Retrieve date from server 
	// Return a string which will be passed to extract_date to decode

	if( !mysql_query( mysql, "SELECT now()" ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row = mysql_fetch_row( res );
	    if( row )
	        date = cp_string( row[0] );
	    else
	        err = mk_string( "Failed to retrieve row/date: ",
	                         mysql_error( mysql ) );
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to retrieve date: ", mysql_error( mysql ));

	return date;
}

struct DTGStrList *MyDTS::list_projects( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	return new_DTGStrList( "jobs" );
}

struct DTGField *MyDTS::two_cols( const char *query )
{
	struct DTGField *list = NULL;
	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row;
	    while( ( row = mysql_fetch_row( res ) ) )
	        list = append_DTGField( list, new_DTGField( row[0], row[1] ) );
	    mysql_free_result( res );
	}
	return list;
}

struct DTGField *MyDTS::field_names()
{
	const char* query = "SELECT field,p4val FROM typemap";
	DTGField *tmlist = two_cols( query );
	return tmlist;
}

struct DTGFieldDesc *MyDTS::get_field_desc( const char *proj, char *&err )
{
	struct DTGFieldDesc *list = NULL;
  
	DTGField *tmlist = field_names();
	if( !tmlist )
	{
	    err = mk_string( "Failed to retrieve field names from typemap." );
	    return NULL;
	}

	struct DTGField *cur = tmlist;

	while( cur )
	{
	    // todo:  need to hide 'job', 'user', _modifieddate/by, etc?
	    int rwflag = 0; // rw by default.
	    struct DTGField *next = cur->next;
	    cur->next = NULL;

	    if( strcmp( cur->name, "_ModDate" ) == 0 )
	        rwflag = 2;
	
	    if( strcmp( cur->name, "_ModBy" ) == 0 )
	        rwflag = 3;

	    if( strcmp( cur->name, "_job" ) == 0 )
	        rwflag = 4;
	
	    DTGStrList* options = NULL;
	
	    if( strcmp( cur->value, "select" ) == 0 )
	    {
	        char* qname = esc_field( cur->name );
	        char* squery = mk_string( "SELECT val FROM select_values ",
	        "WHERE field='", qname, "'" );
	        free( qname );
	        options =  single_col( squery, err );
	        delete[] squery;
	    }
	
	    list = append_DTGFieldDesc( list, 
					new_DTGFieldDesc( cur->name, cur->value,
							  rwflag, options ) );
	    cur = next;
	}

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

	char query[ 256 ];
	sprintf( query, "%s %c%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d%c", 
		"SELECT _job from jobs where _ModDate >=", 
		'"',
		since->year, since->month, since->day,
		since->hour, since->minute, since->second,
		'"'
	);
	struct DTGStrList *list = single_col( query, err );
	return list;
}

struct DTGField *MyDTS::get_defect( const char *defect, char *&err )
{
  	// Retrieve specified defect and return the field values
	char *qdefect = esc_field( defect );
	char *query = mk_string( "SELECT * from jobs where _job = '",
				 qdefect, "'" );
	free( qdefect );
	DTGField *result = single_row( query, err );

	if( !result )
	  err = mk_string( "nothing in get_defect:  ", defect,
			   ", query:  ", query );
	delete[] query;

	return result;
}

char *
MyDTS::save_defect( const char *defect, struct DTGField *fields, char *&err )
{
	if( !fields )
	    return cp_string( defect );

	char* real_defect = NULL;
	struct DTGField* tf = get_field( fields, "_job");

	if( tf )
	    real_defect = esc_field( tf->value );

	if( !strcmp( defect, "new" ) )
	{
	    struct DTGStrList * fs = NULL;
	    struct DTGStrList * ff = NULL;
	
	    for( struct DTGField *f = fields; f; f = f->next )
	    {
	        char* qname = esc_field( f->name );
	        char* qvalue = esc_field( f->value );
	        char *tmp = mk_string(  "`", qname, "`" );
	        fs = append_DTGStrList( fs, tmp );
	        delete[] tmp;
	
	        tmp = mk_string( "\'", qvalue, "\'" );
	        ff = append_DTGStrList( ff, tmp );
	        delete[] tmp;
	        free( qname );
	        free( qvalue );
	    }
	
	    char *sets = join_DTGStrList( fs, ", " );
	    char *sets1 = join_DTGStrList( ff, ", " );
	
	    delete_DTGStrList( fs );
	    delete_DTGStrList( ff );
	
	    char* query = NULL;
	    query = mk_string( "INSERT INTO jobs ( ", sets,
			       " ) VALUES ( ", sets1, " )"  );
	
	    free( sets  );
	    free( sets1 );
	
	    if( query )
	    {
	        if( !mysql_query( mysql, query ) )
	        {
	            int cnt = mysql_affected_rows( mysql );
	            if( cnt < 0 )
	                err = mk_string( "INSERT failed:", query );
			    else if( cnt == 0 )
			      err = mk_string( "No rows inserted:", query );
	            else if( cnt > 1 )
	                err = mk_string( "Too many rows inserted:", query );
	        }
	        else
	            err = mk_string( "save_defect failed: \"", query, "\", ",
	                             mysql_error( mysql ) );
	        delete[] query;
	    }

	    free( real_defect );
	    return cp_string( defect );
	}

	/* Build up SET clause list */
	struct DTGStrList *setlist = NULL;

	for( struct DTGField *f = fields; f; f = f->next )
	{
	    char* qname = esc_field( f->name );
	    char* qvalue = esc_field( f->value );
	    char *tmp = mk_string( "`", qname, "`", "=\'", qvalue, "\'" );
	    free( qname );
	    free( qvalue );
	    setlist = append_DTGStrList( setlist, tmp );
	    delete[] tmp;
	}
	char *query = NULL;
	if( setlist )
	{
	    char *sets = join_DTGStrList( setlist, ", " );
	    delete_DTGStrList( setlist );
	    query = 
	      mk_string( "UPDATE jobs SET ", sets, " WHERE _job = '",
			 real_defect, "'" );
	    free( sets );
	}

	// Run UPDATE
	if( query )
	{
	    if( !mysql_query( mysql, query ) )
	    {
	        int cnt = mysql_affected_rows( mysql );
	        if( cnt < 0 )
	            err = mk_string( "UPDATE failed:", query );
			        else if( cnt == 0 )
			            err = mk_string( "No rows updated:", query );
	        else if( cnt > 1 )
	            err = mk_string( "Too many rows updated:", query );
	    }
	    else
	       err = mk_string( "save_defect failed:", query );
	    delete[] query;
	}
	free( real_defect );
	return cp_string( defect );
}
