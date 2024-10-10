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

#ifndef MYDTS_HEADER
#define MYDTS_HEADER
/*
 * MyDTS.h is an interface class between P4 DTG and a generic
 * defect tracking system. This class encapsulates the DTS's
 * api.
 *
 */

struct DTGStrList;
struct DTGDate;

class MyDTS {
	protected:
	    int valid;
	    int utf8;
	    int wait_time;
	    char *my_server;
	    char *my_user;
	    char *my_pass;
	    char *my_prog_name;
	    char *my_prog_ver;

	    int connected( char *&err );

	    struct DTGStrList *proj_list;

	public:
	    MyDTS( const char *server, 
		const char *user, 
		const char *pass,
		const struct DTGField *attrs,
		const char *prog_name,
		const char *prog_ver,
		char *&err);
	    virtual ~MyDTS();

	    int is_valid() { return valid; };
	    int valid_project( const char *proj );
	    int utf8_ok() { return utf8; };
	    int server_offline( struct DTGError *error );

	    void connect_to_project( const char *proj, char *&err );
	    struct DTGStrList *list_projects( char *&err );
	    struct DTGFieldDesc *get_field_desc( const char *project, 
		char *&err );
	    char *get_server_version( char *&err );
	    char *get_server_date( char *&err );
	    struct DTGStrList *list_jobs( int max_rows, 
		struct DTGDate *since, const char *mod_date_field,
		const char *exclude_user, const char *mod_by_field,
		char *&err );
	    struct DTGField *get_defect( const char *defect, 
				struct DTGStrList *ref_fields, char *&err );
	    char *save_defect( const char *defect,
				struct DTGField *fields, char *&err );
};

#endif
