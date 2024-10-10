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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
typedef struct stat STRUCTSTAT;
#else
#define stat _stat
typedef struct _stat STRUCTSTAT;
#endif

/*
	ON WINDOWS: fd is created and closed for each logging command
*/

#include "Logger.h"
#include <genutils.h>

bool Logger::log_open()
{
#ifdef _WIN32
	return 1;
#endif
	return fd;
}

Logger::Logger( const char *path, int log_level )
{
	fd = fopen( path, "a+" );
	file = mk_string( path );
	level = log_level;
	if( !fd )
	    return;
#ifdef _WIN32
	fclose( fd );
#endif
	log( 0, "Log opened" );
}

Logger::~Logger()
{
#ifndef _WIN32
	delete[] file;
	file = NULL;
	if( !fd )
	    return;
	log( 0, "Log closed" );
	fclose( fd );
#else
	log( 0, "Log closed" );
	delete[] file;
	file = NULL;
#endif
}

int Logger::check_log()
{
	if( !file )
	    return 0;

#ifndef _WIN32
	STRUCTSTAT buf;
	int i = stat( file, &buf );
	if( i && errno == ENOENT )
	{
	    fclose( fd );
	    fd = fopen( file, "a+" );
	    if( !fd )
	        return 0;
	    log( 0, "Log re-opened" );
	}
	return 1;
#else
	fd = fopen( file, "a+" );
	if( !fd )
	    return 0;
	return 1;
#endif
}

void Logger::log( int lvl, const char *msg, bool noflush )
{
#ifndef _WIN32
	if( !fd )
	    return;
#endif
	if( lvl > level || !check_log() )
	    return;
	char *stamp = timestamp();
	fprintf( fd, "%s: %s\n", stamp, msg );
	delete[] stamp;
#ifndef _WIN32
	if( !noflush )
	    fflush( fd );
#else
	fclose( fd );
	fd = NULL;
#endif
}

void Logger::log( int lvl, const char *fmt, const char *p1, bool noflush )
{
#ifndef _WIN32
	if( !fd )
	    return;
#endif
	if( lvl > level || !check_log() )
	    return;
	char *stamp = timestamp();
	fprintf( fd, "%s: ", stamp );
	fprintf( fd, fmt, p1 );
	fprintf( fd, "\n" );
	delete[] stamp;
#ifndef _WIN32
	if( !noflush )
	    fflush( fd );
#else
	fclose( fd );
	fd = NULL;
#endif
}

void Logger::log( int lvl, const char *fmt, 
	const char *p1, const char *p2, 
	bool noflush )
{
#ifndef _WIN32
	if( !fd )
	    return;
#endif
	if( lvl > level || !check_log() )
	    return;
	char *stamp = timestamp();
	fprintf( fd, "%s: ", stamp );
	fprintf( fd, fmt, p1, p2 );
	fprintf( fd, "\n" );
	delete[] stamp;
#ifndef _WIN32
	if( !noflush )
	    fflush( fd );
#else
	fclose( fd );
	fd = NULL;
#endif
}
