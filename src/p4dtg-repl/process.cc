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
#include <ctype.h>
#include <sys/stat.h>
#include <DTGModule.h>
extern "C" {
#include <dtg-utils.h>
}
#include "Unify.h"
#include "DataSource.h"
#include "DataMapping.h"
#include "Settings.h"

#include "utils.h"
#include "Logger.h"
#include <genutils.h>

extern int QUERYLIMIT;
extern long CYCLE_THRESHOLD;
extern long UPDATE_PERIOD;

static void log_large_cycles( Logger *log, long &cnt )
{
	if( CYCLE_THRESHOLD <= 0 || UPDATE_PERIOD <= 0 )
	    return;
	long cur = cnt - CYCLE_THRESHOLD;
	while( cur > 0 )
	    cur -= UPDATE_PERIOD;
	if( !cur )
	{
	    char tmp[64];
	    sprintf( tmp, "%ld", cnt );
	    log->log( 1, "INFO: Processed %s", tmp );
	}
}

static void log_large_cycles( Logger *log, 
				struct DTGStrList *list, const char *type )
{
	if( CYCLE_THRESHOLD <= 0 )
	    return;
	long cnt = 0L;
	for( struct DTGStrList *s = list; s; cnt++, s = s->next );
	if( cnt >= CYCLE_THRESHOLD )
	{
	    char tmp[64];
	    sprintf( tmp, "%ld", cnt );
	    log->log( 1, "INFO: Large cycle: %s items", tmp );
	}
}

char *Unify::format_fix( FixRule *fr, char *fixid )
{
	char *result = NULL;
	if( !fr || !fixid )
	    return cp_string( "Details unknown" );
	struct DTGError *err = new_DTGError( NULL );
	struct DTGFixDesc *fix = 
		scm_mod->proj_describe_fix( scm_projID, fixid, err );
	if( err->message )
	{
	    log->log( 0, "Error: DTS(%s), SCM(%s)", cur_dts, cur_scm, 1 );
	    log->log( 0, "Error: %s", err->message );
	    report_id = 1;
	}
	else
	{
	    int len = 12; // 4 possible \n additions plus room to grown on
	    if( fr->change_number )
	        len += strlen( "Change: " ) + strlen( fix->change );
	    if( fr->fixed_by )
	        len += strlen( ", User: " ) + strlen( fix->user );
	    if( fr->fixed_date )
	        len += strlen( ", Date: " ) + strlen( fix->stamp );
	    if( fr->description )
	        len += strlen( "Description: " ) + strlen( fix->desc );
	    if( fr->file_list )
	    {
	        len += strlen( "Files:\n" );
	        for( struct DTGStrList *item = fix->files; 
			item; 
			item = item->next )
	            len += 2 + strlen( item->value );
	    }
	    if( ( fr->change_number + 
			fr->fixed_by + fr->fixed_date + 
			fr->description + fr->file_list ) == 1 )
	    {
	        result = new char[len + 1];
	        result[0] = '\0';
	        if( fr->change_number )
	            strcat( result, fix->change );
	        else if( fr->fixed_by )
	            strcat( result, fix->user );
	        else if( fr->fixed_date )
	            strcat( result, fix->stamp );
	        else if( fr->description )
	            strcat( result, fix->desc );
	        else if( fr->file_list )
	        {
	            char *tmp_result = result;
	            for( struct DTGStrList *item = fix->files; 
			item; 
			item = item->next )
	            {
	                strcat( tmp_result, item->value );
	                if( item->next )
	                    strcat( tmp_result, "\n" );
	                tmp_result += (strlen( item->value ) - 1);
	            }
	        }
	    }
	    else
	    {
	        result = new char[len + 1];
	        result[0] = '\0';
	        if( fr->change_number || fr->fixed_by || fr->fixed_date )
	        {
	            if( fr->change_number )
	            {
	                strcat( result, "Change: " );
	                strcat( result, fix->change );
	            }
	            if( fr->fixed_by )
	            {
	                if( fr->change_number )
	                    strcat( result, ", User: " );
	                else
	                    strcat( result, "User: " );
	                strcat( result, fix->user );
	            }
	            if( fr->fixed_date )
	            {
	                if( fr->change_number || fr->fixed_by )
	                    strcat( result, ", Date: " );
	                else
	                    strcat( result, "Date: " );
	                strcat( result, fix->stamp );
	            }
	            if( fr->description || fr->file_list )
	                strcat( result, "\n" );
	        }
	        if( fr->description )
	        {
	            strcat( result, "Description: " );
	            strcat( result, fix->desc );
	            if( fr->file_list && result[strlen(result)-1] != '\n' )
	                strcat( result, "\n" );
	        }
	        if( fr->file_list )
	        {
	            strcat( result, "Files:\n" );
	            char *tmp_result = result;
	            for( struct DTGStrList *item = fix->files; 
				item; 
				item = item->next )
	            {
	                strcat( tmp_result, item->value );
	                if( item->next )
	                    strcat( tmp_result, "\n" );
	                tmp_result += (strlen( item->value ) - 1);
	            }
	        }
	    }
	    delete_DTGFixDesc( fix );
	}
	delete_DTGError( err );
	if( result && *result )
	{
	    int last = strlen( result ) - 1;
	    if( result[last] != '\n' )
	        strcat( result, "\n" );
	    else
	    {
	        while( result[last] == '\n' && last > 0 )
	            last--;
	        if( last == 0 )
	            if( result[last] == '\n' )
	                result[0] = '\0';
	            else
	                result[2] = '\0';
	        else
	            result[last+2] = '\0'; // truncate any extra \n's
	    }
	}
	return result;
}

#define SAFE_FREE( x ) { if( x ) free( x ); }

int Unify::fail_on_read_err( struct DTGError *err, const char *type,
				const char *id, const char *field )
{
	if( !err || !err->message )
	    return 0;

	const char *tmp = mk_string( type, "(", id, ")->", field );
	log->log( 0, "Fatal: Unable to retrieve field value: %s\n", tmp );
	delete[] tmp;

	log->log( 0, "Fatal: Error: %s\n", err->message );
	
	force_exit = 1;
	return 1;
}

