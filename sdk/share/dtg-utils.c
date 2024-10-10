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
#include <string.h>
#include "dtg-utils.h"

/*
struct DTGError {
	char *message;
	int can_continue;
};
*/

struct DTGError *new_DTGError( const char *msg )
{
	struct DTGError *err = 
		(struct DTGError *)malloc( sizeof(struct DTGError) );
	if( msg )
	    err->message = strdup( msg );
	else
	    err->message = NULL;
	err->can_continue = 1;
	return err;
}

void delete_DTGError( struct DTGError *err )
{
	if( !err )
	    return;
	if( err->message )
	    free( err->message );
	err->message = NULL;
	free( err );
}

void clear_DTGError( struct DTGError *err )
{
	if( !err )
	    return;
	if( err->message )
	    free( err->message );
	err->message = NULL;
	err->can_continue = 1;
}

void set_DTGError( struct DTGError *err, const char *msg )
{
	if( !err )
	    return;
	if( err->message )
	    free( err->message );
	if( msg )
	    err->message = strdup( msg );
	else
	    err->message = NULL;
}


/*
struct DTGStrList {
	char *value;
	struct DTGStrList *next;
};
*/

struct DTGStrList *new_DTGStrList( const char *txt )
{
	struct DTGStrList *item = 
		(struct DTGStrList *)malloc( sizeof(struct DTGStrList) );
	if( txt )
	    item->value = strdup( txt );
	else
	    item->value = strdup( "" );;
	item->next = NULL;
	return item;
}

void delete_DTGStrList( struct DTGStrList *list )
{
	struct DTGStrList *cur = list;
	while( cur )
	{
	    struct DTGStrList *next = cur->next;
	    cur->next = NULL;
	    if( cur->value )
	    {
	        free( cur->value );
	        cur->value = NULL;
	    }
	    free( cur );
	    cur = next;
	}
}

int in_DTGStrList( const char *item, struct DTGStrList *list )
{
	struct DTGStrList *i;
	if( !item || !list )
	    return 0;
	for( i = list; i; i = i->next )
	    if( !strcmp( i->value, item ) )
	        return 1;
	return 0;
}

struct DTGStrList *append_DTGStrList( struct DTGStrList *list, 
					const char *item )
{
	struct DTGStrList *cur;
	struct DTGStrList *next;
	if( !list )
	    return new_DTGStrList( item );

	cur = list;
	next = cur->next;
	while( next )
	{
	    cur = next;
	    next = cur->next;
	}
	cur->next = new_DTGStrList( item );
	return list;
}

struct DTGStrList *merge_DTGStrList( struct DTGStrList *list1, 
					struct DTGStrList *list2 )
{
	/* Append list2 to the end of list1 */

	struct DTGStrList *l = list1;

	if( !list1 )
	    return list2;
	if( !list2 )
	    return list1;
	while( l->next )
	    l = l->next;
	l->next = list2;
	return list1;
}

struct DTGStrList *split_DTGStrList( const char *txt, char sep )
{
	char *tmp;
	char *str;
	struct DTGStrList *list;
	int i;
	if( !txt )
	    return NULL;

	tmp = strdup( txt );
	str = tmp;
	list = NULL;
	while( *tmp )
	{
	    for( i=0; tmp[i] != sep && tmp[i]; i++ )
	        ;
	    if( tmp[i] )
	    {
	        tmp[i] = '\0';
	        list = append_DTGStrList( list, tmp );
	        tmp = &tmp[i+1];
	    }
	    else
	    {
	        list = append_DTGStrList( list, tmp );
	        tmp = &tmp[i];
	    }
	}
	free( str );
	return list;
}

struct DTGStrList *smart_split_DTGStrList( const char *txt, char sep )
{
	char *tmp;
	char *str;
	struct DTGStrList *list;
	int i;
	if( !txt )
	    return NULL;
	if( sep == '"' )
	    return split_DTGStrList( txt, sep );
	if( sep == '\'' )
	    return split_DTGStrList( txt, sep );

