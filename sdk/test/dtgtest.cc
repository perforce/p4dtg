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
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <DTGModule.h>
#include <DTG-platforms.h>
extern "C" {
#include <dtg-utils.h>
}

#define IF_OBJECT(obj,msg) if(obj){fprintf(stderr, "%s\n", msg); break;}
#define IF_NOT_OBJECT(obj,msg) if(!obj){fprintf(stderr, "%s\n", msg); break;}

void print_help()
{
	printf("\
Library\n\
        LL path                 - new DTGModule(path)\n\
        LT                      - DTGmodule::test()\n\
        LN                      - dt_get_name()\n\
        LV                      - dt_get_module_version()\n\
        LC server user pass     - dt_connect(server, user, pass) -> dtID\n\
        LF                      - delete DTGModule()\n\
        LE<cr>datestring        - extract_date( datestring )\n\
                                - datestring: -as specified by the plugin-\n\
        LI yyyy mm dd hh mm ss  - format_date( date )\n\
        Q{uit}                  - Quit tool. Exit will also quit tool\n\
        H{elp}                  - Show this help text\n\
\n");
	printf("\
Attribute - Optional plugin interfaces\n\
        AL                      - list_attrs()\n\
        AV name value           - validate_attr( name, value )\n\
\n\
        The following are only effective prior to the LC command.\n\
        AS name value           - set attribute\n\
        AD name                 - delete attribute\n\
\n");
	printf("\
Server (these require dtID to be set before use)\n\
        SF                      - dt_free(dtID) Unsets dtID\n\
        SV                      - dt_get_server_version(dtID)\n\
        SW                      - dt_get_server_warnings(dtID)\n\
        SM                      - dt_get_message(dtID)\n\
        SL                      - dt_list_projects(dtID)\n\
        SU                      - dt_accept_utf8(dtID)\n\
        SO                      - dt_server_offline(dtID)\n\
        SD                      - dt_get_server_date(dtID)\n\
                                  May require project connection\n\
        SP project              - dt_get_project(dtID, project) -> projID\n\
\n");
	printf("\
Project (these require projID to be set before use)\n\
        PF                      - proj_free(projID) Unsets projID\n\
        PS                      - proj_list_fields(projID)\n\
        PL n d datef userf user - proj_list_changed_defects(projID, ...)\n\
                n:      max rows to return\n\
                d:      yyyy/mm/dd/hh/mm/ss modified since date\n\
                datef:  ModifiedDate field name\n\
                userf:  ModifiedBy field name\n\
                user:   Exclude user\n\
        PD defect               - proj_get_defect(projID, defect) -> defectID\n\
        PN                      - proj_new_defect(projID) -> defectID\n\
        PR f1 f2 f3             - proj_referenced_fields(projID)\n\
\n\
	PI<cr>			- proj_segment_filters(projID, ...)\n\
	Field1 Opt1 Opt2 ...	  Each field/options set is on its own line\n\
	Field2 Opt1 Opt2 ...\n\
	.			  Terminate with a solo period\n\
\n\
	The following are specific to the Perforce plugin:\n\
        PQ n qual - proj_find_defects(projID, ...)\n\
                n:      max rows to return\n\
                qual:   qualification to use on query\n\
        PC defect               - proj_list_fixes(projID, defect)\n\
        PA fix                  - proj_describe_fix(projID, fix)\n\
\n");
	printf("\
Defect (these require defectID to be set before use)\n\
        DF                      - defect_free(defectID) Unsets defectID\n\
        DL                      - defect_get_fields(defectID)\n\
        DW field value          - defect_set_field(defectID, field,value)\n\
        DR field                - defect_get_field(defectID, field)\n\
        DS                      - defect_save(defectID)\n\
");
}

int echo_mode = 0;

char *joinup( struct DTGStrList *parts )
{
	if( !parts || !parts->value )
	    return NULL;

	struct DTGStrList *p = parts;
	int len = 0;
	while( p )
	{
	    len += 1 + strlen( p->value );
	    p = p->next;
	}
	char *tmp = new char[len+1];
	p = parts;
	strcpy( tmp, p->value );
	p = p->next;
	while( p )
	{
	    strcat( tmp, " " );
	    strcat( tmp, p->value );
	    p = p->next;
	}
	return tmp;
}

