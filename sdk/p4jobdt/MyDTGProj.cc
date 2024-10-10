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
#include <p4/strtable.h>
#include <dtg-str.h>
#include "p4charcvt.h"

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

	testing = dt->testing;
}

MyDTGProj::~MyDTGProj()
{
	if( project )
	    delete[] project;
	if( seg_filters )
	    delete[] seg_filters;
} 

char *MyDTGProj::translate( const char *str, int sm, char *&err, 
				int rev, int cpp )
{
	if( !str )
	    return NULL;

	char *result;
	if( rev )
	    result = p4charcvt( "utf8", in_dt->charset, str, strlen( str ), 
				sm, err );
	else
	    result = p4charcvt( in_dt->charset, "utf8", str, strlen( str ), 
				sm, err );
	if( !result || cpp )
	    return result; // new/delete variant
	char *tmp = strdup( result );
	delete[] result;
	return tmp;
}

struct DTGFixDesc *
MyDTGProj::translate( struct DTGFixDesc *fix, struct DTGError *error )
{
	if( !fix )
	    return NULL;

	clear_DTGError( error );
	char *err = NULL;
	// fix->change, fix->stamp need no translation
	char *tmp = fix->user;
	fix->user = translate( tmp, in_dt->sub_missing, err, 0, 0 );
	if( err )
	{
	    fix->user = tmp;
	    tmp = mk_string( "User: ", fix->user, " - ", err );
	    set_DTGError( error, tmp );
	    delete[] tmp;
	    delete[] err;
	    delete_DTGFixDesc( fix );
	    return NULL;
	}
	tmp = fix->desc;
	fix->desc = translate( tmp, in_dt->sub_missing, err, 0 );
	if( err )
	{
	    fix->desc = tmp;
	    tmp = mk_string( "Description: ", fix->desc, " - ", err );
	    set_DTGError( error, tmp );
	    delete[] tmp;
	    delete[] err;
	    delete_DTGFixDesc( fix );
	    return NULL;
	}
	fix->files = translate( fix->files, error );
	if( error->message )
	{
	    tmp = mk_string( "Files: ", error->message );
	    set_DTGError( error, tmp );
	    delete[] tmp;
	    delete_DTGFixDesc( fix );
	    return NULL;
	}
	return fix;
}

StrDict *
MyDTGProj::translate( StrDict *fields, struct DTGError *error, int rev )
{
	clear_DTGError( error );
	StrRef var, val;
	StrDict *d = new StrBufDict();
	char *name;
	char *value;
	char *err;
	for( int i = 0; fields->GetVar( i, var, val ); i++ )
	{
	    err = NULL;
	    name = translate( var.Text(), 0, err, rev );
	    if( err )
	    {
	        char *tmp = mk_string( "FieldName: ", var.Text(), " - ", err );
	        set_DTGError( error, tmp );
	        delete[] tmp;
	        delete[] err;
	        delete d;
	        return NULL;
	    }
	    value = translate( val.Text(), in_dt->sub_missing, err, rev );
	    if( err )
	    {
	        char *tmp = mk_string( "FieldValue: ", var.Text(), " - ", err );
	        set_DTGError( error, tmp );
	        delete[] tmp;
	        delete[] err;
	        delete d;
	        return NULL;
	    }
	    d->SetVar( name, value );
	    delete[] name;
	    delete[] value;
	}
	return d;
}

struct DTGStrList *
MyDTGProj::translate( struct DTGStrList *list, struct DTGError *error, int rev )
{
	char *option;
	char *err;
	clear_DTGError( error );
	for( struct DTGStrList *item = list; item; item = item->next )
	{
	    err = NULL;
	    option = item->value;
	    item->value = translate( option, 0, err, rev, 0 ); // no sub_missing
	    if( !item->value )
	    {
	        item->value = option;
	        char *tmp = mk_string( "Value: ", option, " - ", err );
	        set_DTGError( error, tmp );
	        delete[] tmp;
	        delete[] err;
	        delete_DTGStrList( list );
	        return NULL;
	    }
	    free( option );
	}
	return list;
}

