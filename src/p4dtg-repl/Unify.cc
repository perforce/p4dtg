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
#include <ctype.h>
#include <DTGModule.h>
extern "C" {
#include <dtg-utils.h>
}
#include "DataSource.h"
#include "DataMapping.h"
#include "Unify.h"
#include "Logger.h"
#include <genutils.h>

void Unify::get_project_id( DataSource *src, void *&dtID, void *&projID )
{
	DTGError *err = new_DTGError( NULL );
	dtID = src->my_mod->dt_connect( 
	    src->server, src->user, src->password, src->fields(), err );
	if( err->message )
	{
	    // FAILED
	    log->log( 0, "Error: For DataSource: %s\n", src->nickname );
	    log->log( 0, "Error: unable to connect: %s\n", err->message );
	    projID = NULL;
	    dtID = NULL;
	    delete_DTGError( err );
	    return;
	}
	int i = src->my_mod->dt_get_message( dtID, err );
	if( i < 4 )
	    log->log( i, err->message );
	projID = src->my_mod->dt_get_project( dtID, src->module, err );
	if( err->message )
	{
	    // FAILED
	    log->log( 0, "Error: For DataSource: %s\n", src->nickname );
	    log->log( 0, "Error: unable to get project: %s\n", err->message );
	    src->my_mod->dt_free( dtID, err );
	    projID = NULL;
	    dtID = NULL;
	    delete_DTGError( err );
	    return;
	}
	i = src->my_mod->dt_get_message( dtID, err );
	if( i < 4 )
	    log->log( i, err->message );

	// Notify plug-in regarding field usage
	struct DTGStrList *used = map->dts_field_references();
	if( used )
	{
	    src->my_mod->proj_referenced_fields( projID, used );
	    delete_DTGStrList( used );
	    clear_DTGError( err );
	    int i = src->my_mod->dt_get_message( dtID, err );
	    if( i < 4 )
	        log->log( i, err->message );
	}

	// Notify plug-in regarding segmentation filters
	struct DTGFieldDesc *filters = NULL;
	if( (src->type == DataSource::SCM) && map->scm_filter_desc )
	    filters = map->scm_filter_desc;
	else if( (src->type == DataSource::DTS) && map->dts_filter_desc )
	    filters = map->dts_filter_desc;
	if( filters )
	{
	    src->my_mod->proj_segment_filters( projID, filters );
	    clear_DTGError( err );
	    int i = src->my_mod->dt_get_message( dtID, err );
	    if( i < 4 )
	    {
	        log->log( i, "%s Segmentation Filter", 
			src->type == DataSource::SCM ? "SCM" : "DTS" );
	        log->log( i, err->message );
	    }
	}

	delete_DTGError( err );
}

Unify::Unify( DataMapping *my_map, Logger *my_log )
{
	abort_run = 0;
	force_exit = 0;
	query_cnt = 0;
	map = my_map;
	since_dts = NULL;
	since_scm = NULL;
	log = my_log;
	stop_file = run_file = err_file = NULL;
	cur_scm = NULL;
	cur_dts = NULL;
	report_id = 0;
	scm_dirty = dts_dirty = 0;
	scm_recheck = NULL;
	scm_failed = NULL;

	// Convert "List of Change Numbers" to DTG_FIXES
	for( CopyRule *cr = map->scm_to_dts_rules; cr; cr = cr->next )
	    if( !strcmp( cr->scm_field, "List of Change Numbers" ) )
	        sprintf( cr->scm_field, "DTG_FIXES" );

	// Connect to servers
	scm = map->scm;
	scm_mod = scm->my_mod;
	scm_dtID = NULL;
	scm_projID = NULL;
	get_project_id( scm, scm_dtID, scm_projID );

	dts = map->dts;
	dts_mod = dts->my_mod;
	dts_dtID = NULL;
	dts_projID = NULL;
	get_project_id( dts, dts_dtID, dts_projID );
}

int Unify::reset_scm()
{
	// Disconnect from server
	DTGError *err = new_DTGError( NULL );
	if( scm_projID )
	    scm_mod->proj_free( scm_projID, err );
	if( scm_dtID )
	    scm_mod->dt_free( scm_dtID, err );
	delete_DTGError( err );

	// Connect to server
	scm_dtID = NULL;
	scm_projID = NULL;
	get_project_id( scm, scm_dtID, scm_projID );

	return scm_dtID && scm_projID;
}