void print_attribute( struct DTGAttribute *attr )
{
	printf( "Attr: %s [%s]\n", attr->name, attr->label );
	printf( "\t%s\n", attr->desc );
	printf( "\tDefault: %s\n", attr->def ? attr->def : "NONE" );
	if( attr->required )
	    printf( "\tREQUIRED\n" );
}

void print_DTGField( const struct DTGField *list )
{
	struct DTGField *cur = (DTGField*)list;
	while( cur )
	{
	    struct DTGField *next = cur->next;
	    if( cur->value )
	        printf( "cur->name:  \"%s\"\ncur->value:  \"%s\"\n\n",
	    cur->name, cur->value );
	    cur = next;
	}
}

/* remove the named field */
const char *rm_DTGField( struct DTGField *&list, const char* name )
{
	if( !name || !list )
	    return NULL;

	struct DTGField *cur  = (struct DTGField*)list;
	struct DTGField *next = NULL;
	struct DTGField *prev = NULL;
	while( cur )
	{
	    next = cur->next;
	    if( !strcmp( cur->name, name ) )
	    {
	        if( prev )
	            prev->next = cur->next;
	        else
	            list = cur->next;

	        cur->next = NULL;
	        delete_DTGField( cur );
	        return name;
	    }
	    prev = cur;
	    cur = next;
	}
	return NULL;
}

void do_attribute_cmds( struct DTGStrList *parts,
			struct DTGField *&attrs,
			DTGModule *&module )
{
	char* val = NULL;

	/* setting an attribute must come before the module is loaded */
	if( (parts->value[1] != 'S' && parts->value[1] != 's') &&
	    (parts->value[1] != 'D' && parts->value[1] != 'd') )
	{
	    if( !module )
	    {
	        fprintf( stderr, "No module is loaded\n" );
	        return;
	    }
	    if( !module->has_attribute_extensions() )
	    {
	        fprintf( stderr, "Module does not support attributes\n" );
	        return;
	    }
	}

	if( (parts->value[1] == 'S' || parts->value[1] == 's') )
	{
	    if( !parts->next )
	        return;
	    val = joinup( parts->next->next );
	}

	DTGAttribute *list;
	DTGField *field;
	char *tmp;
	switch( parts->value[1] )
	{
	default:
	    fprintf( stderr, "Error: Unknown subcommand\n" );
	    break;
	case 'L': case 'l':
	    list = module->dt_list_attrs();
	    printf( "Attribute(s):\n" );
	    for( struct DTGAttribute *item = list; item; item = item->next )
	        print_attribute( item );
	    delete_DTGAttribute( list );
	    break;
	case 'V': case 'v':
	    IF_NOT_OBJECT( parts->next, "No name specified" );
	    IF_NOT_OBJECT( parts->next->value, "No name specified" );
	    IF_NOT_OBJECT( parts->next->next, "No value specified" );
	    IF_NOT_OBJECT( parts->next->next->value, "No value specified" );
	    field = new_DTGField( parts->next->value, 
				parts->next->next->value );
	    tmp = module->dt_validate_attr( field );
	    if( tmp )
	    {
	        printf( "Attr [%s] with value [%s] not valid: [%s]\n",
			field->name, field->value, tmp );
	        free( tmp );
	    }
	    else
	        printf( "Attr [%s] with value [%s] validated\n",
			field->name, field->value );
	    delete_DTGField( field );
	    break;
	case 'S': case 's':
	    IF_NOT_OBJECT( parts->next, "No name specified" );
	    IF_NOT_OBJECT( parts->next->value, "No name specified" );
	    IF_NOT_OBJECT( parts->next->next, "No value specified" );
	    IF_NOT_OBJECT( parts->next->next->value, "No value specified" );

	    if( attrs )
	        attrs =
		        append_DTGField( attrs,
					 new_DTGField( parts->next->value, val )
					 );
	    else
	        attrs = new_DTGField( parts->next->value, val );
	    printf( "Added attribute pair:  %s -> %s\n", parts->next->value,
		    val );
	    break;
	case 'D': case 'd':
	    IF_NOT_OBJECT( parts->next, "No name specified" );
	    IF_NOT_OBJECT( parts->next->value, "No name specified" );
	    if( !attrs )
	    {
	        printf( "Attribute not found:  %s\n", parts->next->value );
	        break;
	    }
	    if( rm_DTGField( attrs, parts->next->value ) )
	        printf( "Deleted attribute:  %s\n", parts->next->value );
	    else
		printf( "Attribute not found:  %s\n", parts->next->value );
	    break;
	}
	if( val )
	    delete[] val;
}

