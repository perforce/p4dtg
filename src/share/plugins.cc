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
#include <string.h>
#include <DTGModule.h>
#include <DTG-interface.h>
#include "plugins.h"
extern "C" {
#include <dtg-utils.h>
}

extern struct DTGStrList *scan_dir( const char *dirname );

DTGModule *load_plugins( const char  *plugindir )
{
	DTGModule *plugins = NULL;
	DTGModule *p4plugin = NULL;
	if( !plugindir )
	    return NULL;

	struct DTGStrList *libs = scan_dir( plugindir );
	if( !libs )
	    return NULL;
	struct DTGError *err = new_DTGError( NULL );
	for( struct DTGStrList *lib = libs; lib; lib = lib->next )
	{
	    char *name = 
		new char[strlen(plugindir)+strlen(lib->value)+2];
	    sprintf( name, "%s%s%s", plugindir, DIRSEPARATOR, lib->value );
	    DTGModule *module = new DTGModule( name );
	    delete[] name;
	    if( module->last_error && *module->last_error )
	    {
                fprintf( stderr, "Error: %s\n", module->last_error );
                delete module;
	    }
	    else
            {
	        const char *tmp = module->dt_get_name( err );
	        if( err->message )
	        {
	            fprintf( stderr, "Error: %s\n", err->message );
	            delete module;
	        }
	        else
	        {
	            if( !strcmp( tmp, "Perforce Jobs" ) )
	                 p4plugin = module;
	            else
	            {
	                module->next = plugins;
	                plugins = module;
	            }
	        }
	    }
	}
	delete_DTGError( err );
	delete_DTGStrList( libs );
	if( !p4plugin )
	{
	    if( plugins )
	        delete plugins;
	    return NULL;
	}
	p4plugin->next = plugins;
	plugins = p4plugin;
	return plugins;
}

