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
#include "Settings.h"
#include "tinyxml.h"
extern "C" {
#include <dtg-utils.h>
}

extern char *cp_string( const char *str );

DTGSettings::DTGSettings()
{
	id = NULL;
	oldid = NULL;
	deleted = 0;
	dirty = 0;
	force = false;
	next = NULL;
	last_update = NULL;
	last_update_scm = new_DTGDate( 0, 0, 0, 0, 0, 0 );
	last_update_dts = new_DTGDate( 0, 0, 0, 0, 0, 0 );
	starting_date = new_DTGDate( 0, 0, 0, 0, 0, 0 );
	notify_email = NULL;
	from_address = NULL;
}

DTGSettings::~DTGSettings()
{
	if( id )
	    delete[] id;
	if( oldid )
	    delete[] oldid;
	if( last_update )
	    delete_DTGDate( last_update );
	if( last_update_scm )
	    delete_DTGDate( last_update_scm );
	if( last_update_dts )
	    delete_DTGDate( last_update_dts );
	if( starting_date )
	    delete_DTGDate( starting_date );
	if( notify_email )
	    delete[] notify_email;
	if( from_address )
	    delete[] from_address;
}

static void date_save( TiXmlElement *elem, struct DTGDate *date )
{
	TiXmlElement *me = new TiXmlElement( "Date" );
	elem->LinkEndChild( me );
	me->SetAttribute( "year", date->year );
	me->SetAttribute( "month", date->month );
	me->SetAttribute( "day", date->day );
	me->SetAttribute( "hour", date->hour );
	me->SetAttribute( "minute", date->minute );
	me->SetAttribute( "second", date->second );
}

static struct DTGDate *date_load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	const char *tmp;
	struct DTGDate *nd = new_DTGDate( 0, 0, 0, 0, 0, 0 );
	tmp = me->Attribute( "year" );	
	if( tmp )
	    nd->year = atoi( tmp );
	tmp = me->Attribute( "month" );	
	if( tmp )
	    nd->month = atoi( tmp );
	tmp = me->Attribute( "day" );	
	if( tmp )
	    nd->day = atoi( tmp );
	tmp = me->Attribute( "hour" );	
	if( tmp )
	    nd->hour = atoi( tmp );
	tmp = me->Attribute( "minute" );	
	if( tmp )
	    nd->minute = atoi( tmp );
	tmp = me->Attribute( "second" );	
	if( tmp )
	    nd->second = atoi( tmp );
	return nd;
}

void
DTGSettings::save( TiXmlElement *doc )
{
	if( deleted )
	{
	    if( next )
	        next->save( doc );
	    return;
	}

	TiXmlElement *me = new TiXmlElement( "DTGSettings" );
	doc->LinkEndChild( me );
	me->SetAttribute( "id", id );
	me->SetAttribute( "force", force );
	if( last_update_scm )
	{
	    TiXmlElement *list = new TiXmlElement( "LastUpdateSCM" );
	    date_save( list, last_update_scm );
	    me->LinkEndChild( list );
	}
	if( last_update_dts )
	{
	    TiXmlElement *list = new TiXmlElement( "LastUpdateDTS" );
	    date_save( list, last_update_dts );
	    me->LinkEndChild( list );
	}
	if( starting_date )
	{
	    TiXmlElement *list = new TiXmlElement( "StartingDate" );
	    date_save( list, starting_date );
	    me->LinkEndChild( list );
	}
	if( notify_email )
	    me->SetAttribute( "notify_email", notify_email );
	if( from_address )
	    me->SetAttribute( "from_address", from_address );

	if( next )
	    next->save( doc );
}

DTGSettings *
DTGSettings::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	const char *tmp;
	DTGSettings *ds = new DTGSettings;
	tmp = me->Attribute( "id" );
	if( tmp )
	    ds->id = cp_string( tmp );
	tmp = me->Attribute( "force" );
	if( tmp )
	    ds->force = atoi( tmp );

	TiXmlElement *sib = me->FirstChildElement( "StartingDate" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "Date" );
	    delete_DTGDate( ds->starting_date );
	    ds->starting_date = date_load( sib );
	}
	sib = me->FirstChildElement( "LastUpdate" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "Date" );
	    delete_DTGDate( ds->last_update );
	    ds->last_update = date_load( sib );
	}
	sib = me->FirstChildElement( "LastUpdateSCM" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "Date" );
	    delete_DTGDate( ds->last_update_scm );
	    ds->last_update_scm = date_load( sib );
	}
	sib = me->FirstChildElement( "LastUpdateDTS" );
	if( sib )
	{
	    sib = sib->FirstChildElement( "Date" );
	    delete_DTGDate( ds->last_update_dts );
	    ds->last_update_dts = date_load( sib );
	}

	tmp = me->Attribute( "notify_email" );
	if( tmp )
	    ds->notify_email = cp_string( tmp );
	tmp = me->Attribute( "from_address" );
	if( tmp )
	    ds->from_address = cp_string( tmp );

	sib = me->NextSiblingElement( "DTGSettings" );
	ds->next = DTGSettings::load( sib );

	// Backward Compatability
	if( ds->last_update )
	{
	    if( !ds->last_update_dts->month )
	        set_DTGDate( ds->last_update_dts, ds->last_update );
	    if( !ds->last_update_scm->month )
	        set_DTGDate( ds->last_update_scm, ds->last_update );
	    delete_DTGDate( ds->last_update );
	    ds->last_update = NULL;
	}

	return ds;
}