void do_library_cmds( struct DTGStrList *parts, 
	const struct DTGField *attrs,
	DTGModule *&module,
	void *&dtID,
	void *&projID,
	void *&defectID )
{
	const char *tmp;
	struct DTGError *err = new_DTGError( NULL );
	switch( parts->value[1] )
	{
	default:
	    fprintf( stderr, "Error: Unknown subcommand\n" );
	    break;
	case 'E': case 'e':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    {
	        char input[256];
	        input[0] = '\0';
	        printf("DateString> ");
	        fflush(0);
	        fgets( input, 256, stdin );
	        char *fix = strrchr( input, '\n' );
	        if( fix )
	            *fix = '\0';
	        if( echo_mode ) 
	            printf( "%s\n", input );
	        struct DTGDate *date = 
			module->extract_date( input );
	        if( !date )
	            printf( "Error: Unable to extract date\n" );
	        else
	        {
	            printf( "Date: %d/%d/%d %d:%d:%d\n", 
			date->year, date->month, date->day,
			date->hour, date->minute, date->second );
	            delete_DTGDate( date );
	        }
	    }
	    break;
	case 'I': case 'i':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    IF_NOT_OBJECT( parts->next, "No year specified" );
	    IF_NOT_OBJECT( parts->next->value, "No year specified" );
	    IF_NOT_OBJECT( parts->next->next, "No month specified" );
	    IF_NOT_OBJECT( parts->next->next->value, "No month specified" );
	    IF_NOT_OBJECT( parts->next->next->next, "No day specified" );
	    IF_NOT_OBJECT( parts->next->next->next->value, 
					"No day specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next, "No hour specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next->value, 
					"No hour specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next->next, 
					"No minute specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next->next->value, 
					"No minute specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next->next->next, 
					"No second specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next->next->next->value, 
					"No second specified" );
	    {
	        struct DTGDate date;
		date.year = atoi( parts->next->value );
		date.month = atoi( parts->next->next->value );
		date.day = atoi( parts->next->next->next->value );
		date.hour = atoi( parts->next->next->next->next->value );
		date.minute = 
		    atoi( parts->next->next->next->next->next->value );
		date.second = 
		    atoi( parts->next->next->next->next->next->next->value );
	        char *date_string = module->format_date( &date );
	        if( !date_string )
	            fprintf( stderr, "Error: Unable to format date\n" );
	        else
	        {
	            printf( "DateString:[%s]\n", date_string );
	            free( date_string );
	        }
	    }
	    break;
	case 'C': case 'c':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    IF_NOT_OBJECT( parts->next, "No server specified" );
	    IF_NOT_OBJECT( parts->next->value, "No server specified" );
	    IF_NOT_OBJECT( parts->next->next, "No user specified" );
	    IF_NOT_OBJECT( parts->next->next->value, "No user specified" );
	    IF_NOT_OBJECT( parts->next->next->next, "No passwd specified" );
	    printf( "Connecting: [%s:%s:%s]\n", 
		parts->next->value,
		parts->next->next->value,
		parts->next->next->next->value );
	    dtID = module->dt_connect( parts->next->value,
					parts->next->next->value,
					parts->next->next->next->value,
					attrs,
					err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "Connected.\n" );
	    break;
	case 'L': case 'l':
	    IF_OBJECT( module, "A module is already loaded" );
	    IF_NOT_OBJECT( parts->next, "No path specified" );
	    IF_NOT_OBJECT( parts->next->value, "No path specified" );
	    printf( "Loading module: [%s]\n", parts->next->value );
	    module = new DTGModule( parts->next->value );
	    if( module->last_error && *module->last_error )
	    {
	        fprintf( stderr, "Error: %s\n", module->last_error );
	        delete module;
	        module = NULL;
	    }
	    else
	        printf( "Module loaded: %s\n", parts->next->value );
	    break;
	case 'T': case 't':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    module->test_module();
	    printf( "Module tests passed\n" );
	    break;
	case 'N': case 'n':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    tmp = module->dt_get_name( err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "dt_get_name():[%s]\n", tmp );
	    break;
	case 'V': case 'v':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    tmp = module->dt_get_module_version( err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "dt_get_module_version():[%s]\n", tmp );
	    break;
	case 'F': case 'f':
	    IF_NOT_OBJECT( module, "No module is loaded" );
	    IF_OBJECT( defectID, "defectID needs to be freed" );
	    IF_OBJECT( projID, "projID needs to be freed" );
	    IF_OBJECT( dtID, "dtID needs to be freed" );
	    delete module;
	    module = NULL;
	    printf( "Module freed\n" );
	    break;
	}
	delete_DTGError( err );
}