	tmp = strdup( txt );
	str = tmp;
	list = NULL;
	while( *tmp )
	{
	    if( *tmp == '"' )
	    {
	        tmp = &tmp[1];
	        for( i=0; tmp[i] != '"' && tmp[i]; i++ )
	            ;
	        if( tmp[i] == '"' )
	            tmp[i++] = '\0';
	    }
	    else if( *tmp == '\'' )
	    {
	        tmp = &tmp[1];
	        for( i=0; tmp[i] != '\'' && tmp[i]; i++ )
	            ;
	        if( tmp[i] == '\'' )
	            tmp[i++] = '\0';
	    }
	    else
	        for( i=0; tmp[i] != sep && tmp[i]; i++ )
	            ;
	    if( tmp[i] )
	    {
	        tmp[i] = '\0';
	        list = append_DTGStrList( list, tmp );
	        tmp = &tmp[i+1];
	    }
	    else
	    {
	        list = append_DTGStrList( list, tmp );
	        tmp = &tmp[i];
	    }
	}
	free( str );
	return list;
}

char *join_DTGStrList( struct DTGStrList *list, const char *sep )
{
	int sep_len;
	int len = 0;
	struct DTGStrList *i;
	char *tmp;
	if( !list )
	    return NULL;
	if( !sep )
	{
	    sep = "";
	    sep_len = 0;
	}
	else
	    sep_len = strlen( sep );
	for( i = list; i; i = i->next )
	    len += strlen( i->value ) + sep_len;
	tmp = (char *)malloc( sizeof(char)*(len + 1) );
	tmp[0] = '\0';
	for( i = list; i; i = i->next )
	{
	    strcat( tmp, i->value );
	    if( i->next )
	        strcat( tmp, sep );
	}
	return tmp;
}

struct DTGStrList *purge_DTGStrList( struct DTGStrList *list )
{
	struct DTGStrList *result = NULL;
	struct DTGStrList *cur;
	if( !list )
	    return NULL;

	for( cur = list; cur; cur = cur->next )
	    if( cur->value && *cur->value )
	        result = append_DTGStrList( result, cur->value );
	delete_DTGStrList( list );
	return result;
}

struct DTGStrList *remove_item_DTGStrList( struct DTGStrList *list, 
					const char *item )
{
	struct DTGStrList *tmp = list;
	if( !list || !item )
	    return list;
	if( !strcmp( list->value, item ) )
	{
	    list = list->next;
	    tmp->next = NULL;
	    delete_DTGStrList( tmp );
	    return list;
	}
	while( tmp->next )
	{
	    if( !strcmp( tmp->next->value, item ) )
	    {
	        struct DTGStrList *del = tmp->next;
	        tmp->next = tmp->next->next;
	        del->next = NULL;
	        delete_DTGStrList( del );
	        return list;
	    }
	    tmp = tmp->next;
	}
	return list;
}

struct DTGStrList *remove_DTGStrList( struct DTGStrList *base, 
					struct DTGStrList *rem )
{
	struct DTGStrList *tmp = copy_DTGStrList( base );
	struct DTGStrList *i;
	for( i = rem; i; i = i->next )
	    tmp = remove_item_DTGStrList( tmp, i->value );
	return tmp;
}


struct DTGStrList *copy_DTGStrList( const struct DTGStrList *list )
{
	struct DTGStrList *dup = NULL;
	const struct DTGStrList *tmp;
	for( tmp = list; tmp; tmp = tmp->next )
	    dup = append_DTGStrList( dup, tmp->value );
	return dup;
}

/*
struct DTGFieldDesc {
	char *name;
	char *type; // word, date, line, text, select
	int readonly;
	struct DTGStrList *select_values;
	struct DTGFieldDesc *next;
};
*/

struct DTGFieldDesc *copy_DTGFieldDesc( const struct DTGFieldDesc *list )
{
	struct DTGFieldDesc *dup = NULL;
	const struct DTGFieldDesc *tmp;
	for( tmp = list; tmp; tmp = tmp->next )
	{
	    struct DTGStrList *opts = copy_DTGStrList( tmp->select_values );
	    dup = append_DTGFieldDesc( dup, 
		new_DTGFieldDesc( tmp->name, tmp->type, tmp->readonly, opts ) );
	}
	return dup;
}

struct DTGFieldDesc *new_DTGFieldDesc( const char *name,
				     const char *type,
				     int readonly,
				     struct DTGStrList *selections )
{
	struct DTGFieldDesc *item = 
		(struct DTGFieldDesc *)malloc( sizeof(struct DTGFieldDesc ) );
	if( name )
	    item->name = strdup( name );
	else
	    item->name = NULL;
	if( type )
	    item->type = strdup( type );
	else
	    item->type = NULL;
	item->readonly = readonly;
	item->select_values = selections;
	item->next = NULL;
	return item;
}

