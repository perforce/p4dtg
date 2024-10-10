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

#ifndef GENUTILS_HEADER
#define GENUTILS_HEADER

extern char *cp_string( const char *str );
extern char *mk_string( const char *s1 = "", 
			const char *s2 = "", 
			const char *s3 = "", 
			const char *s4 = "", 
			const char *s5 = "", 
			const char *s6 = "" );
extern int my_isspace( char a );
extern int chomp_strcmp( const char *s1, const char *s2 );
extern void remove_non_numerics( struct DTGStrList *&list );
extern char *timestamp();

extern int lock_file( const char *filename );
extern void unlock_file( const char *filename );

extern void do_sleep( int seconds );

#endif