void do_server_cmds( struct DTGStrList *parts, 
	DTGModule *module,
	void *&dtID,
	void *&projID,
	void *&defectID )
{
	int i;
	char *tmp_free;
	const char *tmp;
	struct DTGDate *tmp_date;
	DTGStrList *list;
	struct DTGError *err = new_DTGError( NULL );
	switch( parts->value[1] )
	{
	default:
	    fprintf( stderr, "Error: Unknown subcommand\n" );
	    break;
	case 'P': case 'p':
	    IF_NOT_OBJECT( dtID, "Not connected to server"  );
	    IF_OBJECT( projID, "A project is already loaded"  );
	    IF_NOT_OBJECT( parts->next, "No project specified" );
	    IF_NOT_OBJECT( parts->next->value, "No project specified" );
	    tmp_free = join_DTGStrList( parts->next, " " );
	    printf( "Loading project: [%s]\n", tmp_free );
	    projID = module->dt_get_project( dtID, tmp_free, err );
	    free( tmp_free );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "Project loaded.\n" );
	    break;
	case 'F': case 'f':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    IF_OBJECT( defectID, "defectID needs to be freed"  );
	    IF_OBJECT( projID, "projID needs to be freed"  );
	    module->dt_free( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "Disconnected from server.\n" );
	    dtID = NULL;
	    break;
	case 'D': case 'd':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    tmp_date = module->dt_get_server_date( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "dt_server_date():[%d/%d/%d %d:%d:%d]\n", 
			tmp_date->year, tmp_date->month, tmp_date->day,
			tmp_date->hour, tmp_date->minute, tmp_date->second );
	        delete_DTGDate( tmp_date );;
	        tmp_date = NULL;
	    }
	    break;
	case 'U': case 'u':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    i = module->dt_accept_utf8( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "dt_accept_utf8():[%d]\n", i );
	    break;
	case 'O': case 'o':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    i = module->dt_server_offline( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    // Always print result even on error
	    printf( "dt_server_offline():[%d]\n", i );
	    break;
	case 'V': case 'v':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    tmp = module->dt_get_server_version( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "dt_server_version():[%s]\n", tmp );
	    break;
	case 'W': case 'w':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    tmp_free = module->dt_get_server_warnings( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else if( tmp_free )
	    {
	        printf( "dt_server_warnings():[%s]\n", tmp_free );
	        free( tmp_free );
	    }
	    else 
	        printf( "dt_server_warnings():[]\n" );

	    break;
	case 'M': case 'm':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    i = module->dt_get_message( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Message(%d): %s\n", i, err->message );
	    else
	        printf( "dt_get_message():[%i]\n", i );
	    break;
	case 'L': case 'l':
	    IF_NOT_OBJECT( dtID, "Not connected to server" );
	    list = module->dt_list_projects( dtID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "Projects:\n" );
	        for( struct DTGStrList *item = list; item; item = item->next )
	            printf(" [%s]\n", item->value );
		delete_DTGStrList( list );
	    }
	    break;
	}
	delete_DTGError( err );
}

