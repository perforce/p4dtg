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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "MyDTS.h"
#include "MyDTG.h"
extern "C" {
#include <dtg-utils.h>
}
#include <dtg-str.h>

const char *MyDTGProj::MyDTGMagic = "MyDTGProjClass";

MyDTGProj::MyDTGProj( MyDTG *dt, const char *proj, struct DTGError *error  )
{
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
}

// When updating field-processing here, keep extract_filter_string() in sync.
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

	// Synthesize "Product/Component" "select" field.

	// Find fields to combine, copy select values
	// Leave fields for read-only access
	struct DTGFieldDesc *cur = NULL, *prev = NULL;
	struct DTGStrList   *sv  = NULL, *rv   = NULL;
	for( cur = list; cur; cur = cur->next )
	    if( !strcmp( cur->name, "Product" ) )
	    {
	        sv = copy_DTGStrList( cur->select_values );
	        break;
	    }

	// Create a new select list. E.g. "PROD1/COMP11", "PROD1/COMP12".
	struct DTGStrList *srl = NULL, *rvt = NULL;
	struct DTGStrList *top_sv = sv;
	for( sv; sv; sv = sv->next )
	{
	    char *err = NULL;
	    rv = in_dt->dts->get_prod_components( sv->value, err );
	    if( err )
	    {
	        set_DTGError( error, err );
	        error->can_continue = in_dt->dts->is_valid();
	        delete[] err;
	        delete_DTGFieldDesc( list );
	        delete_DTGStrList( top_sv );
	        delete_DTGStrList( rv );
	        return NULL;
	    }
	    if( !rv )
	        srl = append_DTGStrList( srl, sv->value );
	    else
	    {
	        for( rvt = rv; rvt; rvt = rvt->next )
	        {
	            // Skip empty values.
	            if( !rvt->value[0] )
                        continue;
	            char *tmp = mk_string( sv->value, "/", rvt->value );
	            srl = append_DTGStrList( srl, tmp );
	            delete[] tmp;
	        }
	        delete_DTGStrList( rv );
	    }
	}

	struct DTGFieldDesc *sr = new_DTGFieldDesc( "Product/Component",
	                                            "select", 1, srl );
	sr->next = list;
	list = sr;

	delete_DTGStrList( top_sv );

	// Synthesize "Status/Resolution" "select" field.

	// Find fields to combine, copy select values
	// Leave fields for read-only access
	cur = NULL, prev = NULL;
	sv  = NULL, rv   = NULL;
	for( cur = list; cur; cur = cur->next )
	{
	    if( !strcmp( cur->name, "Resolution" ) ||
	        !strcmp( cur->name, "Status" ) )
	    {
	        if( cur->name[0] == 'R' )
	            rv = copy_DTGStrList( cur->select_values );
	        else
	            sv = copy_DTGStrList( cur->select_values );
	    }
	}

	// Create a new select list. E.g. "RESOLVED/FIXED", "RESOLVED/INVALID".
	// Loops over "Status", then "Resolution".
	srl = NULL, rvt = rv;
	top_sv = sv;
	for( sv; sv; sv = sv->next )
	{
	    char *s = mk_string( ",", sv->value, "," ) ;
	    if( strstr( in_dt->closed_states, s ) )
	        for( int j = 0; rvt; rvt = rvt->next, j++ )
	        {
	            // Skip empty values.
	            if( !rvt->value[0] )
                        continue;
	            char *tmp = mk_string( sv->value, "/", rvt->value );
	            srl = append_DTGStrList( srl, tmp );
	            delete[] tmp;
	            if( !rvt->next )
	            {
	                rvt = rv;
	                break;
	            }
	        }
	    else
	        srl = append_DTGStrList( srl, sv->value );
	    delete[] s;
	}

	sr = new_DTGFieldDesc( "Status/Resolution", "select", 0, srl );
	sr->next = list;
	list = sr;

	delete_DTGStrList( top_sv );
	delete_DTGStrList( rv );

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
					seg_filters, err );
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
	MyDTGDefect *item = new MyDTGDefect( this, defect, error );
	if( error->message )
	{
	    delete item;
	    item = NULL;
	}
	return item;
}

MyDTGDefect *MyDTGProj::new_defect( struct DTGError *error )
{
	const char *err = mk_string( "The ", in_dt->get_name( error ),
	                             " plugin does ",
	                             "not support creating new defects." );
	set_DTGError( error, err );
	delete[] err;
	return NULL;
}

