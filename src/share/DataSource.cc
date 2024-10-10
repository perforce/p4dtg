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
#include "DataSource.h"
#include "DataMapping.h"
#include "DataAttr.h"
#include "genutils.h"
#include <DTGModule.h>
extern "C" {
#include <dtg-utils.h>
}
#include "tinyxml.h"
#include <stdhdrs.h>
#include <strbuf.h>
#include <strops.h>

extern char *cp_string( const char *str );

static int check_field( struct DTGFieldDesc *fields, 
			const char *name, const char *type )
{
	for( struct DTGFieldDesc *f = fields; f; f = f->next )
	    if( !strcmp( f->name, name ) )
	        if( !strcasecmp( f->type, type ) && !f->readonly )
	            return 1;
	        else
	            return 0;
	return 0;
}

const char *DTG_FIELDS[] = { 
  "DTG_DTISSUE", "DTG_FIXES", "DTG_ERROR", NULL };

const char *DTG_TYPES[] = { 
  "word",        "text",      "text",      NULL };

static int check_dtg_fields( struct DTGFieldDesc *fields )
{
	if( !fields )
	    return 0;
	for( int i=0; DTG_FIELDS[i]; i++ )
	    if( !check_field( fields, DTG_FIELDS[i], DTG_TYPES[i] ) )
	        return 0;
	return 1;
}

int DataSource::has_required_fields()
{
	int has_moddate, has_modby;
	has_moddate = has_modby = 0;
	for( struct DTGFieldDesc *f = get_fields(); f; f = f->next )
	    if( !strcasecmp( "DATE", f->type ) && f->readonly == 2 )
	        has_moddate = 1;
	    else if( !strcasecmp( "WORD", f->type ) && f->readonly == 3 )
	        has_modby = 1;
	return ( has_moddate && ( type != DataSource::SCM || has_modby ) );
}

void DataSource::adj_refcnt( int amt, const char *set )
{
	refcnt += amt;
	if( set )
	{
	    FilterSet *s;
	    for( s = set_list; s && strcmp( s->name, set ); s = s->next );
	    if( s )
	        s->refcnt += amt;
	}
}

const struct DTGField *DataSource::fields()
{
	if( field_list )
	{
	    delete_DTGField( field_list );
	    field_list = NULL;
	}
	for( DataAttr *a = attrs; a; a = a->next )
	    field_list = append_DTGField( field_list, 
			new_DTGField( a->name, a->value ) );
	if( map )
	{
	    // Append map->id
	    field_list = append_DTGField( field_list, 
			new_DTGField( "DTG-MapID", map->id ) );
	    // Append name
	    field_list = append_DTGField( field_list, 
			new_DTGField( "DTG-SrcID", nickname ) );
	    // Find log_level and add it into list
	    for( DataAttr *a = map->attrs; a; a = a->next )
	        if( !strcmp( a->name, "log_level" ) )
	        {
	            field_list = append_DTGField( field_list, 
			        new_DTGField( "DTG-LogLevel", a->value ) );
	            break;
	        }
	}
	return field_list;
}