struct DTGFieldDesc *
MyDTGProj::translate( struct DTGFieldDesc *list, struct DTGError *error, int r )
{ 
	char *name;
	char *err;
	clear_DTGError( error );
	for( struct DTGFieldDesc *field = list; field; field = field->next )
	{
	    err = NULL;
	    name = field->name;
	    field->name = translate( name, 0, err, r, 0 ); // no sub_missing
	    if( !field->name )
	    {
	        field->name = name;
	        char *tmp = mk_string( "Field: ", name, " - ", err );
	        set_DTGError( error, tmp );
	        delete[] tmp;
	        delete[] err;
	        delete_DTGFieldDesc( list );
	        return NULL;
	    }
	    free( name );
	    field->select_values = translate( field->select_values, error, r );
	    if( error->message )
	    {
	        char *tmp = mk_string( "Field: ", field->name, " - ", 
					error->message );
	        set_DTGError( error, tmp );
	        delete[] tmp;
	        delete_DTGFieldDesc( list );
	        return NULL;
	    }
	}
	return list;
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
	StrDict *dict = in_dt->dts->get_form( "jobspec", NULL, err );
	if( !dict || err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    if( dict )
	        delete dict;
	    return NULL;
	}
	list = process_jobspec( dict );
	delete dict;
	if( in_dt->charset )
	    list = translate( list, error );
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
	if( in_dt->charset )
	{
	    char *mdf = translate( mod_date_field, 0, err, 1 );
	    delete[] err; err = NULL; // Ignore errors
	    char *mbf = translate( mod_by_field, 0, err, 1 );
	    delete[] err; err = NULL; // Ignore errors
	    char *eu = translate( exclude_user, 0, err, 1 );
	    delete[] err; err = NULL; // Ignore errors
	    char *sf = translate( seg_filters, 0, err, 1 );
	    delete[] err; err = NULL; // Ignore errors
	    list = in_dt->dts->list_jobs( max_rows, 
					since, mdf,
					eu, mbf, 
					sf,
					err );
	    delete[] mdf;
	    delete[] mbf;
	    delete[] eu;
	    delete[] sf;
	}
	else
	    list = in_dt->dts->list_jobs( max_rows, 
					since, mod_date_field,
					exclude_user, mod_by_field, 
					seg_filters,
					err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    delete_DTGStrList( list );
	    return NULL;
	}

	if( in_dt->charset )
	    list = translate( list, error );

	return list;
}

MyDTGDefect *MyDTGProj::get_defect( const char *defect, struct DTGError *error )
{
	if( defect && *defect == '-' )
	{
	    char *msg = mk_string( "Unsupported defect id: ", defect );
	    set_DTGError( error, msg );
	    delete[] msg;
	    error->can_continue = 1;
	    return NULL;
	}

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
	MyDTGDefect *item = new MyDTGDefect( this, "new", error );
	if( error->message )
	{
	    delete item;
	    item = NULL;
	}
	return item;
}

int MyDTGProj::is_fix_pending( const char *fixid, struct DTGError *error )
{
	char *err = NULL;
	StrDict *dict = in_dt->dts->get_form( "change", fixid, err );
	if( !dict || err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    if( dict )
	        delete dict;
	    return -1;
	}
	StrPtr *val = dict->GetVar( "Status" );
	int pending;
	if( val )
	    if( !strcasecmp( val->Text(), "pending" ) )
	        pending = 1;
	    else
	        pending = 0;
	else
	    pending = -1;
	delete dict;
	return pending;
}

struct DTGStrList *MyDTGProj::list_fixes( const char *defect, 
					struct DTGError *error )
{
	struct DTGStrList *list = NULL;
	if( testing ) 
	{
	    list = new_DTGStrList( "*change*" );
	    clear_DTGError( error );
	    return list;
	}

	char *err = NULL;
	if( in_dt->charset )
	{
	    char *did = translate( defect, 0, err, 1 );
	    delete[] err; err = NULL; // ignore err
	    list = in_dt->dts->list_fixes( did, err );
	    delete[] err;
	    delete[] did;
	}
	else
	    list = in_dt->dts->list_fixes( defect, err );
	if( err )
	{
	    set_DTGError( error, err );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    delete_DTGStrList( list );
	    return NULL;
	}

	struct DTGStrList *fix;
	clear_DTGError( error );
	for( fix = list; fix; fix = fix->next )
	{
	    if( is_fix_pending( fix->value, error ) )
	        fix->value[0] = '\0';
	    if( error->message )
	    {
	        delete_DTGStrList( list );
	        return NULL;
	    }
	}
	list = purge_DTGStrList( list );

	return list;
}

struct DTGFixDesc *MyDTGProj::describe_fix( const char *fixid, 
					struct DTGError *error )
{
	struct DTGFixDesc *fix = NULL;
	if( testing ) 
	{
	    fix = new_DTGFixDesc();
	    clear_DTGError( error );
	    return fix;
	}

	char *err = NULL;
	fix = in_dt->dts->describe_fix( fixid, err );
	if( !fix || err )
	{
	    if( err )
	        set_DTGError( error, err );
	    else
	        set_DTGError( error, "Unknown error from describe_fix" );
	    error->can_continue = in_dt->dts->is_valid();
	    delete[] err;
	    if( fix )
	        delete_DTGFixDesc( fix );
	    return NULL;
	}
	clear_DTGError( error );
	if( in_dt->charset )
	    fix = translate( fix, error );
	return fix;
}

struct DTGStrList * MyDTGProj::find_defects( int limit, const char *qual, struct DTGError *error )
{
	struct DTGStrList *list;
	if( testing ) 
	{
	    list = new_DTGStrList( "*defect*" );
	    clear_DTGError( error );
	    return list;
	}
	char *err = NULL;
	if( in_dt->charset )
	{
	    char *q = translate( qual, 0, err, 1 );
	    delete[] err; err = NULL; // ignore err
	    list = in_dt->dts->list_jobs( limit, q, err );
	    delete[] q;
	}
	else
	    list = in_dt->dts->list_jobs( limit, qual, err );
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

void MyDTGProj::referenced_fields( struct DTGStrList *fields )
{
	char *tmp = join_DTGStrList( fields, ", " );
	if( in_dt->cur_message )
	    delete[] in_dt->cur_message;
	in_dt->cur_message = mk_string( "Referenced fields are: ", tmp );
	in_dt->cur_message_level = 3;
	free( tmp );
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
	    delete[] sep;
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
