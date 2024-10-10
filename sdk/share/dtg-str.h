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

#ifndef DTGSTR_HEADER
#define DTGSTR_HEADER


/*
 * Perforce Defect Tracking Gateway Developers Kit: 
 *
 * This header define helper functions for string creation using new
 * instead of malloc
 *
 */

extern char *cp_string( const char *str );
extern char *mk_string( const char *s1,
			const char *s2 = NULL,
			const char *s3 = NULL,
			const char *s4 = NULL,
			const char *s5 = NULL,
			const char *s6 = NULL,
			const char *s7 = NULL,
			const char *s8 = NULL,
			const char *s9 = NULL,
			const char *s10 = NULL );

#endif
