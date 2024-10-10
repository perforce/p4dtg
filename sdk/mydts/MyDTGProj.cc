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
#include <string.h>
#include <time.h>

#include "MyDTS.h"
#include "MyDTG.h"
extern "C" {
#include <dtg-utils.h>
}
#include <dtg-str.h>
#include <cstdlib>

const char *MyDTGProj::MyDTGMagic = "MyDTGProjClass";

MyDTGProj::MyDTGProj( MyDTG *dt, const char *proj, struct DTGError *error  )
{
	ref_fields = NULL;
	magic = MyDTGMagic;
	in_dt = dt;
	seg_filters = NULL;
	clear_DTGError( error );
	if( !dt || !proj )
	{
	    set_DTGError( error, "MyDTGProj::MyDTGProj: Undefined arguments" );
	    return;
	}
	project = mk_string( proj );
	char *err = NULL;
	in_dt->dts->connect_to_project( project, err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    return;
	}
	testing = dt->testing;
}

MyDTGProj::~MyDTGProj()
{
	if( project )
	    delete[] project;
	if( seg_filters )
	    delete[] seg_filters;
	delete_DTGStrList( ref_fields );
}

struct DTGFieldDesc *MyDTGProj::list_fields( struct DTGError *error )
{
	struct DTGFieldDesc *list;
	clear_DTGError( error );
	if( testing )
	{
	    list = new_DTGFieldDesc( "JobID", "word", 1, NULL );
	    return list;
	}

	char *err = NULL;
	list = in_dt->dts->get_field_desc( project, err );
	if( !list || err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    delete_DTGFieldDesc( list );
	    return NULL;
	}
	return list;
}

struct DTGStrList *MyDTGProj::list_changed_defects( int max_rows,
						  struct DTGDate *since,
	                                          const char *mod_date_field,
	                                          const char *mod_by_field,
	                                          const char *exclude_user,
	                                          struct DTGError *error )
{
	struct DTGStrList *list;
	if( testing )
	{
	    list = new_DTGStrList( "*defect*" );
	    clear_DTGError( error );
	    return list;
	}
	char *err = NULL;
	list = in_dt->dts->list_jobs( max_rows,
					since, mod_date_field,
					exclude_user, mod_by_field,
					err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    delete_DTGStrList( list );
	    return NULL;
	}

	return list;
}

MyDTGDefect *MyDTGProj::get_defect( const char *defect, struct DTGError *error )
{
	MyDTGDefect *item = new MyDTGDefect( this, defect, ref_fields, error );
	if( error->message )
	{
	    delete item;
	    item = NULL;
	}
	return item;
}

MyDTGDefect *MyDTGProj::new_defect( struct DTGError *error )
{
	MyDTGDefect *item = new MyDTGDefect( this, "new", ref_fields, error );
	if( error->message )
	{
	    delete item;
	    item = NULL;
	}
	return item;
}

void MyDTGProj::referenced_fields( struct DTGStrList *fields )
{
	delete_DTGStrList( ref_fields );
	ref_fields = copy_DTGStrList( fields );
}

static char *extract_filter_string( struct DTGFieldDesc *f )
{
	/*
		f := {
			{ name = x, select_values = { a, b } },
			{ name = y, select_values = { c, d } }
		}

			maps to

		"(x=a|x=b) (y=c|y=d)"
		Note: space is a low precedence AND
	*/
	struct DTGStrList *v = NULL;
	char *query = NULL;
	for(; f; f = f->next )
	{
	    if( !f->select_values )
	    {
	        delete[] query;
	        return NULL;
	    }
	    char *sep = mk_string( "|", f->name, "=" );
	    char *tmp = join_DTGStrList( f->select_values, sep );
	    char *subquery = mk_string( "(", f->name, "=", tmp, ")" );
	    free( tmp );
	    tmp = mk_string( query, " ", subquery );
	    delete[] subquery;
	    delete[] query;
	    query = tmp;
	}
	return query;
}

void MyDTGProj::segment_filters( struct DTGFieldDesc *filters )
{
	delete[] seg_filters;
	seg_filters = extract_filter_string( filters );
	if( in_dt->cur_message )
	    delete[] in_dt->cur_message;
	in_dt->cur_message = mk_string( "Segment filter is: ", seg_filters );
	in_dt->cur_message_level = 1;
}
