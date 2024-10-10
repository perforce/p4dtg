     MSDN Home >  MSDN Library >  Win32 and COM Development >  System Services >  DLLs, Processes and Threads >  DLLs, Processes, and Threads >  Services >  Using Services >  Service Control Program Tasks
 
Starting a Service

To start a service, the following example opens a handle to an installed database and then specifies the handle in a call to the StartService function. It can be used to start either a service or a driver service, but this example assumes that a service is being started. After starting the service, the program uses the members of the SERVICE_STATUS_PROCESS structure returned by the QueryServiceStatusEx function to track the progress of the service.

The function requires a handle to a service control manager database. For more information, see Opening an SCManager Database.

#include <windows.h>
#include <stdio.h>

BOOL StartSampleService(SC_HANDLE schSCManager) 
{ 
    SC_HANDLE schService;
    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;
 
    schService = OpenService( 
        schSCManager,          // SCM database 
        "Sample_Srv",          // service name
        SERVICE_ALL_ACCESS); 
 
    if (schService == NULL) 
    { 
        return 0; 
    }
 
    if (!StartService(
            schService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
        return 0; 
    }
    else 
    {
        printf("Service start pending.\n"); 
    }
 
    // Check the status until the service is no longer start pending. 
 
    if (!QueryServiceStatusEx( 
            schService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            &ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
    {
        return 0; 
    }
 
    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        // Do not wait longer than the wait hint. A good interval is 
        // one tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 
 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status again. 
 
        if (!QueryServiceStatusEx( 
            schService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            &ssStatus,              // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
            break; 
 
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // The service is making progress.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint
                break;
            }
        }
    } 

    CloseServiceHandle(schService); 

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        printf("StartService SUCCESS.\n"); 
        return 1;
    }
    else 
    { 
        printf("\nService not started. \n");
        printf("  Current State: %d\n", ssStatus.dwCurrentState); 
        printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode); 
        printf("  Service Specific Exit Code: %d\n", 
            ssStatus.dwServiceSpecificExitCode); 
        printf("  Check Point: %d\n", ssStatus.dwCheckPoint); 
        printf("  Wait Hint: %d\n", ssStatus.dwWaitHint); 
        return 0;
    } 
}

