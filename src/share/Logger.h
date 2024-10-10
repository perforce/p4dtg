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

#ifndef LOGGING_HEADER
#define LOGGING_HEADER

#include <stdio.h>

class Logger 
{
    protected:
	FILE *fd;
	char *file;
	int level;

	int check_log();

    public:
	// level: 0 = errors, 1 = warnings, 2 = info
	Logger( const char *path, int level = 1 );
	~Logger();

	bool log_open();
	void set_level( int use_level ) { level = use_level; };
	int get_level() { return level; };

	void log( int level, const char *msg, bool noflush = 0 );
	void log( int level, const char *fmt, const char *p1, 
			bool noflush = 0 );
	void log( int level, const char *fmt, 
		const char *p1, const char *p2, 
		bool noflush = 0 );
};

#endif
