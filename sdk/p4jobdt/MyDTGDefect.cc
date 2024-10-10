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
	                struct DTGError *error  )
{
	magic = MyDTGMagic;
	in_proj = proj;
	fields = NULL;
	defect = mk_string( in_defect );
	translated = 0;
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
	if( in_proj->in_dt->charset )
	{
	    char *d = in_proj->translate( defect, 0, err, 1 );
	    delete[] err; err = NULL; // ignore errors
	    fields = in_proj->in_dt->dts->get_defect( d, err );
	    delete[] d;
	}
	else
	    fields = in_proj->in_dt->dts->get_defect( defect, err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_proj->in_dt->dts->is_valid();
	    delete[] err;
	    return;
	}

	clear_DTGError( error );

	if( in_proj->in_dt->charset )
	{
	    StrDict *f = fields;
	    fields = in_proj->translate( fields, error );
	    if( !error->message )
	    {
	        delete f;
	        translated = 1;
	        return;
	    }
	    fields = f;
	}
}

MyDTGDefect::~MyDTGDefect()
{
	if( fields )
	    delete fields;
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
	StrRef var, val;
	for( int i = 0; fields->GetVar( i, var, val ); i++ )
	   list = append_DTGField( list, 
				new_DTGField( var.Text(), val.Text() ) );
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
	StrPtr *val = fields->GetVar( name );
	return val ? strdup( val->Text() ) : NULL;
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

	if( !name )
	{
	    set_DTGError( error, "set_field: Field name not defined");
	    return;
	}

	if( !strcasecmp( name, "Job" ) && value && *value == '-' )
	{
	    char *msg = mk_string( "Invalid Job id: ", value );
	    set_DTGError( error, msg );
	    delete[] msg;
	    return;
	}

	if( in_proj->in_dt->charset && !translated && 
		strcasecmp( name, "DTG_ERROR" ) )
	{
	    char *msg = mk_string( "Only DTG_ERROR may be set" );
	    set_DTGError( error, msg );
	    delete[] msg;
	    return;
	}


	fields->RemoveVar( name );
	fields->SetVar( name, value );
	dirty = 1;
	clear_DTGError( error );
	return;
}

static const char *NAME_CONST = "*defect*";

void MyDTGDefect::set_jobid()
{
	if( !in_proj->in_dt->gen_jobname || !in_proj->in_dt->mapid )
	    return;

	StrPtr *val = fields->GetVar( "Job" );
	if( !val || strcmp( val->Text(), "new" ) )
	    return;

	val = fields->GetVar( "DTG_DTISSUE" );
	if( !val )
	    return;

	char *name = mk_string( val->Text(), "-", in_proj->in_dt->mapid );
	fields->RemoveVar( "Job" );
	fields->SetVar( "Job", name );
	delete[] name;
}

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
	    char *name = NULL;
	    set_jobid();
	    if( translated )
	    {
	        StrDict *f = in_proj->translate( fields, error, 1 );
	        if( error->message )
	            err = cp_string( error->message );
	        else
	        {
	            char *n = in_proj->in_dt->dts->save_defect( defect, f, err);
	            char *terr = NULL;
	            name = in_proj->translate( n, 0, terr );
	            delete[] n;
	            delete[] terr; terr = NULL;
	            /* If name translate fails, its has been saved, so ... */
	            /* ... pretty much have to try to recover.             */
	            if( !name )
	                name = cp_string( defect );
	        }
	        delete f;
	    }
	    else
	        name = in_proj->in_dt->dts->save_defect( defect, fields, err );
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
