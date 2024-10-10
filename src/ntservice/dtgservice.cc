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

extern int real_main( int argc, char **argv );

int __stdcall
WinMain( void * /* hInstance */, 
	void * /* hPrevInstance */, 
	char *lpCmdLine,
	int /* nCmdShow */ )
{
	int argc = 3;
	char *argv[3];
	argv[0] = strdup( "p4dtg-service" );
	argv[1] = lpCmdLine;
	char *tmp = lpCmdLine;
	while( tmp && *tmp && *tmp != ' ' )
	    tmp++;
	if( tmp && *tmp == ' ' )
	    *tmp++ = '\0';
	argv[2] = tmp;
	exit( real_main( argc, argv ) );
}
