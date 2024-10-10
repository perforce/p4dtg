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

#ifndef DTGTYPEDEFS_HEADER
#define DTGTYPEDEFS_HEADER

#include <DTG-interface.h>

extern "C" {

typedef struct DTGDate *(extract_date_ftn)( const char *date_string );
typedef char *(format_date_ftn)( struct DTGDate *date );

typedef const char *(dt_get_name_ftn)( struct DTGError *error );
typedef const char *(dt_get_module_version_ftn)( struct DTGError *error );
typedef const char *(dt_get_server_version_ftn)( void *dtID, 
                                                 struct DTGError *error );
typedef char *(dt_get_server_warnings_ftn)( void *dtID, 
                                            struct DTGError *error );
typedef int (dt_get_message_ftn)( void *dtID, struct DTGError *error );
typedef struct DTGDate *(dt_get_server_date_ftn)( void *dtID, 
						struct DTGError *error );
typedef struct DTGAttribute *(dt_list_attrs_ftn)();
typedef char *(dt_validate_attr_ftn)( const struct DTGField *attr );
typedef void *(dt_connect_ftn)( const char *server, 
                                const char *user, const char *pass, 
	          		const struct DTGField *attrs,
                                struct DTGError *error );
typedef int (dt_accept_utf8_ftn)( void *dtID, struct DTGError *error );
typedef int (dt_server_offline_ftn)( void *dtID, struct DTGError *error );
typedef void (dt_free_ftn)( void *dtID, struct DTGError *error );
typedef struct DTGStrList *(dt_list_projects_ftn)( void *dtID, 
                                                  struct DTGError *error );
typedef void *(dt_get_project_ftn)( void *dtID, const char *project, 
                                    struct DTGError *error );
typedef void (proj_free_ftn)( void *projID, struct DTGError *error );

typedef struct DTGFieldDesc *(proj_list_fields_ftn)( void *projID, 
                                                    struct DTGError *error );
typedef struct DTGStrList *(proj_list_fixes_ftn)( void *projID, 
						const char *defect,
						struct DTGError *error );
typedef struct DTGFixDesc *(proj_describe_fix_ftn)( void *projID, 
						const char *fixid,
                                                struct DTGError *error );
typedef struct DTGStrList *(proj_list_changed_defects_ftn)( void *projID, 
						int max_rows,
						struct DTGDate *since, 
						const char *mod_date_field,
						const char *mod_by_field,
						const char *exclude_mod_user,
						struct DTGError *error );
typedef struct DTGStrList *(proj_find_defects_ftn)( void *projID, 
						int max_rows,
						const char *qualification,
						struct DTGError *error );
typedef void (proj_referenced_fields_ftn)( void *projID, 
						struct DTGStrList *fields );
typedef void (proj_segment_filters_ftn)( void *projID, struct DTGFieldDesc *filters );
typedef void *(proj_get_defect_ftn)( void *projID, const char *defect, 
                                     struct DTGError *error );
typedef void *(proj_new_defect_ftn)( void *projID, struct DTGError *error );
typedef void (defect_free_ftn)( void *defectID, struct DTGError *error );

typedef struct DTGField *(defect_get_fields_ftn)( void *defectID, 
                                                 struct DTGError *error );
typedef char *(defect_get_field_ftn)( void *defectID, const char *field, 
                                            struct DTGError *error );
typedef void (defect_set_field_ftn)( void *defectID, 
                                     const char *name, const char *value, 
                                     struct DTGError *error );
typedef char *(defect_save_ftn)( void *defectID, struct DTGError *error );

typedef void (free_char_ftn)( char *obj );
typedef void (free_dtg_error_ftn)( struct DTGError *obj );
typedef void (free_dtg_str_list_ftn)( struct DTGStrList *obj );
typedef void (free_dtg_field_desc_ftn)( struct DTGFieldDesc *obj );
typedef void (free_dtg_date_ftn)( struct DTGDate *obj );
typedef void (free_dtg_field_ftn)( struct DTGField *obj );
typedef void (free_dtg_fix_desc_ftn)( struct DTGFixDesc *obj );
typedef void (free_dtg_attribute_ftn)( struct DTGAttribute *obj );

}

#endif