void DataSource::check_connection()
{
	if( error )
	    delete[] error;
	error = NULL;
	if( warnings )
	    delete[] warnings;
	warnings = NULL;
	if( version )
	    delete[] version;
	version = NULL;
	if( !my_mod || !server || !user )
	{
	    if( !my_mod )
	        error = cp_string( "Server plug-in not found" );
	    else if( !server )
	        error = cp_string( "Server address unspecified" );
	    else if( !user )
	        error = cp_string( "User information unspecified" );
	    status = DataSource::UNKNOWN;
	    return;
	}

        struct DTGError *err = new_DTGError( NULL );
	void *dtID = my_mod->dt_connect( server, user, password, fields(), err);

	if( err->message )
	{
	    status = DataSource::FAIL;
	    error = cp_string( err->message );
	    clear_DTGError( err );
	    my_mod->dt_free( dtID, err );
	    delete_DTGError( err );
	    return;
	}

	accept_utf8 = my_mod->dt_accept_utf8( dtID, err );
	if( err->message )
	{
	    status = DataSource::FAIL;
	    error = cp_string( err->message );
	    clear_DTGError( err );
	    my_mod->dt_free( dtID, err );
	    delete_DTGError( err );
	    return;
	}

	if( cached_modules )
	    delete_DTGStrList( cached_modules );
	cached_modules = my_mod->dt_list_projects( dtID, err );
	if( err->message )
	{
	    status = DataSource::FAIL;
	    error = cp_string( err->message );
	    clear_DTGError( err );
	    my_mod->dt_free( dtID, err );
	    delete_DTGError( err );
	    return;
	}

	status = DataSource::UNKNOWN;

	if( !module && cached_modules )
	    module = cp_string( cached_modules->value );

	if( module )
	{
	    if( cached_fields )
	        delete_DTGFieldDesc( cached_fields );
	    cached_fields = NULL;
	    void *projID = my_mod->dt_get_project( dtID, module, err );
	    if( err->message )
	    {
	        status = DataSource::FAIL;
	        error = cp_string( err->message );
	        clear_DTGError( err );
	        my_mod->dt_free( dtID, err );
	        delete_DTGError( err );
	        return;
	    }
	    cached_fields = my_mod->proj_list_fields( projID, err );
	    if( err->message )
	    {
	        status = DataSource::FAIL;
	        error = cp_string( err->message );
	        clear_DTGError( err );
	    }
	    else if( type == DataSource::SCM &&
	             check_dtg_fields( cached_fields ) )
	        status = DataSource::READY;
	    else
	        status = DataSource::PASS;
	    if( type == DataSource::SCM )
	        seg_ok = check_field( cached_fields, "DTG_MAPID", "word" );
	    else
	        seg_ok = 1;
	    version = cp_string( my_mod->dt_get_server_version( dtID, err ) );
	    my_mod->proj_free( projID, err );
	}
	char *warn = my_mod->dt_get_server_warnings( dtID, err );
	if( warn )
	{
	    if( !error || strcmp( error, warn ) )
	        warnings = cp_string( warn );
	    free( warn );
	}

	my_mod->dt_free( dtID, err );
	delete_DTGError( err );
}

void DataSource::assign_plugins( DTGModule *plugins, DataSource *srcs )
{
	DTGModule *mod = NULL;
        struct DTGError *err = new_DTGError( NULL );
	for( DataSource *src = srcs; src; src = src->next )
	{
	    if( src->plugin )
	        for( mod = plugins; mod; mod = mod->next )
	        {
	            const char *tmp = mod->dt_get_name( err );
	            if( tmp && !strcmp( tmp, src->plugin ) )
	                break;
	        }
	    src->my_mod = mod;
	}
	delete_DTGError( err );
}

struct DTGStrList *DataSource::get_modules()
{
	return cached_modules;
}

struct DTGFieldDesc *DataSource::get_fields()
{
	return cached_fields;
}

struct DTGAttribute *DataSource::get_attributes()
{
	if( !cached_attributes && 
		my_mod && my_mod->has_attribute_extensions() )
	    cached_attributes = my_mod->dt_list_attrs();
	return cached_attributes;
}

char *DataSource::validate_attribute( DataAttr *attr )
{
	if( !attr )
	    return mk_string( "Missing attribute" );
	if( !attr->name )
	    return mk_string( "Unnamed attribute" );
	if( !attr->value )
	    return mk_string( "No value specified" );
	if( !my_mod )
	    return mk_string( "No plugin available" );
	struct DTGField *field = new_DTGField( attr->name, attr->value );
	char *tmp = my_mod->dt_validate_attr( field );
	delete_DTGField( field );
	return tmp;;
}

DataSource::DataSource(
	SourceType mytype, 
	const char *myplugin, 
	DTGModule *mod)
{
	accept_utf8 = -1;
	seg_ok = 0;
	deleted = 0;
	dirty = 0;
	refcnt = 0;
	type = mytype;
	nickname = NULL;
	oldname = NULL;
	plugin = myplugin ? cp_string( myplugin ) : NULL;
	server = NULL;
	user = NULL;
	password = NULL;
	if( type == DataSource::SCM )
	    module = cp_string( "Jobs" );
	else
	    module = NULL;
	moddate_field = NULL;
	moduser_field = NULL;
	next = NULL;
	cached_fields = NULL;
	cached_modules = NULL;
	cached_attributes = NULL;
	status = DataSource::UNKNOWN;
	error = NULL;
	my_mod = mod;
	warnings = NULL;
	version = NULL;
	set_list = NULL;
	attrs = NULL;
	field_list = NULL;
	map = NULL;
}