void delete_DTGFieldDesc( struct DTGFieldDesc *list )
{
	struct DTGFieldDesc *cur = list;
	while( cur )
	{
	    struct DTGFieldDesc *next = cur->next;
	    cur->next = NULL;
	    if( cur->name )
	    {
	        free( cur->name );
	        cur->name = NULL;
	    }
	    if( cur->type )
	    {
	        free( cur->type );
	        cur->type = NULL;
	    }
	    delete_DTGStrList( cur->select_values );
	    cur->select_values = NULL;
	    free( cur );
	    cur = next;
	}
}

struct DTGFieldDesc *append_DTGFieldDesc( struct DTGFieldDesc *list,
					struct DTGFieldDesc *item )
{
	struct DTGFieldDesc *cur = list;
	struct DTGFieldDesc *next;
	if( !list )
	    return item;

	next = cur->next;
	while( next )
	{
	    cur = next;
	    next = cur->next;
	}
	cur->next = item;
	return list;
}

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
				int hour, int min, int sec )
{
	struct DTGDate *item = (struct DTGDate *)
					malloc( sizeof(struct DTGDate) );
	item->year = year;
	item->month = month;
	item->day = day;
	item->hour = hour;
	item->minute = min;
	item->second = sec;
	return item;
}

struct DTGDate *copy_DTGDate( const struct DTGDate *obj )
{
	if( !obj )
	    return NULL;
	return new_DTGDate( obj->year, obj->month, obj->day,
			obj->hour, obj->minute, obj->second );
}

void set_DTGDate( struct DTGDate *item, struct DTGDate *rev )
{
	item->year = rev->year;
	item->month = rev->month;
	item->day = rev->day;
	item->hour = rev->hour;
	item->minute = rev->minute;
	item->second = rev->second;
}

void delete_DTGDate( struct DTGDate *item )
{
	if( item )
	    free( item );
}

/***********************************
  Return which argument is the latest
  NULL = beginning of time
   Return -1 if d1 > d2
           0 if d1 = d2
           1 if d1 < d2
************************************/
int compare_DTGDate( struct DTGDate *d1, struct DTGDate *d2 )
{
	if( !d1 )
	    if( !d2 )
	        return 0;
	    else
	        return 1;
	if( !d2 )
	    return -1;
	if( d1->year != d2->year )
	    if( d1->year > d2->year )
	        return -1;
	    else
	        return 1;
	if( d1->month != d2->month )
	    if( d1->month > d2->month )
	        return -1;
	    else
	        return 1;
	if( d1->day != d2->day )
	    if( d1->day > d2->day )
	        return -1;
	    else
	        return 1;
	if( d1->hour != d2->hour )
	    if( d1->hour > d2->hour )
	        return -1;
	    else
	        return 1;
	if( d1->minute != d2->minute )
	    if( d1->minute > d2->minute )
	        return -1;
	    else
	        return 1;
	if( d1->second != d2->second )
	    if( d1->second > d2->second )
	        return -1;
	    else
	        return 1;
	return 0;
}


/*
struct DTGField {
	char *name;
	char *value;
	struct DTGField *next;
};
*/

struct DTGField *new_DTGField( const char *name,
				const char *value )
{
	struct DTGField *item = (struct DTGField *)
					malloc( sizeof(struct DTGField) );
	if( name )
	    item->name = strdup( name );
	else
	    item->name = NULL;
	if( value )
	    item->value = strdup( value );
	else
	    item->value = NULL;
	item->next = NULL;
	return item;
}

void delete_DTGField( struct DTGField *list )
{
	struct DTGField *cur = list;
	while( cur )
	{
	    struct DTGField *next = cur->next;
	    cur->next = NULL;
	    if( cur->name )
	    {
	        free( cur->name );
	        cur->name = NULL;
	    }
	    if( cur->value )
	    {
	        free( cur->value );
	        cur->value = NULL;
	    }
	    free( cur );
	    cur = next;
	}
}

struct DTGField *append_DTGField( struct DTGField *list,
				struct DTGField *item )
{
	struct DTGField *cur = list;
	struct DTGField *next;
	if( !list )
	    return item;

	next = cur->next;
	while( next )
	{
	    cur = next;
	    next = cur->next;
	}
	cur->next = item;
	return list;
}

