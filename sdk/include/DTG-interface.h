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

#ifndef DTGINTERFACE_HEADER
#define DTGINTERFACE_HEADER

#include <DTG-platforms.h> /* Included platform variants */

/*
 * Perforce Defect Tracking Software Developers Kit: C Integration Definitions
 *
 * This header defines the required functional interfaces that must be defined
 * in a dynamic module which provides integration with a specific Defect 
 * Tracking System.
 *
 * DATA STRUCTURES:
 *
 * Important: These structures are C based and should be allocated/deallocated
 * using malloc and free. The dtg-utils source provides helper functions for
 * creating, manipulating and deleting these structures.
 *
 * DTGError - Used to return error messages from the integration module
 *            Assign an error message to the message attribute to desscribe
 *            why a particular request failed. If the connection is no longer
 *            useable, then set the can_continue attribute to 0.
 *
 * DTGFieldDesc - A singularly linked-list used to describe type information 
 *               for defect fields. The readonly attribute also can be used to
 *               specify fields which contain the last user to modify a defect,
 *               fields which contain the timestamp of the last change, and
 *               fields which contain the unique defectid for a defect.
 *
 * DTGDate - Describes a timestamp used for searching for modified defects.
 *
 * DTGField - A singularly linked-list used to describe a specific defects
 *            fields and associated values.
 *
 * DTGStrList - A singularly linked-list of strings.
 *
 * DTGAttribute - A singularly linked-list of attributes used for specializing
 *		the use of the plugin.
 *
 * FUNCTIONAL INTERFACES:
 *
 * An dynamic integration module must define all of the following interfaces:
 *
 * Date Conversion between modules:
 *    Retrieving the value of a date field from a specific defect returns
 *    the module specific string representation of the date. The following
 *    two functions convert between modules specific formats using DTGDate
 *
 * struct DTGDate *extract_date( const char *date_string );
 *
 *    Returns the DTGDate value extracted from the module specific string
 *
 * char *format_date( struct DTGDate *date );
 *
 *    Returns the module specific string representation of the date.
 *    The string returned by this function must be allocated using malloc
 *    and will be released using free.
 *
 * const char *dt_get_name( struct DTGError *error );
 *
 *    Returns the name of the integration module itself
 *
 * const char *dt_get_module_version( struct DTGError *error );
 *
 *    Returns the version number of the integration module.
 *
 * struct DTGAttribute *dt_list_attrs();
 *
 *    Returns singularly linked-list of DTGAttributes which may be set for 
 *    a specific source. This interface is optional and is only used by the 
 *    p4dtg-config tool. This may be called prior to dt_connect.
 *
 * char *dt_validate_attr( const struct DTGField *attr );
 *
 *    Returns null if the attr is valid, otherwise returns the text to be
 *    be displayed to the user. This interface is optional. This may be 
 *    called prior to dt_connect and prior to dt_list_attrs. p4dtg-config
 *    uses this interface to validate user input. p4dtg-repl uses this to
 *    confirm valid attribute values prior to initiating replication.
 *
 * void *dt_connect( const char *server, 
 *                   const char *user, const char *pass, 
 *	             const struct DTGField *attrs,
 *                   struct DTGError *error );
 *    Returns an opaque pointer (dtID) to a specific server. A connection to
 *    this server should have been made but an implementation could use a lazy 
 *    connection approach.
 *
 * int dt_accept_utf8( void *dtID, struct DTGError *error );
 *
 *    This is an optional interface. If it is not defined, then connections
 *    to a unicode-enabled Perforce Server will be allowed but warning will
 *    be generated. Return 1 if the current connection accepts UTF-8 strings;
 *    return 0 if the current connection only accepts ASCII strings. Returning
 *    -1 is the same as not having this interface defined.
 *
 *    Define an attribute within the plug-in to enable and disable UTF-8 
 *    processing on a connection by connection basis. 
 *
 * int dt_server_offline( void *dtID, struct DTGError *error );
 *
 *    This is an optional interface. If this interface is defined and there is 
 *    a conection error during replication, the service will pause replication 
 *    for the returned number of seconds before trying again.  It will repeat 
 *    forever until the server returns online. A return value of 0 indicates the
 *    server is online. A return value of -1 indicates an undefined interface
 *    and the value of the Replication Map General Wait Duration will be used.
 *    The number of seconds returned should be between 1 and 100.
 *
 * void dt_free( void *dtID, struct DTGError *error );
 *
 *    Tells the integration module that the system is done with the specified
 *    server.
 *
 * const char *dt_get_server_version( void *dtID, struct DTGError *error );
 *
 *    Returns the version number of the server to which the dtID refers
 *
 * char *dt_get_server_warnings( void *dtID, struct DTGError *error );
 *
 *    Returns any warnings from the server to which the dtID refers
 *    This is used for reporting diminished behavior in the configuration tool
 *    such as a version of the server which is unsupported.
 *    The return value should be released using free.
 *
 * int dt_get_message( void *dtID, struct DTGError *error );
 *
 *    Optional interface for requesting a message be inserted into the
 *    replication log. The return value indicates the log level which is 
 *    required for insertion. The message should be contained in the 
 *    error->msg field and the rest of the error structure is ignored. This
 *    method, if defined, should not contact the server to process the call.
 *
 * struct DTGDate *dt_get_server_date( void *dtID, struct DTGError *error );
 *
 *    Returns the current date/time of the server to which the dtID refers
 *    You should use the new_DTGDate helper function to allocate the result
 *
 * struct DTGStrList *dt_list_projects( void *dtID, struct DTGError *error );
 *
 *    Returns a singularly linked-list of strings listing all of the projects 
 *    contained on the referenced server
 *
 * void *dt_get_project( void *dtID, const char *project, 
 *                       struct DTGError *error );
 *    Returns an opaque pointer (projID) to a specific project on the specified
 *    server.
 *
 * void proj_free( void *projID, struct DTGError *error );
 *
 *    Tells the integration module that the system is done with the specified
 *    project.
 *
 * struct DTGFieldDesc *proj_list_fields( void *projID, 
 *                                       struct DTGError *error );
 *    Returns a singularly linked-list of field descriptions associated with
 *    the specified project. Each field has a type and a modifier describing
 *    special properties of the field. The returned types must be one of
 *    WORD, DATE, LINE, TEXT, or SELECT. The struct DTGFieldDesc also contains
 *    flags for marking which fields are read-only and more importantly which
 *    one contains the last modified date information. See the description of
 *    the field below. Optionally, the type may be FIX, which is used to 
 *    describe a text field which can only have 'Fix details' connected to it.
 *    Field of type FIX must return an error along with the current value for
 *    any call to defect_get_field on that field. This error message should
 *    state that the field may only be the the mapping target of 'Fix details'.
 *
 * struct DTGStrList *proj_list_changed_defects( void *projID, 
 *                                              int max_rows,
 *                                              struct DTGDate *since, 
 *                                              const char *mod_date_field,
 *                                              const char *mod_by_field,
 *                                              const char *exclude_mod_user,
 *                                              struct DTGError *error );
 *    Returns a singularly linked-list of strings listing max_rows of defects
 *    which have been modified since the specified date. If a specific defect
 *    system does not support restricting queries by user or date, then the
 *    provided values may be ignored by the plugin. The mod_by_field and
 *    exclude_mod_user arguments may be NULL.
 *    
 *    The mod_date_field specifies which field to use for date comparison.
 *    mod_by_field and exclude_mod_user specify defects to be excluded from 
 *    the results set. If max_rows is < 1, then all changed defects are to 
 *    be returned.
 *
 * void proj_referenced_fields( void *projID, struct DTGStrList *fields )
 *
 *    This optional interface provides a list of all of the fields which
 *    are required by the DataMapping being used by the replication engine.
 *    This information may provide some plug-ins with performance gains by
 *    restricting what fields are retrieved from the DTS. These fields will
 *    be the only fields which are read or written by the replication engine.
 *    The plug-in should make its own copy of the list of fields.
 *
 * void proj_segment_filters( void *projID, struct DTGFieldDesc *filters )
 *
 *    This optional interface provides information on the segmentation filter
 *    being used by the replication engine. Plug-ins implementing this 
 *    interface should restrict the results from 'proj_list_changed_defects'
 *    to only include defects which match all of the filters passed in by this
 *    interface. Each individual item in 'filters' contains a field name and
 *    the select_values list of matching values for that field. In other words,
 *    the list of items in 'filters' are AND criteria and the select_values of
 *    each item are 'OR' criteria.
 *    The plug-in should make its own copy of the list of filters.
 *
 * void *proj_get_defect( void *projID, const char *defect, 
 *                        struct DTGError *error );
 *    Returns an opaque pointer (defectID) to a specific defect within the 
 *    specified project as identified by the defect argument.
 *
 * void *proj_new_defect( void *projID, struct DTGError *error );
 *
 *    Returns an opaque pointer (defectID) to a newly created defect within the
 *    specified project.
 *
 * void defect_free( void *defectID, struct DTGError *error );
 *
 *    Tells the integration module that the system is done with the specified
 *    defect and it can be freed. Any changes which have not been saved should
 *    be discarded.
 *
 * struct DTGField *defect_get_fields( void *defectID, struct DTGError *error );
 *
 *    Returns a singularly linked-list of field-value pairs for the specified
 *    defect.
 *
 * char *defect_get_field( void *defectID, const char *field, 
 *                               struct DTGError *error );
 *    Returns the current value of the named field in the specified defect.
 *
 * void defect_set_field( void *defectID, 
 *                        const char *name, const char *value, 
 *                        struct DTGError *error );
 *    Sets the value of the named field for the specified defect. This does not
 *    actually save the values to the server. A final call to defect_save is
 *    required to write the data out to the server.
 *
 * char *defect_save( void *defectID, struct DTGError *error );
 *
 *    Saves the specified defect out to the server and returns the defect 
 *    identifier. The return value is mainly intended for returning the
 *    generated identity for newly created defects.
 *
 *
 * Perforce Specific Interface Functions and Types
 * The following functions and types are specific to the Perforce plugin and
 * are not required or used for any other plugin
 *
 * struct DTGStrList *proj_find_defects( void *projID, 
 * 					int max_rows,
 * 					const char *qualification,
 * 					struct DTGError *error );
 * 
 * struct DTGStrList *proj_list_fixes( void *projID, 
 * 					const char *defect,
 * 					struct DTGError *error );
 *
 * struct DTGFixDesc *proj_describe_fix( void *projID, 
 * 					const char *fixid,
 * 					struct DTGError *error );
 *
 */

struct DTGError {
	char *message;
	int can_continue; // 0: connection failure
};

struct DTGStrList {
	char *value;
	struct DTGStrList *next;
};

struct DTGFieldDesc {
	char *name;
	char *type; // word, date, line, text, select, fix
	int readonly; // 0:rw, >= 1:ro, 2:mod-date, 3:mod-user, 4:defectid
	struct DTGStrList *select_values;
	struct DTGFieldDesc *next;
};

struct DTGDate {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
};

struct DTGField {
	char *name;
	char *value;
	struct DTGField *next;
};

struct DTGFixDesc {
	char *change;
	char *user;
	char *stamp;
	char *desc;
	struct DTGStrList *files;
};

struct DTGAttribute {
	char *name;	// Key, will be the name field in DTGField struct
	char *label;	// Displayed in UI
	char *desc;	// Displayed in UI
	char *def;	// default, May be null
	int required;	// 0:Not required, o/w:Required
	struct DTGAttribute *next;
};

#endif
