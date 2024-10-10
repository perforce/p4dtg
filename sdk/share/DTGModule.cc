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
extern "C" {
#include <dtg-utils.h>
}

#ifdef _WIN32

#include <windows.h>
#define DL_GET_FTN_PTR(lib, ftn) GetProcAddress((HINSTANCE)lib,ftn)
#define DL_LOAD_LIB(lib) LoadLibrary(lib)
#define DL_UNLOAD_LIB(lib) FreeLibrary((HINSTANCE)lib)
#define DL_ERROR(msg) msg

#else

#include <dlfcn.h>	/* dlopen(), dlclose(), dlsym() ... */
#define DL_GET_FTN_PTR(lib, ftn) dlsym(lib,ftn)
#define DL_LOAD_LIB(lib) dlopen(lib, RTLD_LAZY)
#define DL_UNLOAD_LIB(lib) dlclose(lib)
#define DL_ERROR(msg) dlerror()

#endif

// Availible in VS since 2015  https://msdn.microsoft.com/en-us/library/2ts7cx93.aspx
#define SNPRINTF snprintf

#include <DTGModule.h>

int DTGModule::has_perforce_extensions()
{
	return ( int_proj_list_fixes &&
		int_proj_describe_fix &&
		int_proj_find_defects );
}

int DTGModule::has_attribute_extensions()
{
	return ( int_dt_list_attrs && 
		int_dt_validate_attr && 
		free_dtg_attribute );
}

void DTGModule::record_error( const char *ftn, const char *error )
{
	SNPRINTF( last_error, MAX_ERR_MSG, "%s: %s", ftn, error );
	// fprintf( stderr, "%s\n", last_error );
}

void *DTGModule::load_function( const char *ftn_name )
{
	void *ftn = DL_GET_FTN_PTR( lib_handle, ftn_name ); 
	const char *error = DL_ERROR("Unable to load function");
	if( !ftn && error ) 
	    record_error( ftn_name, error );
	return ftn;
}

