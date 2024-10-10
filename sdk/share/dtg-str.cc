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

/* cp_string uses 'new' while strdup uses 'malloc' */

char *cp_string( const char *str )
{
	if( !str )
	    return NULL;
	char *tmp = new char[strlen(str)+1];
	tmp[0] = 0;
	strcat( tmp, str );
	return tmp;
}

char *mk_string( const char *s1, 
		const char *s2, 
		const char *s3, 
		const char *s4, 
		const char *s5, 
		const char *s6, 
		const char *s7, 
		const char *s8, 
		const char *s9, 
		const char *s10 )
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
	if( !s7 )
	    s7 = "";
	if( !s8 )
	    s8 = "";
	if( !s9 )
	    s9 = "";
	if( !s10 )
	    s10 = "";
	char *tmp = new char[strlen(s1) + strlen(s2) + strlen(s3) +
				strlen(s4) + strlen(s5) + strlen(s6) + 
				strlen(s7) + strlen(s8) + strlen(s9) + 
				strlen(s10) + 1];
	sprintf( tmp, "%s%s%s%s%s%s%s%s%s%s", 
			s1, s2, s3, s4, s5, s6, s7, s8, s9, s10 );
	return tmp;
}