void Unify::unify_defects( int scm_stat, void *scm_defect, 
			int dts_stat, void *dts_defect,
			struct DTGStrList *add, struct DTGStrList *del )
{
	struct DTGError *err = new_DTGError( NULL );
	char *dts_val = NULL, *scm_val = NULL;
	struct DTGStrList *item = NULL;
	log->log( 2, "Info: Processing DTS:%s SCM:%s", cur_dts, cur_scm );
	for( FixRule *fr = map->fix_rules; fr; fr = fr->next )
	{
	    char *newval = NULL;
	    for( item = add; item; item = item->next )
	    {
	        log->log( 3, "Info: Add Fix: %s", item->value );
	        char *tmp = format_fix( fr, item->value );
	        if( tmp )
	            if( newval && *newval )
	            {
	                char *newval_tmp = newval;
	                newval = mk_string( newval_tmp, "\n", tmp );
	                delete[] newval_tmp;
	                delete[] tmp;
	            }
	            else
	                newval = tmp;
	    }
	    for( item = del; item; item = item->next )
	    {
	        log->log( 3, "Info: Delete Fix: %s", item->value );
	        char *tmp = mk_string( "Deleted change ", item->value );
	        if( tmp )
	            if( newval )
	            {
	                char *newval_tmp = newval;
	                newval = mk_string( newval, "\n", tmp );
	                delete[] newval_tmp;
	                delete[] tmp;
	            }
	            else
	                newval = tmp;
	    }
	    if( newval )
	    {
	        dts_dirty++;
	        char *old_dtsval = dts_mod->defect_get_field( dts_defect,
	                                                      fr->dts_field,
	                                                      err );
	        // ignoring err
	        log->log( 3, "Info: Old[%s] Append[%s]", old_dtsval, newval );

	        if( old_dtsval )
	        {
	            char *tmp = newval;
	            if( old_dtsval[strlen( old_dtsval ) - 1] == '\n' )
	                newval = mk_string( old_dtsval, "\n", tmp );
	            else
	                newval = mk_string( old_dtsval, "\n\n", tmp );
	            SAFE_FREE( old_dtsval );
	            delete[] tmp;
	        }
	        dts_mod->defect_set_field( dts_defect, fr->dts_field,
	                               newval, err );
	        delete[] newval;
	    }
	}
	for( CopyRule *cr = map->mirror_rules; cr; cr = cr->next )
	{
	    scm_val = 
		scm_mod->defect_get_field( scm_defect, cr->scm_field, err );
	    if( fail_on_read_err( err, "SCM", cur_scm, cr->scm_field ) )
	        return;
	    dts_val = 
		dts_mod->defect_get_field( dts_defect, cr->dts_field, err );
	    if( fail_on_read_err( err, "DTS", cur_dts, cr->dts_field ) )
	        return;
	    if( scm_stat > 0 && dts_stat > 0 )
	    {
	        char *new_scm_val = 
			convert( dts_mod, dts_val, cr, scm_mod );
	        char *new_dts_val = 
			convert( scm_mod, scm_val, cr, dts_mod, 1 );
	        switch( cr->mirror_conflicts )
	        {
	          case CopyRule::DTS:
	          default:
	            if( set_field( scm_mod, scm_defect, cr->scm_field, 
					new_scm_val, scm_val, err ) )
	            {
	                log->log( 1, "Warning: DTS(%s), SCM(%s)", 
				    cur_dts, cur_scm, 1 );
	                log->log( 1, "Warning: Mirror of two modified defects");
	                log->log( 3, "Info: Set SCM:%s from DTS:%s", 
				    cr->scm_field, cr->dts_field );
	                log->log( 3, "Info: Old[%s] New[%s]", 
				    scm_val, new_scm_val);
	                scm_dirty++;
	            }
	            break;
	          case CopyRule::SCM:
	            if( set_field( dts_mod, dts_defect, cr->dts_field, 
					new_dts_val, dts_val, err ) )
	            {
	                log->log( 1, "Warning: DTS(%s), SCM(%s)", 
				    cur_dts, cur_scm, 1 );
	                log->log( 1, "Warning: Mirror of two modified defects");
	                log->log( 3, "Info: Set DTS:%s from SCM:%s", 
				    cr->dts_field, cr->scm_field );
	                log->log( 3, "Info: Old[%s] New[%s]", 
				    dts_val, new_dts_val);
	                dts_dirty++;
	            }
	            break;
	        }
	        delete[] new_scm_val;
	        delete[] new_dts_val;
	    }
	    else if( scm_stat > 0 )
	    {
	        char *new_val = 
			convert( scm_mod, scm_val, cr, dts_mod, 1 );
	        if( set_field( dts_mod, dts_defect, cr->dts_field, 
					new_val, dts_val, err ) )
	        {
	            log->log( 3, "Info: Set DTS:%s from SCM:%s",
				cr->dts_field, cr->scm_field );
	            log->log( 3, "Info: Old[%s] New[%s]", 
				dts_val, new_val);
	            dts_dirty++;
	        }
	        delete[] new_val;
	    }
	    else if( dts_stat > 0 )
	    {
	        char *new_val = 
			convert( dts_mod, dts_val, cr, scm_mod );
	        if( set_field( scm_mod, scm_defect, cr->scm_field, 
					new_val, scm_val, err ) )
	        {
	            log->log( 3, "Info: Set SCM:%s from DTS:%s",
				cr->scm_field, cr->dts_field );
	            log->log( 3, "Info: Old[%s] New[%s]", 
				scm_val, new_val);
	            scm_dirty++;
	        }
	        delete[] new_val;
	    }
	    SAFE_FREE( scm_val );
	    SAFE_FREE( dts_val );
	}
	for( CopyRule *cr = map->dts_to_scm_rules; cr; cr = cr->next )
	{
	    scm_val = 
		scm_mod->defect_get_field( scm_defect, cr->scm_field, err );
	    if( fail_on_read_err( err, "SCM", cur_scm, cr->scm_field ) )
	        return;
	    dts_val = 
		dts_mod->defect_get_field( dts_defect, cr->dts_field, err );
	    if( fail_on_read_err( err, "DTS", cur_dts, cr->dts_field ) )
	        return;
	    char *new_val = convert( dts_mod, dts_val, cr, scm_mod );
	    if( set_field( scm_mod, scm_defect, cr->scm_field, 
			   new_val, scm_val, err ) )
	    {
	        log->log( 3, "Info: Set SCM:%s from DTS:%s",
				cr->scm_field, cr->dts_field );
	        log->log( 3, "Info: Old[%s] New[%s]", 
				scm_val, new_val);
	        scm_dirty++;
	    }
	    delete[] new_val;
	    SAFE_FREE( scm_val );
	    SAFE_FREE( dts_val );
	}
	for( CopyRule *cr = map->scm_to_dts_rules; cr; cr = cr->next )
	{
	    scm_val = 
		scm_mod->defect_get_field( scm_defect, cr->scm_field, err );
	    if( fail_on_read_err( err, "SCM", cur_scm, cr->scm_field ) )
	        return;
	    dts_val = 
		dts_mod->defect_get_field( dts_defect, cr->dts_field, err );
	    if( fail_on_read_err( err, "DTS", cur_dts, cr->dts_field ) )
	        return;
	    char *new_val = convert( scm_mod, scm_val, cr, dts_mod );
	    if( set_field( dts_mod, dts_defect, cr->dts_field, 
				new_val, dts_val, err ) )
	    {
	        log->log( 3, "Info: Set DTS:%s from SCM:%s",
			cr->dts_field, cr->scm_field );
	        log->log( 3, "Info: Old[%s] New[%s]", 
			dts_val, new_val);
	        dts_dirty++;
	    }
	    delete[] new_val;
	    SAFE_FREE( scm_val );
	    SAFE_FREE( dts_val );
	}
	delete_DTGError( err );
}