static int set_date( struct DTGDate &date, const char *value )
{
	DTGStrList *parts = split_DTGStrList( value, '/' );
	if( !parts ||  					// yyyy
	    !parts->next || 				// mm
	    !parts->next->next || 			// dd
	    !parts->next->next->next || 		// hh
	    !parts->next->next->next->next || 		// mm
	    !parts->next->next->next->next->next ) 	// ss
	{
		delete_DTGStrList( parts );
	        return 0;
	}
	date.year = atoi( parts->value );
	date.month = atoi( parts->next->value );
	date.day = atoi( parts->next->next->value );
	date.hour = atoi( parts->next->next->next->value );
	date.minute = atoi( parts->next->next->next->next->value );
	date.second = atoi( parts->next->next->next->next->next->value );
	delete_DTGStrList( parts );
	return 1;
}

int valid_type( const char *type )
{
	if( !type )
	    return 0;
	if( !strcasecmp( type, "WORD" ) )
	    return 1;
	if( !strcasecmp( type, "DATE" ) )
	    return 1;
	if( !strcasecmp( type, "LINE" ) )
	    return 1;
	if( !strcasecmp( type, "TEXT" ) )
	    return 1;
	if( !strcasecmp( type, "SELECT" ) )
	    return 1;
	if( !strcasecmp( type, "FIX" ) )
	    return 1;
	return 0;
}

