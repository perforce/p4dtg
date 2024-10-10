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

#include "MyDTS.h"
#include <p4/strtable.h>
#include <p4/i18napi.h>

extern "C" {
#include "dtg-utils.h"
}
#include "dtg-str.h"
#include "StrArr.h"

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
	server_id = NULL;
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

	Error e;
	ui1 = ui2 = NULL;

	// Create ClientApi instances one untagged and one tagged
	client1 = new ClientApi;
	client2 = new ClientApi;

	// Enable UNICODE?
	DTGField *f = get_field( (DTGField*)attrs, "unicode" );
	if( f )
	{
	    int cs = (int)CharSetApi::Lookup( "utf8" );
            client1->SetTrans( cs );
            client2->SetTrans( cs );
	    utf8 = 1;
	}
	else
	    utf8 = 0;

	client1->SetPort( my_server );
	client1->SetUser( my_user );
	client1->SetProtocol( "api", "67" ); // 2010.1 api

	client2->SetPort( my_server );
	client2->SetUser( my_user );
	client2->SetProtocol( "tag", "" );
	client2->SetProtocol( "api", "67" ); // 2010.1 api

	/* Non-empty password */
	if( my_pass && *my_pass )
	{
	    client1->SetPassword( my_pass );
	    client2->SetPassword( my_pass );
	}

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
	if( server_id )
	    delete[] server_id;
	if( my_server )
	    delete[] my_server;
	if( my_user )
	    delete[] my_user;
	if( my_pass )
	    delete[] my_pass;
	delete[] my_prog_name;
	delete[] my_prog_ver;

	// Close connections
	Error e;

	if( ui1 )
	{
	    client1->Final( &e );
	    e.Clear();
	    delete ui1;
	}
	delete client1;

	if( ui2 )
	{
	    client2->Final( &e );
	    e.Clear();
	    delete ui2;
	}
	delete client2;
}

int 
MyDTS::valid_project( const char *proj )
{
	if( !proj || strcmp( proj, "Jobs" ) )
	    return 0;
	return 1;
}

int
MyDTS::connected( char *&err)
{
	if( ui1 && !client1->Dropped() &&
	    ui2 && !client2->Dropped() )
	    return (valid = 1); // already connected

	// Remove any previous ui1 and ui2
	Error e;
	if( ui1 )
	{
	    client1->Final( &e );
	    e.Clear();
	    delete ui1;
	}
	if( ui2 )
	{
	    client2->Final( &e );
	    e.Clear();
	    delete ui2;
	}
	ui1 = ui2 = NULL;

	// Connect to server untagged
	StrBuf msg;
	client1->Init( &e );
	if( e.Test() )
	{
	    e.Fmt( &msg );
	    err = cp_string( msg.Text() );
	    return (valid = 0);
	}

	// Create the P4MetaClient instance
	ui1 = new P4MetaClient;

	// Call 'p4 trust -y' 
	// This will save the fingerprint the first time connecting
	// to the server and check it for each subsequent call.
	ui1->clear_results();
	char *args[] = { (char*)&"-y", 0 , 0};
	client1->SetArgv( 1, args );
	client1->Run( "trust", ui1 );
	if( ui1->err_results )
	{
	    err = cp_string( ui1->err_results->Text() );
	    if( !err )
	        err = cp_string( "trust call failed" );
	    client1->Final( &e );
	    delete ui1;
	    ui1 = NULL;
	    return (valid = 0);
	}

	// Connect to server tagged
	client2->Init( &e );
	if( e.Test() )
	{
	    e.Fmt( &msg );
	    err = cp_string( msg.Text() );
	    client1->Final( &e );
	    delete ui1;
	    return (valid = 0);
	}

	// Label Connections for p4 monitor
	client1->SetProg( my_prog_name );
	client1->SetVersion( my_prog_ver );

	client2->SetProg( my_prog_name );
	client2->SetVersion( my_prog_ver );

	// Create the P4MetaClient instance
	ui2 = new P4MetaClient;

	return (valid = 1);
}

