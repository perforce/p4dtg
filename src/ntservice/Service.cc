/*-----------------------------------------------------------------------------
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 1993 - 2000.  Microsoft Corporation.  All rights reserved.

MODULE:   service.c

PURPOSE:  Implements functions required by all Windows NT services

FUNCTIONS:
  main(int argc, char **argv);
  service_ctrl(DWORD dwCtrlCode);
  service_main(DWORD dwArgc, LPTSTR *lpszArgv);
  CmdInstallService();
  CmdRemoveService();
  GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );

---------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <direct.h>
#include <sys/stat.h>

extern "C" {
#include <dtg-utils.h>
}

#include "Service.h"

// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
char                   szErr[256];

// internal function prototypes
void WINAPI service_ctrl( DWORD dwCtrlCode );
void WINAPI service_main( DWORD dwArgc, LPTSTR *lpszArgv );
void CmdInstallService( int argc, char **argv );
void CmdRemoveService( int argc, char **argv );
void CmdRemoveAllServices();
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );

/* Used to convey the args to the StartService function */
int global_argc = 0;
char **global_argv = NULL;

/* Used to set the service identification based on the map */
char SZSERVICENAME[128] = MYSERVICENAME;
char SZSERVICEDISPLAYNAME[128] = MYSERVICEDISPLAYNAME;

int real_main( int argc, char **argv )
{
	if( ( argc > 1 ) && ( ( *argv[1] == '-' ) || ( *argv[1] == '/' ) ) )
	{
	    if( !_stricmp( "install", argv[1]+1 ) )
	    {
	        _snprintf( SZSERVICENAME, 120, "%s-%s", 
			MYSERVICENAME, argv[2] );
	        _snprintf( SZSERVICEDISPLAYNAME, 120, "%s: %s", 
			MYSERVICEDISPLAYNAME, argv[2] );
	        CmdInstallService( argc - 2, &argv[2] );
	    }
	    else if( !_stricmp( "remove", argv[1]+1 ) )
	    {
	        _snprintf( SZSERVICENAME, 120, "%s-%s", 
			MYSERVICENAME, argv[2] );
	        _snprintf( SZSERVICEDISPLAYNAME, 120, "%s: %s", 
			MYSERVICEDISPLAYNAME, argv[2] );
	        CmdRemoveService( argc - 2, &argv[2] );
	    }
	    else if( !_stricmp( "remove_all", argv[1]+1 ) )
	    {
	        _snprintf( SZSERVICENAME, 120, "%s-%s", 
			MYSERVICENAME, argv[2] );
	        _snprintf( SZSERVICEDISPLAYNAME, 120, "%s: %s", 
			MYSERVICEDISPLAYNAME, argv[2] );
	        CmdRemoveAllServices();
	    }
	    else
	        goto dispatch;
	    return 0;
	}

	// if it doesn't match any of the above parameters
	// the service control manager may be starting the service
	// so we must call StartServiceCtrlDispatcher
dispatch:

	global_argc = argc;
	global_argv = argv;
	SERVICE_TABLE_ENTRY dispatchTable[] = {
	    { SZSERVICENAME, (LPSERVICE_MAIN_FUNCTION)service_main },
	    { NULL, NULL }
	};

	if( !StartServiceCtrlDispatcher( dispatchTable ) )
	    AddToMessageLog( "StartServiceCtrlDispatcher failed." );
	return 0;
}

//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.

void WINAPI service_main( DWORD dwArgc, LPTSTR *lpszArgv )
{
	// register our service control handler:
	sshStatusHandle = RegisterServiceCtrlHandler( SZSERVICENAME, 
							service_ctrl);

	if( !sshStatusHandle )
	    goto cleanup;

	// SERVICE_STATUS members that don't change in example
	ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ssStatus.dwServiceSpecificExitCode = 0;

	// report the status to the service control manager.
	if( !ReportStatusToSCMgr( SERVICE_START_PENDING, // service state
				NO_ERROR,                // exit code
				3000 ) )                 // wait hint
	    goto cleanup;

	ServiceStart( dwArgc, lpszArgv );

cleanup:
	// try to report the stopped status to the service control manager.
	if( sshStatusHandle )
	    (void)ReportStatusToSCMgr( SERVICE_STOPPED, NO_ERROR, 0 );

	return;
}

//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.

void WINAPI service_ctrl( DWORD dwCtrlCode )
{
	// Handle the requested control code.
	switch( dwCtrlCode )
	{
	// case SERVICE_CONTROL_PRESHUTDOWN: // Not supported by msvc2003
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
	    // Stop the service.
	    //
	    // SERVICE_STOP_PENDING should be reported before
	    // setting the Stop Event - hServerStopEvent - in
	    // ServiceStop().  This avoids a race condition
	    // which may result in a 1053 - The Service did not respond...
	    // error.
	    ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
	    ServiceStop();
	    return;

	case SERVICE_CONTROL_INTERROGATE:
	    // Update the service status.
	    break;

	default:
	    // invalid control code
	    break;
	}

	ReportStatusToSCMgr( ssStatus.dwCurrentState, NO_ERROR, 0 );
}

