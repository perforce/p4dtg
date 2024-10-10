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

#include "P4DTG.h"
#include "Help.h"
#include <genutils.h>
#include <QDir>
#include <QStringList>
#include <QListWidget>
#include <QGroupBox>
#include <QTextEdit>
#include <QListWidgetItem>
#include <DTGModule.h>
#include <DataSource.h>
#include <DataMapping.h>
#include <plugins.h>
extern "C" {
#include <dtg-utils.h>
}


P4DTG::P4DTG( const char *use_root )
{
	if( use_root )
	    root = cp_string( use_root );
	else
	    root = cp_string( getenv( "DTG_ROOT" ) );
	if( !root || !*root )
	    root = mk_string( ".", DIRSEPARATOR );
	else
	{
	    const char *dirsep = DIRSEPARATOR;
	    if( root[strlen(root)-1] != dirsep[0] )
	    {
	        char *tmp = mk_string( root, DIRSEPARATOR );
	        delete[] root;
	        root = tmp;
	    }
	}

	p4_srcs = NULL;
	dt_srcs = NULL;
	maps = NULL;
	help = NULL;

	plugins = NULL;
	p4plugin = NULL;
}

void P4DTG::cache_plugins()
{
	char *tmp = mk_string( root, "plugins" );
	p4plugin = plugins = load_plugins( tmp );
	delete[] tmp;
}

void P4DTG::setup_help()
{
	char *tmp = mk_string( root, "help" );
	help = new P4Help( tmp, "index.html" );
	delete[] tmp;
	about(); // generate about page
}

P4DTG::~P4DTG()
{
	if( root )
	    delete[] root;
	if( plugins )
	    delete plugins;
	plugins = NULL;
	if( help )
	    delete help;
}

static DataSource *append_src( DataSource *list, DataSource *src )
{
	if( !list )
	    return src;
	DataSource *tmp;
	for( tmp = list; tmp->next; tmp = tmp->next ) {};
	tmp->next = src;
	return list;
}

void P4DTG::add_maps( DataMapping *my_maps )
{
	maps = my_maps;
}

void P4DTG::add_sources( DataSource *srcs )
{
	p4_srcs = NULL;
	dt_srcs = NULL;
	DataSource *cur = srcs;
	while( cur )
	{
	    DataSource *next = cur->next;
	    cur->next = NULL;
	    if( cur->type == DataSource::SCM )
	        p4_srcs = append_src( p4_srcs, cur );
	    else
	        dt_srcs = append_src( dt_srcs, cur );
	    cur = next;
	}
}

void P4DTG::replace_src( DataSource *old, DataSource *cur )
{
	for( DataMapping *map = maps; map; map = map->next )
	    if( map->scm == old )
	    {
	        map->scm = cur;
	        map->set_filters();
	    }
	    else if( map->dts == old )
	    {
	        map->dts = cur;
	        map->set_filters();
	    }
}

#include "about-head.html"
#include "about-foot.html"

void P4DTG::about()
{
	char *about = mk_string( root, "help", DIRSEPARATOR, "about.html" );
	FILE *af = fopen( about, "w" );
	delete[] about;
	if( !af )
	{
	    printf( "Error: Unable to create 'about.html' file\n" );
	    return;
	}
	fprintf( af, "%s\n", about_header );
	fprintf( af, "<p>%s %s %s</p>\n", ID_Y, ID_M, ID_D );
	fprintf( af, "<p>Rev. p4dtg-config/%s/%s/%s</p>\n", 
		ID_OS,  ID_REL, ID_PATCH );
	fprintf( af, "<hr>\n" );
	fprintf( af, "<p>Loaded Plugins:\n" );
	DTGError *err = new_DTGError(NULL);
	for( DTGModule *p = plugins; p; p = p->next )
	    fprintf( af, "<BR>%s(%s): %s\n", 
		p->dt_get_name( err ), p->dl_name,
		p->dt_get_module_version( err ) );
	fprintf( af, "</p>\n" );
	delete_DTGError( err );
	fprintf( af, "%s\n", about_footer );
	fclose( af );
}