void DataSource::set_module( DTGModule *new_mod )
{
	my_mod = new_mod;
	if( moddate_field )
	    delete[] moddate_field;
	moddate_field = NULL;
	if( moduser_field )
	    delete[] moduser_field;
	moduser_field = NULL;
	if( cached_fields )
	    delete_DTGFieldDesc( cached_fields );
	cached_fields = NULL;
	if( cached_modules )
	    delete_DTGStrList( cached_modules );
	cached_modules = NULL;
	if( cached_attributes )
	    delete_DTGAttribute( cached_attributes );
	cached_attributes = NULL;
	if( attrs )
	    delete attrs;
	attrs = NULL;
	if( field_list )
	    delete_DTGField( field_list );
	field_list = NULL;
}

void DataSource::clear()
{
	if( nickname )
	    delete[] nickname;
	if( oldname )
	    delete[] oldname;
	if( plugin )
	    delete[] plugin;
	if( server )
	    delete[] server;
	if( user )
	    delete[] user;
	if( password )
	    delete[] password;
	if( module )
	    delete[] module;
	if( moddate_field )
	    delete[] moddate_field;
	if( moduser_field )
	    delete[] moduser_field;
	if( next )
	    delete next;
	if( cached_fields )
	    delete_DTGFieldDesc( cached_fields );
	if( cached_modules )
	    delete_DTGStrList( cached_modules );
	if( cached_attributes )
	    delete_DTGAttribute( cached_attributes );
	if( error )
	    delete[] error;
	if( warnings )
	    delete[] warnings;
	if( version )
	    delete[] version;
	if( attrs )
	    delete attrs;
	if( field_list )
	    delete_DTGField( field_list );
	map = NULL;
	if( set_list )
	    delete set_list;
}

DataSource::~DataSource()
{
	clear();
}

DataSource *DataSource::copy( int full )
{
	DataSource *my_next = next;
	next = NULL;
	TiXmlElement elem("Foobar");
	save(&elem, full);
	next = my_next;
	DataSource *dup = load(elem.FirstChildElement());
	dup->cached_fields = copy_DTGFieldDesc( cached_fields );
	dup->cached_modules = copy_DTGStrList( cached_modules );
	dup->cached_attributes = copy_DTGAttribute( cached_attributes );
	dup->status = status;
	dup->error = error ? cp_string(error) : NULL;
	dup->warnings = warnings ? cp_string(warnings) : NULL;
	dup->version = version ? cp_string(version) : NULL;
	dup->my_mod = my_mod;
	dup->deleted = deleted;
	dup->dirty = dirty;
	dup->refcnt = refcnt;
	dup->seg_ok = seg_ok;
	dup->accept_utf8 = accept_utf8;
	dup->oldname = cp_string( oldname );
	dup->set_list = set_list ? set_list->copy( full ) : NULL;
	return dup;
}

static const int MAX_KEY = 64;

static char *simple_encode( const char *k, const char *s1, const char *s2 )
{
	// Long or empty passwords are not encoded
	if( strlen( k ) > MAX_KEY || !strlen( k ) )
	    return NULL;

	int i, len;

	char *r1 = new char[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) r1[i] = '\0';
	i = MAX_KEY;
	len = strlen( s1 );
	while( i > 0 )
	{
	    strncat( r1, s1, i );
	    i -= len;
	}

	char *r2 = new char[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) r2[i] = '\0';
	i = MAX_KEY;
	len = strlen( s2 );
	while( i > 0 )
	{
	    strncat( r2, s2, i );
	    i -= len;
	}

	char *e = new char[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) e[i] = '\0';

	char *k1 = new char[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) k1[i] = '\0';
	strcat( k1, k ); // by definition, strlen( k ) <= MAX_KEY

	for( i=0; i < MAX_KEY; i++ )
	    e[i] = k1[i] ^ (r1[i] ^ r2[i]);

	delete[] r1;
	delete[] r2;
	delete[] k1;
	
	StrBuf hex;
	StrOps::OtoX( (unsigned char *)e, MAX_KEY, hex );
	delete[] e;
	char *result = mk_string( hex.Text() );
	return result;
}

