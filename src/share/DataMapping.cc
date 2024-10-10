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

#include <DTG-interface.h>
#include "DataMapping.h"
#include "DataSource.h"
#include "genutils.h"
#include "tinyxml.h"
#include "Logger.h"
#include "DataAttr.h"
extern "C" {
#include <dtg-utils.h>
}

static DataSource *find_source( const char *id, DataSource *srcs )
{
	DataSource *src;
	for( src = srcs; src; src = src->next )
	    if( src->nickname && !strcasecmp( src->nickname, id ) )
	        break;
	return src;
}

static FilterSet *find_filters( DataSource *src, const char *filter )
{
	if( !src || !filter )
	    return NULL;

	FilterSet *rule;
	for( rule = src->set_list; rule; rule = rule->next )
	    if( rule->name && !strcasecmp( rule->name, filter ) )
	        break;
	return rule;
}

struct DTGAttribute *DataMapping::get_attributes()
{
	if( !cached_attributes )
	{
            cached_attributes = new_DTGAttribute(
                "log_level",
                "Log Level",
		"Specifies the level of detail recorded in the replication "
		"log. Valid values: 0 - Errors only; 1 - Errors and warnings "
		"(default); 2 - Errors, warnings, and information messages; "
		"3 - Maximum detail",
                "2",
                0 );
            cached_attributes = append_DTGAttribute( cached_attributes, 
							new_DTGAttribute(
                "polling_period",
                "Polling Period",
		"Specifies the number of seconds that the replication engine "
		"waits before polling for changes. Minimum is 1; maximum is "
		"100. The default is 5 seconds.",
                "5",
                0 ) );
            cached_attributes = append_DTGAttribute( cached_attributes, 
							new_DTGAttribute(
                "connection_reset",
                "Connection Reset",
		"Specifies the number of replication cycles that occur before "
		"server connections are closed and reopened. Minimum is 1; "
		"maximum is 1,000,000. The default is 1000 cycles.",
                "1000",
                0 ) );
            cached_attributes = append_DTGAttribute( cached_attributes, 
							new_DTGAttribute(
                "wait_duration",
                "General Wait Duration",
		"Specifies how long in seconds the replication engine should "
		"wait before retrying connections. -1 indicates the engine "
		"should exit. DataSource specific wait limits will override "
		"this value unless the server goes offline during a connection "
		"reset event. Default value is 150 seconds.",
                "150",
                0 ) );
            cached_attributes = append_DTGAttribute( cached_attributes, 
							new_DTGAttribute(
                "cycle_threshold",
                "Logging of Large Update Cycles",
		"For logging levels 1 or higher, generate a log entry whenever "
		"the number of defects/issues/jobs being processed during a "
		"replication cycle equals or exceeds this value. Default is 0 "
		"specifying no additional logging is to be generated. ",
                "0",
                0 ) );
            cached_attributes = append_DTGAttribute( cached_attributes, 
							new_DTGAttribute(
                "update_period",
                "Additional Logging during Large Updates",
		"Specifies how often during a logged large update to generate "
		"additional log messages. Requires Logging of Large Update "
		"Cycles to be enabled and a log level of 1 or higher. Default "
		"is 0 indicating no additional logging during processing of "
		"large cycles.",
                "0",
                0 ) );
            cached_attributes = append_DTGAttribute( cached_attributes, 
							new_DTGAttribute(
                "enable_write_to_readonly",
                "Enable writing to read-only fields in Perforce",
		"Use only as directed by Support. This enables the mapping "
		"to be hand-edited to target read-only fields like ModDate "
		"within Perforce. Default 0 (i.e., don't allow). "
		"A value of 1 allows writing to such fields. Note, Perforce "
		"may reject such writes.",
                "0",
                0 ) );
	}
	return cached_attributes;
}

