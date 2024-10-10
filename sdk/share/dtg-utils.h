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

#ifndef DTGUTILS_HEADER
#define DTGUTILS_HEADER


/*
 * Perforce Defect Tracking Gateway Developers Kit: C Integration Definitions
 *
 * This header define helper function for the various data structures
 * used by the Perforce Defect Tracking Gateway. Since both C and C++
 * are supported, these functions provide the correct use of malloc
 * and free for object of these types. 
 * 
 * All pointer attributed in these structures are allocated using malloc
 * and must be released using free. 
 *
 * DATA STRUCTURES:
 *
 * DTGError - Used to return error messages from the integration module along
 *           with whether the error is fatal or mearly a warning.
 *
 * DTGFieldDesc - A singularly linked-list used to describe type information for 
 *               defect fields.
 *
 * DTGDate - Describes a timestamp used for searching for modified defects.
 *
 * DTGField - A singularly linked-list used to describe a specific defects 
 *           fields and associated values.
 *
 * DTGStrList - A singularly linked-list of strings.
 *
 * DTGAttribute - A singularly linked-list of attributes used for specializing
 *		the use of the plugin.
 *
 */

#include <DTG-interface.h>

/*
struct DTGError {
	char *message;
	int can_continue; // 0: connection failure
};
*/

struct DTGError *new_DTGError( const char *msg );

void delete_DTGError( struct DTGError *err );
void clear_DTGError( struct DTGError *err );
void set_DTGError( struct DTGError *err, const char *msg );

/*
struct DTGStrList {
	char *value;
	struct DTGStrList *next;
};
*/

struct DTGStrList *new_DTGStrList( const char *txt );
void delete_DTGStrList( struct DTGStrList *list );
struct DTGStrList *append_DTGStrList( struct DTGStrList *list, 
					const char *item );
struct DTGStrList *merge_DTGStrList( struct DTGStrList *list1, 
					struct DTGStrList *list2 );
struct DTGStrList *copy_DTGStrList( const struct DTGStrList *list );
struct DTGStrList *split_DTGStrList( const char *txt, char sep );
struct DTGStrList *smart_split_DTGStrList( const char *txt, char sep );
char *join_DTGStrList( struct DTGStrList *list, const char *sep );
struct DTGStrList *remove_DTGStrList( struct DTGStrList *list, 
					struct DTGStrList *rem );
struct DTGStrList *purge_DTGStrList( struct DTGStrList *list );
int in_DTGStrList( const char *item, struct DTGStrList *list );

/*
struct DTGFieldDesc {
	char *name;
	char *type; // word, date, line, text, select
	int readonly; // 0:rw, >= 1:ro, 2:mod-date, 3:mod-user, 4:defectid
	struct DTGStrList *select_values;
	struct DTGFieldDesc *next;
};
*/

struct DTGFieldDesc *new_DTGFieldDesc( const char *name,
				     const char *type,
				     int readonly,
				     struct DTGStrList *selections );
void delete_DTGFieldDesc( struct DTGFieldDesc *list );
struct DTGFieldDesc *append_DTGFieldDesc( struct DTGFieldDesc *list,
					struct DTGFieldDesc *item );
struct DTGFieldDesc *copy_DTGFieldDesc( const struct DTGFieldDesc *list );

/*
struct DTGDate {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
};
*/

struct DTGDate *new_DTGDate( int year, int month, int day,
				int hour, int min, int sec );
struct DTGDate *copy_DTGDate( const struct DTGDate *obj );
void delete_DTGDate( struct DTGDate *item );
int compare_DTGDate( struct DTGDate *d1, struct DTGDate *d2 );
void set_DTGDate( struct DTGDate *base, struct DTGDate *revised );

/*
struct DTGField {
	char *name;
	char *value;
	struct DTGField *next;
};
*/
struct DTGField *new_DTGField( const char *name,
				const char *value );
struct DTGField *copy_DTGField( const struct DTGField *obj );
void delete_DTGField( struct DTGField *list );
struct DTGField *append_DTGField( struct DTGField *list,
				struct DTGField *item );

/*
struct DTGFixDesc {
	char *change;
	char *user;
	char *stamp
	char *desc;
	DTGStrList *files;
};
*/
struct DTGFixDesc *new_DTGFixDesc();
struct DTGFixDesc *copy_DTGFixDesc( const struct DTGFixDesc *obj );
void delete_DTGFixDesc( struct DTGFixDesc *item );

/*
struct DTGAttribute {
	char *name;	// Key, will be the name field in DTGField struct
	char *label;	// Displayed in UI
	char *desc;	// Displayed in UI
	char *def;	// default, May be null
	int required;	// 0:Not required, o/w:Required
	struct DTGAtribute *next;
};
*/

struct DTGAttribute *new_DTGAttribute( const char *name,
				     const char *label,
				     const char *desc,
				     const char *def,
				     int required );
void delete_DTGAttribute( struct DTGAttribute *list );
struct DTGAttribute *append_DTGAttribute( struct DTGAttribute *list,
					struct DTGAttribute *item );
struct DTGAttribute *copy_DTGAttribute( const struct DTGAttribute *list );

#endif