/*
	Retrieve p4_defect
	if scm_filters and it doesn't match, continue
	if DTG_ERROR, continue
	if no DTG_* then, create new dt_defect
	else retrieve dt_defect
	Retrieve matching dt_defect
	if it doesn't exist, create a new one
	Unify Defects
	if dt_defect updated, save it
	free dt_defect
	if it's new, check to see it matches any dts_filters
	if it's new, update id field in p4_defect
	if p4_defect updated, save it
	free p4_defect
*/

char *Unify::update_fix_record( const char *id, void *scm_defect,
		struct DTGStrList *&add, struct DTGStrList *&del )
{
	add = del = NULL;
	struct DTGError *err = new_DTGError( NULL );
	struct DTGStrList *fixes = 
		scm_mod->proj_list_fixes( scm_projID, id, err );
	char *oldval = 
		scm_mod->defect_get_field( scm_defect, "DTG_FIXES", err );
	// ignore err
	clear_DTGError( err );

	char *tmpnew = join_DTGStrList( fixes, " " );
	log->log( 3, "Info: Checking fixes for: %s", id );
	log->log( 3, "Info: Old: %s, New: %s", oldval, tmpnew );
	free( tmpnew );

	delete_DTGError( err );
	struct DTGStrList *old = split_DTGStrList( oldval, ' ' );
	SAFE_FREE( oldval );
	remove_non_numerics( old );
	for( struct DTGStrList *f = fixes; f; f = f->next )
	    if( !in_DTGStrList( f->value, old ) )
	        add = append_DTGStrList( add, f->value );
	del = remove_DTGStrList( old, fixes );
	delete_DTGStrList( old );
	char *rev_free = join_DTGStrList( fixes, " " );
	delete_DTGStrList( fixes );
	char *rev = cp_string( rev_free );
	free( rev_free );
	return rev;
}