DTGModule::DTGModule( const char *use_dl )
{
	next = NULL;
	pseudo_attrs = NULL;
	dl_name = strdup( use_dl );
	last_error[0] = '\0';

	/* load the desired shared library */
	lib_handle = DL_LOAD_LIB( dl_name );
	if( !lib_handle )
	{
#ifdef _WIN32
	    LPVOID buf;
	    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			0,
			(LPTSTR) &buf,
			0, NULL );
	    record_error( use_dl, (LPCTSTR)buf );
#else
	    record_error( use_dl, DL_ERROR("Unable to load library") );
#endif
	    return;
	}
	int_extract_date = (extract_date_ftn *)load_function( "extract_date" );
	if( *last_error )
	    return;
	int_format_date = (format_date_ftn *)load_function( "format_date" );
	if( *last_error )
	    return;
	int_dt_get_name = (dt_get_name_ftn *)load_function( "dt_get_name" );
	if( *last_error )
	    return;
	int_dt_get_module_version = 
	  (dt_get_module_version_ftn *)load_function( "dt_get_module_version" );
	if( *last_error )
	    return;
	int_dt_get_server_version = 
	  (dt_get_server_version_ftn *)load_function( "dt_get_server_version" );
	if( *last_error )
	    return;
	int_dt_get_server_warnings = 
	  (dt_get_server_warnings_ftn *)load_function("dt_get_server_warnings");
	if( *last_error )
	    return;
	int_dt_get_server_date = 
	  (dt_get_server_date_ftn *)load_function( "dt_get_server_date" );
	if( *last_error )
	    return;
	int_dt_connect = (dt_connect_ftn *)load_function( "dt_connect" );
	if( *last_error )
	    return;
	int_dt_free = (dt_free_ftn *)load_function( "dt_free" );
	if( *last_error )
	    return;
	int_dt_list_projects = 
	  (dt_list_projects_ftn *)load_function( "dt_list_projects" );
	if( *last_error )
	    return;
	int_dt_get_project = 
	  (dt_get_project_ftn *)load_function( "dt_get_project" );
	if( *last_error )
	    return;
	int_proj_free = (proj_free_ftn *)load_function( "proj_free" );
	if( *last_error )
	    return;
	int_proj_list_fields = 
	  (proj_list_fields_ftn *)load_function( "proj_list_fields" );
	if( *last_error )
	    return;
	int_proj_list_changed_defects = 
	  (proj_list_changed_defects_ftn *)load_function( 
	                                         "proj_list_changed_defects" );
	if( *last_error )
	    return;
	int_proj_get_defect = 
	  (proj_get_defect_ftn *)load_function( "proj_get_defect" );
	if( *last_error )
	    return;
	int_proj_new_defect = 
	  (proj_new_defect_ftn *)load_function( "proj_new_defect" );
	if( *last_error )
	    return;
	int_defect_free = (defect_free_ftn *)load_function( "defect_free" );
	if( *last_error )
	    return;
	int_defect_get_fields = 
	  (defect_get_fields_ftn *)load_function( "defect_get_fields" );
	if( *last_error )
	    return;
	int_defect_get_field = 
	  (defect_get_field_ftn *)load_function( "defect_get_field" );
	if( *last_error )
	    return;
	int_defect_set_field = 
	  (defect_set_field_ftn *)load_function( "defect_set_field" );
	if( *last_error )
	    return;
	int_defect_save = (defect_save_ftn *)load_function( "defect_save" );
	if( *last_error )
	    return;
	free_char = (free_char_ftn *)load_function( "free_char" );
	if( *last_error )
	    return;
	free_dtg_error = 
		(free_dtg_error_ftn *)load_function( "free_dtg_error" );
	if( *last_error )
	    return;
	free_dtg_str_list = 
		(free_dtg_str_list_ftn *)load_function( "free_dtg_str_list" );
	if( *last_error )
	    return;
	free_dtg_field_desc = 
	    (free_dtg_field_desc_ftn *)load_function( "free_dtg_field_desc" );
	if( *last_error )
	    return;
	free_dtg_date = (free_dtg_date_ftn *)load_function( "free_dtg_date" );
	if( *last_error )
	    return;
	free_dtg_field = 
		(free_dtg_field_ftn *)load_function( "free_dtg_field" );
	if( *last_error )
	    return;
	free_dtg_fix_desc = 
		(free_dtg_fix_desc_ftn *)load_function( "free_dtg_fix_desc" );
	if( *last_error )
	    return;

	/* Perforce specific interfaces */
	/* Not an error if they do not exist otherwise */
	int_proj_find_defects = 
	  (proj_find_defects_ftn *)load_function( "proj_find_defects" );
	int_proj_list_fixes = 
	  (proj_list_fixes_ftn *)load_function( "proj_list_fixes" );
	int_proj_describe_fix = 
	  (proj_describe_fix_ftn *)load_function( "proj_describe_fix" );
	if( *last_error )
	    last_error[0] = '\0';


	/* Optional interfaces */
	int_dt_get_message = 
	  (dt_get_message_ftn *)load_function("dt_get_message");
	int_proj_referenced_fields =
	  (proj_referenced_fields_ftn *)load_function( 
						     "proj_referenced_fields" );
	int_proj_segment_filters =
	  (proj_segment_filters_ftn *)load_function( "proj_segment_filters" );
	int_dt_accept_utf8 = 
		(dt_accept_utf8_ftn *)load_function( "dt_accept_utf8" );
	int_dt_server_offline = 
		(dt_server_offline_ftn *)load_function( "dt_server_offline" );
	free_dtg_attribute = 
		(free_dtg_attribute_ftn *)load_function( "free_dtg_attribute" );
	int_dt_list_attrs = 
		(dt_list_attrs_ftn *)load_function( "dt_list_attrs" );
	int_dt_validate_attr =
		(dt_validate_attr_ftn *)load_function( "dt_validate_attr" );
	if( *last_error )
	    last_error[0] = '\0';
	if( int_dt_list_attrs )
	    if( !int_dt_validate_attr || !free_dtg_attribute )
	        SNPRINTF( last_error, MAX_ERR_MSG, 
			"Incomplete implementation of DTGAttribute detected" );
	    else
	    {
	        DTGAttribute *list = dt_list_attrs();
	        for( DTGAttribute *item = list; item; item = item->next )
	            if( !item->name || !item->label || !item->desc )
	            {
	                SNPRINTF( last_error, MAX_ERR_MSG,
				"Incomplete attribute definition for %s",
				item->name ? item->name : "Unnamed" );
	                break;
	            }
	        delete_DTGAttribute( list );
	    }
}