int
MyDTS::server_offline( struct DTGError *error )
{
	char *err = NULL;
	int ret = 0; // assume success

	// Dropped() interface only works if you tried something
	char *date = get_server_date( err );
	if( date )
	    delete[] date;
	if( err )
	{
	    delete[] err;
	    err = NULL;
	}

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

	char *args2[] = { 0 , 0};
	client2->SetArgv( 0, args2 );
	ui2->clear_results();
	client2->Run( "info", ui2 );

	if( ui2->err_results )
	{
	    err = cp_string( ui2->err_results->Text() );
	    return NULL;
	}

	StrPtr *ver = ui2->stat_results ? 
		ui2->stat_results->GetVar( "serverVersion" ) :
		NULL; 

	if( !server_id )
	{
	    StrPtr *id = client2->GetProtocol( "server2" );
	    if( id )
	        server_id = cp_string( id->Text() );
	}

	return ver ? cp_string( ver->Text() ) : NULL;
}

char *MyDTS::get_server_date( char *&err )
{
	if( !connected( err ) )
	{
	    if( !err )
	        err = cp_string( "Unable to connect for date" );
	    return NULL;
	}

	char *args2[] = { 0 , 0};
	client2->SetArgv( 0, args2 );
	ui2->clear_results();
	client2->Run( "info", ui2 );

	if( ui2->err_results )
	{
	    err = cp_string( ui2->err_results->Text() );
	    if( !err )
	        err = cp_string( "info call failed" );
	    return NULL;
	}

	StrPtr *date = ui2->stat_results ? 
		ui2->stat_results->GetVar( "serverDate" ) :
		NULL; 
	if( date )
	    return cp_string( date->Text() );

	/* Check for dropped connection */
	connected( err );
	if( !err )
	    err = cp_string( "No serverDate value found" );
	return NULL;
}

struct DTGStrList *MyDTS::list_projects( char *&err )
{
	if( !connected( err ) )
	    return NULL;

	struct DTGStrList *list = NULL;

	// There is only one project for Perforce
	const char *plist[] = { "Jobs", NULL };

	for( int i = 0; plist[i]; i++ )
	    list = append_DTGStrList( list, plist[i] );

	return list;
}

struct DTGStrList *MyDTS::list_jobs( int max_rows, 
		struct DTGDate *since, const char *mod_date_field,
		const char *exclude_user, const char *mod_by_field,
	        const char *segment_filters,
		char *&err )
{
	// Build up qualifier
	StrBuf qualifier;
	qualifier.Clear();
	if( mod_date_field && since )
	{
	    qualifier << mod_date_field << ">=" << since->year << "/";
	    qualifier << since->month << "/" << since->day << ":";
	    qualifier << since->hour << ":" << since->minute;
	    qualifier << ":" << since->second;
	}
	if( mod_by_field && exclude_user )
	{
	    if( mod_date_field && since )
	        qualifier << " & ";
	    qualifier << "^" << mod_by_field << "=" << exclude_user;
	}
	if( segment_filters )
	    qualifier << " & " << segment_filters;
	return list_jobs( max_rows, qualifier.Text(), err );
}

struct DTGStrList *MyDTS::list_jobs( int max_rows,const char *qualifier, char *&err )
{
	if( !connected( err ) )
	    return NULL;

	// Run the command
	DTGStrList *list = NULL;
	char *args1[] = { 0, 0, 0, 0, 0 };
	int i = 0;
	if( max_rows > 0 )
	{
	    char num[32];
	    snprintf(num, 31, "%d", max_rows );
	    args1[i++] = (char*)&"-m";
	    args1[i++] = num;
	}
	if( qualifier && *qualifier )
	{
	    args1[i++] = (char*)&"-e";
	    args1[i++] = (char *)qualifier;
	}
	client1->SetArgv( i, args1 );
	ui1->clear_results();
	client1->Run( "jobs", ui1 );
	if( ui1->err_results )
	{
	    err = cp_string( ui1->err_results->Text() );
	    return NULL;
	}

	char *val;
	if( !ui1->info_results )
	    return NULL;
	for( int i = 0; val = (char *)ui1->info_results->Get( i ); i++ )
	{
	    int j;
	    for( j = 0; val[j] != ' ' && val[j]; j++ );
	    if( val[j] == ' ' )
	    {
	        val[j] = '\0';
	        list = append_DTGStrList( list, val );
	    }
	}

	return list;
}

StrDict *MyDTS::get_defect( const char *id, char *&err )
{
	return get_form( "job", id, err );
}

char *MyDTS::save_defect( const char * /*name*/, StrDict *fields, char *&err )
{
	return save_form( "job", fields, err );
}