void Unify::process_scm_defect( const char *defect, int last_chance )
{
	log->log( 3, "Info: process_scm_defect( %s )", defect );
	report_id = scm_dirty = dts_dirty = 0;
	struct DTGError *err = new_DTGError( NULL );
	void *scm_defect = scm_mod->proj_get_defect( scm_projID, defect, err );
	if( !scm_defect )
	{
	    log->log( 0, "Error: Unable to retrieve scm defect: %s", defect );
	    log->log( 0, "Error: %s", err->message );
	    delete_DTGError( err );
	    return;
	}
	if( err->message )
	{
	    char *err_msg = mk_string( 
		    "Loading failure for defect: SCM: ", cur_scm, ": ",
		    err->message );
	    log->log( 0, "Error: %s", err_msg );
	    scm_failed = append_DTGField( scm_failed,
					new_DTGField( cur_scm, err_msg ) );
	    delete[] err_msg;
	    scm_mod->defect_free( scm_defect, err );
	    delete_DTGError( err );
	    return;
	}
	int ll = scm_mod->dt_get_message( scm_dtID, err );
	if( ll < 4 )
	    log->log( ll, err->message );

	char *value = 
		scm_mod->defect_get_field( scm_defect, "DTG_ERROR", err );
	// ignore err
	clear_DTGError( err );
	if( value && *value )
	{
	    free( value );
	    log->log( 1, "Warning: Skipping broken scm defect: %s", defect );
	    log->log( 1, "Warning: Any changes from scm are not replicated" );
	    scm_mod->defect_free( scm_defect, err );
	    delete_DTGError( err );
	    return;
	}
	char *filter_msg = NULL;
	if( map->scm_filter_desc && 
		(filter_msg = 
		    pass_filter( map->scm_filter_desc, scm_mod, scm_defect ) ) )
	{
	    if( map->scm->seg_ok ) // DTG_MAPID exists
	    {
	        value = 
		    scm_mod->defect_get_field( scm_defect, "DTG_MAPID", err );
	        // ignore err
	        clear_DTGError( err );
	        if( value && !strcasecmp( value, map->id ) )
	            log->log( 0, 
			"Error: SCM(%s) fails filter but matches current map",
			cur_scm );
	        SAFE_FREE( value );
	    }

	    log->log( 2, "Notice: Filtering scm defect: %s", defect );
	    log->log( 2, "Notice: %s", filter_msg );
	    delete[] filter_msg;
	    scm_mod->defect_free( scm_defect, err );
	    delete_DTGError( err );
	    return;
	}

	if( map->scm->seg_ok ) // DTG_MAPID exists
	{
	    value = scm_mod->defect_get_field( scm_defect, "DTG_MAPID", err );
	    // ignore err
	    clear_DTGError( err );
	    if( value && 
		value[0] == '"' && value[1] == '"' && value[2] == '\0' )
	    {
	        /* Treat "" as unset */
	        SAFE_FREE( value );
	        value = NULL;
	    }
	    if( !value || !*value )
	    {
	        scm_mod->defect_set_field( scm_defect, 
						"DTG_MAPID", map->id, err );
	        log->log( 3, "Info: Set SCM:DTG_MAPID to [%s]", map->id );
	        scm_dirty++;
	    }
	    else if( strcasecmp( value, map->id ) )
	    {
	        log->log( 0, 
			"Error: SCM(%s) DTG_MAPID does not match, has %s",
			cur_scm, value );
		log_fatal( 1, "DTG_MAPID does not match current map" );
	        scm_mod->defect_free( scm_defect, err );
	        delete_DTGError( err );
	        SAFE_FREE( value );
	        return;
	    }
	    SAFE_FREE( value );
	}
	
	value = scm_mod->defect_get_field( scm_defect, "DTG_DTISSUE", err );
	// ignore err
	clear_DTGError( err );
	if( value && value[0] == '"' && value[1] == '"' && value[2] == '\0' )
	{
	    /* Treat "" as unset */
	    SAFE_FREE( value );
	    value = NULL;
	}
	int is_new;
	void *dts_defect;
	if( !value || value[0] == '\0' )
	{
	    is_new = 1;
	    dts_defect = dts_mod->proj_new_defect( dts_projID, err );
	    if( !dts_defect && err->message )
	    {
	        log->log( 0, 
		    "Error: Unable to create dts defect: SCM %s: %s", 
		    cur_scm, err->message );

	        char *err_msg = mk_string( 
		    "Unable to create dts defect: SCM: ", cur_scm, ": ",
		    err->message );
	        scm_failed = append_DTGField( scm_failed,
					new_DTGField( cur_scm, err_msg ) );
	        delete[] err_msg;

	        scm_mod->defect_free( scm_defect, err );
	        delete_DTGError( err );
	        SAFE_FREE( value );
	        return;
	    }
	    dts_dirty = 1;
	    cur_dts = cp_string( "new" );
	}
	else
	{
	    is_new = 0;
	    dts_defect = dts_mod->proj_get_defect( dts_projID, value, err );
	    cur_dts = cp_string( value );
	}
	ll = dts_mod->dt_get_message( dts_dtID, err );
	if( ll < 4 )
	    log->log( ll, err->message );
	if( !dts_defect )
	{
	    log->log( 0, 
		"Error: Unable to retrieve matching dts defect: SCM %s DTS %s", 
		cur_scm, cur_dts );
	    log->log( 0, "Error: %s", err->message );

	    char *err_msg = mk_string( 
		"Unable to retrieve/create matching dts defect: ", 
		err->message );
	    scm_failed = append_DTGField( scm_failed,
					new_DTGField( cur_scm, err_msg ) );
	    delete[] err_msg;

	    scm_mod->defect_free( scm_defect, err );
	    delete_DTGError( err );
	    SAFE_FREE( value );
	    return;
	}
	SAFE_FREE( value );

	struct DTGStrList *add, *del;
	char *rev = update_fix_record( defect, scm_defect, add, del);
	value = scm_mod->defect_get_field( scm_defect, "DTG_FIXES", err );
	// ignore err
	clear_DTGError( err );
	char *old_fixes = NULL;
	if( set_field( scm_mod, scm_defect, "DTG_FIXES", rev, value, err ) )
	{
	    log->log( 3, "Info: Set SCM:DTG_FIXES to [%s]", rev );
	    scm_dirty++;
	    // SAVE OLD DTG_FIXES
	    old_fixes = mk_string( value );
	}
	if( rev )
	    delete[] rev;
	SAFE_FREE( value );

	if( is_new )
	{
	    unify_defects( 1, scm_defect, -1, dts_defect, add, del );
	    if( force_exit )
	        return; // leaking memory on exit
	}
	else
	{
	    value = 
	      dts_mod->defect_get_field( dts_defect, dts->moddate_field, err );
	    // ignore err
	    clear_DTGError( err );
	    if( !value || !*value )
	    {
	        log->log( 0, 
		    "Error: Unable to retrieve dts moddate: SCM %s DTS %s", 
		    cur_scm, cur_dts );

	        char *err_msg = mk_string( 
		    "Unable to retrieve dts moddate: DTS ", cur_dts );
	        scm_failed = append_DTGField( scm_failed,
					new_DTGField( cur_scm, err_msg ) );
	        delete[] err_msg;

	        scm_mod->defect_free( scm_defect, err );
	        delete_DTGError( err );
	        delete_DTGStrList( add );
	        delete_DTGStrList( del );
	        SAFE_FREE( value );
	        return;
	    }
	    struct DTGDate *stamp = dts_mod->extract_date( value );
	    if( !stamp )
	    {
	        log->log( 0, 
			"Error: Unable to extract dts moddate: DTS %s Date %s", 
			cur_dts, value );

	        char *err_msg = mk_string( 
			"Unable to extract dts moddate: DTS ", cur_dts,
			"Date ", value );
	        scm_failed = append_DTGField( scm_failed,
					new_DTGField( cur_scm, err_msg ) );
	        delete[] err_msg;

	        scm_mod->defect_free( scm_defect, err );
	        delete_DTGError( err );
	        delete_DTGStrList( add );
	        delete_DTGStrList( del );
	        SAFE_FREE( value );
	        return;
	    }
	    log->log( 2, "Notice: process_scm_defect( SCM:%s, DTS:%s )", 
			cur_scm, cur_dts );
	    char tmp_string[255];
	    sprintf( tmp_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
			stamp->year, stamp->month, stamp->day,
			stamp->hour, stamp->minute, stamp->second );
	    log->log( 2, "Notice: dts stamp: %s", tmp_string );
	    sprintf( tmp_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
			since_dts->year, since_dts->month, since_dts->day,
			since_dts->hour, since_dts->minute, since_dts->second );
	    log->log( 2, "Notice: since_dts stamp: %s", tmp_string );
	    if( compare_DTGDate( stamp, since_dts ) > 0 )
	        unify_defects( 1, scm_defect, 0, dts_defect, add, del );
	    else
	        unify_defects( 1, scm_defect, 1, dts_defect, add, del );
	    if( force_exit )
	        return; // leaking memory on exit
	    delete_DTGDate( stamp );
	    free( value );
	}

	char *id = NULL;

	if( map->dts_filter_desc && 
		(filter_msg = 
		    pass_filter( map->dts_filter_desc, dts_mod, dts_defect ) ) )
	{
	    log->log( 0, "Error: New DTS issue fails filter test, aborted: %s", 
		defect );
	    log->log( 0, "Error: %s", filter_msg );
	    delete[] filter_msg;
	    filter_msg = NULL;
	    dts_dirty = 0; // don't save it, it doesn't replicate
	    log_fatal( 1, "Resulting DTS defect does not pass filter test" );
	    scm_dirty = 0; // don't save it, log_fatal will update
	}
	if( map->scm_filter_desc && 
		(filter_msg = 
		    pass_filter( map->scm_filter_desc, scm_mod, scm_defect ) ) )
	{
	    log->log( 0, 
		"Error: Updated SCM issue fails filter test, aborted: %s", 
		defect );
	    log->log( 0, "Error: %s", filter_msg );
	    delete[] filter_msg;
	    filter_msg = NULL;
	    dts_dirty = 0; // don't save it, it doesn't replicate
	    log_fatal( 1, "Updated SCM defect does not pass filter test" );
	    scm_dirty = 0; // don't save it, log_fatal will update
	}

	log->log( 3, "Info: Finished processing mappings" );

	if( dts_dirty )
	{
	    log->log( 3, "Info: DTS has changes" );
	    id = dts_mod->defect_save( dts_defect, err );
	    if( err->message )
	    {
	        log->log( 0, "Error: saving dts defect(%s): scm:%s", 
			cur_dts, defect );
	        log->log( 0, "[%s]", err->message );
	        log->log( 1, "Warning: Set DTG_ERROR: %s", err->message );
	        char *tmp_err = mk_string( err->message );
	        scm_mod->defect_set_field( scm_defect, "DTG_ERROR", 
			tmp_err, err );
	        delete[] tmp_err;
	        scm_dirty++;
	        // RESTORE DTG_FIXES
	        if( old_fixes )
	            scm_mod->defect_set_field( scm_defect, "DTG_FIXES",
						old_fixes, err );
	    }
	    else if( id )
	    {
	        if( is_new )
	        {
	            log->log( report_id ? 0 : 2, 
			"create dts defect(%s): scm:%s", id, defect );
	            scm_mod->defect_set_field( scm_defect, "DTG_DTISSUE", 
						id, err );
	            delete[] cur_dts;
	            cur_dts = mk_string( "new:", id );
	            scm_dirty++;
	            /* Copy rule uses ID, schedule re-unification of defects */
	            if( map->recheck_on_new_dts )
	                scm_recheck = append_DTGStrList( scm_recheck, cur_scm );
	        }
	        else
	            log->log( 2, "saving dts defect(%s): scm:%s", id, defect );
	        int ll = dts_mod->dt_get_message( dts_dtID, err );
	        if( ll < 4 )
	            log->log( ll, err->message );
	    }
	    else
	    {
	        log->log( 0, 
			"Error:dts defect_save returned null: dts:%s scm:%s", 
			cur_dts, defect );
	        log->log( 1, "Warning: Set DTG_ERROR:No id returned from DTS" );
	        scm_mod->defect_set_field( scm_defect, "DTG_ERROR", 
			"No id returned from DTS for save_defect", err );
	        scm_dirty++;
	        // RESTORE DTG_FIXES
	        if( old_fixes )
	            scm_mod->defect_set_field( scm_defect, "DTG_FIXES",
						old_fixes, err );
	    }
	    SAFE_FREE( id );
	}
	if( old_fixes )
	    delete[] old_fixes;
	if( scm_dirty )
	{
	    log->log( 3, "Info: SCM has changes" );
	    id = scm_mod->defect_save( scm_defect, err );
	    if( err->message )
	        // Only retry if it failed updating not creating
	        if( is_new )
	        {
		    log_fatal( 1, err->message );
	            log->log( 0, 
			"Error: Orphan dts defect created dts:%s (scm: %s)", 
			cur_dts, cur_scm );
	        }
	        else
	            log_fatal( last_chance, err->message );
	    else
	    {
	        log->log( 2, "saving scm defect: %s", id );
	        int ll = scm_mod->dt_get_message( scm_dtID, err );
	        if( ll < 4 )
	            log->log( ll, err->message );
	    }
	    SAFE_FREE( id );
	}

	scm_mod->defect_free( scm_defect, err );
	dts_mod->defect_free( dts_defect, err );
	delete_DTGStrList( add );
	delete_DTGStrList( del );
	delete_DTGError( err );
}