static int is_number( const char *str )
{
	if( !str )
	    return 0;

	if( *str == '.' )
	    return 0;

	// a negative number?
	if( strlen( str ) > 1 )
	{
	    if( *str == '-' && !isdigit( *(str + 1 )) )
	        return 0;
	    else
	        str += 1;
	}

	for( const char *i = str; *i; i++ )
	    if( !isdigit( *i ) )
	        return 0;

	return 1;
}

char *DataMapping::validate_attribute( DataAttr *a )
{
	if( !a )
	    return strdup( "Missing attribute" );
	if( !a->name )
	    return strdup( "Unnamed attribute" );
	if( !a->value )
	    return strdup( "No value specified" );
	if( !strcmp( a->name, "enable_write_to_readonly" ) )
	{
	    if( !is_number( a->value ) || 
		*a->value < '0' || *a->value > '1' ||
		a->value[1] )
	        return strdup( "Enable writing: Must be either 0 or 1" );
	    return NULL;
	}
	if( !strcmp( a->name, "log_level" ) )
	{
	    if( !is_number( a->value ) || 
		*a->value < '0' || *a->value > '3' ||
		a->value[1] )
	        return strdup( "Log level: Must be a number between 0 and 3" );
	    return NULL;
	}
	if( !strcmp( a->name, "polling_period" ) )
	{
	    if( !is_number( a->value ) )
	        return strdup( 
			"Polling period: Must be a number between 1 and 100" );
	    int n = atoi( a->value );
	    if( n < 1 || n > 100 )
	        return strdup( 
			"Polling period: Must be a number between 1 and 100" );
	    return NULL;
	}
	if( !strcmp( a->name, "connection_reset" ) )
	{
	    if( !is_number( a->value ) )
	        return strdup( 
		    "Connection reset: Must be a number between 1 and 1M" );
	    int n = atoi( a->value );
	    if( n < 1 || n > 1000000 )
	        return strdup( 
		    "Connection reset: Must be a number between 1 and 1M" );
	    return NULL;
	}
	if( !strcmp( a->name, "wait_duration" ) )
	{
	    int n = atoi( a->value );
	    if( !is_number( a->value ) || ( n < 1 && n != -1 ) )
	        return strdup( 
			"General Wait Duration: Must be a number > 0 or -1" );
	    return NULL;
	}
	if( !strcmp( a->name, "cycle_threshold" ) )
	{
	    int n = atoi( a->value );
	    if( !is_number( a->value ) || n < 0 )
	        return strdup( "Logging of Large Update Cycles: Must be a "
				"number equal to or greater than 0" );
	    return NULL;
	}
	if( !strcmp( a->name, "update_period" ) )
	{
	    int n = atoi( a->value );
	    if( !is_number( a->value ) || n < 0 )
	        return strdup( "Additional Logging during Large Updates: Must "
				"be a number equal to or greater than 0" );
	    return NULL;
	}
	return strdup( "Unknown attribute" );
}

void DataMapping::set_filters()
{
	FilterSet *set;
	if( scm )
	{
	    set = find_filters( scm, scm_filter );
	    delete_DTGFieldDesc( scm_filter_desc );
	    if( set )
	    {
	        scm_filter_desc = set->filter_list->extract_filter();
	        scm_filters = set->filter_list;
	    }
	    else
	    {
	        scm_filter_desc = NULL;
	        scm_filters = NULL;
	    }
	}
	if( dts )
	{
	    set = find_filters( dts, dts_filter );
	    delete_DTGFieldDesc( dts_filter_desc );
	    if( set )
	    {
	        dts_filter_desc = set->filter_list->extract_filter();
	        dts_filters = set->filter_list;
	    }
	    else
	    {
	        dts_filter_desc = NULL;
	        dts_filters = NULL;
	    }
	}
}

void DataMapping::cross_reference( DataSource *srcs, DataMapping *maps )
{
	FilterSet *set;
	for( DataMapping *map = maps; map; map = map->next )
	{
	    map->scm = find_source(map->scm_id, srcs);
	    map->dts = find_source(map->dts_id, srcs);
	    map->set_filters();
	    if( map->scm )
	        map->scm->adj_refcnt( +1, map->scm_filter );
	    if( map->dts )
	        map->dts->adj_refcnt( +1, map->dts_filter );
	}
}

