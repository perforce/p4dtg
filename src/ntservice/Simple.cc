/*--------------------------------------------------------------------------
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 1993 - 2000.  Microsoft Corporation.  All rights reserved.]

MODULE:   simple.c

PURPOSE:  Implements the body of the service.
          Creates a socket and calls Bind.

FUNCTIONS:
          ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
          ServiceStop( );

COMMENTS: The functions implemented in simple.c are
          prototyped in service.h
--------------------------------------------------------------------------*/

#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <direct.h>
#include "Service.h"
#include <QProcess>

extern int global_argc;
extern char **global_argv;

// this event is signalled when the
// service should end

HANDLE  hServerStopEvent = NULL;

//    The service stops when hServerStopEvent is signalled.

void ServiceStart( DWORD dwArgc, LPTSTR *lpszArgv )
{
	HANDLE hEvents[1] = {NULL};
	DWORD  dwWait;
	QStringList args;

	/* Figure out correct path */
	char szPath[512];
	szPath[0] = '\0';
	if( GetModuleFileName( NULL, szPath, 512 ) )
	{
	    int j;
	    for( j = strlen( szPath ) - 1; j > 0 && szPath[j] != '\\'; j-- );
	    if( szPath[j] == '\\' )
	        szPath[j] = '\0';
	    _chdir( szPath );
	}

	// Service initialization
	// report the status to the service control manager.

	if( !ReportStatusToSCMgr( SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                 // wait hint
	    goto cleanup;

	// create the event object. The control handler function signals
	// this event when it receives the "stop" control code.

	hServerStopEvent = CreateEvent( NULL,    // no security attributes
				TRUE,    // manual reset event
				FALSE,   // not-signalled
				NULL );   // no name

	if( !hServerStopEvent )
	    goto cleanup;

	hEvents[0] = hServerStopEvent;

	// report the status to the service control manager.

	if( !ReportStatusToSCMgr( SERVICE_START_PENDING, // service state
				NO_ERROR,              // exit code
				3000) )                 // wait hint
	    goto cleanup;

	/* Start Application */
	FILE *fd;
#ifdef DEBUGGING
	fd = fopen( "abc123.txt", "a+" );
	if( !fd )
	{
	    char szMsg[255];
	    _stprintf( szMsg, "failed to open file" );
	    AddToMessageLog( szMsg );
	    goto cleanup;
	}
	fprintf( fd, "Start:[%s]\n", szPath );
	for( int i = 0; i < global_argc; i++ )
	{
	    fprintf( fd, "Start: %d:[%s]\n", i, global_argv[i] );
	}
	fprintf( fd, "Invoke: %s\\p4dtg-repl.exe %s %s\n", 
		szPath, global_argv[1], szPath );
#endif

	args << QString( global_argv[1] );
	args << QString( szPath );
	if( QProcess::startDetached( 
		QString( szPath ) + QString( "\\p4dtg-repl.exe" ), args ) )
	{
#ifdef DEBUGGING
	    fprintf( fd, "Process launched!\n" );
	    fclose( fd );
#endif
	}
	else
	{
#ifdef DEBUGGING
	    fprintf( fd, "Process failed to launch!\n" );
	    fclose( fd );
#endif
	    goto cleanup;
	}

	// report the status to the service control manager.
	if( !ReportStatusToSCMgr( SERVICE_RUNNING,       // service state
				NO_ERROR,              // exit code
				0))                    // wait hint
	{
	    goto cleanup;
	}

	// Use event logging
	HANDLE hEventSource = RegisterEventSource(NULL, SZSERVICENAME);
	LPCSTR  lpszStrings[1];
	char szMsg[255];
	_stprintf(szMsg, "\n\np4dtg-service %s started", global_argv[1] );
	lpszStrings[0] = szMsg;
	ReportEvent( hEventSource,		// handle of event source
		EVENTLOG_INFORMATION_TYPE,  	// event type
		0,                    		// event category
		0,                    		// event ID
		NULL,                 		// current user's SID
 		1,                    		// strings in lpszStrings
		0,                    		// no bytes of raw data
		lpszStrings,          		// array of error strings
		NULL );                		// no raw data
	(void)DeregisterEventSource( hEventSource );
 
	// End of initialization

	// Service is now running, perform work until shutdown
	dwWait = WaitForMultipleObjects( 1, hEvents, FALSE, INFINITE );

cleanup:
	if( hServerStopEvent )
	    CloseHandle( hServerStopEvent );

	// Request replication to stop
	char stopfile[256];
	_snprintf( stopfile, 250, "repl\\stop-%s", global_argv[1] );
	fd = fopen( stopfile, "a+" );
	if( fd )
	    fclose( fd );

	// Report stop event
	hEventSource = RegisterEventSource( NULL, SZSERVICENAME );
	_stprintf(szMsg, "\n\np4dtg-service %s stopped", global_argv[1] );
	lpszStrings[0] = szMsg;
	ReportEvent( hEventSource,		// handle of event source
		EVENTLOG_INFORMATION_TYPE,  	// event type
		0,                    		// event category
		0,                    		// event ID
		NULL,                 		// current user's SID
 		1,                    		// strings in lpszStrings
		0,                    		// no bytes of raw data
		lpszStrings,          		// array of error strings
		NULL );                		// no raw data
	(void)DeregisterEventSource( hEventSource );
}

//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
void ServiceStop()
{
	char szPath[512];
	szPath[0] = '\0';
	if( GetModuleFileName( NULL, szPath, 512 ) )
	{
	    int j;
	    for( j = strlen( szPath ) - 1; j > 0 && szPath[j] != '\\'; j-- );
	    if( szPath[j] == '\\' )
	        szPath[j] = '\0';
	    _chdir( szPath );
	}

	if( hServerStopEvent )
	{
#ifdef DEBUGGING
	    FILE *fd = fopen( "abc123.txt", "a+" );
	    if( fd )
	    {
	        fprintf( fd, "ServiceStop:[%s]\n", szPath );
	        for( int i = 0; i < global_argc; i++ )
	            fprintf( fd, "ServiceStop: %d:[%s]\n", i, global_argv[i] );
	        fclose( fd );
	    }
#endif

	    SetEvent( hServerStopEvent );
	}
}
