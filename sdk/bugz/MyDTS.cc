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
#include <time.h>
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
	if( !id )
	    return NULL;
	while( fields && strcmp( fields->name, id ) )
	    fields = fields->next;
	return fields;
}

const char *MyDTS::find_value( struct DTGField *fields, const char *id )
{
	if( !id )
	    return NULL;
	while( fields && strcmp( fields->value, id ) )
	    fields = fields->next;
	return fields ? fields->name : id;
}

const char *find_field( struct DTGField *fields, const char *id )
{
	fields = get_field( fields, id );
	return fields ? fields->value : id;
}

// SQL-escapes a string.  Does not enclose the string in surrounding quotes.
char* MyDTS::esc_field( const char* fld )
{
	if( !fld )
	    return NULL;

	unsigned long fldLen = strlen( fld );
	// allow empty strings! or I have to handle nulls :(
	//if( !fldLen )
	 //   return NULL;

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
	        fields = append_DTGField( fields, new_DTGField(
	                        find_field( field_map, cols[i].name ),
	                        row[i] ) );
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

char *MyDTS::get_description( const char *bugid, char *&err )
{
	char *query = mk_string(
"SELECT who, bug_when, isprivate, already_wrapped, thetext FROM longdescs",
" WHERE bug_id = ", bugid,
" ORDER BY bug_when" );

	char *desc = cp_string( "" );

	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    if( !res )
	        return NULL;
	    MYSQL_ROW row;
	    int cnt = 0;
	    char cnttxt[11];
	    while( ( row = mysql_fetch_row( res ) ) )
	    {
	        sprintf( cnttxt, "%d", cnt++ );
	        char *tmp = mk_string( row[4], "\n",
					"--- Comment ", cnttxt,
					": From ",
					find_field( profile_map, row[0] ),
					" at ", row[1] );
	        char *tmp2 = mk_string( desc, tmp, "\n\n" );
	        delete[] tmp;
	        delete[] desc;
	        desc = tmp2;
	    }
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to retrieve description: ",
	                      mysql_error( mysql ) );

	delete[] query;

	return desc;
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

	use_profile = NULL;
	field_map = NULL;
	profile_map = NULL;
	product_map = NULL;
	component_map = NULL;

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
	const char* i0 =  strchr( my_server, ':' );
	const char* i1 = strrchr( my_server, ':' );

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

	// run a privilege check?
	f = get_field( (DTGField*)attrs, "ck_privs" );
	if( f )
	    ck_privs = 0;
	else
	    ck_privs = 1;

	// SQL database name for Bugzilla
	f = get_field( (DTGField*)attrs, "bz_db" );
	if( f )
	    bz_db = cp_string( f->value );
	else
	    bz_db = cp_string( "bugs" );

	// custom field in Bugzilla that we use to store fix information in.
	f = get_field( (DTGField*)attrs, "bz_cf" );
	if( f )
	    bz_cf = cp_string( f->value );
	else
	    bz_cf = cp_string( "cf_p4fixes" );

	// Enable UNICODE?
	f = get_field( (DTGField*)attrs, "unicode" );
	if( f )
	    utf8 = 1;
	else
	    utf8 = 0;

	// Enable Private Fixes?
	f = get_field( (DTGField*)attrs, "private_fixes" );
	if( f )
	    private_fixes = "1";
	else
	    private_fixes = "0";

	f = get_field( (DTGField*)attrs, "wait_time" );
	if( f )
	    wait_time = atoi( f->value );
	else
	    wait_time = 10;

	mysql = mysql_init( NULL );

	// Try connecting
	valid = 0;
	valid = connected( err );
	if( err )
	    return;

	if( ck_privs )
	    if( no_privs( err ) )
	        return;

	char *qbz_db = esc_field( bz_db );
	char *uq = mk_string( "USE `", qbz_db, "`" );
	if( mysql_query( mysql, uq ) )
	{
	    err = mk_string( "Failed to USE database: \"", qbz_db, "\" - ",
                              mysql_error( mysql ) );
	    delete[] uq;
	    free( qbz_db );
	    return;
	}
	free( qbz_db );
	delete[] uq;

	if( valid )
	    profile_map = two_cols( "SELECT userid, login_name FROM profiles" );

	f = get_field( (DTGField*)attrs, "bugz_user" );
	if( f && strlen( f->value ) )    /* use the supplied bugzilla account */
	{
	    const char* pmname = find_value( profile_map, f->value );
	    if( !strcmp( pmname, f->value ) )
	        err = mk_string( "\nFailed to find the supplied Bugzilla user ",
	                        "account.  Looked for \"", f->value, "\".\n" );
	    else
	        use_profile = cp_string( find_value( profile_map, pmname ) );
	}
	else    /* no specific bugzilla user name set, so use the sql name */
	{
	    const char* pmname = find_value( profile_map, my_user );
	    if( !pmname || !strcmp( pmname, my_user ) )
	        err = mk_string( "\nFailed to find the same Bugzilla user ",
	            "name as that of your MySQL account.  Looked for ",
	            "\"", my_user, "\".\n", "\nTry the \"Bugzilla ",
	            "username\" field under the \"Edit ",
	            "attributes...\" button.\n" );
	    else
	        use_profile = cp_string( find_value( profile_map, my_user ) );
	}
}