void possible_modules( DataSource *src, QStringList &list )
{
	if( !src )
	    return;
	for( struct DTGStrList *mod = src->get_modules(); mod; mod = mod->next )
	{
	    list << QUTF8( mod->value );
	    if( !src->module )
	        src->module = mk_string( mod->value );
	}
}

void possible_defectid( DataSource *src, QStringList &list )
{
	if( !src )
	    return;
	struct DTGFieldDesc *field;
	for( field = src->get_fields(); field; field = field->next )
	    if( !strcasecmp( "WORD", field->type ) && field->readonly == 4 )
	        list << QUTF8( field->name );
	if( list.empty() )
	    for( field = src->get_fields(); field; field = field->next )
	        if( !strcasecmp( "WORD", field->type ) )
	            list << QUTF8( field->name );
}

void possible_moddate( DataSource *src, QStringList &list )
{
	if( !src )
	    return;
	struct DTGFieldDesc *field;
	for( field = src->get_fields(); field; field = field->next )
	    if( !strcasecmp( "DATE", field->type ) && field->readonly == 2 )
	        list << QUTF8( field->name );
}

void possible_moduser( DataSource *src, QStringList &list )
{
	if( !src )
	    return;
	struct DTGFieldDesc *field;
	for( field = src->get_fields(); field; field = field->next )
	    if( !strcasecmp( "WORD", field->type ) && field->readonly == 3 )
	        list << QUTF8( field->name );
}

QListWidgetItem *find_item( QListWidget *list, 
				const char *suffix, const char *field )
{
	QList<QListWidgetItem *> items = 
	    list->findItems( QString( 
		QUTF8( *field ? field : "<empty>" ) ) + QUTF8( suffix ), 
		Qt::MatchStartsWith );
	if( !items.empty() )
	    return items.first();
	else
	    return NULL;
}

int P4DTG::unique_nickname( const char *name )
{
	DataSource *src;
	for( src = p4_srcs; src; src = src->next )
	    if( !strcasecmp( name, src->nickname ) )
	        return 0;
	for( src = dt_srcs; src; src = src->next )
	    if( !strcasecmp( name, src->nickname ) )
	        return 0;
	return 1;
}

int P4DTG::unique_mapid( const char *name )
{
	for( DataMapping *map = maps; map; map = map->next )
	    if( !strcasecmp( name, map->id ) )
	        return 0;
	return 1;
}

void set_server_status( DataSource *obj, QTextEdit *status, QGroupBox *box )
{
	if( !obj || !status )
	    return;
	switch( obj->status )
	{
	default:
	case DataSource::UNKNOWN:
	    if( obj->error )
	        status->append( QString( QUTF8( 
				"Unknown server status:" ) ) );
	    else
	        status->append( QString( QUTF8( 
				"Unknown server status." ) ) );
	    if( box ) box->setEnabled( false );
	    break;
	case DataSource::FAIL: 
	    if( obj->error )
	        status->append( QString( QUTF8( 
				"Unable to connect to server:" ) ) );
	    else
	        status->append( QString( QUTF8( 
				"Unable to connect to server." ) ) );
	    if( box ) box->setEnabled( false );
	    break;
	case DataSource::PASS:
	    if( obj->has_required_fields() )
	        if( obj->type == DataSource::SCM )
	            status->append( QString( QUTF8( 
			"Valid connection, "
			"server not configured." ) ) );
	        else
	            status->append( QString( QUTF8( 
			"Valid connection to server.") ) );
	    else
	        status->append( QString( QUTF8( 
			"Valid connection, "
			"server is missing one or more required fields." ) ) );
	    if( box ) box->setEnabled( true );
	    break;
	case DataSource::READY:
	    if( obj->has_required_fields() )
	        status->append( QString( QUTF8(
			"Valid connection and server configured." ) ) );
	    else
	        status->append( QString( QUTF8(
			"Valid connection, "
			"server missing one or more required fields." ) ) );
	    if( box ) box->setEnabled( true );
	    break;
	}
}