DataMapping::DataMapping()
{
	deleted = 0;
	dirty = 0;
	valid = -1;

	recheck_on_new_dts = 0;
	recheck_on_new_scm = 0;

	id = NULL;
	oldid = NULL;
	scm_id = NULL;
	scm = NULL;
	scm_filter = NULL;
	scm_filters = NULL;
	scm_filter_desc = NULL;
	dts_id = NULL;
	dts = NULL;
	dts_filter = NULL;
	dts_filters = NULL;
	dts_filter_desc = NULL;
	fix_rules = NULL;
	mirror_conflicts = DataMapping::ERROR;
	mirror_rules = NULL;
	scm_to_dts_rules = NULL;
	dts_to_scm_rules = NULL;
	attrs = NULL;
	cached_attributes = NULL;
	next = NULL;
}

DataMapping::~DataMapping()
{
	if( id )
	    delete[] id;
	if( oldid )
	    delete[] oldid;
	if( scm_id )
	    delete[] scm_id;
	if( dts_id )
	    delete[] dts_id;
	if( scm_filter )
	    delete[] scm_filter;
	if( scm_filter_desc )
	    delete_DTGFieldDesc( scm_filter_desc );
	if( dts_filter )
	    delete[] dts_filter;
	if( dts_filter_desc )
	    delete_DTGFieldDesc( dts_filter_desc );
	if( fix_rules )
	    delete fix_rules;
	if( mirror_rules )
	    delete mirror_rules;
	if( scm_to_dts_rules )
	    delete scm_to_dts_rules;
	if( dts_to_scm_rules )
	    delete dts_to_scm_rules;
	if( attrs )
	    delete attrs;
	if( cached_attributes )
	    delete_DTGAttribute( cached_attributes );
	if( next )
	    delete next;
}

DataMapping* DataMapping::copy()
{
	DataMapping *my_next = next;
	next = NULL;
	TiXmlElement elem("Foobar");
	save(&elem);
	next = my_next;
	DataMapping *dup = load(elem.FirstChildElement());
	dup->scm = scm;
	dup->dts = dts;
	dup->set_filters();
	dup->oldid = cp_string( oldid );
	return dup;
}

void
DataMapping::save( TiXmlElement *doc )
{
	if( deleted )
	{
	    if( next )
	        next->save( doc );
	    return;
	}

	TiXmlElement *me = new TiXmlElement( "DataMapping" );
	doc->LinkEndChild( me );
	if( id )
	    me->SetAttribute( "id", id );
	if( scm && scm->nickname )
	    me->SetAttribute( "scm_id", scm->nickname );
	else if( scm_id )
	    me->SetAttribute( "scm_id", scm_id );
	if( scm_filter )
	    me->SetAttribute( "scm_filter", scm_filter );
	if( dts && dts->nickname )
	    me->SetAttribute( "dts_id", dts->nickname );
	else if( dts_id )
	    me->SetAttribute( "dts_id", dts_id );
	if( dts_filter )
	    me->SetAttribute( "dts_filter", dts_filter );
	switch ( mirror_conflicts )
	{
	case DataMapping::SCM:
	    me->SetAttribute( "mirror_conflicts", "SCM" );
	    break;
	case DataMapping::DTS:
	    me->SetAttribute( "mirror_conflicts", "DTS" );
	    break;
	case DataMapping::NEWER:
	    me->SetAttribute( "mirror_conflicts", "NEWER" );
	    break;
	case DataMapping::ERROR:
	default:
	    me->SetAttribute( "mirror_conflicts", "ERROR" );
	    break;
	}
	if( fix_rules )
	    fix_rules->save( me );
	if( mirror_rules )
	{
	    TiXmlElement *list = new TiXmlElement( "MirrorRules" );
	    mirror_rules->save( list );
	    me->LinkEndChild( list );
	}
	if( scm_to_dts_rules )
	{
	    TiXmlElement *list = new TiXmlElement( "ScmToDtsRules" );
	    scm_to_dts_rules->save( list );
	    me->LinkEndChild( list );
	}
	if( dts_to_scm_rules )
	{
	    TiXmlElement *list = new TiXmlElement( "DtsToScmRules" );
	    dts_to_scm_rules->save( list );
	    me->LinkEndChild( list );
	}
	if( attrs )
	{
	    TiXmlElement *list = new TiXmlElement( "Attributes" );
	    attrs->save( list );
	    me->LinkEndChild( list );
	}
	// if( next )
	    // next->save( doc );
}