struct DTGField *copy_DTGField( const struct DTGField *list )
{
	struct DTGField *dup = NULL;
	const struct DTGField *tmp;
	for( tmp = list; tmp; tmp = tmp->next )
	    dup = append_DTGField( dup, new_DTGField( tmp->name, tmp->value ) );
	return dup;
}


struct DTGFixDesc *new_DTGFixDesc()
{
	struct DTGFixDesc *item = 
		(struct DTGFixDesc *)malloc( sizeof(struct DTGFixDesc) );
	item->change = NULL;
	item->user = NULL;
	item->stamp = NULL;
	item->desc = NULL;
	item->files = NULL;
	return item;
};

struct DTGFixDesc *copy_DTGFixDesc( const struct DTGFixDesc *obj )
{
	struct DTGFixDesc *item = new_DTGFixDesc();

	if( !obj )
	{
	    delete_DTGFixDesc( item );
	    return NULL;
	}

	if( obj->change )
	    item->change = strdup( obj->change );
	if( obj->user )
	    item->user = strdup( obj->user );
	if( obj->stamp )
	    item->stamp = strdup( obj->stamp );
	if( obj->desc )
	    item->desc = strdup( obj->desc );
	if( obj->files )
	{
	    struct DTGStrList *dup = NULL;
	    const struct DTGStrList *tmp;
	    for( tmp = obj->files; tmp; tmp = tmp->next )
	    {
	        dup = item->files;
	        item->files = new_DTGStrList( tmp->value );
	        item->files->next = dup;
	    }
	}
	return item;
};

void delete_DTGFixDesc( struct DTGFixDesc *item )
{
	if( !item )
	    return;

	if( item->change )
	    free( item->change );
	if( item->user )
	    free( item->user );
	if( item->stamp )
	    free( item->stamp );
	if( item->desc )
	    free( item->desc );
	delete_DTGStrList( item->files );
	item->change = NULL;
	item->user = NULL;
	item->stamp = NULL;
	item->desc = NULL;
	item->files = NULL;
	free( item );
};

/*
struct DTGAttribute {
	char *name;	// Key, will be the name field in DTGField struct
	char *label;	// Displayed in UI
	char *desc;	// Displayed in UI
	char *def;	// Default, May be null
	int required;	// 0:Not required, o/w:Required
	struct DTGAtribute *next;
};
*/

struct DTGAttribute *copy_DTGAttribute( const struct DTGAttribute *list )
{
	struct DTGAttribute *dup = NULL;
	const struct DTGAttribute *tmp;
	for( tmp = list; tmp; tmp = tmp->next )
	{
	    dup = append_DTGAttribute( dup, 
		new_DTGAttribute( tmp->name, tmp->label, 
				tmp->desc, tmp->def, tmp->required ) );
	}
	return dup;
}

struct DTGAttribute *new_DTGAttribute( const char *name,
				     const char *label,
				     const char *desc,
				     const char *def,
				     int required )
{
	struct DTGAttribute *item = 
		(struct DTGAttribute *)malloc( sizeof(struct DTGAttribute ) );
	if( name )
	    item->name = strdup( name );
	else
	    item->name = NULL;
	if( label )
	    item->label = strdup( label );
	else
	    item->label = NULL;
	if( desc )
	    item->desc = strdup( desc );
	else
	    item->desc = NULL;
	if( def )
	    item->def = strdup( def );
	else
	    item->def = NULL;
	item->required = required;
	item->next = NULL;
	return item;
}

void delete_DTGAttribute( struct DTGAttribute *list )
{
	struct DTGAttribute *cur = list;
	while( cur )
	{
	    struct DTGAttribute *next = cur->next;
	    cur->next = NULL;
	    if( cur->name )
	    {
	        free( cur->name );
	        cur->name = NULL;
	    }
	    if( cur->label )
	    {
	        free( cur->label );
	        cur->label = NULL;
	    }
	    if( cur->desc )
	    {
	        free( cur->desc );
	        cur->desc = NULL;
	    }
	    if( cur->def )
	    {
	        free( cur->def );
	        cur->def = NULL;
	    }
	    free( cur );
	    cur = next;
	}
}

struct DTGAttribute *append_DTGAttribute( struct DTGAttribute *list,
					struct DTGAttribute *item )
{
	struct DTGAttribute *cur = list;
	struct DTGAttribute *next;
	if( !list )
	    return item;

	next = cur->next;
	while( next )
	{
	    cur = next;
	    next = cur->next;
	}
	cur->next = item;
	return list;
}