void do_project_cmds( struct DTGStrList *parts, 
	DTGModule *module,
	void *&projID,
	void *&defectID )
{
	struct DTGStrList *list = NULL;
	struct DTGFixDesc *fix = NULL;
	struct DTGFieldDesc *fields = NULL;
	struct DTGFieldDesc *filters = NULL;
	struct DTGDate date;
	struct DTGError *err = new_DTGError( NULL );
	switch( parts->value[1] )
	{
	default:
	    fprintf( stderr, "Error: Unknown subcommand\n" );
	    break;
	case 'S': case 's':
	    IF_NOT_OBJECT( projID, "No project loaded"  );
	    fields = module->proj_list_fields( projID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "Fields:\n" );
	        for( struct DTGFieldDesc *field = fields; 
			field; 
			field = field->next )
	        {
	            printf(" [%s]:[%s][%d]", field->name, field->type, 
			field->readonly );
	            if( !valid_type( field->type ) )
	                printf( " INVALID TYPE\n" );
	            else
	                printf( "\n" );
	            for( struct DTGStrList *i = field->select_values;
			i; i = i->next )
	                printf("   [%s]\n", i->value );
	        }
		delete_DTGFieldDesc( fields );
	    }
	    break;
	case 'I': case 'i':
	    IF_NOT_OBJECT( projID, "No project loaded" );

	    delete_DTGFieldDesc( filters );
	    filters = NULL;
	    while( 1 )
	    {
	        /* Read filters until ^. */
	        char input[256];
	        input[0] = '\0';
	        printf("Field Opts...> ");
	        fflush(0);
	        fgets( input, 256, stdin );
	        char *fix = strrchr( input, '\n' );
	        if( fix )
	            *fix = '\0';
	        if( echo_mode ) 
	            printf( "%s\n", input );
	        if( !input || !*input )
	        {
	             fprintf( stderr, "Error: Invalid input, end with .\n" );
	             continue;
	        }
	        if( input && *input == '.' )
	            break;
	        struct DTGStrList *p = split_DTGStrList( input, ' ' );
	        if( !p || !p->next )
	        {
	             delete_DTGStrList( p );
	             fprintf( stderr, 
			"Error: Format is 'FieldName Opt1 Opt2 ...\n" );
	             continue;
	        }
	        filters = append_DTGFieldDesc( filters, 
			new_DTGFieldDesc( p->value, "select", 1, 
					copy_DTGStrList( p->next ) ) );
	        delete_DTGStrList( p );
	    }

	    module->proj_segment_filters( projID, filters );
	    delete_DTGFieldDesc( filters );
	    filters = NULL;
	    break;
	case 'F': case 'f':
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_OBJECT( defectID, "defectID needs to be freed"  );
	    module->proj_free( projID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "Project freed.\n" );
	    projID = NULL;
	    break;
	case 'Q': case 'q':
	    IF_NOT_OBJECT( module->has_perforce_extensions(),
				"Perforce only extension" );
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_NOT_OBJECT( parts->next, "No max rows specified" );
	    IF_NOT_OBJECT( parts->next->value, "No max rows specified" );
	    IF_NOT_OBJECT( parts->next->next, "No qualification specified" );
	    IF_NOT_OBJECT( parts->next->next->value, 
				"No qualification specified" );
	    list = module->proj_find_defects( 
		projID, 
		parts->next->value ? atoi(parts->next->value) : 4, 
		parts->next->next->value, 
		err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "Defects:\n" );
	        for( struct DTGStrList *item = list; item; item = item->next )
	            printf(" [%s]\n", item->value );
		delete_DTGStrList( list );
	    }
	    break;
	case 'L': case 'l':
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_NOT_OBJECT( parts->next, "No max rows specified" );
	    IF_NOT_OBJECT( parts->next->value, "No max rows specified" );
	    IF_NOT_OBJECT( parts->next->next, "No date specified" );
	    IF_NOT_OBJECT( parts->next->next->value, "No date specified" );
	    IF_NOT_OBJECT( parts->next->next->next, "No date field specified" );
	    IF_NOT_OBJECT( parts->next->next->next->value, 
				"No date field specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next, 
				"No modified by field specified" );
	    IF_NOT_OBJECT( parts->next->next->next->next->next, 
				"No excluded user specified" );
	    if( !set_date( date, parts->next->next->value ) )
	    {
		fprintf( stderr, "Error: Invalid date\n" );
		break;
	    }
	    printf( "Since Date: %d/%d/%d %d:%d:%d\n", 
			date.year, date.month, date.day,
			date.hour, date.minute, date.second );
	    list = module->proj_list_changed_defects( 
		projID, 
		parts->next->value ? atoi(parts->next->value) : 4, 
		&date,
		parts->next->next->next->value, 
		parts->next->next->next->next->value, 
		parts->next->next->next->next->next->value, 
		err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "Defects:\n" );
	        for( struct DTGStrList *item = list; item; item = item->next )
	            printf(" [%s]\n", item->value );
		delete_DTGStrList( list );
	    }
	    break;
	case 'D': case 'd':
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_OBJECT( defectID, "A defectID is currently loaded"  );
	    IF_NOT_OBJECT( parts->next, "No defect specified" );
	    IF_NOT_OBJECT( parts->next->value, "No defect specified" );
	    defectID = 
		module->proj_get_defect( projID, parts->next->value, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "Defect loaded:[%s]\n", parts->next->value );
	    break;
	case 'C': case 'c':
	    IF_NOT_OBJECT( module->has_perforce_extensions(),
				"Perforce only extension" );
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_NOT_OBJECT( parts->next, "No defect specified" );
	    IF_NOT_OBJECT( parts->next->value, "No defect specified" );
	    list = 
		module->proj_list_fixes( projID, parts->next->value, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "Fixes:\n" );
	        for( struct DTGStrList *item = list; item; item = item->next )
	            printf(" [%s]\n", item->value );
		delete_DTGStrList( list );
	    }
	    break;
	case 'A': case 'a':
	    IF_NOT_OBJECT( module->has_perforce_extensions(),
				"Perforce only extension" );
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_NOT_OBJECT( parts->next, "No fix specified" );
	    IF_NOT_OBJECT( parts->next->value, "No fix specified" );
	    fix = module->proj_describe_fix( projID, parts->next->value, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "Fix details:\n" );
	        printf( "  Change: %s, User: %s, Stamp: %s\n", 
			fix->change, fix->user, fix->stamp );
	        printf( "  Description: %s\n", fix->desc );
	        printf( "  Files:\n" );
	        for( struct DTGStrList *item = fix->files; 
			item; 
			item = item->next )
	            printf("     %s\n", item->value );
	        delete_DTGFixDesc( fix );
	    }
	    break;
	case 'N': case 'n':
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_OBJECT( defectID, "A defectID is currently loaded"  );
	    defectID = 
		module->proj_new_defect( projID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "New defect loaded.\n" );
	    break;
	case 'R': case 'r':
	    IF_NOT_OBJECT( projID, "No project loaded" );
	    IF_NOT_OBJECT( parts->next, "Specify at least one field" );
	    IF_NOT_OBJECT( parts->next->value, "Specify at least one field" );
	    {
	        struct DTGStrList *list = NULL;
	        while( parts->next )
	        {
	            if( parts->next->value )
	                list = append_DTGStrList( list, parts->next->value );
	            parts = parts->next;
	        }
	        module->proj_referenced_fields( projID, list );
	        delete_DTGStrList( list );
	    }
	    printf( "Processed call\n" );
	    break;
	}
	delete_DTGError( err );
}