DataMapping *
DataMapping::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	DataMapping *dm = new DataMapping();
	const char *tmp;
	tmp = me->Attribute( "id" );	
	if( tmp )
	    dm->id = cp_string( tmp );
	tmp = me->Attribute( "scm_id" );	
	if( tmp )
	    dm->scm_id = cp_string( tmp );
	tmp = me->Attribute( "dts_id" );	
	if( tmp )
	    dm->dts_id = cp_string( tmp );
	tmp = me->Attribute( "mirror_conflicts" );
	if( tmp )
	    if( !strcmp( tmp, "SCM" ) )
	        dm->mirror_conflicts = DataMapping::SCM;
	    else if( !strcmp( tmp, "DTS" ) )
	        dm->mirror_conflicts = DataMapping::DTS;
	    else if( !strcmp( tmp, "NEWER" ) )
	        dm->mirror_conflicts = DataMapping::NEWER;
	    else if( !strcmp( tmp, "ERROR" ) )
	        dm->mirror_conflicts = DataMapping::ERROR;

	TiXmlElement *sib = me->FirstChildElement( "FixRule" );
	dm->fix_rules = FixRule::load( sib );

	tmp = me->Attribute( "scm_filter" );	
	if( tmp )
	    dm->scm_filter = cp_string( tmp );

	tmp = me->Attribute( "dts_filter" );	
	if( tmp )
	    dm->dts_filter = cp_string( tmp );

	sib = me->FirstChildElement( "MirrorRules" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "CopyRule" );
	    dm->mirror_rules = CopyRule::load( sib );
	}

	sib = me->FirstChildElement( "ScmToDtsRules" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "CopyRule" );
	    dm->scm_to_dts_rules = CopyRule::load( sib );
	}

	sib = me->FirstChildElement( "DtsToScmRules" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "CopyRule" );
	    dm->dts_to_scm_rules = CopyRule::load( sib );
	}

	sib = me->FirstChildElement( "Attributes" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "DataAttr" );
	    dm->attrs = DataAttr::load( sib );
	}

	sib = me->NextSiblingElement( "DataMapping" );
	dm->next = DataMapping::load( sib );
	return dm;
}

FixRule::FixRule()
{
	deleted = 0;
	dts_field = NULL;
	copy_type = FixRule::REPLACE;
	file_list = false;
	change_number = false;
	description = false;
	fixed_by = false;
	fixed_date = false;
	next = NULL;
}

FixRule::~FixRule()
{
	if( dts_field )
	    delete[] dts_field;
	if( next )
	    delete next;
}

void
FixRule::save( TiXmlElement *data_mapping )
{
	if( deleted )
	{
	    if( next )
	        next->save( data_mapping );
	    return;
	}

	TiXmlElement *me = new TiXmlElement( "FixRule" );
	data_mapping->LinkEndChild( me );
	if( dts_field )
	    me->SetAttribute( "dts_field", dts_field );
	switch( copy_type )
	{
	case FixRule::REPLACE:
	    me->SetAttribute( "copy_type", "REPLACE" );
	    break;
	case FixRule::APPEND:
	default:
	    me->SetAttribute( "copy_type", "APPEND" );
	    break;
	}
	me->SetAttribute( "file_list", file_list );
	me->SetAttribute( "change_number", change_number );
	me->SetAttribute( "description", description );
	me->SetAttribute( "fixed_by", fixed_by );
	me->SetAttribute( "fixed_date", fixed_date );

	if( next )
	    next->save( data_mapping );
}

