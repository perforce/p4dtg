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
#include "MyDTG.h"

#ifdef _WIN32
#include <windows.h>
#define DL_EXPORT_FTN extern "C" __declspec(dllexport)
#else
#define DL_EXPORT_FTN extern "C"
#endif

#ifdef DEBUG
FILE *log = NULL;
FILE *useLog()
{
	if( !log )
	    log = fopen("mysql-dtg.log", "a+");
	return log;
}
#endif

DL_EXPORT_FTN
struct DTGDate *extract_date( const char *date_string )
{
#ifdef DEBUG
	fprintf( useLog(), "extract_date(%s)\n", date_string );
#endif
	return MyDTG::extract_date( date_string );
}

DL_EXPORT_FTN
char *format_date( struct DTGDate *date )
{
#ifdef DEBUG
	fprintf( useLog(), "format_date(%d,%d,%d)\n",
		date->year, date->month, date->day,
		date->hour, date->minute, date->second
		 );
#endif
	return MyDTG::format_date( date );
}

DL_EXPORT_FTN
struct DTGAttribute *dt_list_attrs()
{
#ifdef DEBUG
	fprintf( useLog(), "dt_list_attrs()\n" );
#endif
	return MyDTG::list_attrs();
}

DL_EXPORT_FTN
char *dt_validate_attr( const struct DTGField *attr )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_validate_attr()\n" );
#endif
	return MyDTG::validate_attr( attr );
}

DL_EXPORT_FTN
const char *dt_get_name( struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_get_name()\n" );
#endif
	return MyDTG::get_name( error );
}

DL_EXPORT_FTN
const char *dt_get_module_version( struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_get_module_version()\n" );
#endif
	return MyDTG::get_module_version( error );
}

DL_EXPORT_FTN
void *dt_connect( const char *server, 
	          const char *user, const char *pass, 
	          const struct DTGField *attrs,
	          struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_connect(%s,%s,%s)\n", server, user, pass );
#endif
	MyDTG *dt = new MyDTG( server, user, pass, attrs, error );
	if( error->message )
	{
	    delete dt;
	    dt = NULL;
	}

	return dt;
}

DL_EXPORT_FTN
int dt_accept_utf8( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_accept_utf8()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_accept_utf8: Unknown dtID" );
	    return -1; // UNKNOWN
	}

	return mydt->accept_utf8();
}

DL_EXPORT_FTN
int dt_server_offline( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_server_offline()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_server_offline: Unknown dtID" );
	    return -1; // UNKNOWN
	}

	return mydt->server_offline( error );
}

DL_EXPORT_FTN
void dt_free( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_free()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_free: Unknown dtID" );
	    return;
	}

	delete mydt;
	clear_DTGError( error );
	return;
}

DL_EXPORT_FTN
const char *dt_get_server_version( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_get_server_version()\n" );
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_get_server_version: Unknown dtID" );
	    return NULL;
	}

	return mydt->get_server_version( error );
}

DL_EXPORT_FTN
char *dt_get_server_warnings( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_get_server_warnings()\n" );
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_get_server_warnings: Unknown dtID" );
	    return NULL;
	}

	return mydt->get_server_warnings( error );
}

DL_EXPORT_FTN
struct DTGDate *dt_get_server_date( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_get_server_date()\n" );
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_get_server_date: Unknown dtID" );
	    return NULL;
	}

	return mydt->get_server_date( error );
}

DL_EXPORT_FTN
struct DTGStrList *dt_list_projects( void *dtID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_list_projects()\n" );
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_list_projects: Unknown dtID" );
	    return NULL;
	}

	return mydt->list_projects( error );
}

DL_EXPORT_FTN
void *dt_get_project( void *dtID, const char *project, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "dt_get_project(%s)\n", project );
#endif
	MyDTG *mydt = MyDTG::convert( dtID );
	if( !mydt )
	{
	    set_DTGError( error, "dt_get_project: Unknown dtID" );
	    return NULL;
	}

	return mydt->get_project( project, error );
}

DL_EXPORT_FTN
void proj_free( void *projID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "proj_free()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	MyDTGProj *mydtproj = MyDTGProj::convert( projID );
	if( !mydtproj )
	{
	    set_DTGError( error, "proj_free: Unknown projID" );
	    return;
	}

	delete mydtproj;
	clear_DTGError( error );
}

DL_EXPORT_FTN
struct DTGFieldDesc *proj_list_fields( void *projID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "proj_list_fields()\n" );
#endif
	MyDTGProj *mydtproj = MyDTGProj::convert( projID );
	if( !mydtproj )
	{
	    set_DTGError( error, "proj_list_fields: Unknown projID" );
	    return NULL;
	}

	return mydtproj->list_fields( error );
}

