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
#include <time.h>

extern "C" {
#include "dtg-utils.h"
}
#include "dtg-str.h"

#include "P4MetaClient.h"
#include <p4/strtable.h>
#include "dtg-utils.h"
#include "StrArr.h"

P4MetaClient::P4MetaClient() : ClientUser()
{
	text_results = NULL;
	info_results = NULL;
	stat_results = NULL;
	err_results = NULL;

	fix = NULL;

	data_set = NULL;
}

P4MetaClient::~P4MetaClient()
{
	clear_results();
}

#define DELETE_OBJECT(obj) if( obj != NULL ) { delete obj; obj = NULL; }

void
P4MetaClient::clear_results()
{
	DELETE_OBJECT( text_results )
	DELETE_OBJECT( info_results )
	DELETE_OBJECT( stat_results )
	DELETE_OBJECT( err_results )
	if( fix )
	{
	    delete_DTGFixDesc( fix );
	    fix = NULL;
	}

	DELETE_OBJECT( data_set )
}

/* This virtual method is to handle broken p4-like servers */
void
P4MetaClient::OutputError( const char *err )
{
	if( !err_results )
	    err_results = new StrBuf;
	err_results->Clear();
	err_results->Append( "[UnknownError]" );
	err_results->Append( err );
}

void
P4MetaClient::HandleError( Error *err )
{
	StrBuf buf;
	err->Fmt( buf, EF_NEWLINE );
	if( !err_results )
	    err_results = new StrBuf;
	err_results->Clear();
	switch( err->GetSeverity() )
	{
	case E_EMPTY:
	    err_results->Append( "[NoError]" );
	    break;
	case E_INFO:
	    err_results->Append( "[Info]" );
	    break;
	case E_WARN:
	    err_results->Append( "[MinorError]" );
	    break;
	case E_FAILED:
	    err_results->Append( "[UsageError]" );
	    break;
	case E_FATAL:
	    err_results->Append( "[FatalError]" );
	    break;
	default:
	    err_results->Append( "[UnknownError]" );
	    break;
	}
	err_results->Append( buf.Text() );
}

void
P4MetaClient::InputData( StrBuf *buf, Error * )
{
	buf->Clear();
	buf->Set( data_set );
	buf->Terminate();
}

void
P4MetaClient::OutputInfo( char level, const char *data )
{
	if( info_results == NULL )
	    info_results = new StrArr;
	info_results->Add( data );
}

void
P4MetaClient::OutputText( const char *data, int length )
{
	if( text_results != NULL )
	    delete text_results;
	text_results = new StrBuf();
	text_results->Append( data, length );
}

static void add_file( struct DTGFixDesc *fix, 
		const char *path, const char *rev, const char *action )
{
	if( !action || !path || !rev || !fix )
	    return;

	char *tmp = mk_string( action, " ", path, "#", rev );
	struct DTGStrList *nxt = fix->files;
	fix->files = new_DTGStrList( tmp );
	fix->files->next = nxt;
	delete[] tmp;
}

static char *timestamp_free( char *seconds )
{
	if( !seconds )
	    return strdup( "unknown" );

	time_t t = atol( seconds );
	struct tm *mytm;
	if( !(mytm = localtime( &t ) ) )
	    return strdup( "unknown" );
	    
	char *tmp = (char *)malloc( sizeof(char)*20 );
	sprintf( tmp, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", 
		mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday,
		mytm->tm_hour, mytm->tm_min, mytm->tm_sec );
	return tmp;
}

void
P4MetaClient::OutputStat( StrDict *dict )
{
	if( !fix )
	{
	    if( stat_results == NULL )
	        stat_results = new StrBufDict();
	    StrRef var, val;
	    for( int i = 0; dict->GetVar( i, var, val ); i++ )
	        if( strcmp( var.Text(), "func" ) && 
	            strcmp( var.Text(), "specFormatted" ) &&
	            strcmp( var.Text(), "altArg" ) )
	            stat_results->SetVar( var, val );
	    return;
	}
	
	/* Process FIX details */
	StrRef var, val;
	struct DTGStrList *tmp;
	const char *action = NULL;
	const char *path = NULL;
	const char *rev = NULL;
	int cur = -1;
	int n;
	for( int i = 0; dict->GetVar( i, var, val ); i++ )
	{
	    switch( var.Text()[0] )
	    {
	    default:
	    case 'f': // func
	        break;
	    case 's': // specFormatted
	        break;
	    case 'c': // change
	        if( strcmp( var.Text(), "change" ) )
	           break;
	        if( fix->change )
	            free( fix->change );
                fix->change = 
			val.Text() ? strdup( val.Text() ) : strdup( "unknown" );
	        break;
	    case 'u': // user
	        if( strcmp( var.Text(), "user" ) )
	           break;
	        if( fix->user )
	            free( fix->user );
                fix->user = 
			val.Text() ? strdup( val.Text() ) : strdup( "unknown" );
	        break;
	    case 't': // time
	        if( strcmp( var.Text(), "time" ) )
	           break;
	        if( fix->stamp )
	            free( fix->stamp );
                fix->stamp = timestamp_free( val.Text() );
	        break;
	    case 'd': // depotFileNN (or) desc
	        if( !strcmp( var.Text(), "desc" ) )
	        {
	            if( fix->desc )
	                free( fix->desc );
                    fix->desc = 
			val.Text() ? strdup( val.Text() ) : strdup( "unknown" );
	        }
	        else if( !strncmp( var.Text(), "depotFile", 9 ) )
	        {
	            n = atoi( &(var.Text()[9]) );
	            if( n != cur && cur > -1 )
	            {
	                add_file( fix, path, rev, action );
	                path = rev = action = NULL;
	            }
	            cur = n;
	            path = val.Text();
	        }
	        break;
	    case 'a': // actionNN (or) altArg
	        if( !strcmp( var.Text(), "altArg" ) )
	           break;
	        if( strncmp( var.Text(), "action", 6 ) )
	           break;
	        n = atoi( &(var.Text()[6]) );
	        if( n != cur && cur > -1 )
	        {
	            add_file( fix, path, rev, action );
	            path = rev = action = NULL;
	        }
	        cur = n;
	        action = val.Text();
	        break;
	    case 'r': // revNN
	        if( strncmp( var.Text(), "rev", 3 ) )
	           break;
	        n = atoi( &(var.Text()[3]) );
	        if( n != cur && cur > -1 )
	        {
	            add_file( fix, path, rev, action );
	            path = rev = action = NULL;
	        }
	        cur = n;
	        rev = val.Text();
	        break;
	    }
	}
	if( fix && path && rev && action )
	    add_file( fix, path, rev, action );
}

void
P4MetaClient::print_data()
{
	if( stat_results != NULL )
	{
	    printf("StatResults:\n");
	    StrRef var, val;
	    for( int i = 0; stat_results->GetVar( i, var, val ); i++ )
	        printf("[%s]:[%s]\n", var.Text(), val.Text() );
	    printf("\n");
	}
	if( info_results != NULL )
	{
	    const char *val;
	    printf("InfoResults:\n");
	    for( int i = 0; val = info_results->Get( i ); i++ )
	        printf("[%s]\n", val );
	    printf("\n");
	}
	if( text_results != NULL )
	    printf("TextResults: [%s]\n", text_results->Text() );
	if( err_results != NULL )
	    printf("ErrResults: [%s]\n", err_results->Text() );
}
