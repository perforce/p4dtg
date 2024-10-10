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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <DTGModule.h>
extern "C" {
#include <dtg-utils.h>
}
#include <DataSource.h>
#include <DataMapping.h>
#include <DataAttr.h>
#include <Settings.h>
#include <DTGxml.h>
#include <plugins.h>
#include "Unify.h"
#include "utils.h"
#include "Logger.h"
#include <genutils.h>
#ifndef _WIN32
#include <unistd.h>
#endif

DTGSettings *settings = NULL;
int QUERYLIMIT = 1000;
int WAITTIME = 150;
long CYCLE_THRESHOLD = 0L;
long UPDATE_PERIOD = 0L;

#ifndef LOGLEVEL
#define LOGLEVEL 1
#endif

extern const char *stop_service( const char *service );

void cleanup( const char *id, Logger *log, const char *file )
{
#ifdef _WIN32
	if( id )
	{
	    // So far only cleanup needed is for WIN32
	    char *unify = mk_string( "p4dtg-service-", id );
	    const char *errmsg = stop_service( unify );
	    if( *errmsg && log )
	        log->log( 0, "Unable to stop service %s: %s", unify, errmsg );
	    delete[] unify;
	}
#endif
	if( file )
	    unlink( file );
	if( log )
	{
	    log->log( 2, "Info: cleanup called on %s", id );
	    delete log;
	}
}

static void process_sleep( int dur, const char *stop_file )
{
	if( !stop_file )
	    return;

	const int period = 2; // check every 2 seconds for stop request
	struct stat buf;
	while( dur > 0 )
	{
	    if( dur < period )
	        do_sleep( dur );
	    else
	        do_sleep( period );

	    dur -= period;

	    if( !stat( stop_file, &buf ) )
	        break;
	}
}