void
Unify::log_fatal( int last_chance, const char *msg )
{
	if( !last_chance )
	{
	    scm_recheck = append_DTGStrList( scm_recheck, cur_scm );
	    log->log( 0, "Retry: saving scm defect(%s): dts:%s", 
			cur_scm, cur_dts );
	    log->log( 0, "[%s]", msg );
	    return;
	}

	log->log( 0, "Error: saving scm defect(%s): dts:%s", cur_scm, cur_dts );
	log->log( 0, "[%s]", msg );
	scm_failed = append_DTGField( scm_failed, 
					new_DTGField( cur_scm, msg ) );
}

void
Unify::fail_scm( struct DTGField *pair )
{
	// Load job
	log->log( 3, "Info: fail_scm( %s )", pair->name );
	struct DTGError *err = new_DTGError( NULL );
	void *scm_defect = 
		scm_mod->proj_get_defect( scm_projID, pair->name, err );
	if( scm_defect )
	{
	    // Set DTG_ERROR and save job
	    log->log( 1, "Warning: Set DTG_ERROR: %s", pair->value );
	    scm_mod->defect_set_field( scm_defect, "DTG_ERROR", pair->value, 
					err );
	    char *tmp = scm_mod->defect_save( scm_defect, err );
	    SAFE_FREE( tmp );
	    if( !err->message )
	    {
	        scm_mod->defect_free( scm_defect, err );
	        delete_DTGError( err );
	        return;
	    }
	    log->log( 0, "Fatal: saving scm defect(%s):", pair->name );
	    log->log( 0, "SaveError[%s]", err->message );
	    scm_mod->defect_free( scm_defect, err );
	    // Time to abort replication
	}
	else
	{
	    log->log( 0, "Fatal: saving scm defect(%s):", pair->name );
	    log->log( 0, "BaseError[%s]", pair->value );
	}
	delete_DTGError( err );

	// if fail then ...
	abort_run += 1;
	FILE *fd = fopen( err_file, "a" );
	if( fd )
	{
	    fprintf( fd, "Fix defect: %s\n", pair->name );
	    fprintf( fd, "            %s\n", pair->value );
	    fclose( fd );
	}
}

/*
	Retrieve dt_defect
	if dts_filters and it doesn't match, continue
	Retrieve matching p4_defect
	if it doesn't exist, create a new one and set id field
	Unify Defects
	if it's new, check to see it matches any p4_filters
	if p4_defect updated, save it
	free p4_defect
	if dts_defect updated, save it
	free dt_defect
*/