/*
	Takes the list_fields() output and decomposes it into an SQL query to be
	appended to what MyDTS::list_jobs() uses.

	It's safe to only query these fields once and not worry about new fields
	being created in Bugzilla since DTG segmentation is based off of
	pre-existing fields.

	The field names passed in are not Bugzilla database table names, so they
	must be converted.  E.g. 'OS/Version' -> `op_sys`
*/
static char *extract_filter_string( MyDTG *in_dt, struct DTGFieldDesc *f )
{
	struct DTGField *field_names = in_dt->dts->field_names(),
	                *products = in_dt->dts->product_names();
	char *query = NULL, *tmp = NULL;

	for( ; f; f = f->next )
	{
	    if( !f->select_values )
	    {
	        delete[] query;
	        delete_DTGField( field_names );
	        delete_DTGField( products );
	        return NULL;
	    }

	    // DTG pseudo-fields are no-ops.  E.g. DTGConfig-User
	    if( !strncmp( "DTG", f->name, 3 ) )
	        continue;

	    // E.g. f->name = 'Severity', f->select_values = trivial,enhancement
	    for( struct DTGStrList *i = f->select_values; i; i = i->next )
	    {
	        char *tmp1 = NULL;

	        // Single-field Status/Resolution values are Status, and
	        // select_values are strings and sql-safe.
	        if( !strcmp( f->name, "Status/Resolution" ) )
	        {
	            char *stat_val = i->value, *res_val = NULL;
	            char *idx = strchr( stat_val, '/' );
	            if( idx )
	            {
	                *idx = '\0';
	                res_val = idx + 1;
	            }
	            const char *res_tab = in_dt->dts->find_value( field_names,
	                                                          "Resolution");
	            const char *stat_tab = in_dt->dts->find_value( field_names,
	                                                           "Status" );
	            tmp1 = mk_string( "`", stat_tab, "` = '", stat_val, "'",
	                              " AND `", res_tab, "` = '", res_val, "'");
	            tmp = mk_string( tmp, tmp1 );
	            delete[] tmp1;
	            if( idx )
	                *idx = '/';
	        }
	        else if( !strcmp( f->name, "Product/Component" ) )
	        {
	            char *prod_name = i->value, *comp_name = NULL;
	            char *idx = strchr( prod_name, '/' );

	            if( idx )
	            {
	                *idx = '\0';
	                comp_name = idx + 1;
	            }
	            // These aren't in the field map, so hard-code them.
	            const char *prod_tab = "product_id";
	            const char *comp_tab = "component_id";

	            const char *prod_val = in_dt->dts->find_value( products,
	                                                           prod_name );
	                  char *comp_val = in_dt->dts->component_name(
	                                                 comp_name, prod_val );
	            tmp1 = mk_string( "`", prod_tab, "` = ", prod_val, " AND `",
	                              comp_tab, "` = ", comp_val );
	            free( comp_val );
	            tmp = mk_string( tmp, tmp1 );
	            delete[] tmp1;
	            if( idx )
	                *idx = '/';
	        }
	        else if( !strcmp( f->name, "Product" ) )
	        {
	            // These aren't in the field map, so hard-code them.
	            const char *prod_val = in_dt->dts->find_value( products,
	                                                           i->value );
	            tmp1 = mk_string( tmp, "`product_id` = ", prod_val );
	            delete[] tmp;
	            tmp = tmp1;
	        }
	        else
	        {
	            // These fields are more free-form, so we escape them.
	            char *esc = in_dt->dts->esc_field( i->value );
	            tmp1 = mk_string( tmp, "`", in_dt->dts->find_value(
	                             field_names, f->name ), "` = '", esc, "'");
	            delete[] tmp;
	            tmp = tmp1;
	            free( esc );
	        }

	        if( i->next )
	        {
	            tmp1 = mk_string( tmp, " OR " );
	            delete[] tmp;
	            tmp = tmp1;
	        }
	    }

	    char *subquery = mk_string( "( ", tmp, " )" );
	    delete[] tmp;
	    tmp = mk_string( query, " AND ", subquery );
	    delete[] subquery;
	    delete[] query;
	    query = tmp;
	}
	delete_DTGField( field_names );
	delete_DTGField( products );
	return query;
}

void MyDTGProj::segment_filters( struct DTGFieldDesc *filters )
{
	delete[] seg_filters;
	seg_filters = extract_filter_string( in_dt, filters );
	if( in_dt->cur_message )
	    delete[] in_dt->cur_message;
	in_dt->cur_message = mk_string( "Segment filter is: ", seg_filters );
	in_dt->cur_message_level = 1;
}

