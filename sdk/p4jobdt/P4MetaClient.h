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

#ifndef P4METACLIENT_HEADER
#define P4METACLIENT_HEADER
/*
 * P4MetaClient.h is a simple interface for accessing p4
 *
 */

#include <p4/clientapi.h>

class StrArr;
class StrBufDict;

struct DTGFixDesc;

class P4MetaClient : public ClientUser {
	public:
	    P4MetaClient();
	    virtual ~P4MetaClient();
	    virtual void OutputText( const char *data, int length );
	    virtual void OutputInfo( char level, const char *data );
	    virtual void OutputStat( StrDict *dict );
	    virtual void InputData( StrBuf *buf, Error *err );
	    virtual void HandleError( Error *err );
	    virtual void OutputError( const char *err ); // For broken servers

	    StrBuf *text_results;
	    StrArr *info_results;
	    StrBufDict *stat_results;
	    StrBuf *err_results;

	    struct DTGFixDesc *fix;

	    StrBuf *data_set;

	    void print_data();
	    void clear_results();
};

#endif