void Unify::process_dts_defect( const char *defect, int last_chance )
{
	log->log( 3, "Info: process_dts_defect( %s )", defect );
	report_id = scm_dirty = dts_dirty = 0;
	struct DTGError *err = new_DTGError( NULL );
	void *dts_defect = dts_mod->proj_get_defect( dts_projID, defect, err );
	if( !dts_defect )
	{
	    log->log( 0, "Error: Unable to retrieve dts defect: %s", defect );
	    log->log( 0, "Error: %s", err->message );
	    delete_DTGError( err );
	    return;
	}
	int ll = dts_mod->dt_get_message( dts_dtID, err );
	if( ll < 4 )
	    log->log( ll, err->message );
	char *filter_msg = NULL;
	if( map->dts_filter_desc && 
		(filter_msg = 
		    pass_filter( map->dts_filter_desc, dts_mod, dts_defect ) ) )
	{
	    log->log( 2, "Notice: Filtering dts defect: %s", defect );
	    log->log( 2, "Notice: %s", filter_msg );
	    delete[] filter_msg;
	    filter_msg = NULL;
	    dts_mod->defect_free( dts_defect, err );
	    delete_DTGError( err );
	    return;
	}
	char *modby = 
	    dts_mod->defect_get_field( dts_defect, dts->moduser_field, err );
	// ignore err
	clear_DTGError( err );
	if( modby && !strcmp( modby, dts->user ) )
		// THE ABOVE MAY NEED (!set->force) && to force review of issues
		// that exist in the DTS but not in SCM. Unlikely that this will
		// be needed but in case, I'm adding this note.
	{
	    // Skip if last mod was admin user for those dts that cannot 
	    // restrict list by user
	    free( modby );
	    dts_mod->defect_free( dts_defect, err );
	    delete_DTGError( err );
	    return;
	}
	SAFE_FREE( modby );
	if( !set->force )
	{
	    char *moddate = dts_mod->defect_get_field( dts_defect, 
						dts->moddate_field, err );
	    // ignore err
	    clear_DTGError( err );
	    if( !moddate || !*moddate )
	    {
	        // FAIL
	        log->log( 0, "Error: Unable to retrieve dts moddate: %s", 
			defect );
	        if( moddate )
	            free( moddate );
	        dts_mod->defect_free( dts_defect, err );
	        delete_DTGError( err );
	        return;
	    }
	    struct DTGDate *mystamp = dts_mod->extract_date( moddate );
	    if( !mystamp )
	    {
	        // FAIL
	        log->log( 0, "Error: Unable to extract dts moddate: %s date %s", 
			defect, moddate );
	        free( moddate );
	        dts_mod->defect_free( dts_defect, err );
	        delete_DTGError( err );
	        return;
	    }
	    if( compare_DTGDate( since_dts, mystamp ) < 0 )
	    {
	        // Skip if last mod was before requested date
	        log->log( 2, "Notice: Skipping unmodified dts defect: %s", 
			defect );
	        free( moddate );
	        dts_mod->defect_free( dts_defect, err );
	        delete_DTGError( err );
	        delete_DTGDate( mystamp );
	        return;
	    }
	    delete_DTGDate( mystamp );
	    SAFE_FREE( moddate );
	}

	char *qual;
	int matched_mapid = 1;
	if( map->scm->seg_ok )
	    qual = mk_string( "DTG_DTISSUE=", defect, 
				" DTG_MAPID=", map->id );
	else
	    qual = mk_string( "DTG_DTISSUE=", defect );

	struct DTGStrList *jobs = 
		scm_mod->proj_find_defects( scm_projID, 1, qual, err );
	delete[] qual;

	int is_new;
	void *scm_defect;

	char *loading_err = NULL;
	if( !jobs )
	{
	    is_new = 1;
	    scm_defect = scm_mod->proj_new_defect( scm_projID, err );
	    scm_dirty = 1;
	    cur_scm = cp_string( "new" );
	}
	else
	{
	    is_new = 0;
	    scm_defect = 
		scm_mod->proj_get_defect( scm_projID, jobs->value, err );
	    loading_err = cp_string( err->message );
	    cur_scm = cp_string( jobs->value );
	    delete_DTGStrList( jobs );
	    char *value = 
		scm_mod->defect_get_field( scm_defect, "DTG_ERROR", err );
	    // ignore err
	    clear_DTGError( err );
	    if( value && *value )
	    {
	        free( value );
	        log->log( 1, "Warning: Skipping broken scm defect: %s", 
			cur_scm );
	        log->log( 1, 
			"Warning: Any changes from dts are not replicated: %s",
			cur_dts );
	        scm_mod->defect_free( scm_defect, err );
	        dts_mod->defect_free( dts_defect, err );
	        if( loading_err )
		    delete[] loading_err;
	        delete_DTGError( err );
	        return;
	    }
	}
	if( !scm_defect )
	{
	    delete[] loading_err;
	    log->log( 0, 
		"Error: Unable to retrieve matching scm defect: %s(%s)", 
		defect, jobs ? jobs->value : "new" );
	    log->log( 0, "Error: %s", err->message );
	    dts_mod->defect_free( dts_defect, err );
	    delete_DTGError( err );
	    return;
	}
	if( loading_err )
	{
	    char *err_msg = mk_string( 
		    "Loading failure for defect: SCM: ", cur_scm, ": ",
		    loading_err );
	    delete[] loading_err;
	    log->log( 0, "Error: %s", err_msg );
	    scm_failed = append_DTGField( scm_failed,
					new_DTGField( cur_scm, err_msg ) );
	    delete[] err_msg;
	    scm_mod->defect_free( scm_defect, err );
	    dts_mod->defect_free( dts_defect, err );
	    delete_DTGError( err );
	    return;
	}
	ll = scm_mod->dt_get_message( scm_dtID, err );
	if( ll < 4 )
	    log->log( ll, err->message );

	char *old_fixes = NULL;
	if( is_new )
	{
	    unify_defects( -1, scm_defect, 1, dts_defect, NULL, NULL );
	    if( force_exit )
	        return; // leaking memory on exit
	}
	else
	{
	    struct DTGStrList *add, *del;
	    char *rev = update_fix_record( cur_scm, scm_defect, add, del);
	    char *value = 
		scm_mod->defect_get_field( scm_defect, "DTG_FIXES", err );
	    // ignore err
	    clear_DTGError( err );
	    if( set_field( scm_mod, scm_defect, "DTG_FIXES", rev, value, err ) )
	    {
	        log->log( 3, "Info: Set SCM:DTG_FIXES to [%s]", rev );
	        scm_dirty++;
	        // SAVE OLD DTG_FIXES
	        old_fixes = mk_string( value );
	    }
	    if( rev )
	        delete[] rev;
	    SAFE_FREE( value );

	    value = NULL;
	    if( add || del )
	    {
	        value = cp_string( "9999/01/01 01:01:01" );
	        scm_dirty++;
	    }
	    else
	    {
	        char *tmp_free = scm_mod->defect_get_field( 
					scm_defect, scm->moddate_field, err );
	        // ignore err
	        clear_DTGError( err );
	        value = cp_string( tmp_free );
	        SAFE_FREE( tmp_free );
	    }
	    if( value )
	    {
	        struct DTGDate *stamp = scm_mod->extract_date( value );
	        log->log( 2, "Details: process_dts_defect( SCM:%s, DTS:%s )", 
			cur_scm, cur_dts );
	        char tmp_string[255];
	        sprintf( tmp_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
			stamp->year, stamp->month, stamp->day,
			stamp->hour, stamp->minute, stamp->second );
	        log->log( 2, "Details: scm stamp: %s", tmp_string );
	        sprintf( tmp_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
			since_scm->year, since_scm->month, since_scm->day,
			since_scm->hour, since_scm->minute, since_scm->second );
	        log->log( 2, "Details: since_scm stamp: %s", tmp_string );
	        if( compare_DTGDate( stamp, since_scm ) > 0 )
	            unify_defects( 0, scm_defect, 1, dts_defect, add, del );
	        else
	            unify_defects( 1, scm_defect, 1, dts_defect, add, del );
		if( force_exit )
	            return; // leaking memory on exit
	        if( value )
	            delete[] value;
	        delete_DTGDate( stamp );
	    }
	    else
	    {
	        log->log( 2, 
		    "Details: process_dts_defect( SCM:%s, DTS:%s ) no value", 
		    cur_scm, cur_dts );
	        unify_defects( 1, scm_defect, 1, dts_defect, add, del );
	        if( force_exit )
	            return; // leaking memory on exit
	    }
	}

	char *id = NULL;

	if( map->scm_filter_desc && 
		( filter_msg = 
		    pass_filter( map->scm_filter_desc, scm_mod, scm_defect ) ) )
	{
	    if( !is_new )
	    {
	    	log->log( 0, 
		    "Error: New SCM issue fails filter test, aborted: %s", 
			defect );
	        log_fatal( 1, "Updated SCM defect fails filter test" );
	    }
	    else
	        log->log( 0, 
		    "Error: Updated SCM issue fails filter test, aborted: %s", 
			defect );
	    log->log( 0, "Error: %s", filter_msg );
	    delete[] filter_msg;
	    filter_msg = NULL;
	    scm_dirty = 0; // don't save it, it doesn't replicate
	    dts_dirty = 0; // don't save it, it doesn't replicate
	}
	if( map->dts_filter_desc && 
		(filter_msg = 
		    pass_filter( map->dts_filter_desc, dts_mod, dts_defect ) ) )
	{
	    log->log( 0, 
		"Error: Updated DTS issue fails filter test, aborted: %s", 
		defect );
	    if( !is_new )
	        log_fatal( 1, "Updated DTS defect fails filter test" );
	    log->log( 0, "Error: %s", filter_msg );
	    delete[] filter_msg;
	    filter_msg = NULL;
	    scm_dirty = 0; // don't save it, it doesn't replicate
	    dts_dirty = 0; // don't save it, it doesn't replicate
	}

	log->log( 3, "Info: Finished processing mappings" );

	if( dts_dirty )
	{
	    log->log( 3, "Info: DTS has changes" );
	    id = dts_mod->defect_save( dts_defect, err );
	    if( err->message )
	    {
	        log->log( 0, "Error: saving dts defect(%s): scm: %s", 
			defect, cur_scm );
	        log->log( 0, "[%s]", err->message );
	        log->log( 1, "Warning: Set DTG_ERROR: %s", err->message );
	        char *tmp_err = mk_string( err->message );
	        scm_mod->defect_set_field( scm_defect, "DTG_ERROR",
					tmp_err, err );
	        delete[] tmp_err;
	        scm_dirty++;
	        // RESTORE DTG_FIXES
	        if( old_fixes )
	            scm_mod->defect_set_field( scm_defect, "DTG_FIXES",
						old_fixes, err );
	    }
	    else if( id )
	    {
	        log->log( 2, "saving dts defect: %s", id );
	        int ll = dts_mod->dt_get_message( dts_dtID, err );
	        if( ll < 4 )
	            log->log( ll, err->message );
	    }
	    else
	    {
	        log->log( 0, 
			"Error:dts defect_save returned null: dts:%s scm:%s", 
			cur_dts, cur_scm );
	        log->log( 1, "Warning: Set DTG_ERROR:No id returned from DTS" );
	        scm_mod->defect_set_field( scm_defect, "DTG_ERROR", 
			"No id returned from DTS for save_defect", err );
	        scm_dirty++;
	        // RESTORE DTG_FIXES
	        if( old_fixes )
	            scm_mod->defect_set_field( scm_defect, "DTG_FIXES",
						old_fixes, err );
	    }
	    SAFE_FREE( id );
	}
	if( old_fixes )
	    delete[] old_fixes;

	if( is_new && ( scm_dirty || dts_dirty ) )
	{
	    scm_mod->defect_set_field( scm_defect, "DTG_DTISSUE", defect, err );
	    if( map->scm->seg_ok )
	        scm_mod->defect_set_field( scm_defect, 
						"DTG_MAPID", map->id, err );
	    scm_dirty++;
	}
	if( scm_dirty )
	{
	    log->log( 3, "Info: SCM has changes" );
	    id = scm_mod->defect_save( scm_defect, err );
	    if( err->message )
	    {
	        log->log( 0, "Error: saving scm defect(%s): dts:%s", 
			cur_scm, cur_dts );
	        log->log( 0, "[%s]", err->message );
	        if( !is_new )
	            log_fatal( last_chance, err->message );
	        // else error?
	    }
	    else 
	    {
	        if( is_new )
	        {
	            log->log( report_id ? 0 : 2, "create scm defect: %s", id );
	            /* Copy rule uses ID, schedule re-unification of defects */
	            if( map->recheck_on_new_scm )
	                scm_recheck = append_DTGStrList( scm_recheck, id );
	        }
	        else
	            log->log( 2, "saving scm defect: %s", id );
	        int ll = scm_mod->dt_get_message( scm_dtID, err );
	        if( ll < 4 )
	            log->log( ll, err->message );
	    }
	    SAFE_FREE( id );
	}

	scm_mod->defect_free( scm_defect, err );
	dts_mod->defect_free( dts_defect, err );
	delete_DTGError( err );
}

