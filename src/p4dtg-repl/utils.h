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

#ifndef UNIFYUTILS_HEADER
#define UNIFYUTILS_HEADER

class Logger;

extern char *pass_filter( struct DTGFieldDesc *filters, 
			DTGModule *mod, 
			void *defectID );
/* scm_stat/dts_stat: -1 = new, 0 = unchanged, 1 = changed */
extern int set_field( DTGModule *mod, void *defectID, const char *field, 
		const char *new_val, const char *old_val,
		struct DTGError *&err );

#endif
