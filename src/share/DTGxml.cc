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
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
typedef struct stat STRUCTSTAT;
#else
#define stat _stat
typedef struct _stat STRUCTSTAT;
#endif

#include <DTG-platforms.h>
#include "DataSource.h"
#include "DataMapping.h"
#include "Settings.h"
#include "tinyxml.h"
#include "genutils.h"
#include "DTGxml.h"

extern "C" {
#include <dtg-utils.h>
}

extern struct DTGStrList *scan_dir( const char *dirname );

static int file_exists( const char *file )
{
	STRUCTSTAT buf;
	int i = stat( file, &buf );
	if( i && errno == ENOENT )
	    return 0;
	else
	    return 1;
}

static DataSource *append_src( DataSource *list, DataSource *item )
{
	if( !list )
	    return item;
	DataSource *i;
	for( i = list; i->next; i = i->next );
	i->next = item;
	return list;
}

static DataMapping *append_map( DataMapping *list, DataMapping *item )
{
	if( !list )
	    return item;
	DataMapping *i;
	for( i = list; i->next; i = i->next );
	i->next = item;
	return list;
}

static DTGSettings *append_set( DTGSettings *list, DTGSettings *item )
{
	if( !list )
	    return item;
	DTGSettings *i;
	for( i = list; i->next; i = i->next );
	i->next = item;
	return list;
}

void load_config( const char  *dir, 
		DataSource *&srcs, 
		DataMapping *&maps,
		DTGSettings *&sets )
{
	if( !dir )
	    return;

	struct DTGStrList *items = scan_dir( dir );
	for( struct DTGStrList *item = items; item; item = item->next )
	{
	    char *name = new char[strlen(dir)+strlen(item->value)+2];
	    sprintf( name, "%s%s%s", dir, DIRSEPARATOR, item->value );

	    DataSource *src = NULL;
	    DataMapping *map = NULL;
	    DTGSettings *set = NULL;
	    if( !strncasecmp( item->value, "map-", 4 ) ||
	        !strncasecmp( item->value, "src-", 4 ) )
	    {
	        if( load_p4dtg_config( name, src, map ) < 1 )
	            printf( "Error loading: %s\n", item->value );
	        srcs = append_src( srcs, src );
	        maps = append_map( maps, map );
	    }
	    else if( !strncasecmp( item->value, "set-", 4 ) )
	    {
	        if( load_p4dtg_settings( name, set ) < 1 )
	            printf( "Error loading: %s\n", item->value );
	        sets = append_set( sets, set );
	    }

	    delete[] name;
	}
	delete_DTGStrList( items );
}

int load_p4dtg_config( const char *file, 
			DataSource *&sources, 
			DataMapping *&mappings )
{
	sources = NULL;
	mappings = NULL;
	if( !file )
	    return 0;
	TiXmlDocument docIN( file );
	if( !docIN.LoadFile() )
	{
	    STRUCTSTAT buf;
	    int i = stat( file, &buf );
	    if( i && errno == ENOENT )
	    {
	        /* If file does not exist, start blank */
	        sources = NULL;
	        mappings = NULL;
	        return 1;
	    }
	    return 0;
	}
	
	TiXmlElement *me = docIN.FirstChildElement( "DTGConfiguration" );
	const char *tmp = me ? me->Attribute( "version" ) : NULL;
	if( tmp )
	{
	    if( atoi( tmp ) != 1 )
	        return -1;
	}
	else
	    return -1;
	TiXmlHandle hdocIN( me );
	sources = DataSource::load(
		hdocIN.FirstChildElement( "DataSource" ).Element() );
	mappings = DataMapping::load(
		hdocIN.FirstChildElement( "DataMapping" ).Element() );
	return 1;
}

static int create_backup( const char *file )
{
	FILE *cur = fopen( file, "r" );
	if( !cur )
	    return errno == ENOENT;
 
	char *backup = mk_string( file, ".old" );
	if( unlink( backup ) && errno != ENOENT )
	{
	    delete[] backup;
	    fclose( cur );
	    return 0;
	}

	FILE *old = fopen( backup, "w" );
	if( !old )
	{
	    delete[] backup;
	    fclose( cur );
	    return 0;
	}
 
	char copybuf[1024];
	int r = 0;
	int w = 0;
	while( ( r = fread( copybuf, 1, sizeof( copybuf ), cur ) ) > 0 )
	{
	    w = fwrite( copybuf, 1, r, old );
	    if( w < r )
	    {
	        unlink( backup );
	        delete[] backup;
	        fclose( old );
	        fclose( cur );
	        return 0;
	    }
	}
 
	delete[] backup;
	fclose( old );
	fclose( cur );
	return 1;
}

int save_p4dtg_config( const char *file, 
			DataSource *source, 
			DataMapping *mapping,
			int all )
{
	TiXmlDocument docOUT;
	TiXmlDeclaration *decl = new TiXmlDeclaration( "1.0", "", "" );
	docOUT.LinkEndChild( decl );
	TiXmlElement *me = new TiXmlElement( "DTGConfiguration" );
	docOUT.LinkEndChild( me );
	me->SetAttribute( "version", "1" );
	char *stamp = timestamp();
	me->SetAttribute( "updated", stamp );
	delete[] stamp;
	DataSource *src = source;
	while( src )
	{
	    src->save( me );
	    if( !all )
	        break;
	    src = src->next;
	}
	DataMapping *map = mapping;
	while( map )
	{
	    map->save( me );
	    if( !all )
	        break;
	    map = map->next;
	}
	if( docOUT.SaveFile( file ) )
	{
	    src = source;
	    while( src )
	    {
	        src->dirty = 0;
	        if( !all )
	            break;
	        src = src->next;
	    }
	    map = mapping;
	    while( map )
	    {
	        map->dirty = 0;
	        if( !all )
	            break;
	        map = map->next;
	    }
	    return 0; // save succeeded
	}
	return 1; // save failed
}