DTGModule::~DTGModule()
{
	if( dl_name )
	    free( dl_name );
	if( lib_handle )
	    DL_UNLOAD_LIB( lib_handle );
	if( next )
	    delete next;
	delete_DTGField( pseudo_attrs );
}

static void die( struct DTGError *err, const char *msg )
{
	if( err->message )
	    fprintf( stderr, "Error: %s %s\n", msg, err->message );
	else
	    fprintf( stderr, "Error: %s returned invalid value\n", msg );
	exit(1);
}

void
DTGModule::test_module()
{
	struct DTGError *err = new_DTGError( NULL );

	fprintf( stderr, "Beginning test:\n" );
	const char *name = dt_get_name( err );
	if( err->message || !name || !*name  )
	    die( err, "dt_get_name" );
	fprintf( stderr, "dt_get_name passed:\n" );

	const char *module_version = dt_get_module_version( err );
	if( err->message || !module_version || !*module_version  )
	    die( err, "dt_get_module_version" );
	fprintf( stderr, "dt_get_module_version passed:\n" );

	void *dtID = dt_connect( "*server*", "*userid*", "*passwd*", 
					NULL, err );
	if( err->message || !dtID )
	    die( err, "dt_connect" );
	fprintf( stderr, "dt_connect passed:\n" );

	dt_free( dtID, err );
	if( err->message )
	    die( err, "dt_free" );
	fprintf( stderr, "dt_free passed:\n" );

	dtID = dt_connect( "*server*", "*userid*", "*passwd*", 
					NULL, err );
	if( err->message || !dtID )
	    die( err, "dt_connect" );
	fprintf( stderr, "dt_connect(2) passed:\n" );

	const char *server_version = dt_get_server_version( dtID, err );
	if( err->message || !server_version || !*server_version  )
	    die( err, "dt_get_server_version" );
	fprintf( stderr, "dt_get_server_version passed:\n" );

	char *server_warnings = dt_get_server_warnings( dtID, err );
	if( err->message || !server_warnings || !*server_warnings  )
	    die( err, "dt_get_server_warnings" );
	free( server_warnings );
	fprintf( stderr, "dt_get_server_warnings passed:\n" );

	struct DTGStrList *projects = dt_list_projects( dtID, err );
	if( err->message || !projects )
	    die( err, "dt_list_projects" );
	delete_DTGStrList( projects );
	fprintf( stderr, "dt_list_projects passed:\n" );

	void *projID = dt_get_project( dtID, "*project*", err );
	if( err->message || !projID )
	    die( err, "dt_get_project" );
	fprintf( stderr, "dt_get_project passed:\n" );

	proj_free( projID, err );
	if( err->message )
	    die( err, "proj_free" );
	fprintf( stderr, "proj_free passed:\n" );

	projID = dt_get_project( dtID, "*project*", err );
	if( err->message || !projID )
	    die( err, "dt_get_project" );
	fprintf( stderr, "dt_get_project(2) passed:\n" );

	struct DTGFieldDesc *field_descs = proj_list_fields( projID, err );
	if( err->message || !field_descs )
	    die( err, "proj_list_fields" );
	delete_DTGFieldDesc( field_descs );
	fprintf( stderr, "proj_list_fields passed:\n" );

	void *defectID = proj_new_defect( projID, err );
	if( err->message || !defectID )
	    die( err, "proj_new_defect" );
	fprintf( stderr, "proj_new_defect passed:\n" );

	char *d = defect_save( defectID, err );
	if( err->message || !d || !*d )
	    die( err, "defect_save" );
	char *defect = new char[strlen(d)+1];
	strcpy( defect, d );
	free( d );
	fprintf( stderr, "defect_save passed:\n" );

	defect_free( defectID, err );
	defectID = NULL;
	if( err->message )
	    die( err, "defect_free" );
	fprintf( stderr, "defect_free passed:\n" );

	struct DTGDate since;
	since.year = 2005;
	since.month = 12;
	since.day = 25;
	since.hour = 12;
	since.minute = 34;
	since.second = 56;
	struct DTGStrList *defects = 
	  proj_list_changed_defects( projID, 4, &since, NULL, NULL, NULL, err );
	if( err->message || !defects )
	    die( err, "proj_list_changed_defects" );
	delete_DTGStrList( defects );
	fprintf( stderr, "proj_list_changed_defects passed:\n" );

	defectID = proj_get_defect( projID, defect, err );
	delete[] defect;
	if( err->message || !defectID )
	    die( err, "proj_get_defect" );
	fprintf( stderr, "proj_get_defect passed:\n" );

	struct DTGField *fields = defect_get_fields( defectID, err );
	if( err->message || !fields )
	    die( err, "defect_get_fields" );
	delete_DTGField( fields );
	fprintf( stderr, "defect_get_fields passed:\n" );

	char *value = defect_get_field( defectID, "*name*", err );
	if( err->message || !value || !*value  )
	    die( err, "defect_get_field" );
	free( value );
	fprintf( stderr, "defect_get_field passed:\n" );

	defect_set_field( defectID, "*name*", "*value*", err );
	if( err->message )
	    die( err, "defect_set_field" );
	fprintf( stderr, "defect_set_field passed:\n" );

	d = defect_save( defectID, err );
	if( err->message || !d || !*d  )
	    die( err, "defect_save" );
	free( d );
	fprintf( stderr, "defect_save(2) passed:\n" );

	defect_free( defectID, err );
	defectID = NULL;
	if( err->message )
	    die( err, "defect_free" );
	fprintf( stderr, "defect_free(2) passed:\n" );

	proj_free( projID, err );
	if( err->message )
	    die( err, "proj_free" );
	fprintf( stderr, "proj_free(2) passed:\n" );

	dt_free( dtID, err );
	if( err->message )
	    die( err, "dt_free" );
	fprintf( stderr, "dt_free(2) passed:\n" );

	delete_DTGError( err );
}