DL_EXPORT_FTN
struct DTGStrList *proj_list_changed_defects( void *projID,
					int max_rows,
					struct DTGDate *since,
					const char *mod_date_field,
					const char *mod_by_field,
					const char *exclude_user,
					struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), 
	"proj_list_changed_defects(%d,%d/%d/%d/%d/%d/%d,%s,%s,%s)\n",
		max_rows, 
	since->year, since->month, since->day,
	since->hour, since->minute, since->second,
		mod_date_field, mod_by_field, exclude_user );
	fflush( useLog() );
#endif
	MyDTGProj *mydtproj = MyDTGProj::convert( projID );
	if( !mydtproj )
	{
	    set_DTGError( error, "proj_list_changed_defects:Unknown projID" );
	    return NULL;
	}

	return mydtproj->list_changed_defects( max_rows, since, 
			mod_date_field, mod_by_field, exclude_user, error );
}

DL_EXPORT_FTN
void *proj_get_defect( void *projID, const char *defect, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "proj_get_defect(%s)\n", defect );
#endif
	MyDTGProj *mydtproj = MyDTGProj::convert( projID );
	if( !mydtproj )
	{
	    set_DTGError( error, "proj_get_defect: Unknown projID" );
	    return NULL;
	}

	return mydtproj->get_defect( defect, error );
}

DL_EXPORT_FTN
void *proj_new_defect( void *projID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "proj_new_defect()\n" );
#endif
	MyDTGProj *mydtproj = MyDTGProj::convert( projID );
	if( !mydtproj )
	{
	    set_DTGError( error, "proj_new_defect: Unknown projID" );
	    return NULL;
	}

	return mydtproj->new_defect( error );
}

DL_EXPORT_FTN
void defect_free( void *defectID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "defect_free()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	MyDTGDefect *mydtdefect = MyDTGDefect::convert( defectID );
	if( !mydtdefect )
	{
	    set_DTGError( error, "defect_free: Unknown defectID" );
	    return;
	}

	delete mydtdefect;
	clear_DTGError( error );
}

DL_EXPORT_FTN
struct DTGField *defect_get_fields( void *defectID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "defect_get_fields()\n" );
#endif
	MyDTGDefect *mydtdefect = MyDTGDefect::convert( defectID );
	if( !mydtdefect )
	{
	    set_DTGError( error, "defect_get_fields: Unknown defectID" );
	    return NULL;
	}

	return mydtdefect->get_fields( error );
}

DL_EXPORT_FTN
char *defect_get_field( void *defectID, 
	                      const char *field, 
	                      struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "defect_get_field(%s)\n", field );
#endif
	MyDTGDefect *mydtdefect = MyDTGDefect::convert( defectID );
	if( !mydtdefect )
	{
	    set_DTGError( error, "defect_get_field: Unknown defectID" );
	    return NULL;
	}

	return mydtdefect->get_field( field, error );
}

DL_EXPORT_FTN
void defect_set_field( void *defectID, 
	               const char *name, const char *value, 
	               struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "defect_set_field(%s,%s)\n", name, value );
#endif
	MyDTGDefect *mydtdefect = MyDTGDefect::convert( defectID );
	if( !mydtdefect )
	{
	    set_DTGError( error, "defect_set_field: Unknown defectID" );
	    return;
	}

	mydtdefect->set_field( name, value, error );
}

DL_EXPORT_FTN
char *defect_save( void *defectID, struct DTGError *error )
{
#ifdef DEBUG
	fprintf( useLog(), "defect_save()\n" );
#endif
	MyDTGDefect *mydtdefect = MyDTGDefect::convert( defectID );
	if( !mydtdefect )
	{
	    set_DTGError( error, "defect_save: Unknown defectID" );
	    return NULL;
	}

	return mydtdefect->save( error );
}

DL_EXPORT_FTN
void free_char( char *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_char(%s)\n", obj );
	fclose( useLog() );
	log = NULL;
#endif
	if( obj )
	    free( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_error( struct DTGError *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_error()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGError( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_str_list( struct DTGStrList *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_str_list()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGStrList( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_field_desc( struct DTGFieldDesc *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_field_desc()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGFieldDesc( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_date( struct DTGDate *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_date()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGDate( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_field( struct DTGField *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_field()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGField( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_fix_desc( struct DTGFixDesc *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_fix_desc()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGFixDesc( obj );
	return;
}

DL_EXPORT_FTN
void free_dtg_attribute( struct DTGAttribute *obj )
{
#ifdef DEBUG
	fprintf( useLog(), "free_dtg_attribute()\n" );
	fclose( useLog() );
	log = NULL;
#endif
	delete_DTGAttribute( obj );
	return;
}