//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint

BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;
	BOOL fResult = TRUE;

	if( dwCurrentState == SERVICE_START_PENDING )
	    ssStatus.dwControlsAccepted = 0;
	else
	    ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
					SERVICE_ACCEPT_SHUTDOWN;
		// SERVICE_ACCEPT_PRESHUTDOWN;  // Not supported by msvc2003

	ssStatus.dwCurrentState = dwCurrentState;
	ssStatus.dwWin32ExitCode = dwWin32ExitCode;
	ssStatus.dwWaitHint = dwWaitHint;

	if( ( dwCurrentState == SERVICE_RUNNING ) ||
	    ( dwCurrentState == SERVICE_STOPPED ) )
	    ssStatus.dwCheckPoint = 0;
	else
	    ssStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the service control manager.
	if( !( fResult = SetServiceStatus( sshStatusHandle, &ssStatus ) ) )
	    AddToMessageLog( "SetServiceStatus" );
	return fResult;
}

//  PURPOSE: Allows any thread to log an error message

void AddToMessageLog( LPTSTR lpszMsg )
{
	char szMsg [(sizeof(SZSERVICENAME) / sizeof(char)) + 100];
	HANDLE  hEventSource;
	LPCTSTR  lpszStrings[2];

	dwErr = GetLastError();

	// Use event logging to log the error.
	hEventSource = RegisterEventSource( NULL, SZSERVICENAME );

	_stprintf( szMsg, "%s error: %d", SZSERVICENAME, dwErr );
	lpszStrings[0] = szMsg;
	lpszStrings[1] = lpszMsg;

	if( hEventSource )
	{
	    ReportEvent( hEventSource, // handle of event source
			EVENTLOG_ERROR_TYPE,  // event type
			0,                    // event category
			0,                    // event ID
			NULL,                 // current user's SID
			2,                    // strings in lpszStrings
			0,                    // no bytes of raw data
			lpszStrings,          // array of error strings
			NULL );               // no raw data

	    (void)DeregisterEventSource( hEventSource );
	}
}

void CmdInstallService( int argc, char **argv )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;

	char szPath[512];

	if( !GetModuleFileName( NULL, szPath, 512 ) )
	{
	    _tprintf( "Unable to install %s - %s\n", 
			SZSERVICEDISPLAYNAME, 
			GetLastErrorText( szErr, 256 ) );
	    return;
	}
	
	char rootPath[512];
	rootPath[0] = '\0';
	strcat( rootPath, szPath );
	int j;
	for( j = strlen( rootPath ) - 1; 
		j > 0 && rootPath[j] != '\\'; 
		j-- );
	if( rootPath[j] == '\\' )
	    rootPath[j] = '\0';
	_chdir( rootPath );

	for( int i = 0; i < argc; i++ )
	{
	    strcat( szPath, " " );
	    strcat( szPath, argv[i] );
	}

	char tmp_file[512];
	_snprintf( tmp_file, 510, "%s\\config\\map-%s.xml", rootPath, argv[0] );
	struct stat buf;
	if( stat( tmp_file, &buf ) )
	{
	    // _tprintf( "Associated map file does not exist - %s\n", tmp_file );
	    return;
	}
	_snprintf( tmp_file, 510, "%s\\repl\\run-%s", rootPath, argv[0] );
	if( !stat( tmp_file, &buf ) )
	{
	    // _tprintf( "Replication is currently running - %s\n", tmp_file );
	    return;
	}
	_snprintf( tmp_file, 510, "%s\\config\\svc-%s", rootPath, argv[0] );
	if( !stat( tmp_file, &buf ) )
	{
	    // _tprintf( "Service file already exists - %s\n", tmp_file );
	    return;
	}

	schSCManager = OpenSCManager( 
		NULL,                     // machine (NULL == local)
		NULL,                     // database (NULL == default)
		SC_MANAGER_CONNECT | 
		SC_MANAGER_CREATE_SERVICE // access required
	);
	if( schSCManager )
	{
	    schService = CreateService(
			schSCManager,               // SCManager database
			SZSERVICENAME,        // name of service
			SZSERVICEDISPLAYNAME, // name to display
			SERVICE_QUERY_STATUS,       // desired access
			SERVICE_WIN32_OWN_PROCESS,  // service type
			SERVICE_DEMAND_START,       // start type
			SERVICE_ERROR_NORMAL,       // error control type
			szPath,                     // service's binary
			NULL,                       // no load ordering group
			NULL,                       // no tag identifier
			SZDEPENDENCIES,       // dependencies
			NULL,                       // LocalSystem account
			NULL );                     // no password

	    if( schService )
	    {
	        // _tprintf( "%s installed.\n", SZSERVICEDISPLAYNAME );
	        FILE *fd = fopen( tmp_file, "a+" );
	        if( fd )
	            fclose( fd );
	        CloseServiceHandle( schService );
	    }
	    // else
	        // _tprintf( "CreateService failed - %s\n", 
				// GetLastErrorText( szErr, 256 ) );

	    CloseServiceHandle( schSCManager );
	}
	// else
	    // _tprintf( "OpenSCManager failed - %s\n", 
			// GetLastErrorText( szErr,256 ) );
}