static char *simple_decode( const char *k, const char *s1, const char *s2 )
{
	int i, len;

	char *r1 = new char[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) r1[i] = '\0';
	i = MAX_KEY;
	len = strlen( s1 );
	while( i > 0 )
	{
	    strncat( r1, s1, i );
	    i -= len;
	}

	char *r2 = new char[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) r2[i] = '\0';
	i = MAX_KEY;
	len = strlen( s2 );
	while( i > 0 )
	{
	    strncat( r2, s2, i );
	    i -= len;
	}

	StrRef hex = k;
	StrBuf oct;
	StrOps::XtoO( hex, oct );

	char e[MAX_KEY+1];
	for( i=0; i <= MAX_KEY; i++ ) e[i] = '\0';

	for( i=0; i < MAX_KEY; i++ )
	    e[i] = (oct.Text()[i] ^ (r1[i] ^ r2[i]));

	delete[] r1;
	delete[] r2;

	return mk_string( e );
}

void
DataSource::save( TiXmlElement *doc, int full )
{
	if( deleted )
	{
	    // if( next )
	        // next->save( doc );
	    return;
	}

	TiXmlElement *me = new TiXmlElement( "DataSource" );
	doc->LinkEndChild( me );
	switch( type )
	{
	case DataSource::SCM:
	default:
	    me->SetAttribute( "type", "SCM" );
	    break;
	case DataSource::DTS:
	    me->SetAttribute( "type", "DTS" );
	    break;
	}
	if( plugin )
	    me->SetAttribute( "plugin", plugin );
	if( nickname )
	    me->SetAttribute( "nickname", nickname );
	if( server )
	    me->SetAttribute( "server", server );
	if( user )
	    me->SetAttribute( "user", user );
	if( password )
	{
	    char *tmp = simple_encode( password, nickname, server );
	    if( tmp )
	    {
	        me->SetAttribute( "epassword", tmp );
	        delete[] tmp;
	    }
	    else
	        me->SetAttribute( "password", password );
	}
				
	if( module )
	    me->SetAttribute( "module", module );
	if( moddate_field )
	    me->SetAttribute( "moddate_field", moddate_field );
	if( moduser_field )
	    me->SetAttribute( "moduser_field", moduser_field );
	if( full )
	    me->SetAttribute( "refcnt", refcnt );
	if( attrs )
	{
	    TiXmlElement *list = new TiXmlElement( "Attributes" );
	    attrs->save( list );
	    me->LinkEndChild( list );
	}
	if( set_list )
	{
	    TiXmlElement *list = new TiXmlElement( "FilterSets" );
	    set_list->save( list );
	    me->LinkEndChild( list );
	}

	// if( next )
	    // next->save( doc );
}

DataSource *
DataSource::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	const char *tmp;
	DataSource *ds = new DataSource();
	tmp = me->Attribute( "type" );
	if( tmp )
	    if( !strcmp( tmp, "SCM" ) )
	        ds->type = DataSource::SCM;
	    else if( !strcmp( tmp, "DTS" ) )
	        ds->type = DataSource::DTS;
	tmp = me->Attribute( "nickname" );
	if( tmp )
	    ds->nickname = cp_string( tmp );
	tmp = me->Attribute( "plugin" );
	if( tmp )
	    ds->plugin = cp_string( tmp );
	tmp = me->Attribute( "server" );
	if( tmp )
	    ds->server = cp_string( tmp );
	tmp = me->Attribute( "user" );
	if( tmp )
	    ds->user = cp_string( tmp );
	tmp = me->Attribute( "password" );
	if( tmp )
	    ds->password = cp_string( tmp );
	tmp = me->Attribute( "epassword" );
	if( tmp )
	{
	    if( ds->password )
	        delete[] ds->password;
	    ds->password = simple_decode( tmp, ds->nickname, ds->server );
	}
	tmp = me->Attribute( "module" );
	if( tmp )
	{
	    if( ds->module )
	        delete[] ds->module;
	    ds->module = cp_string( tmp );
	}
	tmp = me->Attribute( "moddate_field" );
	if( tmp )
	    ds->moddate_field = cp_string( tmp );
	tmp = me->Attribute( "moduser_field" );
	if( tmp )
	    ds->moduser_field = cp_string( tmp );
	tmp = me->Attribute( "refcnt" );
	if( tmp )
	    ds->refcnt = atoi( tmp );

	TiXmlElement *sib = me->FirstChildElement( "Attributes" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "DataAttr" );
	    ds->attrs = DataAttr::load( sib );
	}

	sib = me->FirstChildElement( "FilterSets" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "FilterSet" );
	    ds->set_list = FilterSet::load( sib );
	}

	sib = me->NextSiblingElement( "DataSource" );
	ds->next = DataSource::load( sib );

	return ds;
}

