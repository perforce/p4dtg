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
	defect = mk_string( in_defect );
	if( !proj || !defect )
	{
	    set_DTGError( error, 
			"MyDTGDefect::MyDTGDefect: Undefined arguments");
	    return;
	}
	dirty = !strcmp( defect, "new" );
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
	clear_DTGError( error );
	return list;
}

static const char *VALUE_CONST = "*value*";

char *MyDTGDefect::get_field( const char *name, struct DTGError *error )
{
	if( testing )
	    if( name && !strcmp( name, "*name*" ) )
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
	for( struct DTGField *f = fields; f; f = f->next )
	    if( !strcmp( f->name, name ) )
	        return f->value ? strdup( f->value ) : NULL;
	return NULL;
}

void MyDTGDefect::set_field( const char *name, const char *value,
	                    struct DTGError *error )
{
	if( testing )
	{
	    if( name && value && !strcmp( name, "*name*" ) )
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

	struct DTGField *f;
	for( f = fields; f; f = f->next )
	    if( !strcmp( f->name, name ) )
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
	    f->next = fields;
	    fields = f;
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
		in_proj->in_dt->dts->save_defect( defect, fields, err );
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