#ifdef _WIN32
int __stdcall
WinMain( void * /* hInstance */, 
	void * /* hPrevInstance */, 
	char *lpCmdLine,
	int /* nCmdShow */ )
{
	int argc = 2;
	char *argv[3];
	argv[0] = strdup( "p4dtg-repl" );
	argv[1] = lpCmdLine;
	char *tmp = lpCmdLine;
	while( tmp && *tmp && *tmp != ' ' )
	    tmp++;
	if( tmp && *tmp == ' ' );
	    *tmp++ = '\0';
	argv[2] = tmp;
#else
int 
main( int argc, char *argv[] )
{
#endif
	char *tag1 = mk_string( "Rev. p4dtg-repl/", 
		ID_OS, "/", ID_REL, "/", ID_PATCH );
	char *tag2 = mk_string( ID_Y, " ", ID_M, " ", ID_D );
	if( argc == 2 && argv[1][0] == '-' )
	    if( argv[1][1] == 'h' )
	    {
	        printf( "Usage: %s MAPPING {DTG_ROOT} \n", argv[0] );
	        printf( "       MAPPING defines the map-MAPPING.xml to use\n" );
	        printf( "       Override any DTG_ROOT setting\n" );
	        return 0;
	    }
	    else if( argv[1][1] == 'V' )
	    {
	        printf( "%s\n%s\n", tag1, tag2 );
	        delete[] tag1;
	        delete[] tag2;
	        return 0;
	    }
	if( argc < 2 )
	{
	    printf( "Usage: %s MAPPING {DTG_ROOT} \n", argv[0] );
	    printf( "       MAPPING defines the map-MAPPING.xml to use\n" );
	    printf( "       Override any DTG_ROOT setting\n" );
	    return 1;
	}

	char *root;
	if( argc == 3 )
	    root = cp_string( argv[2] );
	else if( argc == 2 )
	    root = cp_string( getenv( "DTG_ROOT" ) );
	else
	{
	    printf( "Usage: %s MAPPING {DTG_ROOT} \n", argv[0] );
	    printf( "       MAPPING defines the map-MAPPING.xml to use\n" );
	    printf( "       Override any DTG_ROOT setting\n" );
	    return 1;
	}
	if( !root )
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
	const char *mapname = argv[1];
	char *run_file = 
		mk_string( root, "repl", DIRSEPARATOR, "run-", mapname );
	struct stat buf;
	if( !stat( run_file, &buf ) )
	{
	    printf( "Error: Replication server is already running\n" );
	    // No cleanup so it does not mess up currently running version
	    return 1;
	}

	FILE *fd = fopen( run_file, "a" );
	if( !fd )
	{
	    printf( "Error: Unable to create run-file: %s\n", run_file );
	    cleanup( mapname, NULL, NULL );
	    return 1;
	}
	fclose( fd );

	char *log_file = 
	    mk_string( root, "repl", DIRSEPARATOR, "log-", mapname, ".log" );
	Logger *log = new Logger( log_file, LOGLEVEL );
	if( !log->log_open() )
	{
	    printf( "Error: Unable to open log file: %s\n", log_file );
	    delete log; // Cannot use log so delete it
	    cleanup( mapname, NULL, run_file );
	    return 1;
	}

	sprintf( run_file, "%srepl%serr-%s", root, DIRSEPARATOR, mapname );
	if( !stat( run_file, &buf ) )
	{
	    log->log( 0, "Fatal: Review error log and correct fatal errors" );
	    log->log( 0, "Error Log: %s", run_file );
	    log->log( 0, "Delete error log to enable restarting of engine" );
	    sprintf( run_file, "%srepl%srun-%s", root, DIRSEPARATOR, mapname );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	sprintf( run_file, "%srepl%srun-%s", root, DIRSEPARATOR, mapname );

	log->log( 0, tag1 );
	delete[] tag1;
	log->log( 0, tag2 );
	delete[] tag2;
	log->log( 0, "Root: %s", root );

	char *setting_file = mk_string( root, "config", DIRSEPARATOR, 
		"set-", mapname, ".xml" );
	log->log( 0, "SettingFile: %s", setting_file );
	if( !load_p4dtg_settings( setting_file, settings ) )
	{
	    log->log( 0, "Unable to load settings: %s", setting_file );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !settings )
	{
	    log->log( 0, "Settings file does not contain settings" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	char *setdate = new char[64];
	snprintf( setdate, 64, "Start: %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		settings->starting_date->year,
		settings->starting_date->month,
		settings->starting_date->day,
		settings->starting_date->hour,
		settings->starting_date->minute,
		settings->starting_date->second );
	log->log( 0, setdate );
	if( settings->last_update )
	{
	    snprintf( setdate, 64, 
	        "LastUpdate(obsolete): %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		settings->last_update->year,
		settings->last_update->month,
		settings->last_update->day,
		settings->last_update->hour,
		settings->last_update->minute,
		settings->last_update->second );
	    log->log( 0, setdate );
	}
	snprintf( setdate, 64, "SCMLast: %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		settings->last_update_scm->year,
		settings->last_update_scm->month,
		settings->last_update_scm->day,
		settings->last_update_scm->hour,
		settings->last_update_scm->minute,
		settings->last_update_scm->second );
	log->log( 0, setdate );
	snprintf( setdate, 64, "DTSLast: %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		settings->last_update_dts->year,
		settings->last_update_dts->month,
		settings->last_update_dts->day,
		settings->last_update_dts->hour,
		settings->last_update_dts->minute,
		settings->last_update_dts->second );
	log->log( 0, setdate );
	delete[] setdate;
	log->log( 0, "Force Update: %s", settings->force ? "Yes" : "No" );

	DataSource *sources = NULL;
	DataMapping *map = NULL;
	char *mapping_file = mk_string( root, "config", DIRSEPARATOR, 
		"map-", mapname, ".xml" );
	log->log( 0, "MappingFile: %s", mapping_file );
	int status = load_p4dtg_config( mapping_file, sources, map );
	delete[] mapping_file;
	switch( status )
	{
	case -1:
	    log->log( 0, "Incompatable mapping file" );
	    cleanup( mapname, log, run_file );
	    return 1;
	case 0:
	    log->log( 0, "Failed to load mapping file" );
	    cleanup( mapname, log, run_file );
	    return 1;
	default:
	    break;
	}
	if( sources )
	{
	    log->log( 0, "Mapping file contains source definitions" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !map )
	{
	    log->log( 0, "Mapping file does not contain mapping definition" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( map->next )
	{
	    log->log( 0, "Mapping file contains multiple mapping definitions" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !map->scm_id || !map->dts_id )
	{
	    log->log( 0,
		"Mapping file does not define both scm_id and dts_id" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !map->dts_to_scm_rules &&
	    !map->scm_to_dts_rules &&
	    !map->mirror_rules )
	{
	    log->log( 0, "Mapping file does not contain any field mappings" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}

	char *source_file = mk_string( root, "config", DIRSEPARATOR, 
		"src-", map->scm_id, ".xml" );
	log->log( 0, "SCMSourceFile: %s", source_file );
	DataMapping *no_maps = NULL;
	status = load_p4dtg_config( source_file, sources, no_maps );
	delete[] source_file;
	switch( status )
	{
	case -1:
	    log->log( 0, "Incompatable source file" );
	    cleanup( mapname, log, run_file );
	    return 1;
	case 0:
	    log->log( 0, "Failed to load source file" );
	    cleanup( mapname, log, run_file );
	    return 1;
	default:
	    break;
	}
	if( no_maps )
	{
	    log->log( 0, "Source file contains mapping definitions" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !sources )
	{
	    log->log( 0, "Source file does not contain source definition" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( sources->next )
	{
	    log->log( 0, "Source file contains multiple source definitions" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	map->scm = sources;
	map->scm->map = map;

	source_file = mk_string( root, "config", DIRSEPARATOR, 
		"src-", map->dts_id, ".xml" );
	log->log( 0, "DTSSourceFile: %s", source_file );
	sources = NULL;
	no_maps = NULL;
	status = load_p4dtg_config( source_file, sources, no_maps );
	delete[] source_file;
	switch( status )
	{
	case -1:
	    log->log( 0, "Incompatable source file" );
	    cleanup( mapname, log, run_file );
	    return 1;
	case 0:
	    log->log( 0, "Failed to load source file" );
	    cleanup( mapname, log, run_file );
	    return 1;
	default:
	    break;
	}
	if( no_maps )
	{
	    log->log( 0, "Source file contains mapping definitions" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !sources )
	{
	    log->log( 0, "Source file does not contain source definition" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( sources->next )
	{
	    log->log( 0, "Source file contains multiple source definitions" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !sources->moddate_field )
	{
	    log->log( 0, "Source file missing definition of required field" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	map->dts = sources;
	map->dts->map = map;

	char *plugin_dir = mk_string( root, "plugins" );
	log->log( 0, "PluginDir: %s", plugin_dir );
	DTGModule *plugins = load_plugins( plugin_dir );
	delete[] plugin_dir;
	if( !plugins )
	{
	    log->log( 0, "Missing plugins" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	if( !plugins->has_perforce_extensions() )
	{
	    log->log( 0, "Perforce plugin missing vital interfaces" );
	    cleanup( mapname, log, run_file );
	    return 1;
	}
	DTGError *err = new_DTGError(NULL);
	for( DTGModule *p = plugins; p; p = p->next )
	{
	    log->log( 0, "Loaded Plugin: %s:", p->dl_name );
	    log->log( 0, "%s (%s)", p->dt_get_name( err ), 
		p->dt_get_module_version( err ) );
	}
	delete_DTGError( err );

	sources = map->scm;
	sources->next = map->dts;
	DataSource::assign_plugins( plugins, sources );

	// Process Map Level Attributes
	int polling_period = 5;
	int enable_write_to_readonly = 0;
	DataAttr *a;
	for( a = map->attrs; a; a = a->next )
	{
	    log->log( 0, "Map Attr: %s=%s", a->name, a->value );
	    if( a->name && a->value )
	        if( !strcmp( a->name, "log_level" ) )
	            log->set_level( atoi( a->value ) );
	        else if( !strcmp( a->name, "polling_period" ) )
	            polling_period = atoi( a->value );
	        else if( !strcmp( a->name, "connection_reset" ) )
	            QUERYLIMIT = atoi( a->value );
	        else if( !strcmp( a->name, "wait_duration" ) )
	            WAITTIME = atoi( a->value );
	        else if( !strcmp( a->name, "cycle_threshold" ) )
	            CYCLE_THRESHOLD = atol( a->value );
	        else if( !strcmp( a->name, "update_period" ) )
	            UPDATE_PERIOD = atol( a->value );
	        else if( !strcmp( a->name, "enable_write_to_readonly" ) )
	            enable_write_to_readonly = atol( a->value );
	}
	if( polling_period < 1 )
	    polling_period = 1;
	else if( polling_period > 100 )
	    polling_period = 100;
	char intstr[32];
	sprintf( intstr, "%d", log->get_level() );
	log->log( 0, "Log Level: %s", intstr );

	sprintf( intstr, "%d", polling_period );
	log->log( 0, "Polling Period: %s", intstr );

	if( QUERYLIMIT < 1 )
	    QUERYLIMIT = 1;
	else if( QUERYLIMIT > 1000000 )
	    QUERYLIMIT = 1000000;
	sprintf( intstr, "%d", QUERYLIMIT );
	log->log( 0, "Connection Reset: %s", intstr );

	if( WAITTIME < 1 )
	{
	    WAITTIME = -1;
	    log->log( 0, "No general offline wait duration" );
	}
	else
	{
	    sprintf( intstr, "%d", WAITTIME );
	    log->log( 0, "General offline wait duration: %s", intstr );
	}

	if( CYCLE_THRESHOLD < 0 )
	    CYCLE_THRESHOLD = 0L;
	if( UPDATE_PERIOD < 0 )
	    UPDATE_PERIOD = 0L;
	sprintf( intstr, "%ld", CYCLE_THRESHOLD );
	log->log( 0, "Cycle Logging Threshold: %s", intstr );
	sprintf( intstr, "%ld", UPDATE_PERIOD );
	log->log( 0, "Cycle Logging Update Period: %s", intstr );
	sprintf( intstr, "%ld", enable_write_to_readonly );
	log->log( 0, "Enable write to SCM read-only: %s", intstr );

	char *stop_file = 
		mk_string( root, "repl", DIRSEPARATOR, "stop-", map->id );
	if( !stat( stop_file, &buf ) )
	{
	    log->log( 0, "Removing stop_file: %s", stop_file );
	    unlink( stop_file );
	}

	for( DataSource *src = sources; src; src = src->next )
	{
	    src->check_connection();
	    if( src->error )
	        log->log( 0, "Error from check_connection: %s", src->error );
	}

	while( WAITTIME > 0 && 
		( map->scm->status != DataSource::READY || 
		    ( map->dts->status != DataSource::PASS && 
		      map->dts->status != DataSource::READY ) ) )
	{
	    log->log( 0, "At least one of the servers is not ready" );
	    log->log( 0, "Waiting general offline wait duration" );
	    
	    int n = WAITTIME;

	    process_sleep( n, stop_file );

	    if( !stat( stop_file, &buf ) )
	    {
	        log->log( 0, "Discovered stop_file, beginning shutdown" );
	        break;
	    }

	    for( DataSource *src = sources; src; src = src->next )
	        src->check_connection();
	}
	delete[] stop_file;
	stop_file = NULL;

	if( map->validate( enable_write_to_readonly, log ) > 0 )
	{
	    log->log( 0, "Recheck on new SCM: %s",
	              map->recheck_on_new_scm ? "Y" : "N" );
	    log->log( 0, "Recheck on new DTS: %s",
	              map->recheck_on_new_dts ? "Y" : "N" );

	    char *tmp = mk_string( map->scm_id, ": ", map->scm->server );
	    log->log( 0, "Valid map SCM - %s", tmp, 1 );
	    delete[] tmp;
	    if( map->scm->version )
	        log->log( 0, "Server version: %s", map->scm->version );
	    if( map->scm->error )
	        log->log( 0, "Connection error: %s", map->scm->error );
	    if( map->scm->error )
	        log->log( 0, "Connection warning: %s", map->scm->warnings );
	    for( a = map->scm->attrs; a; a = a->next )
	    {
	        log->log( 0, "SCM Attr: %s=%s", a->name, a->value );
	        tmp = map->scm->validate_attribute( a );
	        if( tmp )
	        {
	            log->log( 0, "FATAL: SCM Attribute %s = %s is invalid", 
				a->name, a->value );
	            log->log( 0, "FATAL: Validation error: %s", tmp );
	            delete[] tmp;
	            cleanup( mapname, log, run_file );
	            return 1;
	        }
	    }

	    tmp = mk_string( map->dts_id, ": ", map->dts->server );
	    log->log( 0, "Valid map DTS - %s", tmp );
	    delete[] tmp;
	    if( map->dts->version )
	        log->log( 0, "Server version: %s", map->dts->version );
	    if( map->dts->error )
	        log->log( 0, "Connection error: %s", map->dts->error );
	    if( map->dts->error )
	        log->log( 0, "Connection warning: %s", map->dts->warnings );
	    for( a = map->dts->attrs; a; a = a->next )
	    {
	        log->log( 0, "DTS Attr: %s=%s", a->name, a->value );
	        tmp = map->dts->validate_attribute( a );
	        if( tmp )
	        {
	            log->log( 0, "FATAL: DTS Attribute %s = %s is invalid", 
				a->name, a->value );
	            log->log( 0, "FATAL: Validation error: %s", tmp );
	            delete[] tmp;
	            cleanup( mapname, log, run_file );
	            return 1;
	        }
	    }

	    /* Link in any filter rules */
	    if( map->dts_filter )
	    {
	        FilterSet *set = map->dts->set_list;
	        while( set && strcmp( set->name, map->dts_filter ) )
	            set = set->next;
	        if( !set )
	        {
	            log->log( 0, "FATAL: DTS filter %s is invalid", 
				map->dts_filter );
	            cleanup( mapname, log, run_file );
	            return 1;
	        }
	        map->dts_filter_desc = set->filter_list->extract_filter();
	    }

	    if( map->scm_filter )
	    {
	        FilterSet *set = map->scm->set_list;
	        while( set && strcmp( set->name, map->scm_filter ) )
	            set = set->next;
	        if( !set )
	        {
	            log->log( 0, "FATAL: SCM filter %s is invalid", 
				map->scm_filter );
	            cleanup( mapname, log, run_file );
	            return 1;
	        }
	        map->scm_filter_desc = set->filter_list->extract_filter();
	    }

	    // Check for unicode compatibility
	
	    int scm_uni = map->scm->accept_utf8;
	    int dts_uni = map->dts->accept_utf8;

	    /* SCM 	DTS 	Action
		-1	-1	Fatal - p4 plug-in out-of-date
		-1	 0	Fatal - p4 plug-in out-of-date
		-1	 1	Fatal - p4 plug-in out-of-date

		 0	-1	Possible mismatch warning
		 1	-1	Possible mismatch warning

		 0	 1	Fatal mismatch error
		 1	 0	Fatal mismatch error

		 0	 0	OK
		 1	 1	OK
	    */
	    if( scm_uni == -1 )
	    {
	        log->log( 0, "FATAL: Perforce plug-in is out-of-date" );
	        cleanup( mapname, log, run_file );
	        return 1;
	    }
	    if( dts_uni == -1 )
	        if( scm_uni )
	            log->log( 0, "WARNING: SCM is using UTF-8, DTS may not "
				 "support it." );
	        else
	            log->log( 0, "WARNING: SCM is not using UTF-8, DTS may be "
				 " using UTF-8." );
	    else if( dts_uni != scm_uni )
	    {
	        if( scm_uni )
	            log->log( 0, "FATAL: SCM is using UTF-8, DTS does not "
				 "support it." );
	        else
	            log->log( 0, "FATAL: DTS is using UTF-8, SCM does not "
				 "support it." );
	        cleanup( mapname, log, run_file );
	        return 1;
	    }

	    Unify *uni_map = new Unify( map, log );
	    uni_map->stop_file = 
		mk_string( root, "repl", DIRSEPARATOR, "stop-", map->id );
	    if( !stat( uni_map->stop_file, &buf ) )
	    {
	        log->log( 0, "Removing stop_file: %s", uni_map->stop_file );
	        unlink( uni_map->stop_file );
	    }
	    uni_map->run_file = 
		mk_string( root, "repl", DIRSEPARATOR, "run-", map->id );
	    uni_map->err_file = 
		mk_string( root, "repl", DIRSEPARATOR, "err-", map->id );

	    // Check for any pending messages from plug-ins
	    DTGError *err = new_DTGError( NULL );
	    int i = 
		map->scm->my_mod->dt_get_message( uni_map->get_scmID(), err );
	    if( i < 4 )
	        log->log( i, err->message );
	    clear_DTGError( err );
	    i = map->dts->my_mod->dt_get_message( uni_map->get_dtsID(), err );
	    if( i < 4 )
	        log->log( i, err->message );
	    delete_DTGError( err );

	    while( 1 )
	    {
	        if( uni_map->stop_exists() )
	            break;

	        int ret = uni_map->unify( settings );

	        if( !ret )
	            break;

	        if( ret == -1 )
	        {
	            DTGError * error = new_DTGError( NULL );
	            int scm = map->scm->my_mod->dt_server_offline(
	                             uni_map->get_scmID(), error
	                           );

	            if( scm == -1 )
	            {
	                clear_DTGError( error );
	                struct DTGDate *d = 
				map->scm->my_mod->dt_get_server_date( 
					uni_map->get_scmID(), error );
	                delete_DTGDate( d );
	                if( error->message )
	                {
	                    log->log( 0, "SCM is offline." );
	                    if( WAITTIME < 0 )
	                    {
	                        delete_DTGError( error );
	                        break;
	                    }
	                    log->log( 0, "Using general wait duration." );
	                    scm = WAITTIME;
	                }
	                else
	                    scm = 0;
	            }
	            else if( scm > 0 )
	                log->log( 0, "SCM offline, waiting" );

	            clear_DTGError( error );

	            int dts = map->dts->my_mod->dt_server_offline(
	                             uni_map->get_dtsID(), error
	                           );

	            if( dts == -1 )
	            {
	                clear_DTGError( error );
	                struct DTGDate *d = 
				map->dts->my_mod->dt_get_server_date( 
					uni_map->get_dtsID(), error );
	                delete_DTGDate( d );
	                if( error->message )
	                {
	                    log->log( 0, "DTS is offline." );
	                    if( WAITTIME < 0 )
	                    {
	                        delete_DTGError( error );
	                        break;
	                    }
	                    log->log( 0, "Using general wait duration." );
	                    dts = WAITTIME;
	                }
	                else
	                    dts = 0;
	            }
	            else if( dts > 0 )
	                log->log( 0, "DTS offline, waiting" );

	            delete_DTGError( error );
	            
	            int wait_time = 0;
	            if( scm >= dts )
	                wait_time = scm;
	            else
	                wait_time = dts;

	            char sec[ 255 ];
	            if( wait_time < 1 )
	                // Always wait at least 1 second
	                wait_time = 1;
	            sprintf( sec, "%d", wait_time );
	            log->log( 0, "Sleeping for:  %s seconds.", sec );

	            int n = wait_time;

	            process_sleep( n, uni_map->stop_file );

	            continue;
	        } 
		// failed during reset_servers
	        else if( ret == -2 )
	        {
	            if( WAITTIME <= 0 )
	            {
	                log->log( 0, "At least one server failed to connect "
				 "during server reset. General Wait Duration "
				 "requires replication to halt." );
	                break;
	            }

	            log->log( 0, "At least one server failed to connect during "
				 "server reset. Attempting to restore contact");
	            while( !uni_map->reset_servers() )
	            {
	                int n = WAITTIME;
	                char sec[ 255 ];
	                sprintf( sec, "%d", n );
	                log->log( 0, "Sleeping for:  %s seconds.", sec );

	                process_sleep( n, uni_map->stop_file );

	                log->log( 0, "Attempting to reset servers" );
	                if( uni_map->stop_exists() )
	                    break;
	            }
	            continue;
	        }

	        if( settings->force )
	        {
	            settings->force = 0;
	            log->log( 0, "Unsetting force flag" );
	            set_DTGDate( settings->last_update_scm, 
				settings->starting_date );
	            set_DTGDate( settings->last_update_dts, 
				settings->starting_date );
	        }
	        if( lock_file( setting_file ) ) // will block
	        {
	            int failed = save_p4dtg_settings( setting_file, settings );
	            unlock_file( setting_file );
	            if( failed )
	            {
	                log->log( 0, "Fatal: Failed to save settings file" );
	                break;
	            }
	        }
	        else
	            log->log( 0, "Error obtaining lock on: %s", setting_file );
	        process_sleep( polling_period, uni_map->stop_file );
	    }
	    delete uni_map;
	}
	else
	    log->log( 0, "Failed to validate mapping" );
	
	if( settings )
	    delete settings;
	if( sources )
	    delete sources;
	if( map )
	    delete map;
	if( plugins )
	    delete plugins;

	stop_file = mk_string( root, "repl", DIRSEPARATOR, "stop-", mapname );
	if( stat( stop_file, &buf ) )
	    // Non-requested stop, need to stop service
	    cleanup( mapname, log, run_file );
	else
	    // Requested stop, no need to stop service
	    cleanup( NULL, log, run_file );
	delete[] stop_file;

	delete[] setting_file;
	delete[] run_file;
	delete[] root;
	delete[] log_file;
	return 0;
}