FilterSet::FilterSet()
{
	name = NULL;
	filter_list = NULL;
	next = NULL;
	refcnt = 0;
}

FilterSet::~FilterSet()
{
	if( name )
	    delete[] name;
	if( filter_list )
	    delete filter_list;
	if( next )
	    delete next;
}

FilterSet *
FilterSet::copy( int full )
{
	TiXmlElement elem("Foobar");
	save( &elem, full );
	FilterSet *dup = load(elem.FirstChildElement());
	return dup;
}

void
FilterSet::save( TiXmlElement *source, int full )
{
	TiXmlElement *me = new TiXmlElement( "FilterSet" );
	source->LinkEndChild( me );
	if( name )
	    me->SetAttribute( "name", name );
	if( full )
	    me->SetAttribute( "refcnt", refcnt );
	if( filter_list )
	{
	    TiXmlElement *list = new TiXmlElement( "FilterRules" );
	    filter_list->save( list );
	    me->LinkEndChild( list );
	}

	if( next )
	    next->save( source, full );
}

FilterSet *
FilterSet::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	const char *tmp;
	FilterSet *nr = new FilterSet();
	tmp = me->Attribute( "name" );	
	if( tmp )
	    nr->name = cp_string( tmp );
	tmp = me->Attribute( "refcnt" );	
	if( tmp )
	    nr->refcnt = atoi( tmp );
	TiXmlElement *sib = me->FirstChildElement( "FilterRules" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "FilterRule" );
	    nr->filter_list = FilterRule::load( sib );
	}

	sib = me->NextSiblingElement( "FilterSet" );
	nr->next = FilterSet::load( sib );
	return nr;
}

struct DTGFieldDesc *FilterRule::extract_filter()
{
	/* Loop through the FilterRule list summarizing options by field */

	struct DTGFieldDesc *filters = NULL;
	FilterRule *rule = this;
	while( rule )
	{
	    const char *cur_field = rule->field;
	    struct DTGStrList *options = NULL;
	    while( rule && !strcmp( cur_field, rule->field ) )
	    {
	        options = append_DTGStrList( options, rule->pattern );
	        rule = rule->next;
	    }

	    /* Look for a previous ungrouped field of the same name */
	    struct DTGFieldDesc *f;
	    for( f = filters; f && strcmp( f->name, cur_field ); f = f->next );
		/* Just a search loop */
	    if( f )
	        f->select_values = 
			merge_DTGStrList( f->select_values, options );
	    else
	        filters = append_DTGFieldDesc( filters, 
			new_DTGFieldDesc( cur_field, "select", 1, options ) );
	}
	return filters;
}

FilterRule::FilterRule()
{
	field = NULL;
	pattern = NULL;
	next = NULL;
	old = 1;
}

FilterRule::~FilterRule()
{
	if( field )
	    delete[] field;
	if( pattern )
	    delete[] pattern;
	if( next )
	    delete next;
}

void
FilterRule::save( TiXmlElement *source )
{
	TiXmlElement *me = new TiXmlElement( "FilterRule" );
	source->LinkEndChild( me );
	if( field )
	    me->SetAttribute( "field", field );
	if( pattern )
	    me->SetAttribute( "pattern", pattern );

	if( next )
	    next->save( source );
}

FilterRule *
FilterRule::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	const char *tmp;
	FilterRule *nr = new FilterRule();
	tmp = me->Attribute( "field" );	
	if( tmp )
	    nr->field = cp_string( tmp );
	tmp = me->Attribute( "pattern" );	
	if( tmp )
	    nr->pattern = cp_string( tmp );

	TiXmlElement *sib = me->NextSiblingElement( "FilterRule" );
	nr->next = FilterRule::load( sib );
	return nr;
}