FixRule *
FixRule::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	FixRule *fr = new FixRule();
	const char *tmp;
	tmp = me->Attribute( "dts_field" );
	if( tmp )
	    fr->dts_field = cp_string( tmp );
	tmp = me->Attribute( "copy_type" );
	if( tmp )
	    if( !strcmp( tmp, "REPLACE" ) )
	        fr->copy_type = FixRule::REPLACE;
	    else if( !strcmp( tmp, "APPEND" ) )
	        fr->copy_type = FixRule::APPEND;
	tmp = me->Attribute( "file_list" );
	if( tmp )
	    fr->file_list = tmp[0] == '1';
	tmp = me->Attribute( "change_number" );
	if( tmp )
	    fr->change_number = tmp[0] == '1';
	tmp = me->Attribute( "description" );
	if( tmp )
	    fr->description = tmp[0] == '1';
	tmp = me->Attribute( "fixed_by" );
	if( tmp )
	    fr->fixed_by = tmp[0] == '1';
	tmp = me->Attribute( "fixed_date" );
	if( tmp )
	    fr->fixed_date = tmp[0] == '1';

	TiXmlElement *sib = me->NextSiblingElement( "FixRule" );
	fr->next = FixRule::load( sib );
	return fr;
}


CopyRule::CopyRule()
{
	deleted = 0;
	truncate = 0;
	scm_field = NULL;
	dts_field = NULL;
	copy_type = CopyRule::TEXT;
        mirror_conflicts = CopyRule::DTS;
	mappings = NULL;
	next = NULL;
}

CopyRule::~CopyRule()
{
	if( scm_field )
	    delete[] scm_field;
	if( dts_field )
	    delete[] dts_field;
	if( mappings )
	    delete mappings;
	if( next )
	    delete next;
}

void
CopyRule::save( TiXmlElement *list )
{
	if( deleted )
	{
	    if( next )
	        next->save( list );
	    return;
	}
	TiXmlElement *me = new TiXmlElement( "CopyRule" );
	list->LinkEndChild( me );
	if( scm_field )
	    me->SetAttribute( "scm_field", scm_field );
	if( dts_field )
	    me->SetAttribute( "dts_field", dts_field );
	switch( copy_type )
	{
	case CopyRule::TEXT:
	default:
	    me->SetAttribute( "copy_type", "TEXT" );
	    break;
	case CopyRule::WORD:
	    me->SetAttribute( "copy_type", "WORD" );
	    break;
	case CopyRule::LINE:
	    me->SetAttribute( "copy_type", "LINE" );
	    break;
	case CopyRule::DATE:
	    me->SetAttribute( "copy_type", "DATE" );
	    break;
	case CopyRule::UNMAP:
	    me->SetAttribute( "copy_type", "UNMAP" );
	    break;
	case CopyRule::MAP:
	    me->SetAttribute( "copy_type", "MAP" );
	    break;
	}
	switch( mirror_conflicts )
	{
	case CopyRule::DTS:
	default:
	    me->SetAttribute( "mirror_conflicts", "DTS" );
	    break;
	case CopyRule::SCM:
	    me->SetAttribute( "mirror_conflicts", "SCM" );
	    break;
	}
	me->SetAttribute( "truncate", truncate );
	if( mappings )
	    mappings->save( me );
	if( next )
	    next->save( list );
}

