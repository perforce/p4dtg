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

#include "MyDTG.h"
#include "MyDTS.h"
extern "C" {
#include <dtg-utils.h>
}
#include <dtg-str.h>

const char *MyDTGDefect::MyDTGMagic = "MyDTGDefectClass";

MyDTGDefect::MyDTGDefect( MyDTGProj *proj,
	                const char *in_defect,
	                struct DTGStrList *ref_fields,
	                struct DTGError *error  )
{
	magic = MyDTGMagic;
	in_proj = proj;
	fields = NULL;
	changes = NULL;
	defect = mk_string( in_defect );
	if( !proj || !defect )
	{
	    set_DTGError( error,
			"MyDTGDefect::MyDTGDefect: Undefined arguments");
	    return;
	}
	dirty = !strcasecmp( defect, "new" );
	testing = in_proj->testing;
	if( testing )
	{
	    testing = 1;
	    clear_DTGError( error );
	    return;
	}

	char *err = NULL;
	fields = in_proj->in_dt->dts->get_defect( defect, ref_fields, err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_proj->in_dt->dts->is_valid();
	    delete[] err;
	    return;
	}

	clear_DTGError( error );
}

MyDTGDefect::~MyDTGDefect()
{
	if( fields )
	    delete_DTGField( fields );
	if( changes )
	    delete_DTGField( changes );
	if( defect )
	    delete[] defect;
}

struct DTGField *MyDTGDefect::get_fields( struct DTGError *error )
{
	struct DTGField *list = NULL;
	if( testing )
	{
	    list = new_DTGField( "*name*", "*value*" );
	    clear_DTGError( error );
	    return list;
	}

	if( !fields )
	{
	    set_DTGError( error,
			"MyDTGDefect::get_fields: Currently not implemented");
	    return NULL;
	}
	for( struct DTGField *f = fields; f; f = f->next )
	   list = append_DTGField( list, new_DTGField( f->name, f->value ) );

	// Create the "Status/Resolution" field.
	struct DTGField *f;
	struct DTGField *s = NULL, *r = NULL;
	for( f = fields; f; f = f->next )
	    if( !strcasecmp( "Status", f->name ) )
	        s = f;
	    else if( !strcasecmp( "Resolution", f->name ) )
	        r = f;
	char *tmp;
	if( s )
	{
	    if( r && r->value && *r->value )
	        tmp = mk_string( s->value, "/", r->value );
	    else
	        tmp = mk_string( s->value );
	    list = append_DTGField( list,
				    new_DTGField( "Status/Resolution", tmp ));
	    delete[] tmp;
	}

	// Update for changes

	clear_DTGError( error );
	return list;
}

static const char *VALUE_CONST = "*value*";

char *MyDTGDefect::get_field( const char *name, struct DTGError *error )
{
	if( testing )
	    if( name && !strcasecmp( name, "*name*" ) )
	    {
	        clear_DTGError( error );
	        return strdup( VALUE_CONST );
	    }
	    else
	    {
	        set_DTGError( error,
				"MyDTGDefect::get_field: Field not defined");
	        return NULL;
	    }

	if( !fields )
	{
	    set_DTGError( error,
				"MyDTGDefect::get_field: Fields not defined");
	    return NULL;
	}

	clear_DTGError( error );

	if( !strcasecmp( "Fixes", name ) )
	{
	    set_DTGError( error, "\"Fixes\" is a special field, and can only be"
	                  " the destination for \"Fix Details\"." );
	    error->can_continue = 0;
	    return NULL;
	}

	struct DTGField *f;

	if( !strcasecmp( "Status/Resolution", name ) )
	{
	    struct DTGField *s = NULL, *r = NULL;
	    for( f = changes; f; f = f->next )
	        if( !strcasecmp( "Status", f->name ) )
	            s = f;
	        else if( !strcasecmp( "Resolution", f->name ) )
	            r = f;
	    for( f = fields; (!s || !r) && f; f = f->next )
	        if( !s && !strcasecmp( "Status", f->name ) )
	            s = f;
	        else if( !r && !strcasecmp( "Resolution", f->name ) )
	            r = f;
	    char *ret = NULL;
	    if( r && r->value && strlen( r->value ) )
	    {
	        char *tmp = mk_string( s->value, "/", r->value );
	        ret = strdup( tmp );
	        delete[] tmp;
	    }
	    else
	    {
	        if( s && s->value )
	            ret = strdup( s->value );
	    }

	    return ret;
	}

	for( f = changes; f; f = f->next )
	    if( !strcasecmp( f->name, name ) )
	        return f->value ? strdup( f->value ) : NULL;
	for( f = fields; f; f = f->next )
	    if( !strcasecmp( f->name, name ) )
	        return f->value ? strdup( f->value ) : NULL;
	return NULL;
}

