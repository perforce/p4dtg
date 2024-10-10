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

#ifndef DTGSTDCIF_HEADER
#define DTGSDDCIF_HEADER

#include <DTG-typedefs.h>

class DTGModule {

    public:
	void *lib_handle;
	char *dl_name;

	DTGModule *next;

	static const int MAX_ERR_MSG = 1000;
	char last_error[MAX_ERR_MSG + 1];

    protected:
	void *load_function( const char *ftn_name );
	void record_error( const char *ftn, const char *error );

	/* Memory re-allocation needed for these interfaces */
	extract_date_ftn *int_extract_date;
	format_date_ftn *int_format_date;
	dt_get_server_warnings_ftn *int_dt_get_server_warnings;
	dt_get_server_date_ftn *int_dt_get_server_date;
	dt_list_projects_ftn *int_dt_list_projects;
	proj_list_fields_ftn *int_proj_list_fields;
	proj_list_fixes_ftn *int_proj_list_fixes;
	proj_describe_fix_ftn *int_proj_describe_fix;
	proj_list_changed_defects_ftn *int_proj_list_changed_defects;
	proj_find_defects_ftn *int_proj_find_defects;
	defect_get_fields_ftn *int_defect_get_fields;
	defect_get_field_ftn *int_defect_get_field;
	defect_save_ftn *int_defect_save;

	dt_get_name_ftn *int_dt_get_name;
	dt_get_module_version_ftn *int_dt_get_module_version;
	dt_get_server_version_ftn *int_dt_get_server_version;
	dt_connect_ftn *int_dt_connect;
	dt_free_ftn *int_dt_free;
	dt_get_project_ftn *int_dt_get_project;
	proj_free_ftn *int_proj_free;
	proj_get_defect_ftn *int_proj_get_defect;
	proj_new_defect_ftn *int_proj_new_defect;
	defect_set_field_ftn *int_defect_set_field;
	defect_free_ftn *int_defect_free;

	/* Optional Interfaces */
	dt_accept_utf8_ftn *int_dt_accept_utf8;
	dt_server_offline_ftn *int_dt_server_offline;
	dt_list_attrs_ftn *int_dt_list_attrs;
	dt_validate_attr_ftn *int_dt_validate_attr;
	dt_get_message_ftn *int_dt_get_message;
	proj_referenced_fields_ftn *int_proj_referenced_fields;
	proj_segment_filters_ftn *int_proj_segment_filters;

    public:
	int has_perforce_extensions();
	int has_attribute_extensions();

	struct DTGDate *extract_date( const char *date_string );
	char *format_date( struct DTGDate *date );
	char *dt_get_server_warnings( void *dtID, struct DTGError *error );
	int dt_get_message( void *dtID, struct DTGError *error );
	int dt_accept_utf8( void *dtID, struct DTGError *error );
	int dt_server_offline( void *dtID, struct DTGError *error );
	struct DTGDate *dt_get_server_date( void *dtID, 
					struct DTGError *error );
	struct DTGStrList *dt_list_projects( void *dtID, 
					struct DTGError *error );
	struct DTGFieldDesc *proj_list_fields( void *projID, 
					struct DTGError *error );
	struct DTGStrList *proj_list_fixes( void *projID, 
					const char *defect,
					struct DTGError *error );
	struct DTGFixDesc *proj_describe_fix( void *projID, 
					const char *fixid,
					struct DTGError *error );
	struct DTGStrList *proj_list_changed_defects( void *projID, 
					int max_rows,
					struct DTGDate *since, 
					const char *mod_date_field,
					const char *mod_by_field,
					const char *exclude_mod_user,
					struct DTGError *error );
	struct DTGStrList *proj_find_defects( void *projID, 
					int max_rows,
					const char *qualification,
					struct DTGError *error );
	void proj_referenced_fields( void *projID, struct DTGStrList *fields );
	void proj_segment_filters( void *projID, struct DTGFieldDesc *filters );
	struct DTGField *defect_get_fields( void *defectID, 
					struct DTGError *error );
	char *defect_get_field( void *defectID, const char *field, 
					struct DTGError *error );
	char *defect_save( void *defectID, struct DTGError *error );

	const char *dt_get_name( struct DTGError *error );
	const char *dt_get_module_version( struct DTGError *error );
	const char *dt_get_server_version( void *dtID, struct DTGError *error );
	void *dt_connect( const char *server,
					const char *user, const char *pass,
					const struct DTGField *attrs,
					struct DTGError *error );
	void dt_free( void *dtID, struct DTGError *error );
	void *dt_get_project( void *dtID, const char *project,
					struct DTGError *error );
	void proj_free( void *projID, struct DTGError *error );
	void *proj_get_defect( void *projID, const char *defect,
					struct DTGError *error );
	void *proj_new_defect( void *projID, struct DTGError *error );
	void defect_free( void *defectID, struct DTGError *error );
	void defect_set_field( void *defectID,
					const char *name, const char *value,
					struct DTGError *error );

	/* Optional interfaces */
	struct DTGAttribute *dt_list_attrs();
	char *dt_validate_attr( const struct DTGField *attr );

	/* De-allocation of memory allocated in the shared library */
	free_char_ftn *free_char;
	free_dtg_error_ftn *free_dtg_error;
	free_dtg_str_list_ftn *free_dtg_str_list;
	free_dtg_field_desc_ftn *free_dtg_field_desc;
	free_dtg_date_ftn *free_dtg_date;
	free_dtg_field_ftn *free_dtg_field;
	free_dtg_fix_desc_ftn *free_dtg_fix_desc;
	free_dtg_attribute_ftn *free_dtg_attribute;

	DTGModule(const char *dl_name);
	~DTGModule();

	void test_module();

    private:
	struct DTGField *pseudo_attrs;
};

#endif