extern struct DTGStrList *scan_dir( const char *path );

void CmdRemoveAllServices()
{
	char rootPath[512];

	if( !GetModuleFileName( NULL, rootPath, 512 ) )
	{
	    _tprintf( "Unable to remove all - %s\n", 
			GetLastErrorText( szErr, 256 ) );
	    return;
	}
	int j;
	for( j = strlen( rootPath ) - 1; 
		j > 0 && rootPath[j] != '\\'; 
		j-- );
	if( rootPath[j] == '\\' )
	    rootPath[j] = '\0';
	strcat( rootPath, "\\config" );
	struct DTGStrList *files = scan_dir( rootPath );
	char *args[1];
	for( struct DTGStrList *f = files; f; f = f->next )
	{
	    if( !strncmp( f->value, "svc-", 4 ) )
	    {
	        args[0] = &(f->value[4]);
	        _snprintf( SZSERVICENAME, 120, "%s-%s", 
			MYSERVICENAME, &(f->value[4]) );
	        _snprintf( SZSERVICEDISPLAYNAME, 120, "%s: %s", 
			MYSERVICEDISPLAYNAME, &(f->value[4]) );
	        CmdRemoveService( 1, args );
	    }
	}
	delete_DTGStrList( files );
}

void CmdRemoveService( int argc, char **argv )
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;

	char rootPath[512];

	if( !GetModuleFileName( NULL, rootPath, 512 ) )
	{
	    // _tprintf( "Unable to remove %s - %s\n", 
			// SZSERVICEDISPLAYNAME, 
			// GetLastErrorText( szErr, 256 ) );
	    return;
	}
	int j;
	for( j = strlen( rootPath ) - 1; 
		j > 0 && rootPath[j] != '\\'; 
		j-- );
	if( rootPath[j] == '\\' )
	    rootPath[j] = '\0';
	_chdir( rootPath );
	char tmp_file[512];
	_snprintf( tmp_file, 510, "%s\\config\\svc-%s", rootPath, argv[0] );

	schSCManager = OpenSCManager( NULL,      // machine (NULL == local)
					NULL,    // database (NULL == default)
					SC_MANAGER_CONNECT   // access required
					);
	if( schSCManager )
	{
	    schService = OpenService( schSCManager, SZSERVICENAME, 
				DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS );

	    if( schService )
	    {
	        // try to stop the service
	        if( ControlService( schService, 
					SERVICE_CONTROL_STOP, &ssStatus ) )
	        {
	            _tprintf( "Stopping %s.", SZSERVICEDISPLAYNAME );
	            Sleep( 1000 );

	            while( QueryServiceStatus( schService, &ssStatus ) )
	            {
	                if( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
	                {
	                    // _tprintf( "." );
	                    Sleep( 1000 );
	                }
	                else
	                    break;
	            }

	            // if( ssStatus.dwCurrentState == SERVICE_STOPPED )
	                // _tprintf( "\n%s stopped.\n", 
					// SZSERVICEDISPLAYNAME );
	            // else
	                // _tprintf( "\n%s failed to stop.\n", 
					// SZSERVICEDISPLAYNAME );
	        }

	        // now remove the service
	        if( DeleteService(schService) )
	        {
	            unlink( tmp_file );
	            // _tprintf( "%s removed.\n", SZSERVICEDISPLAYNAME );
	        }
	        // else
	            // _tprintf( "DeleteService failed - %s\n", 
					// GetLastErrorText( szErr,256 ) );

	        CloseServiceHandle(schService);
	    }
	    // else
	        // _tprintf( "OpenService failed - %s\n", 
					// GetLastErrorText( szErr,256 ) );

	    CloseServiceHandle(schSCManager);
	}
	// else
	    // _tprintf( "OpenSCManager failed - %s\n", 
					// GetLastErrorText( szErr,256 ) );
}

//  PURPOSE: copies error message text to string

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
	    _stprintf( lpszBuf, "%s (0x%x)", lpszTemp, GetLastError() );
	}

	if( lpszTemp )
	LocalFree( (HLOCAL)lpszTemp );

	return lpszBuf;
}
