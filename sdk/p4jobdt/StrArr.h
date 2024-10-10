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

#ifndef STRARR_HEADER
#define STRARR_HEADER
/*
 * StrArr.h is a simple class which is an array of strings
 *
 */

#include <p4/vararray.h>

class StrArr {
	public:
	    StrArr() { len = 0; };
	    ~StrArr() 
	    {
	        char *item;
	        for( int i = 0; item = (char *)Get( i ); i++ )
	            delete[] item;
	    };
	    void Add( const char *item ) 
	    { 
	        char *tmp = new char[strlen(item)+1];
	        tmp[0] = '\0';
	        strcat( tmp, item );
	        elems.Put( tmp ); 
	        len++; 
	    };
	    const char *Get( int i ) 
	    { 
	        return (const char *)elems.Get( i ); 
	    };
	protected:
	    VarArray elems;
	    int len;
};

#endif
