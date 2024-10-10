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
class TcpXML;

class MyDTS {
	protected:
	    int valid;
	    int wait_time;
	    char *tcp_port;
	    char *tcp_server;
	    char *java_opts;
	    char *my_server;
	    char *my_user;
	    char *my_pass;
	    char *my_prog_name;
	    char *my_prog_ver;
	    char *my_config;
	    int defect_batch;

	    char *dtID;
	    char *projID;
	    char *defectName;
	    int sent_ref_fields;

	    TcpXML *tcp;

	    int connected( char *&err );

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
	    int server_offline( struct DTGError *error );

	    void connect_to_project( const char *proj, char *&err );
	    struct DTGStrList *list_projects( char *&err );
	    struct DTGFieldDesc *get_field_desc( char *&err );
	    char *get_server_version( char *&err );
	    char *get_server_date( char *&err );
	    void segment_filters( const char *projs, const char *segs, 
		char *&err );
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
