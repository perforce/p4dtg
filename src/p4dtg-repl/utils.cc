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
#include <DTGModule.h>
extern "C" {
#include <dtg-utils.h>
}
#include "Unify.h"
#include "DataSource.h"
#include "DataMapping.h"
#include <genutils.h>
#include <Logger.h>

char *pass_filter( struct DTGFieldDesc *filters, DTGModule *mod, void *defectID )
{
	if( !filters )
	    return NULL;
	if( !mod || !defectID )
	    return mk_string( "Unknown defect or module" );
	struct DTGError *err = new_DTGError( NULL );
	char *msg = NULL;
	for( struct DTGFieldDesc *f = filters; !msg && f; f = f->next )
	{
	    char *value = 
		mod->defect_get_field( defectID, f->name, err );
	    if( !in_DTGStrList( value, f->select_values ) )
	        msg = mk_string( "Field: ", f->name, "Value: ", value );
	    free( value );
	}
	delete_DTGError( err );
	return msg;
}

/* scm_stat/dts_stat: -1 = new, 0 = unchanged, 1 = changed */

int set_field( DTGModule *mod, void *defectID, const char *field, 
		const char *new_val, const char *old_val,
		struct DTGError *&err )
{
	if( new_val )
	{
	    if( old_val )
	    {
	        if( !chomp_strcmp( new_val, old_val ) )
	            return 0;
	    }
	    else if( !*new_val )
	        return 0;
	}
	else if( !old_val || !*old_val )
	    return 0;
	// printf( "%s: [%s] -> [%s]\n", field, old_val, new_val );
	mod->defect_set_field( defectID, field, new_val, err );
	return 1;
}