void do_defect_cmds( struct DTGStrList *parts, 
	DTGModule *module,
	void *&defectID )
{
	char *tmp_free;
	char *fieldname;
	DTGField *fields;
	struct DTGError *err = new_DTGError( NULL );
	switch( parts->value[1] )
	{
	default:
	    fprintf( stderr, "Error: Unknown subcommand\n" );
	    break;
	case 'F': case 'f':
	    IF_NOT_OBJECT( defectID, "No defect loaded"  );
	    module->defect_free( defectID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "Defect freed.\n" );
	    defectID = NULL;
	    break;
	case 'L': case 'l':
	    IF_NOT_OBJECT( defectID, "No defect loaded" );
	    fields = module->defect_get_fields( defectID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "DefectFields:\n" );
	        for( struct DTGField *item = fields; item; item = item->next )
	            printf(" [%s]:[%s]\n", item->name, item->value );
		delete_DTGField( fields );
	    }
	    break;
	case 'W': case 'w':
	    IF_NOT_OBJECT( defectID, "No defect loaded" );
	    IF_NOT_OBJECT( parts->next, "No field specified" );
	    IF_NOT_OBJECT( parts->next->value, "No field specified" );
	    IF_NOT_OBJECT( parts->next->next, "No value specified" );
	    IF_NOT_OBJECT( parts->next->next->value, "No value specified" );
	    tmp_free = join_DTGStrList( parts->next->next, " " );
	    module->defect_set_field( defectID, parts->next->value, 
		tmp_free, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	        printf( "defect_set_field(%s):[%s]\n", 
			parts->next->value, tmp_free );
	    free( tmp_free );
	    break;
	case 'R': case 'r':
	    IF_NOT_OBJECT( defectID, "No defect loaded" );
	    IF_NOT_OBJECT( parts->next, "No field specified" );
	    IF_NOT_OBJECT( parts->next->value, "No field specified" );
	    fieldname = joinup( parts->next );
	    tmp_free = 
		module->defect_get_field( defectID, fieldname, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "defect_get_field(%s):[%s]\n", 
			fieldname, tmp_free );
	        free( tmp_free );
	    }
	    delete[] fieldname;
	    break;
	case 'S': case 's':
	    IF_NOT_OBJECT( defectID, "No defect loaded" );
	    tmp_free = module->defect_save( defectID, err );
	    if( err->message )
	        fprintf( stderr, "Error(%d): %s\n", 
			err->can_continue, err->message );
	    else
	    {
	        printf( "defect_save():[%s]\n", tmp_free );
	        free( tmp_free );
	    }
	    break;
	}
	delete_DTGError( err );
}

static void nullify_items( struct DTGStrList *parts )
{
	for( struct DTGStrList *item = parts; item; item = item->next )
	    if( item->value && !strcmp( item->value, "NULL" ) )
	    {
	        free( item->value );
	        item->value = strdup( "" );
	    }
}