void MyDTGDefect::set_field( const char *name, const char *value,
	                    struct DTGError *error )
{
	if( testing )
	{
	    if( name && value && !strcasecmp( name, "*name*" ) )
	        clear_DTGError( error );
	    else
	        set_DTGError( error,
				"MyDTGDefect::set_field: Field not defined");
	    return;
	}

	if( !fields )
	{
	    set_DTGError( error, "MyDTGDefect::set_field: Fields not defined");
	    return;
	}

	if( !strcasecmp( "Status/Resolution", name ) )
	{
	    // location of the slash.
	    const char *sl = strchr( value, '/' );
	    char *sv, *rv;
	    if( sl )
	    {
	        // the "Status" value.
	        sv = (char*)malloc( sl - value + 1 );
	        strncpy( sv, value, sl - value );
	        sv[ sl - value ] = '\0';
	        // the "Resolution" value.
	        rv = strdup( sl + 1 );
	    }
	    else // Status only, no resolution
	    {
	        sv = strdup( value );
	        rv = strdup( "" );;
	    }

	    int status_found = 0;
	    int resolution_found = 0;
	    for( struct DTGField *f = changes; f; f = f->next )
	        if( !strcasecmp( "Status", f->name ) )
	        {
	            if( f->value )
	                free( f->value );
	            f->value = sv;
	            dirty = 1;
	            status_found++;
	        }
	        else if( !strcasecmp( "Resolution", f->name ) )
	        {
	            if( f->value )
	                free( f->value );
	            f->value = rv;
	            dirty = 1;
	            resolution_found++;
	        }

	    if( !status_found )
	    {
	        struct DTGField *f = new_DTGField( "Status", sv );
	        free( sv );
	        f->next = changes;
	        changes = f;
	        dirty = 1;
	    }
	    if( !resolution_found )
	    {
	        struct DTGField *f = new_DTGField( "Resolution", rv );
	        free( rv );
	        f->next = changes;
	        changes = f;
	        dirty = 1;
	    }
	    clear_DTGError( error );
	    return;
	}

	struct DTGField *f;
	for( f = changes; f; f = f->next )
	    if( !strcasecmp( f->name, name ) )
	    {
	        if( f->value )
	            free( f->value );
	        f->value = strdup( value ); // struct DTGField uses free()
	        dirty = 1;
	        break;
	    }
	if( !f )
	{
	    f = new_DTGField( name, value );
	    f->next = changes;
	    changes = f;
	    dirty = 1;
	}

	clear_DTGError( error );
	return;
}

static const char *NAME_CONST = "*defect*";

char *MyDTGDefect::save( DTGError *error )
{
	if( testing )
	{
	    clear_DTGError( error );
	    return strdup( NAME_CONST );
	}

	if( !fields )
	{
	    set_DTGError( error, "MyDTGDefect::save: Fields not defined");
	    return NULL;
	}

	if( dirty )
	{
	    char *err = NULL;
	    char *name =
		in_proj->in_dt->dts->save_defect( defect, changes, err );
	    if( defect )
	        delete[] defect;
	    defect = name;
	    if( err )
	    {
	        set_DTGError( error, err );
	        error->can_continue = in_proj->in_dt->dts->is_valid();
	        delete[] err;
	    }
	    else
	        dirty = 0;
	}
	return defect ? strdup( defect ) : NULL;
}
