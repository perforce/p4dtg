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
#include <sys/stat.h>
#include <ctype.h>
extern "C" {
#include <dtg-utils.h>
}
#include "genutils.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

char *cp_string( const char *str )
{	
	if( !str )
	    return NULL;
	char *tmp = new char[strlen(str)+1];
	tmp[0] = 0;
	strcat( tmp, str );
	return tmp;
}

char *timestamp()
{
	char stamp[32];
	const time_t now = time(NULL);
	struct tm *hmm = gmtime( &now );
	sprintf(stamp, 
		"%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d UTC", 
		hmm->tm_year + 1900,
		hmm->tm_mon + 1,
		hmm->tm_mday,
		hmm->tm_hour,
		hmm->tm_min,
		hmm->tm_sec );
	return cp_string( stamp );
}

char *mk_string( const char *s1, 
		const char *s2, 
		const char *s3, 
		const char *s4, 
		const char *s5, 
		const char *s6 )
{
	if( !s1 )
	    s1 = "";
	if( !s2 )
	    s2 = "";
	if( !s3 )
	    s3 = "";
	if( !s4 )
	    s4 = "";
	if( !s5 )
	    s5 = "";
	if( !s6 )
	    s6 = "";
	char *tmp = new char[strlen(s1) + strlen(s2) + strlen(s3) +
				strlen(s4) + strlen(s5) + strlen(s6) + 1];
	sprintf( tmp, "%s%s%s%s%s%s", s1, s2, s3, s4, s5, s6 );
	return tmp;
}

int my_isspace( char a )
{
	switch( a )
	{
	case ' ': case '\t': case '\n':
	    return 1;
	default:
	    return 0;
	}
}

/* Ignore any leading or trailing whitespace */

int chomp_strcmp( const char *s1, const char *s2 )
{
	int i;
	char *t1;
	if( *s1 == '"' )
	{
	    for( i = 1; s1[i] && my_isspace(s1[i]); i++ );
	    t1 = cp_string( &s1[i] );
	    t1[strlen(t1)-1] = '\0';
	    int j;
	    for( i = j = 0; t1[j]; i++, j++ )
	    {
	        t1[i] = t1[j];
	        if( t1[i] == '\n' && t1[i+1] == '\t' )
	            j++;
	    }
	    t1[i] = '\0';
	}
	else
	{
	    for( i = 0; s1[i] && my_isspace(s1[i]); i++ );
	    t1 = cp_string( &s1[i] );
	}

	char *t2;
	if( *s2 == '"' )
	{
	    for( i = 1; s2[i] && my_isspace(s2[i]); i++ );
	    t2 = cp_string( &s2[i] );
	    t2[strlen(t2)-1] = '\0';
	    int j;
	    for( i = j = 0; t2[j]; i++, j++ )
	    {
	        t2[i] = t2[j];
	        if( t2[i] == '\n' && t2[i+1] == '\t' )
	            j++;
	    }
	    t2[i] = '\0';
	}
	else
	{
	    for( i = 0; s2[i] && my_isspace(s2[i]); i++ );
	    t2 = cp_string( &s2[i] );
	}
	for( i = strlen(t1) - 1; i >= 0 && my_isspace(t1[i]); i-- );
	t1[i+1] = '\0';
	for( i = strlen(t2) - 1; i >= 0 && my_isspace(t2[i]); i-- );
	t2[i+1] = '\0';
	int res = strcmp( t1, t2 );
	delete[] t1;
	delete[] t2;
	return res;
}

void remove_non_numerics( struct DTGStrList *&list )
{
	if( !list )
	    return;
	struct DTGStrList *tmp;
	if( !isdigit( list->value[0] ) )
	{
	    tmp = list;
	    list = list->next;
	    tmp->next = NULL;
	    delete_DTGStrList( tmp );
	}
	else
	{
	    int i;
	    for( i = 0; isdigit( list->value[i] ); i++ );
	    list->value[i] = '\0';
	}
	struct DTGStrList *item = list;
	while( item && item->next )
	{
	    if( !isdigit( item->next->value[0] ) )
	    {
	        tmp = item->next;
	        item->next = item->next->next;
	        tmp->next = NULL;
	        delete_DTGStrList( tmp );
	    }
	    else
	    {
	        int i;
	        for( i = 0; isdigit( item->next->value[i] ); i++ );
	        item->next->value[i] = '\0';
	    }
	    item = item->next;
	}
}


#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32

#include <sys/stat.h>
#include <io.h>
#define open _open
#define UNIQUE (_O_CREAT | _O_EXCL)
#define USEMODE (_S_IREAD | _S_IWRITE)

#else

#include <unistd.h>

extern int errno;
#define UNIQUE (O_CREAT | O_EXCL)
#define USEMODE (S_IRUSR | S_IWUSR)

#endif

int lock_file( const char *filename )
{
	int locked = 1;
	char *lock_file = mk_string( filename, "-lock" );
	int result;
	result = open( lock_file, UNIQUE, USEMODE );
	while( result < 0 )
	{
	    locked++;
	    if( locked > 5 || errno != EEXIST )
	    {
	        locked = 0;
	        break;
	    }
	    errno = 0;
	    result = open( lock_file, UNIQUE, USEMODE );
	}
	if( result > 0 )
	    close( result );
	delete[] lock_file;
	return locked;
}

void unlock_file( const char *filename )
{
	char *lock_file = mk_string( filename, "-lock" );
	unlink( lock_file );
	delete[] lock_file;
}

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void do_sleep( int i )
{
#ifdef _WIN32
	Sleep( i*1000 );
#else
	sleep( i );
#endif
}
