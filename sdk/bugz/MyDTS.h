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
#ifdef _WIN32
#include <my_global.h>
#endif
#include <mysql.h>

class MyDTS {
	protected:
	    int valid;
	    int wait_time;
	    int utf8;
	    const char *private_fixes;
	    int my_port;
	    int ck_privs;
	    char *my_server;
	    char *my_user;
	    char *my_pass;
	    const char *bz_db;
	    const char *bz_cf;

	    int connected( char *&err );
	    int no_privs( char *&err );
	    struct DTGStrList *get_options( const char *table, char *&err );
	    struct DTGField *single_row( const char *query, char *&err );
	    struct DTGStrList *single_col( const char *query, char *&err );
	    struct DTGField *two_cols( const char *query );
	    char *get_description( const char *bugid, char *&err );
	    void append_fix( const char *defect, const char *fix, 
				int stamped, char *&err );

	    MYSQL *mysql;
	    char *use_profile;
	    struct DTGField *field_map;
	    struct DTGField *profile_map;
	    struct DTGField *product_map;
	    struct DTGField *component_map;

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
	    struct DTGFieldDesc *get_field_desc( const char *defect, 
		char *&err );
	    struct DTGStrList *get_prod_components( const char *product, 
							char *&err );
	    char *get_server_version( char *&err );
	    char *get_server_date( int offset, char *&err );
	    struct DTGStrList *list_jobs( int max_rows, 
		struct DTGDate *since, const char *mod_date_field,
		const char *exclude_user, const char *mod_by_field,
		const char *segment_filters, char *&err );
	    struct DTGField *get_defect( const char *defect, char *&err );

	    void insert_activity(const char *now, const char *qvalue, const char *qdefect, const char *filedid, char *&err);
	    void split_and_send(char *val, const char *defect, const char *filedid, char *&err);
	    char *save_defect( const char *defect,
				struct DTGField *fields, char *&err );
	    char* esc_field( const char* fld );
	    struct DTGField *field_names();
	    const char *find_value( struct DTGField *fields, const char *id );
	    struct DTGField *product_names();
	    char *component_name( const char * comp_name, const char *prod_id );
};

#endif
