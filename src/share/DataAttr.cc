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
#include "DataAttr.h"
#include "tinyxml.h"
#include "genutils.h"

extern char *cp_string( const char *str );

DataAttr::DataAttr()
{
	name = NULL;
	value = NULL;
	next = NULL;
}

DataAttr::DataAttr( const char *in_name, const char *in_value )
{
	name = mk_string( in_name );
	value = mk_string( in_value );
	next = NULL;
}

DataAttr::~DataAttr()
{
	if( name )
	    delete[] name;
	if( value )
	    delete[] value;

	if( next )
	    delete next;
}

DataAttr *DataAttr::copy()
{
	TiXmlElement elem("Foobar");
	save(&elem);
	DataAttr *dup = load(elem.FirstChildElement());
	return dup;
}

void
DataAttr::save( TiXmlElement *doc )
{
	TiXmlElement *me = new TiXmlElement( "DataAttr" );
	doc->LinkEndChild( me );
	if( name )
	    me->SetAttribute( "name", name );
	if( value )
	    me->SetAttribute( "value", value );

	if( next )
	    next->save( doc );
}

DataAttr *
DataAttr::load( TiXmlElement *me )
{
	if( !me )
	    return NULL;
	const char *tmp;
	DataAttr *ds = new DataAttr();
	tmp = me->Attribute( "name" );
	if( tmp )
	    ds->name = cp_string( tmp );
	tmp = me->Attribute( "value" );
	if( tmp )
	    ds->value = cp_string( tmp );

	TiXmlElement *sib = me->NextSiblingElement( "DataAttr" );
	ds->next = DataAttr::load( sib );

	return ds;
}
