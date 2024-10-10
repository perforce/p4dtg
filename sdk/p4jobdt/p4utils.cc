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

#include "MyDTS.h"
#include <p4/strtable.h>
extern "C" {
#include <dtg-utils.h>
}

static void add_to_dict( StrBufDict &dict, char *in )
{
	int i;
	for( i = 0; in[i] != ' ' && in[i]; i++ );
	if( in[i] == ' ' )
	{
	    in[i] = '\0';
	    dict.SetVar( in, &in[i+1] );
	}
}

struct DTGFieldDesc *process_jobspec( StrDict *dict )
{
	StrBufDict fields;
	StrBufDict values;
	StrBufDict presets;

	StrRef var, val;
	for( int i = 0; dict->GetVar( i, var, val ); i++ )
	{
	    char *txt = var.Text();
	    int j;
	    char *tmp = val.Text();
	    switch( *txt )
	    {
	    case 'F': // Fields[0-9]
	        for( j = 0; tmp[j] != ' ' && tmp[j]; j++ );
	        if( tmp[j] == ' ' )
	        {
	            tmp[j] = '\0';
	            add_to_dict( fields, &tmp[j+1] );
	        }
	        break;
	    case 'V': // Values[0-9]
	        add_to_dict( values, tmp );
	        break;
	    case 'P': // Presets[0-9]
	        add_to_dict( presets, tmp );
	        break;
	    case 'C': // Comments
	        break;
	    default:
	        break;
	    }
	}

	DTGFieldDesc *jobfields = NULL;
	for( int i = 0; fields.GetVar( i, var, val ); i++ )
	{
	    DTGStrList *parts = split_DTGStrList( val.Text(), ' ' );
	    int ro;
	    if( !strcmp( parts->next->next->value, "always" ) )
	    {
	        ro = 1;
	        StrPtr *defval = presets.GetVar( var.Text() );
	        if( defval && !strcmp( defval->Text(), "$now") )
	            ro = 2;
	        if( defval && !strcmp( defval->Text(), "$user") )
	            ro = 3;
	    }
	    else if( !strcmp( parts->next->next->value, "once" ) )
	        ro = 1;
	    else
	        ro = 0;
	    if( !strcmp( var.Text(), "Job" ) )
	        ro = 4;

	    DTGFieldDesc *item = new_DTGFieldDesc( 
	        var.Text(), 
	        strcasecmp( parts->value, "BULK" ) ? parts->value : "text", 
	        ro,
	        NULL );
	    delete_DTGStrList( parts );
	    if( !strcmp( item->type, "select" ) )
	    {
	        StrPtr *vals = values.GetVar( item->name );
	        if( vals )
	            item->select_values = 
			split_DTGStrList( vals->Text(), '/' ); 
	    }
	    jobfields = append_DTGFieldDesc( jobfields, item );
	}

	return jobfields;
}