MyDTS::~MyDTS()
{
	if( my_server )
	    delete[] my_server;
	if( my_user )
	    delete[] my_user;
	if( my_pass )
	    delete[] my_pass;
	if( use_profile )
	    delete[] use_profile;
	if( field_map )
	    delete_DTGField( field_map );
	if( profile_map )
	    delete_DTGField( profile_map );
	if( product_map )
	    delete_DTGField( product_map );
	if( component_map )
	    delete_DTGField( component_map );
	if( bz_db )
	  delete[] bz_db;
	if( bz_cf )
	  delete[] bz_cf;

	// Close connections
	if( mysql )
	    mysql_close( mysql );
}

int
MyDTS::valid_project( const char *proj )
{
	if( !proj )
	    return 0;
	if( !strcmp( "Bugs", proj ) )
	    return 1;
	return 0;
}

void
MyDTS::connect_to_project( const char *proj, char *&err )
{
	// XXX Add any DTS specific processing that needs to be done
	// Nothing to do for bugzilla
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
		bz_db, my_port, NULL, 0 );
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
	char *ver = NULL;

	// Retrieve version from server

	const char *tmp = mysql_get_server_info( mysql );

	if( !mysql_query( mysql, "SELECT version FROM bz_schema" ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row = mysql_fetch_row( res );
	    if( row )
	        ver = mk_string( "Schema:", row[0], ":MySQL:", tmp );
	    else
	        err = cp_string( "Failed to retrieve row" );
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to retrieve date: ", mysql_error( mysql ));

	return ver;
}

char *MyDTS::get_server_date( int offset, char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char *date = NULL;

	// Retrieve date from server
	// Return a string which will be passed to extract_date to decode

	char query[128];
	sprintf( query, "%s%d%s", "SELECT ADDTIME(NOW(), -", offset, ");" );

	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row = mysql_fetch_row( res );
	    if( row )
	        date = cp_string( row[0] );
	    else
	        err = mk_string( "Failed to retrieve row: ",
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

	struct DTGStrList *list = NULL;

	// Retrieve list of projects from server
	const char *plist[] = { "Bugs", NULL };

	for( int i = 0; plist[i]; i++ )
	    list = append_DTGStrList( list, plist[i] );

	return list;
}

struct DTGStrList *MyDTS::get_options( const char *table, char *&err )
{
	struct DTGStrList *list = NULL;
	char *query = mk_string( "SELECT * FROM ", table );
	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row;
	    while( ( row = mysql_fetch_row( res ) ) )
	        list = append_DTGStrList( list, row[1] );
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to get options: ", mysql_error( mysql ) );
	delete[] query;
	return list;
}

struct DTGStrList *MyDTS::get_prod_components( const char *product, char *&err )
{
	struct DTGStrList *list = NULL;
	char* qproduct = esc_field( product );
	char *query = mk_string( 
		"SELECT * FROM components,products WHERE ", 
		"components.product_id=products.id AND ", 
		"products.name='", qproduct, "'" );
	free( qproduct );
	if( !mysql_query( mysql, query ) )
	{
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row;
	    while( ( row = mysql_fetch_row( res ) ) )
	        list = append_DTGStrList( list, row[1] );
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to get components: ", 
				mysql_error( mysql ) );
	delete[] query;
	return list;
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
	struct DTGField *list =
		two_cols( "SELECT name, description FROM fielddefs" );
	if( list )
	{
	    list = append_DTGField( list,
				new_DTGField( "product_id", "Product" ) );
	    list = append_DTGField( list,
				new_DTGField( "component_id", "Component" ) );
	}
	return list;
}

struct DTGField *MyDTS::product_names()
{
	return two_cols( "SELECT id, name FROM products" );
}

char *MyDTS::component_name( const char *comp_name, const char *prod_id )
{
	char* qcomp = esc_field( comp_name );
	char *q = mk_string( "SELECT id FROM components",
	                     " where name = '", qcomp, "' and product_id = ",
	                     prod_id );
	free( qcomp );
	char *err = NULL;
	struct DTGStrList *r = single_col( q, err );
	delete[] q;
	char *n = strdup( r->value );
	delete_DTGStrList( r );
	return n;
}

struct DTGFieldDesc *MyDTS::get_field_desc( const char *proj, char *&err )
{
	struct DTGFieldDesc *list = NULL;
	if( !field_map )
	    field_map = field_names();
	if( !mysql_query( mysql, "describe bugs" ) )
	{
	    list = append_DTGFieldDesc( list,
		new_DTGFieldDesc( "Description", "text", 1, NULL ) );
	    list = append_DTGFieldDesc( list,
		new_DTGFieldDesc( "Fixes", "fix", 0, NULL ) );
	    MYSQL_RES *res = mysql_store_result( mysql );
	    MYSQL_ROW row;
	    while( ( row = mysql_fetch_row( res ) ) )
	    {
	        int rwflag = 1; // by default everything is ro
	        const char *type = NULL;
	        DTGStrList *options = NULL;
	        if( !strcmp( row[0], bz_cf ) )
	        {
	            rwflag = 0;
	            type = "line";
	        }
	        if( !strcmp( row[0], "bug_id" ) )
	        {
	            rwflag = 4;
	            type = "word";
	        }
	        else if( !strcmp( row[0], "creation_ts" ) )
	        {
	            rwflag = 1;
	            type = "date";
	        }
	        else if( !strcmp( row[0], "delta_ts" ) )
	        {
	            rwflag = 2;
	            type = "date";
	        }
	        else if( !strcmp( row[0], "bug_status" ) )
	        {
	            rwflag = 1;
	            type = "select";
	            options = get_options( "bug_status", err );
	        }
	        else if( !strcmp( row[0], "resolution" ) )
	        {
	            rwflag = 1;
	            type = "select";
	            options = get_options( "resolution", err );
	        }
	        else if( !strcmp( row[0], "priority" ) )
	        {
	            rwflag = 0;
	            type = "select";
	            options = get_options( "priority", err );
	        }
	        else if( !strcmp( row[0], "product_id" ) )
	        {
	            rwflag = 1;
	            type = "select";
	            options = get_options( "products", err );
	        }
	        else if( !strcmp( row[0], "component_id" ) )
	        {
	            // Can't segment on Component, so change its type.
	            rwflag = 1;
	            type = "line";
	            options = get_options( "components", err );
	        }
	        else if( !strcmp( row[0], "version" ) )
	        {
	            rwflag = 0;
	            type = "select";
	            options = get_options( "versions", err );
	        }
	        else if( !strcmp( row[0], "op_sys" ) )
	        {
	            rwflag = 0;
	            type = "select";
	            options = get_options( "op_sys", err );
	        }
	        else if( !strcmp( row[0], "bug_severity" ) )
	        {
	            rwflag = 0;
	            type = "select";
	            options = get_options( "bug_severity", err );
	        }
	        else if( !strcmp( row[0], "short_desc" ) )
	        {
	            rwflag = 0;
	            type = "line";
	        }
	        else if( !strcmp( row[0], "assigned_to" ) ||
	                 !strcmp( row[0], "reporter" ) ||
	                 !strcmp( row[0], "qa_contact" ) )
	        {
	            rwflag = 1;
	            type = "word";
	        }
	        else 
	        {
	            rwflag = 0;
	            switch( row[1][0] )
	            {
	            case 'm': // mediumint, mediumtext
	                if( !strncmp( row[1], "mediumint(", 10 ) )
	                    type = "word";
	                else if( !strcmp( row[1], "mediumtext" ) )
	                    type = "text";
	                break;
	            case 't': // text, tinyint
	                if( !strcmp( row[1], "text" ) )
	                    type = "text";
	                else if( !strncmp( row[1], "tinyint(", 9 ) )
	                    type = "word";
	                break;
	            case 'd': // datetime, decimal
	                if( !strcmp( row[1], "datetime" ) )
	                    type = "date";
	                else if( !strncmp( row[1], "decimal(", 8 ) )
	                    type = "word";
	                break;
	            case 'v': // varchar
	                if( !strncmp( row[1], "varchar(", 8 ) )
	                    type = "line";
	                break;
	            case 's': // smallint
	                if( !strncmp( row[1], "smallint(", 9 ) )
	                    type = "word";
	                break;
	            default:
	                break;
	            }
	        }

	        if( type )
	            list = append_DTGFieldDesc( list,
			new_DTGFieldDesc( find_field( field_map, row[0] ),
					type, rwflag, options ) );
	    }
	    mysql_free_result( res );
	}
	else
	    err = mk_string( "Failed to get fields: ", mysql_error( mysql ) );
	return list;
}

struct DTGStrList *
MyDTS::list_jobs( int max_rows,
		struct DTGDate *since, const char *mod_date_field,
		const char *exclude_user, const char *mod_by_field,
		const char *seg_filters, char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char q0[256];
	sprintf( q0, "%s %c%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d%c",
		"SELECT bug_id FROM bugs WHERE delta_ts >=",
		'"',
		since->year, since->month, since->day,
		since->hour, since->minute, since->second,
		'"'
	);
	char *query = mk_string( q0, seg_filters );
	struct DTGStrList *list = single_col( query, err );
	delete[] query;

	return list;
}

struct DTGField *MyDTS::get_defect( const char *defect, char *&err )
{
	// Initialize maps if needed
	if( !profile_map )
	    profile_map = two_cols( "SELECT userid, login_name FROM profiles" );
	if( !field_map )
	    field_map = field_names();
	if( !product_map )
	    product_map = two_cols( "SELECT id, name FROM products" );
	if( !component_map )
	    component_map = two_cols( "SELECT id, name FROM components" );

	// Retrieve specified defect and return the field values
	char* qdefect = esc_field( defect );
	char *query = 
		mk_string( "SELECT * FROM bugs WHERE bug_id = ", qdefect );
	free( qdefect );
	DTGField *result = single_row( query, err );
	DTGField *field = get_field( result, "AssignedTo" );
	if( field && field->value )
	{
	    const char *tmp = find_field( profile_map, field->value );
	    free( field->value );
	    if( tmp )
	        field->value = strdup( tmp );
	    else
	        field->value = strdup( "NotFound" );
	}
	field = get_field( result, "QAContact" );
	if( field && field->value )
	{
	    // QAContact can be null on bugs logged before the QAContact
	    // feature was enabled on the Bugzilla instance.
	    const char *tmp = find_field( profile_map, field->value );
	    free( field->value );
	    if( tmp )
	        field->value = strdup( tmp );
	    else
	        field->value = strdup( "NotFound" );
	}
	field = get_field( result, "ReportedBy" );
	if( field && field->value )
	{
	    const char *tmp = find_field( profile_map, field->value );
	    free( field->value );
	    if( tmp )
	        field->value = strdup( tmp );
	    else
	        field->value = strdup( "NotFound" );
	}
	field = get_field( result, "Product" );
	if( field && field->value )
	{
	    const char *tmp = find_field( product_map, field->value );
	    free( field->value );
	    if( tmp )
	        field->value = strdup( tmp );
	    else
	        field->value = strdup( "NotFound" );
	}
	field = get_field( result, "Component" );
	if( field && field->value )
	{
	    const char *tmp = find_field( component_map, field->value );
	    free( field->value );
	    if( tmp )
	        field->value = strdup( tmp );
	    else
	        field->value = strdup( "NotFound" );
	}
	delete[] query;
	if( !result )
	    err = cp_string( "Defect not found" );
	else
	{
	    char *desc = get_description( defect, err );
	    if( desc )
	        result = append_DTGField( result,
				new_DTGField( "Description", desc ) );
	    delete[] desc;
	}
	return result;
}

void
MyDTS::append_fix( const char *defect, const char *fix, int stamped,
			char *&err )
{
	if( !fix || !defect )
	    return;

	// Skip leading return
	if( fix[0] == '\n' )
	    fix = &fix[1];

	char* qdefect = esc_field( defect );
	char* qfix    = esc_field( fix    );

	// append the fix changelist info to the comment field
	char *query = mk_string(
		"INSERT INTO longdescs ( bug_id, isprivate, who, bug_when, ",
		"thetext, already_wrapped ) VALUES ( ", qdefect, ", ", 
		private_fixes, ", ", use_profile, ", now(), \"",
		qfix, "\", 1 )" );
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
	    err = mk_string( "Append fix failed: \"", query, "\", ",
	                     mysql_error( mysql ) );
	delete[] query;

	// a fix might be applied independently of any other updates, so we
	// have to update the bug's modified-date. if not, the fix's changelist
	// description that was just inserted into the bug's comments will
	// not get sent back to the corresponding p4 job.
	if( !stamped )
	{
	    query = mk_string( "UPDATE bugs SET delta_ts = now() WHERE ",
	                          "bug_id = ", qdefect );

	    if( !mysql_query( mysql, query ) )
	    {
	        int cnt = mysql_affected_rows( mysql );
	        if( cnt < 0 )
	            err = mk_string( "INSERT failed:", query );
	        else if( cnt > 1 )
	            err = mk_string( "Too many rows inserted:", query );
	        // else if( cnt == 0 )
	        //   It already is set to the current time, so OK
	    }
	    else
	        err = mk_string( "Append fix failed: \"", query, "\", ",
	                         mysql_error( mysql ) );
	    delete[] query;
	}

	free( qdefect );
	free( qfix    );
}
	

// passed to split_and_send() to write an entry in the activity table

void
MyDTS::insert_activity(const char *now, const char* qvalue, const char* qdefect, 
	const char* fieldid, char *&err)
{
	const char* q0 = mk_string("INSERT INTO bugs_activity ( bug_id, ",
		"who, bug_when, fieldid, removed, ",
		"added ) VALUES ( ", qdefect, ", ", use_profile, ", '", now, "', ");


	const char* query = mk_string(q0, fieldid, ", '', \"", qvalue,
		"\" )");
	delete[] q0;

	if (!mysql_query(mysql, query))
	{
		int cnt = mysql_affected_rows(mysql);
		if (cnt < 0)
			err = mk_string("INSERT failed:", query);
		else if (cnt == 0)
			err = mk_string("No rows inserted:", query);
		else if (cnt > 1)
			err = mk_string("Too many rows inserted:", query);
	}
	else
		err = mk_string("save_defect append history failed: \"", query,
			"\", ", mysql_error(mysql));
	delete[] query;
}


// split a null terminated string into segments of MAX_LEN  or less and send each segment to the sender function.
// This routine trys to send whole lines, but if a line is longer than MAX_LEN , it will be split.
// Warning! The buffer pointed to by val is modified by this routine.

#define MAX_LEN 255

void
MyDTS::split_and_send(char* val, const char *qdefect, const char *fieldid, char *&err) 
{
	char buffer[MAX_LEN];
	
	char* top = buffer + MAX_LEN;
	char* buf_ptr = buffer;
	int buf_available = MAX_LEN;

    // get the server date-time, use it for all our sends 

	char *server_time = get_server_date(0, err); 

    // printf("server_time: %s\n",server_time);

	// split value (by \n preferably) into segments
	// write each segment to the database as a separate entry in the bugs_activity table.
	// bugs_activity table has a 256 char limit on the "added" field.

	char* line_ptr = val;


	char* lf_ptr = strpbrk(line_ptr, "\n");
	if (lf_ptr) {  // if there is a \n, replace it with a null terminator
			*lf_ptr = 0;
	}

	// for each line in val
	while (line_ptr)
	{
	    // escape the line
		char* esc_buf = esc_field(line_ptr);
		char* esc_ptr = esc_buf;

		int seg_len = (int)strlen(esc_buf); // size of escaped line
		int buf_len = 0;

		// if the buffer is too full, flush it
		if (seg_len > buf_available) {
			// flush the buffer, to make MAX_LEN room
			if (buf_available != MAX_LEN) {
		        insert_activity(server_time, buffer, qdefect, fieldid, err);

				// reset to start of buffer
				buf_ptr = buffer;
				buf_available = MAX_LEN;
			}
		}

		// now send blocks of MAX_LEN bytes 
		while (seg_len > buf_available) {
			strncpy(buffer, esc_ptr, MAX_LEN - 1);
			buffer[MAX_LEN - 1] = 0;
		    insert_activity(server_time, buffer, qdefect, fieldid, err);

			// advance the pointer
			buf_len = (int)strlen(buffer);
			esc_ptr += buf_len;
			seg_len -= buf_len;
		}

		// store the left-overs
		if (seg_len) {
			strcpy(buf_ptr, esc_ptr);
			buf_len = (int)strlen(esc_ptr);
			// store the rest of the line in the buffer
			buf_ptr += buf_len;
		}

		// add an LF if necessary
		if (lf_ptr) {
			strcat(buf_ptr, "\n");
			buf_ptr++;
			buf_available -= (buf_len + 1);
		} else {
			buf_available -= buf_len;
		}
		
		free( esc_buf );  // we used malloc!

		// last line in val?
		if (!lf_ptr) { // check for null termination
			break;
		}
        // get next line
		line_ptr = ++lf_ptr;
		lf_ptr = strpbrk(line_ptr, "\n");
       

		// null terminate the line (if not the last line)
		if (lf_ptr) {
			*lf_ptr = 0;
		}
	}
	
	// no more lines, send any buffered left-overs
	if (buf_available != MAX_LEN) {
		insert_activity(server_time, buffer, qdefect, fieldid, err);
	}
 	delete server_time;

}


char *
MyDTS::save_defect( const char *defect, struct DTGField *fields, char *&err )
{
	if( !fields )
	    return cp_string( defect );

	char* qdefect = esc_field( defect );

	/* Build up SET clause list */
	struct DTGStrList *setlist = NULL;
	int fix = 0;
	for( struct DTGField *f = fields; f; f = f->next )
	{
	    const char *rn = find_value( field_map, f->name );
	    if( !strcmp( f->name, "Fixes" ) )
	    {
	        fix = 1;
	        continue;
	    }
	    else if( !strcmp( f->name, "Description" ) ||
			!strcmp( rn, "bug_id" ) )
	        continue;

	    char *qvalue = esc_field( f->value );
	    char *tmp = mk_string( rn, "=\"", qvalue, "\"" );
	    setlist = append_DTGStrList( setlist, tmp );
	    delete[] tmp;
	    free( qvalue );
	}
	char *query = NULL;
	int stamped = 0;
	if( setlist )
	{
	    stamped = 1;
	    setlist = append_DTGStrList( setlist, "delta_ts = now()" );
	    char *sets = join_DTGStrList( setlist, ", " );
	    delete_DTGStrList( setlist );
	    query =
	      mk_string( "UPDATE bugs SET ", sets, " WHERE bug_id = ", qdefect);
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
	        err = mk_string( "save_defect failed: \"", query, "\", ",
	                         mysql_error( mysql ) );
	    delete[] query;
	}

	// get the field id numbers so we can properly update bugs_activity.
	const char *histquery = "SELECT id,description FROM fielddefs";
	struct DTGField *histids = two_cols( histquery );
	if( !histids )
	    err = mk_string( "save_defect:  unable to run the histquery." );
	else
	for( struct DTGField *f = fields; f; f = f->next )
	{
	    // fix history is handled in append_fix().
	    if( strcmp( "Fixes", f->name) == 0 )
	        continue;

	    const char *fieldid = find_value( histids, f->name );

	    if( strcmp( fieldid, f->name ) == 0 )
	        err = cp_string( "save_defect:  field id not found." );

	    split_and_send(f->value, qdefect, fieldid, err);

	}

	delete_DTGField( histids );
	free( qdefect );

	if( fix )
	    append_fix( defect, find_field( fields, "Fixes" ), stamped, err );
	return cp_string( defect );
}