struct DTGDate *DTGModule::extract_date( const char *date_string )
{
	struct DTGDate *tmp = int_extract_date( date_string );
	struct DTGDate *res = copy_DTGDate( tmp );
	free_dtg_date( tmp );
	return res;
}

struct DTGAttribute *DTGModule::dt_list_attrs()
{
	struct DTGAttribute *tmp = int_dt_list_attrs();
	struct DTGAttribute *res = copy_DTGAttribute( tmp );
	free_dtg_attribute( tmp );
	return res;
}

char *DTGModule::dt_validate_attr( const struct DTGField *attr )
{
	char *tmp = int_dt_validate_attr( attr );
	char *res;
	if( tmp )
	{
	    res = strdup( tmp );
	    free_char( tmp );
	}
	else
	    res = NULL;
	return res;
}

char *DTGModule::format_date( struct DTGDate *date )
{
	char *tmp = int_format_date( date );
	char *res;
	if( tmp )
	{
	    res = strdup( tmp );
	    free_char( tmp );
	}
	else
	    res = NULL;
	return res;
}

char *DTGModule::dt_get_server_warnings( void *dtID, 
					struct DTGError *error )
{
	clear_DTGError( error );
	char *tmp = int_dt_get_server_warnings( dtID, error );
	char *res;
	if( tmp )
	{
	    res = strdup( tmp );
	    free_char( tmp );
	}
	else
	    res = NULL;
	if( error->message )
	{
	    tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

int DTGModule::dt_get_message( void *dtID, struct DTGError *error )
{
	clear_DTGError( error );
	if( int_dt_get_message )
	{
	    int tmp = int_dt_get_message( dtID, error );
	    if( error->message )
	    {
	        char *msg = error->message;
	        error->message = strdup( msg );
	        free_char( msg );
	    }
	    return tmp;
	}
	else
	    return 10; // Impossible log level to avoid usage
}

struct DTGDate *DTGModule::dt_get_server_date( void *dtID, 
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGDate *tmp = int_dt_get_server_date( dtID, error );
	struct DTGDate *res = copy_DTGDate( tmp );
	free_dtg_date( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	return res;
}

int DTGModule::dt_accept_utf8( void *dtID, struct DTGError *error )
{
	int accept;
	if( int_dt_accept_utf8 )
	{
	    clear_DTGError( error );
	    accept = int_dt_accept_utf8( dtID, error );
	    if( error->message )
	    {
	        char *str = error->message;
	        error->message = strdup( str );
	        free_char( str );
	    }
	}
	else
	    accept = -1;
	return accept;
}

int DTGModule::dt_server_offline( void *dtID, struct DTGError *error )
{
	int wait;
	if( int_dt_server_offline )
	{
	    clear_DTGError( error );
	    wait = int_dt_server_offline( dtID, error );
	    if( error->message )
	    {
	        char *str = error->message;
	        error->message = strdup( str );
	        free_char( str );
	    }
	}
	else
	    wait = -1;  // the interface isn't defined.
			// The replication engine can decide whether to wait

	if( wait < -1 ) // invalid return value
	    wait = -1;

	return wait;
}

struct DTGStrList *DTGModule::dt_list_projects( void *dtID, 
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGStrList *tmp = int_dt_list_projects( dtID, error );
	struct DTGStrList *res = copy_DTGStrList( tmp );
	free_dtg_str_list( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	return res;
}

struct DTGFieldDesc *DTGModule::proj_list_fields( void *projID, 
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGFieldDesc *tmp = int_proj_list_fields( projID, error );
	struct DTGFieldDesc *res = copy_DTGFieldDesc( tmp );
	free_dtg_field_desc( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	struct DTGField *fields = pseudo_attrs;
	while( fields )
	{
	    res = append_DTGFieldDesc( res, new_DTGFieldDesc( fields->name,
	                                    "select",
	                                    1, new_DTGStrList( fields->value)));
	    fields = fields->next;
	}
	return res;
}

struct DTGStrList *DTGModule::proj_list_fixes( void *projID, 
					const char *defect,
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGStrList *tmp = int_proj_list_fixes( projID, defect, error );
	struct DTGStrList *res = copy_DTGStrList( tmp );
	free_dtg_str_list( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	return res;
}

struct DTGFixDesc *DTGModule::proj_describe_fix( void *projID, 
					const char *fixid,
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGFixDesc *tmp = int_proj_describe_fix( projID, fixid, error );
	struct DTGFixDesc *res = copy_DTGFixDesc( tmp );
	free_dtg_fix_desc( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	return res;
}

struct DTGStrList *DTGModule::proj_list_changed_defects( void *projID, 
					int max_rows,
					struct DTGDate *since, 
					const char *mod_date_field,
					const char *mod_by_field,
					const char *exclude_mod_user,
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGStrList *tmp = int_proj_list_changed_defects( projID, 
					max_rows, since, mod_date_field,
					mod_by_field, exclude_mod_user, 
					error );
	struct DTGStrList *res = copy_DTGStrList( tmp );
	free_dtg_str_list( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	return res;
}

void DTGModule::proj_referenced_fields( void *projID, struct DTGStrList *f )
{
	if( projID && f && int_proj_referenced_fields )
	    int_proj_referenced_fields( projID, f );
}

void DTGModule::proj_segment_filters( void *projID, struct DTGFieldDesc *f )
{
	if( projID && f && int_proj_segment_filters )
	    int_proj_segment_filters( projID, f );
}

struct DTGStrList *DTGModule::proj_find_defects( void *projID, 
					int max_rows,
					const char *qualification,
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGStrList *tmp = int_proj_find_defects( projID, max_rows,
							qualification, error );
	struct DTGStrList *res = copy_DTGStrList( tmp );
	free_dtg_str_list( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	return res;
}

struct DTGField *DTGModule::defect_get_fields( void *defectID, 
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGField *tmp = int_defect_get_fields( defectID, error );
	struct DTGField *res = copy_DTGField( tmp );
	free_dtg_field( tmp );
	if( error->message )
	{
	    char *str = error->message;
	    error->message = strdup( str );
	    free_char( str );
	}
	struct DTGField *copy = copy_DTGField( pseudo_attrs );
	res = append_DTGField( res, copy );
	
	return res;
}

char *DTGModule::defect_get_field( void *defectID, const char *field, 
					struct DTGError *error )
{
	clear_DTGError( error );
	struct DTGField* fields = pseudo_attrs;
	while( fields && strcmp( fields->name, field ) )
	    fields = fields->next;
	if( fields )
	    return strdup( fields->value );

	char *tmp = int_defect_get_field( defectID, field, error );
	char *res;
	if( tmp )
	{
	    res = strdup( tmp );
	    free_char( tmp );
	}
	else
	    res = NULL;
	if( error->message )
	{
	    tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

char *DTGModule::defect_save( void *defectID, struct DTGError *error )
{
	clear_DTGError( error );
	char *tmp = int_defect_save( defectID, error );
	char *res;
	if( tmp )
	{
	    res = strdup( tmp );
	    free_char( tmp );
	}
	else
	    res = NULL;
	if( error->message )
	{
	    tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

const char *DTGModule::dt_get_name( struct DTGError *error )
{
	clear_DTGError( error );
	const char *res = int_dt_get_name( error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

const char *DTGModule::dt_get_module_version( struct DTGError *error )
{
	clear_DTGError( error );
	const char *res = int_dt_get_module_version( error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

const char *DTGModule::dt_get_server_version( void *dtID, 
						struct DTGError *error )
{
	clear_DTGError( error );
	const char *res = int_dt_get_server_version( dtID, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

static 
char *join_strings( const char *a, const char *b )
{
	char *s = new char[strlen(a) + strlen(b) + 1];
	strcpy( s, a );
	strcat( s, b );
	return s;
}

static
struct DTGField *new_pseudoDTGField( int config, 
				const char *name, const char *value )
{
	char *field;
	if( config )
	    field = join_strings( "DTGConfig-", name );
	else
	    field = join_strings( "DTGAttribute-", name );
	struct DTGField *ret = new_DTGField( field, value );
	delete[] field;
	return ret;
}

void *DTGModule::dt_connect( const char *server, 
			const char *user, 
			const char *pass, 
			const struct DTGField *attrs, 
			struct DTGError *error )
{
	clear_DTGError( error );
	void *res = int_dt_connect( server, user, pass, attrs, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}

	delete_DTGField( pseudo_attrs );
	pseudo_attrs = new_pseudoDTGField( 1, "User", user );
	pseudo_attrs = append_DTGField( pseudo_attrs,
				new_pseudoDTGField( 1, "Server", server ) );

	if( !has_attribute_extensions() )
	    return res;

	/* Add in attributes */
	struct DTGAttribute *all_list = dt_list_attrs();
	const struct DTGField *a;
	for( struct DTGAttribute *all = all_list; all; all = all->next )
	{
	    for( a = attrs; 
		a && strcmp( a->name, all->name );
		a = a->next );
	    if( a )
	        pseudo_attrs = append_DTGField( pseudo_attrs,
				new_pseudoDTGField( 0, all->label, a->value) );
	    else if( all->def && *all->def )
	        pseudo_attrs = append_DTGField( pseudo_attrs,
				new_pseudoDTGField( 0, all->label, all->def));
	}
	delete_DTGAttribute( all_list );

	return res;
}

void DTGModule::dt_free( void *dtID, struct DTGError *error )
{
	clear_DTGError( error );
	int_dt_free( dtID, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
}

void *DTGModule::dt_get_project( void *dtID, 
				const char *project, 
				struct DTGError *error )
{
	clear_DTGError( error );
	void *res = int_dt_get_project( dtID, project, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	pseudo_attrs = append_DTGField( pseudo_attrs,
				new_pseudoDTGField( 1, "Project", project ) );
	return res;
}

void DTGModule::proj_free( void *projID, struct DTGError *error )
{
	clear_DTGError( error );
	int_proj_free( projID, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
}

void *DTGModule::proj_get_defect( void *projID, 
				const char *defect, 
				struct DTGError *error )
{
	clear_DTGError( error );
	void *res = int_proj_get_defect( projID, defect, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

void *DTGModule::proj_new_defect( void *projID, struct DTGError *error )
{
	clear_DTGError( error );
	void *res = int_proj_new_defect( projID, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
	return res;
}

void DTGModule::defect_free( void *defectID, struct DTGError *error )
{
	clear_DTGError( error );
	int_defect_free( defectID, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
}

void DTGModule::defect_set_field( void *defectID, 
				const char *name, 
				const char *value, 
				struct DTGError *error )
{
	clear_DTGError( error );
	int_defect_set_field( defectID, name, value, error );
	if( error->message )
	{
	    char *tmp = error->message;
	    error->message = strdup( tmp );
	    free_char( tmp );
	}
}