int save_config( const char  *dir, 
		DataSource *srcs1, 
		DataSource *srcs2, 
		DataMapping *maps,
		DTGSettings *sets )
{
	int failures = 0;
	DataSource *src;
	for( src = srcs1; src; src = src->next )
	{
	    if( !src->dirty )
	        continue;
	    if( src->oldname )
	    {
	        char *oldname = mk_string( dir, "src-", src->oldname, ".xml" );
	        unlink( oldname );
	        delete[] oldname;
	    }
	    char *name = mk_string( dir, "src-", src->nickname, ".xml" );
	    if( src->deleted )
	        if( unlink( name ) && errno != ENOENT )
	            failures++;
	        else
	            src->dirty = 0;
	    else if( save_p4dtg_config( name, src, NULL ) )
	        failures++;
	    else
	        src->dirty = 0;
	    delete[] name;
	}
	for( src = srcs2; src; src = src->next )
	{
	    if( !src->dirty )
	        continue;
	    if( src->oldname )
	    {
	        char *oldname = mk_string( dir, "src-", src->oldname, ".xml" );
	        unlink( oldname );
	        delete[] oldname;
	    }
	    char *name = mk_string( dir, "src-", src->nickname, ".xml" );
	    if( src->deleted )
	        if( unlink( name ) && errno != ENOENT )
	            failures++;
	        else
	            src->dirty = 0;
	    else if( save_p4dtg_config( name, src, NULL ) )
	        failures++;
	    else
	        src->dirty = 0;
	    delete[] name;
	}
	for( DataMapping *map = maps; map; map = map->next )
	{
	    if( !map->dirty && 
		(!map->scm || !map->scm->dirty) && 
		(!map->dts || !map->dts->dirty) )
	        continue;
	    if( map->oldid )
	    {
	        char *oldid = mk_string( dir, "map-", map->oldid, ".xml" );
	        unlink( oldid );
	        delete[] oldid;
	    }
	    char *id = mk_string( dir, "map-", map->id, ".xml" );
	    if( map->deleted )
	        if( unlink( id ) && errno != ENOENT )
	            failures++;
	        else
	        {
	            map->dirty = 0;
	            char *set = mk_string( dir, "set-", map->id, ".xml" );
	            unlink( set );
	            delete[] set;
	        }
	    else if( save_p4dtg_config( id, NULL, map ) )
	        failures++;
	    else
	        map->dirty = 0;
	    delete[] id;
	}
	for( DTGSettings *set = sets; set; set = set->next )
	{
	    if( !set->dirty )
	        continue;
	    set->dirty = 0;
	    if( set->oldid )
	    {
	        char *oldid = mk_string( dir, "set-", set->oldid, ".xml" );
	        unlink( oldid );
	        delete[] oldid;
	    }
	    char *id = mk_string( dir, "set-", set->id, ".xml" );
	    if( set->deleted )
	        if( unlink( id ) && errno != ENOENT )
	            failures++;
	        else
	            set->dirty = 0;
	    else if( save_p4dtg_settings( id, set ) )
	        failures++;
	    else
	        set->dirty = 0;
	    delete[] id;
	}
	return failures;
}

int load_p4dtg_settings( const char *file, DTGSettings *&settings )
{
	settings = NULL;
	if( !file )
	    return 0;
	TiXmlDocument docIN( file );
	if( !docIN.LoadFile() )
	{
	    STRUCTSTAT buf;
	    int i = stat( file, &buf );
	    if( i && errno == ENOENT )
	        /* If file does not exist, start blank */
	        return 1;
	    return 0;
	}
	
	TiXmlElement *me = docIN.FirstChildElement( "DTGGateway" );
	const char *tmp = me ? me->Attribute( "version" ) : NULL;
	if( tmp )
	{
	    if( atoi( tmp ) != 1 )
	        return -1;
	}
	else
	    return -1;
	TiXmlHandle hdocIN( me );
	settings = DTGSettings::load(
		hdocIN.FirstChildElement( "DTGSettings" ).Element() );
	return 1;
}

int save_p4dtg_settings( const char *file, DTGSettings *settings )
{
	TiXmlDocument *docOUT = new TiXmlDocument;
	TiXmlDeclaration *decl = new TiXmlDeclaration( "1.0", "", "" );
	docOUT->LinkEndChild( decl );
	TiXmlElement *me = new TiXmlElement( "DTGGateway" );
	docOUT->LinkEndChild( me );
	me->SetAttribute( "version", "1" );
	char *stamp = timestamp();
	me->SetAttribute( "updated", stamp );
	delete[] stamp;
	if( settings )
	    settings->save( me );

	/* Try to create backup, if it fails, the previous values will be ok */
	int failure = 1;
	if( create_backup( file ) )
	    failure = !docOUT->SaveFile( file );
	if( !failure && settings )
	    settings->dirty = 0;
	delete docOUT;
	return failure;
}