int main( int argc, char *argv[] )
{
	DTGStrList *parts = NULL;
	struct DTGField *attrs = NULL;
	void *dtID = NULL;
	void *projID = NULL;
	void *defectID = NULL;
	DTGModule *module = NULL;
	int done = 0;
	
	if( argc > 1 && argv[1][0] == '-' && argv[1][1] == 'V' )
	{
	    printf( "Rev. p4dtg-test/%s/%s/%s\n", ID_OS, ID_REL, ID_PATCH );
	    printf( "%s %s %s\n", ID_Y, ID_M, ID_D );
	    return 0;
	}

	if( argc > 1 && argv[1] )
	{
	    printf( "Loading module: [%s]\n", argv[1] );
	    module = new DTGModule( argv[1] );
	    if( module->last_error && *module->last_error )
	    {
	        fprintf( stderr, "Error: %s\n", module->last_error );
	        delete module;
	        module = NULL;
	    }
	    else
	        printf( "Module loaded: %s\n", argv[1] );
	}

	char input[1024];
	input[0] = '\0';

	printf("> ");
	fflush(0);
	while( !done && fgets( input, 1023, stdin ) )
	{
	    char *fix = strrchr( input, '\n' );
	    if( fix )
	        *fix = '\0';
	    if( echo_mode )
	        printf( "%s\n", input );
	    parts = smart_split_DTGStrList( input, ' ' );
	    nullify_items( parts );
	    int delay;
	    if( parts && parts->value )
	    {
	        if( strlen( parts->value ) > 3 )
	            if( !strcasecmp( parts->value, "help" ) )
	                parts->value[1] = '\0'; // truncate to H
	            else if( !strcasecmp( parts->value, "quit" ) )
	                parts->value[1] = '\0'; // truncate to Q
	            else if( !strcasecmp( parts->value, "exit" ) )
	            {
	                parts->value[0] = 'Q';  // convert to Q
	                parts->value[1] = '\0';
	            }
	            else
	                parts->value[0] = '\0'; // force an error

	        switch( parts->value[0] )
	        {
	        default:
	            fprintf(stderr, "Error: Unknown command\n");
	            break;
	        case 'E': case 'e':
	            if( parts->value[1] )
	                fprintf(stderr, "Error: Unknown command\n");
	            else if( !echo_mode )
	            {
	                printf( "E\n" );
	                echo_mode = 1;
	            }
	            else
	                echo_mode = 0;
	            break;
	        case 'H': case 'h':
	            if( parts->value[1] )
	                fprintf(stderr, "Error: Unknown command\n");
	            else
	                print_help();
	            break;
	        case 'Q': case 'q':
	            if( parts->value[1] )
	                fprintf(stderr, "Error: Unknown command\n");
	            else
	                done = 1;
	            break;
	        case 'L': case 'l':
	            do_library_cmds( parts, attrs, module, 
					dtID, projID, defectID );
	            break;
	        case 'A': case 'a':
		    do_attribute_cmds( parts, attrs, module );
	            break;
	        case 'S': case 's':
	            do_server_cmds( parts, module, dtID, projID, defectID );
	            break;
	        case 'P': case 'p':
	            do_project_cmds( parts, module, projID, defectID );
	            break;
	        case 'D': case 'd':
	            do_defect_cmds( parts, module, defectID );
	            break;
	        case 'W': case 'w':
	            if( parts->value[1] )
	            {
	                fprintf(stderr, "Error: Unknown command\n");
	                break;
	            }
	            if( parts && parts->next && parts->next->value )
	                delay = atoi( parts->next->value );
	            else
	                delay = 5;
	            printf( "Waiting %d secs\n", delay );
#ifdef _WIN32
	            Sleep( delay*1000 );
#else
	            sleep( delay );
#endif
	            break;
	        }
	        delete_DTGStrList( parts );
	    }
	    if( !done )
	    {
	        input[0] = '\0';
	        printf("> ");
	        fflush(0);
	    }
	}
	if( !module )
	    exit(0);
	struct DTGError *err = new_DTGError( NULL );
	if( attrs )
	    delete_DTGField( attrs );
	if( defectID )
	{
	    module->defect_free( defectID, err );
	    printf( "Freeing defectID\n" );
	}
	if( projID )
	{
	    module->proj_free( projID, err );
	    printf( "Freeing projID\n" );
	}
	if( dtID )
	{
	    module->dt_free( dtID, err );
	    printf( "Freeing dtID\n" );
	}
	delete module;
	printf( "Freeing module\n" );
	delete_DTGError( err );
}
