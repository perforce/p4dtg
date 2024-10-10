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

#include <windows.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <process.h>
// #include <tchar.h>
// #include <direct.h>

LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
	DWORD dwRet;
	LPTSTR lpszTemp = NULL;

	dwRet = FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_ARGUMENT_ARRAY,
			NULL,
			GetLastError(),
			LANG_NEUTRAL,
			(LPTSTR)&lpszTemp,
			0,
			NULL );

	// supplied buffer is not long enough
	if( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
	    lpszBuf[0] = '\0';
	else
	{
	    // remove cr and newline character
	    lpszTemp[lstrlen(lpszTemp)-2] = '\0';  
	    sprintf( lpszBuf, "%s (0x%x)", lpszTemp, GetLastError() );
	}

	if( lpszTemp )
	LocalFree( (HLOCAL)lpszTemp );

	return lpszBuf;
}

const char *start_service( const char *service, const char *map )
{
	SC_HANDLE schService;
	SC_HANDLE schSCManager;
	SERVICE_STATUS ssStatus;
	static char szErr[256];
	szErr[0] = '\0';
	const char *mapname[2];
	mapname[0] = map;

	schSCManager = OpenSCManager( 
				NULL,      // machine (NULL == local)
				NULL,      // database (NULL == default)
				SC_MANAGER_CONNECT ); // access required
	if( schSCManager )
	{
	    schService = OpenService( schSCManager, service, 
				SERVICE_START | SERVICE_QUERY_STATUS );

	    if( schService )
	    {
	        // try to start the service
	        if( StartService( schService, 1, mapname ) )
	        {
	            printf( "Starting %s.", service );
	            Sleep( 1000 );

	            while( QueryServiceStatus( schService, &ssStatus ) )
	            {
	                if( ssStatus.dwCurrentState == SERVICE_START_PENDING )
	                    Sleep( 1000 );
	                else
	                    break;
	            }

	            if( ssStatus.dwCurrentState != SERVICE_RUNNING )
	                GetLastErrorText( szErr,256 );
	        }
	        else
	            GetLastErrorText( szErr,256 );

	        CloseServiceHandle(schService);
	    }
	    else
	        GetLastErrorText( szErr,256 );

	    CloseServiceHandle(schSCManager);
	}
	else
	    GetLastErrorText( szErr,256 );
	return szErr;
};

const char *stop_service( const char *service )
{
	SC_HANDLE schService;
	SC_HANDLE schSCManager;
	SERVICE_STATUS ssStatus;
	static char szErr[256];
	szErr[0] = '\0';

	schSCManager = OpenSCManager( 
				NULL,      // machine (NULL == local)
				NULL,      // database (NULL == default)
				SC_MANAGER_CONNECT ); // access required
	if( schSCManager )
	{
	    schService = OpenService( schSCManager, service, 
				SERVICE_STOP | SERVICE_QUERY_STATUS );

	    if( schService )
	    {
	        // try to start the service
	        if( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
	        {
	            Sleep( 1000 );

	            while( QueryServiceStatus( schService, &ssStatus ) )
	            {
	                if( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
	                    Sleep( 1000 );
	                else
	                    break;
	            }

	            if( ssStatus.dwCurrentState != SERVICE_STOPPED )
	                GetLastErrorText( szErr,256 );
	        }
	        else
	            GetLastErrorText( szErr,256 );

	        CloseServiceHandle(schService);
	    }
	    else
	        GetLastErrorText( szErr,256 );

	    CloseServiceHandle(schSCManager);
	}
	else
	    GetLastErrorText( szErr,256 );
	return szErr;
}
