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

#ifndef MYDTG_HEADER
#define MYDTG_HEADER

extern "C" {
#include <DTG-interface.h>
}

class MyDTS;
class MyDTGProj;
class MyDTGDefect;

class MyDTG {
    public:
	static const char *MyDTGMagic;
	const char *magic;
	int testing;
	char *server_version;
	MyDTS *dts;
	char *cur_message;
	int cur_message_level;
	struct DTGStrList *proj_list;

	int allow_creation;

	MyDTG( const char *server, const char *user, const char *pass,
	      const struct DTGField *attrs, struct DTGError *error  );
	~MyDTG();

	static MyDTG *convert( void *obj )
	{
	    MyDTG *me = (MyDTG *)obj;
	    if( me && me->magic == MyDTGMagic )
	        return me;
	    else
	        return NULL;
	}

	static struct DTGDate *extract_date( const char *date_string );
	static char *format_date( struct DTGDate *date );

	static struct DTGAttribute *list_attrs();
	static char *validate_attr( const struct DTGField *attr );

	static const char *get_name( struct DTGError *error );
	static const char *get_module_version( struct DTGError *error );

	int accept_utf8();
	int server_offline( struct DTGError *error );
	const char *get_server_version( struct DTGError *error );
	char *get_server_warnings( struct DTGError *error );
	int get_message( struct DTGError *error );
	struct DTGDate *get_server_date( struct DTGError *error );
	struct DTGStrList *list_projects( struct DTGError *error );
	MyDTGProj *get_project( const char *project, struct DTGError *error );
};

class MyDTGProj {
    public:
	static const char *MyDTGMagic;
	const char *magic;
	int testing;
	char *project;
	MyDTG *in_dt;
	struct DTGStrList *ref_fields;
	char *seg_filters;
	char *proj_names;

	MyDTGProj( MyDTG *dt, const char *project, struct DTGError *error );
	~MyDTGProj();

	static MyDTGProj *convert( void *obj )
	{
	    MyDTGProj *me = (MyDTGProj *)obj;
	    if( me && me->magic == MyDTGMagic )
	        return me;
	    else
	        return NULL;
	};

	struct DTGFieldDesc *list_fields( struct DTGError *error );
	struct DTGStrList *list_changed_defects( int max_rows,
						struct DTGDate *since,
						const char *mod_date_field,
						const char *mod_by_field,
						const char *exclude_user,
	                                        struct DTGError *error );
	MyDTGDefect *get_defect( const char *defect, struct DTGError *error );
	MyDTGDefect *new_defect( struct DTGError *error );
	
	void referenced_fields( struct DTGStrList *fields );
	void segment_filters( struct DTGFieldDesc *filters );
};

class MyDTGDefect {
    public:
	static const char *MyDTGMagic;
	const char *magic;
	int testing;
	MyDTGProj *in_proj;
	struct DTGField *fields;
	struct DTGField *changes;
	int dirty;
	char *defect;

	MyDTGDefect( MyDTGProj *proj, 
			const char *defect, 
			struct DTGStrList *ref_fields, 
			struct DTGError *error );
	~MyDTGDefect();

	static MyDTGDefect *convert( void *obj )
	{
	    MyDTGDefect *me = (MyDTGDefect *)obj;
	    if( me && me->magic == MyDTGMagic )
	        return me;
	    else
	        return NULL;
	};

	struct DTGField *get_fields( struct DTGError *error );
	char *get_field( const char *field, struct DTGError *error );
	void set_field( const char *field, const char *value,
	                struct DTGError *error );
	char *save( struct DTGError *error );
};

#endif