CopyRule *
CopyRule::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	CopyRule *cr = new CopyRule();
	const char *tmp;
	tmp = me->Attribute( "scm_field" );
	if( tmp )
	    cr->scm_field = cp_string( tmp );
	tmp = me->Attribute( "dts_field" );
	if( tmp )
	    cr->dts_field = cp_string( tmp );
	tmp = me->Attribute( "copy_type" );
	if( tmp )
	    if( !strcmp( tmp, "TEXT" ) )
	        cr->copy_type = CopyRule::TEXT;
	    else if( !strcmp( tmp, "WORD" ) )
	        cr->copy_type = CopyRule::WORD;
	    else if( !strcmp( tmp, "LINE" ) )
	        cr->copy_type = CopyRule::LINE;
	    else if( !strcmp( tmp, "UNMAP" ) )
	        cr->copy_type = CopyRule::UNMAP;
	    else if( !strcmp( tmp, "MAP" ) )
	        cr->copy_type = CopyRule::MAP;
	    else if( !strcmp( tmp, "DATE" ) )
	        cr->copy_type = CopyRule::DATE;
	tmp = me->Attribute( "mirror_conflicts" );
	if( tmp )
	    if( !strcmp( tmp, "DTS" ) )
	        cr->mirror_conflicts = CopyRule::DTS;
	    else if( !strcmp( tmp, "SCM" ) )
	        cr->mirror_conflicts = CopyRule::SCM;
	tmp = me->Attribute( "truncate" );
	if( tmp )
	    cr->truncate = atoi( tmp );
	TiXmlElement *sib = me->FirstChildElement( "CopyMap" );
	cr->mappings = CopyMap::load( sib );

	sib = me->NextSiblingElement( "CopyRule" );
	cr->next = CopyRule::load( sib );
	return cr;
}

CopyMap::CopyMap()
{
	value1 = NULL;
	value2 = NULL;
	next = NULL;
}

CopyMap::~CopyMap()
{
	if( value1 )
	    delete[] value1;
	if( value2 )
	    delete[] value2;
	if( next )
	    delete next;
}

void
CopyMap::save( TiXmlElement *copy_rule )
{
	TiXmlElement *me = new TiXmlElement( "CopyMap" );
	copy_rule->LinkEndChild( me );
	if( value1 )
	    me->SetAttribute( "value1", value1 );
	if( value2 )
	    me->SetAttribute( "value2", value2 );
	if( next )
	    next->save( copy_rule );
}

CopyMap *
CopyMap::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	CopyMap *cm = new CopyMap();
	const char *tmp;
	tmp = me->Attribute( "value1" );
	if( tmp )
	    cm->value1 = cp_string( tmp );
	tmp = me->Attribute( "value2" );
	if( tmp )
	    cm->value2 = cp_string( tmp );

	TiXmlElement *sib = me->NextSiblingElement( "CopyMap" );
	cm->next = CopyMap::load( sib );
	return cm;
}

CopyMap *
CopyMap::copy()
{
	CopyMap *cm = new CopyMap();
	cm->value1 = value1 ? cp_string( value1 ) : NULL;
	cm->value2 = value2 ? cp_string( value2 ) : NULL;
	if( next )
	    cm->next = next->copy();
	return cm;
}

static DTGFieldDesc *find_field( struct DTGFieldDesc *list, const char *field )
{
	if( !field )
	    return NULL;
	for( struct DTGFieldDesc *f = list; f; f = f->next )
	    if( !strcasecmp( f->name, field ) )
	        return f;
	return NULL;
}

