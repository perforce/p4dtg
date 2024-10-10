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
#include <dtg-str.h>
#include <p4/i18napi.h>
#include <p4/charcvt.h>

/* 
    Convert 'from' string from encoding 'from_set' to 'to_set'
    if sub_missing, then substitute '?' for missing characters
    On error, returns NULL and sets 'err' with error message
*/
char *p4charcvt(const char *from_set,
		const char *to_set,
		const char *from,
		int from_len,
		int sub_missing,
		char *&err )
{
	char *tmp = NULL;

	if( !from_set || !to_set )
	{
	    err = cp_string( "Missing FROM/TO charset" );
	    return NULL;
	}
	if( !from )
	    return NULL;

	if( !strcmp( from_set, to_set ) )
	{
	    tmp = new char[from_len + 1];
	    memcpy( tmp, from, from_len );
	    tmp[from_len] = '\0';
	    return tmp;
	}

	CharSetCvt::CharSet fromset = CharSetCvt::Lookup( from_set );
	if( fromset == (CharSetCvt::CharSet)-1 )
	{
	    tmp = mk_string( "Invalid FROM charset: ", from_set );
	    err = tmp;
	    return NULL;
	}
	CharSetCvt::CharSet toset = CharSetCvt::Lookup( to_set );
	if( toset == (CharSetCvt::CharSet)-1 )
	{
	    tmp = mk_string( "Invalid TO charset: ", to_set );
	    err = tmp;
	    return NULL;
	}
	CharSetCvt *cvter = CharSetCvt::FindCvt( fromset, toset );
	if( !cvter )
	{
	    tmp = mk_string( "Invalid conversion from ", from_set,
					" to ", to_set );
	    err = tmp;
	    return NULL;
	}

	cvter->IgnoreBOM(); // Useful only for file content
	cvter->ResetErr();

	/*
	FastCvt(const char *, int len, int *retlen = 0 )
	    convert buffer into an managed buffer, caller must copy result
	    out before calling this again using the same converter

	FastCvtQues(const char *, int len, int *retlen = 0 )
	    convert buffer into an managed buffer, caller must copy result
	    out before calling this again using the same converter - 
	    substitute '?' for bad mappings 
	*/

	int rlen = 0;
	const char *to;
	if( sub_missing )
	    to = cvter->FastCvtQues( from, from_len, &rlen );
	else
	    to = cvter->FastCvt( from, from_len, &rlen );
	switch( cvter->LastErr() )
	{
	case CharSetCvt::NOMAPPING:
	    if( sub_missing )
	        break;
	    tmp = mk_string( "Missing character mapping" );
	    err = tmp;
	    delete cvter;
	    return NULL;
	case CharSetCvt::PARTIALCHAR:
	    tmp = mk_string( "Partial character found" );
	    err = tmp;
	    delete cvter;
	    return NULL;
	}
	
	tmp = new char[rlen + 1];
	memcpy( tmp, to, rlen );
	tmp[rlen] = '\0';

	delete cvter;

	return tmp;
}