struct DTGFixDesc *MyDTS::describe_fix( const char *id, char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char *args2[] = { (char*)&"-s", 0 , 0};
	if( id )
	{
	    args2[1] = (char *)id;
	    client2->SetArgv( 2, args2 );
	}
	else
	    client2->SetArgv( 1, args2 );
	ui2->clear_results();
	ui2->fix = new_DTGFixDesc();
	client2->Run( "describe", ui2 );

	struct DTGFixDesc *results = ui2->fix;
	ui2->fix = NULL;

	if( ui2->err_results )
	{
	    err = cp_string( ui2->err_results->Text() );
	    delete_DTGFixDesc( results );
	    return NULL;
	}

	return results;
}

struct DTGStrList *MyDTS::list_fixes( const char *id, char *&err )
{
	if( !connected( err ) )
	    return NULL;

	// Run the command
	struct DTGStrList *list = NULL;
	char *args[] = { (char*)&"-j", 0, 0, 0, 0 };
	args[1] = (char *)id;
	client1->SetArgv( 2, args );
	ui1->clear_results();
	client1->Run( "fixes", ui1 );
	if( ui1->err_results )
	{
	    err = cp_string( ui1->err_results->Text() );
	    return NULL;
	}

	char *val;
	if( !ui1->info_results )
	    return NULL;
	for( int i = 0; val = (char *)ui1->info_results->Get( i ); i++ )
	    list = append_DTGStrList( list, val );

	struct DTGStrList *fixes = NULL;
	char tmp[32];
	for( struct DTGStrList *f = list; f; f = f->next )
	{
	    char *i = strstr( f->value, " by change " );
	    if( i )
	    {
	        int num = atoi( &i[11] );
	        snprintf( tmp, 32, "%d", num );
	        fixes = append_DTGStrList( fixes, tmp );
	    }
	}
	delete_DTGStrList( list );

	return fixes;
}

StrDict *MyDTS::get_form( const char *type, const char *id, char *&err )
{
	if( !connected( err ) )
	    return NULL;

	char *args2[] = { (char*)&"-o", 0 , 0};
	if( id )
	{
	    args2[1] = (char *)id;
	    client2->SetArgv( 2, args2 );
	}
	else
	    client2->SetArgv( 1, args2 );
	ui2->clear_results();
	client2->Run( type, ui2 );

	if( ui2->err_results )
	{
	    err = cp_string( ui2->err_results->Text() );
	    return NULL;
	}

	StrDict *results = ui2->stat_results;
	ui2->stat_results = NULL;
	return results;
}

char *MyDTS::save_form( const char *type, StrDict *fields, char *&err )
{
	if( !connected( err ) || !fields || !type )
	    return NULL;

	ui2->clear_results(); // Comes before setting of data_set
	StrRef var, val;
	if( ui2->data_set == NULL )
	    ui2->data_set = new StrBuf;
	ui2->data_set->Clear();
	for( int i = 0; fields->GetVar( i, var, val ); i++ )
	{
	    struct DTGStrList *value = split_DTGStrList( val.Text(), '\n' );
	    ui2->data_set->Append(var.Text());
	    if( value && value->next )
		// Multiple lines get put on their own lines
	        ui2->data_set->Append(":\n");
	    else
		// Put a single line on the same line
	        ui2->data_set->Append(":");
	    for( struct DTGStrList *line = value; line; line = line->next )
	    {
	        ui2->data_set->Append("\t");
	        ui2->data_set->Append( line->value );
	        ui2->data_set->Append("\n");
	    }
	    ui2->data_set->Append("\n");
	    delete_DTGStrList( value );
	}

	// FILE *fff = fopen("save.job", "w" );
	// fprintf(fff, "%s", ui2->data_set->Text() );
	// fclose(fff);

	// Run the command
	char *args2[] = { (char*)&"-i", 0};
	if( ui2->stat_results )
	    delete ui2->stat_results;
	ui2->stat_results = NULL;
	if( ui2->info_results )
	    delete ui2->info_results;
	ui2->info_results = NULL;
	client2->SetArgv( 1, args2 );
	client2->Run( type, ui2 );
	char *jobid = NULL;
	if( ui2->err_results )
	    err = cp_string( ui2->err_results->Text() );
	else if( ui2->info_results )
	{
	    DTGStrList *parts = 
		split_DTGStrList( ui2->info_results->Get( 0 ), ' ' );
	    if( parts != NULL && parts->next != NULL )
	        jobid = cp_string( parts->next->value );
	    delete_DTGStrList( parts );
	}
	
	return jobid;
}