int DataMapping::validate( int enable_write_to_readonly, Logger *log )
{
	valid = 1;
	struct DTGFieldDesc *scm_fields = NULL;
	struct DTGFieldDesc *dts_fields = NULL;
	if( !scm )
	{
	    valid = 0;
	    log->log( 0,
	      "SCM plugin missing" );
	}
	if( scm->status != DataSource::READY )
	{
	    valid = 0;
	    if( scm->status == DataSource::PASS )
	        log->log( 0, "SCM not configured" );
	    else
	        log->log( 0, "SCM unable to connect: %s", scm->error );
	}
	else if( scm_filter && !scm->seg_ok )
	{
	    valid = 0;
	    log->log( 0,
	      "SCM is segmented but does not have the DTG_MAPID field" );
	}
	else
	{
	    scm_fields = scm->get_fields();
	    if( !scm->has_required_fields() )
	    {
	        valid = 0;
	        log->log( 0,
	          "SCM is missing one or more required fields" );
	    }
	}

	if( !dts )
	{
	    valid = 0;
	    log->log( 0, "DTS plugin missing" );
	}
	else if( dts->status != DataSource::PASS )
	{
	    valid = 0;
	    log->log( 0, "DTS plugin unable to connect: %s", dts->error );
	}
	else
	{
	    dts_fields = dts->get_fields();
	    if( !dts->has_required_fields() )
	    {
	        valid = 0;
	        log->log( 0,
	          "DTS is missing one or more required fields" );
	    }
	}

	if( !valid )
	    return valid;

	CopyRule *cr;
	struct DTGFieldDesc *scm_field;
	struct DTGFieldDesc *dts_field;
	
	scm_field = find_field( scm_fields, scm->moddate_field );
	if( !scm_field || scm_field->readonly != 2 )
	{
	    valid = 0;
	    log->log( 0, "Invalid SCM ModDate field: %s", 
				scm->moddate_field );
	}
	scm_field = find_field( scm_fields, scm->moduser_field );
	if( !scm_field || scm_field->readonly != 3 )
	{
	    valid = 0;
	    log->log( 0, "Invalid SCM ModUser field: %s", 
				scm->moduser_field );
	}
	dts_field = find_field( dts_fields, dts->moddate_field );
	if( !dts_field || dts_field->readonly != 2 )
	{
	    valid = 0;
	    log->log( 0, "Invalid DTS ModDate field: %s", 
				dts->moddate_field );
	}
	if( dts->moduser_field && 
		strcmp( dts->moduser_field, "*Not Supported*" ) )
	{
	    dts_field = find_field( dts_fields, dts->moduser_field );
	    if( !dts_field || dts_field->readonly != 3 )
	    {
	        valid = 0;
	        log->log( 0, "Invalid DTS ModUser field: %s", 
				dts->moduser_field );
	    }
	}

	for( cr = mirror_rules; cr; cr = cr->next )
	{
	    if( cr->copy_type == CopyRule::UNMAP )
	    {
	        valid = 0;
	        log->log( 0, "Incomplete mirror mapping: DTS %s SCM %s", 
			cr->dts_field,
			cr->scm_field );
	    }
	    scm_field = find_field( scm_fields, cr->scm_field );
	    dts_field = find_field( dts_fields, cr->dts_field );
	    if( !scm_field )
	    {
	        valid = 0;
	        log->log( 0, "SCM missing field: %s", cr->scm_field );
	    }
	    if( !dts_field )
	    {
	        valid = 0;
	        log->log( 0, "DTS missing field: %s", cr->dts_field );
	    }
	    if( scm_field && dts_field && 
		( scm_field->readonly || dts_field->readonly ) )
	    {
	        if( !enable_write_to_readonly || dts_field->readonly )
	        {
	          valid = 0;
	          log->log( 0, "Mirror target(s) readonly: S:%s D:%s", 
			  cr->scm_field, cr->dts_field );
	        }
	        else
	        {
	          log->log( 0, "Mirror target(s) readonly: S:%s D:%s OVERRIDE", 
			  cr->scm_field, cr->dts_field );
	        }
	    }
	}
	for( cr = scm_to_dts_rules; cr; cr = cr->next )
	{
	    if( cr->copy_type == CopyRule::UNMAP )
	    {
	        valid = 0;
	        log->log( 0, "Incomplete SCM->DTS mapping: DTS %s SCM %s", 
			cr->dts_field,
			cr->scm_field );
	    }
	    scm_field = find_field( scm_fields, cr->scm_field );
	    dts_field = find_field( dts_fields, cr->dts_field );
	    if( strcmp( cr->scm_field, "List of Change Numbers" ) && 
		!scm_field )
	    {
	        valid = 0;
	        log->log( 0, "SCM missing field: %s", cr->scm_field );
	    }
	    if( !dts_field )
	    {
	        valid = 0;
	        log->log( 0, "DTS missing field: %s", cr->dts_field );
	    }
	    if( dts_field && dts_field->readonly )
	    {
	        valid = 0;
	        log->log( 0, "Copy target readonly: S:%s -> D:%s", 
				cr->scm_field, cr->dts_field );
	    }
	    /* Check for DefectID field being the source */
	    if( scm_field && scm_field->readonly == 4 )
	        recheck_on_new_scm = 1;
	}
	for( cr = dts_to_scm_rules; cr; cr = cr->next )
	{
	    if( cr->copy_type == CopyRule::UNMAP )
	    {
	        valid = 0;
	        log->log( 0, "Incomplete DTS->SCM mapping: DTS %s SCM %s", 
			cr->dts_field,
			cr->scm_field );
	    }
	    scm_field = find_field( scm_fields, cr->scm_field );
	    dts_field = find_field( dts_fields, cr->dts_field );
	    if( !scm_field )
	    {
	        valid = 0;
	        log->log( 0, "SCM missing field: %s", cr->scm_field );
	    }
	    if( !dts_field )
	    {
	        valid = 0;
	        log->log( 0, "DTS missing field: %s", cr->dts_field );
	    }
	    if( scm_field && scm_field->readonly )
	    {
	        if( !enable_write_to_readonly )
	        {
	          valid = 0;
	          log->log( 0, "Copy target readonly: S:%s <- D:%s", 
				cr->scm_field, cr->dts_field );
	        }
	        else
	        {
	          log->log( 0, "Copy target readonly: S:%s <- D:%s OVERRIDE", 
				cr->scm_field, cr->dts_field );
	        }
	    }
	    /* Check for DefectID field being the source */
	    if( dts_field && dts_field->readonly == 4 )
	        recheck_on_new_dts = 1;
	}
	for( FixRule *fr = fix_rules; fr; fr = fr->next )
	{
	    dts_field = find_field( dts_fields, fr->dts_field );
	    if( !dts_field )
	    {
	        valid = 0;
	        log->log( 0, "DTS missing field: %s", fr->dts_field );
	    }
	    if( dts_field && dts_field->readonly )
	    {
	        valid = 0;
	        log->log( 0, "Copy target readonly: FIX -> D:%s", 
				cr->dts_field );
	    }
	}
	return valid;
}