int Unify::stop_exists()
{
	struct stat buf;
	if( stat( run_file, &buf ) )
	{
	    log->log( 0, "Error: run_file missing, recreating: %s", run_file );
	    FILE *fd = fopen( run_file, "a" );
	    if( fd )
	        fclose( fd );
	}
	if( !stat( stop_file, &buf ) )
	{
	    log->log( 0, "Discovered stop_file, beginning shutdown" );
	    return 1;
	}
	if( force_exit )
	{
	    log->log( 0, "Forcing exit, beginning shutdown" );
	    FILE *fd = fopen( stop_file, "a" );
	    if( fd )
	        fclose( fd );
	    return 1;
	}
	return 0;
}

int Unify::unify( DTGSettings *in_set )
{
	set = in_set;
	DTGError *err = new_DTGError( NULL );

	if( set->force )
	{
	    set_DTGDate( set->last_update_scm, set->starting_date );
	    set_DTGDate( set->last_update_dts, set->starting_date );
	}
	since_scm = set->last_update_scm;
	since_dts = set->last_update_dts;

	struct DTGDate *scm_date = scm_mod->dt_get_server_date( scm_dtID, err );
	if( err->message )
	{
	    log->log( 0, "Error: Unable to retrieve scm date: %s", 
			err->message );
	    if( scm_date )
	        delete_DTGDate( scm_date );
	    clear_DTGError( err );
	    int scm = scm_mod->dt_server_offline( scm_dtID, err );
	    delete_DTGError( err );
	    if( scm <= 0 && !reset_scm() )
	        return -2;
	    return -1;
	}
	if( !scm_date )
	{
	    log->log( 0, "Error: Invalid plugin behavior by SCM plugin" );
	    log->log( 0, "Error: Neither date nor error message returned" );
	    clear_DTGError( err );
	    int scm = scm_mod->dt_server_offline( scm_dtID, err );
	    delete_DTGError( err );
	    if( scm <= 0 && !reset_scm() )
	        return -2;
	    return -1; // XXX This may be incorrect - bad plugin should exit
			// XXX Though a disconnected server at the wrong time
			// XXX may generate this so leave it as is for now
	}

	struct DTGDate *dts_date = dts_mod->dt_get_server_date( dts_dtID, err );
	if( err->message )
	{
	    log->log( 0, "Error: Unable to retrieve dts date: %s", 
			err->message );
	    delete_DTGDate( scm_date );
	    if( dts_date )
	        delete_DTGDate( dts_date );
	    clear_DTGError( err );
	    int dts = dts_mod->dt_server_offline( dts_dtID, err );
	    delete_DTGError( err );
	    if( dts <= 0 && !reset_dts() )
	        return -2;
	    return -1;
	}
	if( !dts_date )
	{
	    log->log( 0, "Error: Invalid plugin behavior by DTS plugin" );
	    log->log( 0, "Error: Neither date nor error message returned" );
	    delete_DTGDate( scm_date );
	    clear_DTGError( err );
	    int dts = dts_mod->dt_server_offline( dts_dtID, err );
	    delete_DTGError( err );
	    if( dts <= 0 && !reset_dts() )
	        return -2;
	    return -1; // XXX This may be incorrect - bad plugin should exit
			// XXX Though a disconnected server at the wrong time
			// XXX may generate this so leave it as is for now
	}

	int stop_process = 0;
	char since_string[255];
	sprintf( since_string, "SCM: %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		scm_date->year, scm_date->month, scm_date->day,
		scm_date->hour, scm_date->minute, scm_date->second );
	log->log( 2, "Start date at %s", since_string );
	sprintf( since_string, "DTS: %4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		dts_date->year, dts_date->month, dts_date->day,
		dts_date->hour, dts_date->minute, dts_date->second );
	log->log( 2, "Start date at %s", since_string );
	sprintf( since_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d%s",
		since_dts->year, since_dts->month, since_dts->day,
		since_dts->hour, since_dts->minute, since_dts->second,
		set->force ? " Force" : "" );
	log->log( 2, "List DTS Defects since: %s", since_string );
	query_cnt++;

	struct DTGStrList *dts_defects = dts_mod->proj_list_changed_defects( 
						dts_projID,
						0,
						since_dts,
						dts->moddate_field,
						set->force ? NULL :
							dts->moduser_field,
						set->force ? NULL :
							dts->user,
						err );
	if( err->message )
	{
	    log->log( 0, "Error: Retrieving DTS defect list: %s", 
			err->message );
	    delete_DTGDate( scm_date );
	    delete_DTGDate( dts_date );
	    if( dts_defects )
	        delete_DTGStrList( dts_defects );
	    clear_DTGError( err );
	    int dts = dts_mod->dt_server_offline( dts_dtID, err );
	    delete_DTGError( err );
	    if( dts <= 0 && !reset_dts() )
	        return -2;
	    return -1;
	}
	int ll = dts_mod->dt_get_message( dts_dtID, err );
	if( ll < 4 )
	    log->log( ll, err->message );
	long items = 0L;
	log_large_cycles( log, dts_defects, "DTS" );
	for( struct DTGStrList *dts_d = dts_defects; 
		dts_d && !stop_process; 
		dts_d = dts_d->next )
	{
	    if( cur_dts ) delete[] cur_dts;
	    if( cur_scm ) delete[] cur_scm;
	    cur_dts = cp_string( dts_d->value );
	    cur_scm = NULL;
	    log_large_cycles( log, ++items );
	    process_dts_defect( dts_d->value );
	    stop_process = stop_exists();
	}
	if( stop_process || !dts_defects && stop_exists() )
	{
	    delete_DTGDate( scm_date );
	    delete_DTGDate( dts_date );
	    delete_DTGError( err );
	    delete_DTGStrList( dts_defects );
	    return 0;
	}
	delete_DTGStrList( dts_defects );
	
	sprintf( since_string, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d%s",
		since_scm->year, since_scm->month, since_scm->day,
		since_scm->hour, since_scm->minute, since_scm->second,
		set->force ? " Force" : "" );
	log->log( 2, "List SCM Defects since: %s", since_string );
	struct DTGStrList *scm_defects = scm_mod->proj_list_changed_defects( 
						scm_projID,
						0,
						since_scm,
						scm->moddate_field,
						set->force ? NULL :
							scm->moduser_field,
						set->force ? NULL :
							scm->user,
						err );
	if( err->message )
	{
	    log->log( 0, 
		"Error: Retrieving SCM defect list: %s", err->message );
	    delete_DTGDate( scm_date );
	    delete_DTGDate( dts_date );
	    if( scm_defects )
	        delete_DTGStrList( scm_defects );
	    clear_DTGError( err );
	    int scm = scm_mod->dt_server_offline( scm_dtID, err );
	    delete_DTGError( err );
	    if( scm <= 0 && !reset_scm() )
	        return -2;
	    return -1;
	}
	ll = scm_mod->dt_get_message( scm_dtID, err );
	if( ll < 4 )
	    log->log( ll, err->message );
	items = 0L;
	log_large_cycles( log, scm_defects, "SCM" );
	for( struct DTGStrList *scm_d = scm_defects; 
		scm_d && !stop_process; 
		scm_d = scm_d->next )
	{
	    if( cur_dts ) delete[] cur_dts;
	    if( cur_scm ) delete[] cur_scm;
	    cur_scm = cp_string( scm_d->value );
	    cur_dts = NULL;
	    log_large_cycles( log, ++items );
	    process_scm_defect( scm_d->value );
	    stop_process = stop_exists();
	}
	if( stop_process || !scm_defects && stop_exists() )
	{
	    delete_DTGDate( scm_date );
	    delete_DTGDate( dts_date );
	    delete_DTGError( err );
	    delete_DTGStrList( scm_defects );
	    return 0;
	}
	delete_DTGStrList( scm_defects );

	for( struct DTGStrList *scm_d = scm_recheck; 
		scm_d && !stop_process; 
		scm_d = scm_d->next )
	{
	    if( cur_dts ) delete[] cur_dts;
	    if( cur_scm ) delete[] cur_scm;
	    cur_scm = cp_string( scm_d->value );
	    cur_dts = NULL;
	    process_scm_defect( scm_d->value, 1 );
	    stop_process = stop_exists();
	}
	delete_DTGStrList( scm_recheck );
	scm_recheck = NULL;
	if( stop_process || stop_exists() )
	{
	    delete_DTGDate( scm_date );
	    delete_DTGDate( dts_date );
	    delete_DTGError( err );
	    return 0;
	}

	for( struct DTGField *bad_scm = scm_failed; 
		bad_scm && !stop_process; 
		bad_scm = bad_scm->next )
	{
	    if( cur_dts ) delete[] cur_dts;
	    if( cur_scm ) delete[] cur_scm;
	    cur_scm = NULL;
	    cur_dts = NULL;
	    fail_scm( bad_scm );
	    stop_process = stop_exists();
	}
	delete_DTGField( scm_failed );
	scm_failed = NULL;
	if( stop_process || stop_exists() )
	{
	    delete_DTGDate( scm_date );
	    delete_DTGDate( dts_date );
	    delete_DTGError( err );
	    return 0;
	}

	set_DTGDate( set->last_update_scm, scm_date );
	set_DTGDate( set->last_update_dts, dts_date );
	delete_DTGDate( scm_date );
	delete_DTGDate( dts_date );
	delete_DTGError( err );

	if( abort_run )
	{
	    char cnt[10];
	    sprintf( cnt, "%8d", abort_run );
	    log->log( 0, "Fatal: Review error log and correct fatal errors" );
	    log->log( 0, "Fatal errors encountered: %s", cnt );
	    log->log( 0, "Error Log: %s", err_file );
	    log->log( 0, "Delete error log to enable restarting of engine" );
	    FILE *fd = fopen( err_file, "a" );
	    if( fd )
	        fclose( fd );
	    return 0;
	}

	if( set->force || query_cnt > QUERYLIMIT )
	{
	    char cnt[10];
	    sprintf( cnt, "%8d", query_cnt );
	    log->log( 2, "Resetting servers at %s", cnt );
	    query_cnt = 0;
	    if( !reset_servers() )
	        return -2;
	}

	return 1;
}
