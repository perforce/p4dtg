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

#ifndef DTGXML_HEADER
#define DTGXML_HEADER

class DataSource;
class DataMapping;
class DTGSettings;

extern void load_config( const char  *dir, 
		DataSource *&srcs, 
		DataMapping *&maps,
		DTGSettings *&sets );

extern int load_p4dtg_config( const char *file, 
			DataSource *&sources, 
			DataMapping *&mappings );

extern int save_p4dtg_config( const char *file, 
			DataSource *src, 
			DataMapping *map,
			int all = 0 );

extern int save_config( const char *file, 
			DataSource *srcs1, 
			DataSource *srcs2, 
			DataMapping *maps,
			DTGSettings *sets );

extern int load_p4dtg_settings( const char *file, DTGSettings *&setting );

extern int save_p4dtg_settings( const char *file, DTGSettings *setting );

#endif