struct DTGStrList *DataMapping::dts_field_references()
{
	if( !dts || dts->status != DataSource::PASS )
	    return NULL;

	CopyRule *cr;
	struct DTGStrList *fields = NULL;
	for( cr = mirror_rules; cr; cr = cr->next )
	    if( !in_DTGStrList( cr->dts_field, fields ) &&
		strncmp( cr->dts_field, "DTGConfig-", 10 ) &&
		strncmp( cr->dts_field, "DTGAttribute-", 13 ) )
	        fields = append_DTGStrList( fields, cr->dts_field );

	for( cr = scm_to_dts_rules; cr; cr = cr->next )
	    if( !in_DTGStrList( cr->dts_field, fields ) &&
		strncmp( cr->dts_field, "DTGConfig-", 10 ) &&
		strncmp( cr->dts_field, "DTGAttribute-", 13 ) )
	        fields = append_DTGStrList( fields, cr->dts_field );

	for( cr = dts_to_scm_rules; cr; cr = cr->next )
	    if( !in_DTGStrList( cr->dts_field, fields ) &&
		strncmp( cr->dts_field, "DTGConfig-", 10 ) &&
		strncmp( cr->dts_field, "DTGAttribute-", 13 ) )
	        fields = append_DTGStrList( fields, cr->dts_field );

	for( FixRule *fr = fix_rules; fr; fr = fr->next )
	    if( !in_DTGStrList( fr->dts_field, fields ) &&
		strncmp( fr->dts_field, "DTGConfig-", 10 ) &&
		strncmp( fr->dts_field, "DTGAttribute-", 13 ) )
	        fields = append_DTGStrList( fields, fr->dts_field );

	return fields;
}