int Unify::reset_dts()
{
	// Disconnect from server
	DTGError *err = new_DTGError( NULL );
	if( dts_projID )
	    dts_mod->proj_free( dts_projID, err );
	if( dts_dtID )
	    dts_mod->dt_free( dts_dtID, err );
	delete_DTGError( err );

	// Connect to server
	dts_dtID = NULL;
	dts_projID = NULL;
	get_project_id( dts, dts_dtID, dts_projID );

	return dts_dtID && dts_projID;
}

int Unify::reset_servers()
{
	(void)reset_scm();
	(void)reset_dts();
	return scm_dtID && dts_dtID && scm_projID && dts_projID;
}

Unify::~Unify()
{
	if( stop_file )
	    delete[] stop_file;
	if( run_file )
	    delete[] run_file;
	if( err_file )
	    delete[] err_file;
	if( cur_scm )
	    delete[] cur_scm;
	if( cur_dts )
	    delete[] cur_dts;
	if( scm_recheck )
	    delete_DTGStrList( scm_recheck );
	if( scm_failed )
	    delete_DTGField( scm_failed );

	// Disconnect from servers
	DTGError *err = new_DTGError( NULL );
	scm_mod->proj_free( scm_projID, err );
	scm_mod->dt_free( scm_dtID, err );
	dts_mod->proj_free( dts_projID, err );
	dts_mod->dt_free( dts_dtID, err );
	delete_DTGError( err );
}

char *Unify::convert( DTGModule *from, const char *new_val, 
			CopyRule *cr, DTGModule *to, int rev )
{
	char *tmp_free = NULL;
	char *i;
	struct DTGDate *date;
	if( !new_val )
	    new_val = "";
	switch( cr->copy_type )
	{
	case CopyRule::MAP:
	    if( rev )
	    {
	        for( CopyMap *cm = cr->mappings; cm; cm = cm->next )
	            if( !strcasecmp( new_val, cm->value2 ) )
	            {
	                tmp_free = strdup( cm->value1 );
	                break;
	            }
	    }
	    else
	    {
	        for( CopyMap *cm = cr->mappings; cm; cm = cm->next )
	            if( !strcasecmp( new_val, cm->value1 ) )
	            {
	                tmp_free = strdup( cm->value2 );
	                break;
	            }
	    }
	    if( !tmp_free )
	    {
	        if( *new_val )
	        {
	            log->log( 0, "Error: DTS(%s) SCM(%s)", cur_dts, cur_scm, 1);
	            log->log( 0, "Error: Field: DTS(%s) SCM(%s)", 
			    cr->dts_field, cr->scm_field, 1 );
	            log->log( 0, "Error: Unknown map value: %s", new_val );
	        }
	        tmp_free = strdup( "" );
	    }
	    break;
	case CopyRule::WORD:
	    tmp_free = strdup( new_val );
	    for( i = tmp_free; 
		*i && (isalnum( *i ) || ispunct( *i )); 
		i++ );
	    *i = '\0';
	    break;
	case CopyRule::LINE:
	    tmp_free = strdup( new_val );
	    for( i = tmp_free; 
		*i && *i != '\n' && *i != '\r'; 
		i++ );
	    *i = '\0';
	    break;
	case CopyRule::DATE:
	    if( *new_val )
	    {
	        date = from->extract_date( new_val );
	        tmp_free = to->format_date( date );
	        delete_DTGDate( date );
	    }
	    else
	        tmp_free = strdup( new_val );
	    break;
	default:
	case CopyRule::TEXT:
	    tmp_free = strdup( new_val );
	    break;
	}
	char *tmp;
	if( tmp_free )
	{
	    if( *tmp_free == '"' )
	    {
	        char *tst = strchr( &tmp_free[1], '"' );
	        if( !tst )
	            tmp = cp_string( &tmp_free[1] );
	        else
	            tmp = cp_string( tmp_free );
	    }
	    else
	        tmp = cp_string( tmp_free );
	    free( tmp_free );
	}
	else
	    tmp = cp_string( "" );
	return tmp;
}
