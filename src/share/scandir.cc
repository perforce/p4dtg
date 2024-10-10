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

extern "C" {
#include <dtg-utils.h>
}

#include "stdio.h"
#include "string.h"
#include "genutils.h"

#ifndef _WIN32

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

struct DTGStrList *scan_dir( const char *dir )
{
	struct DTGStrList *list = NULL;
	DIR *dp;
	struct dirent *dir_entry;
	struct stat stat_info;

	if( !(dp = opendir(dir)) )
	    return list;

	char *tmp = new char[strlen(dir)+256];
	tmp[0] = '\0';
	while( (dir_entry = readdir( dp )) )
	{
	    sprintf( tmp, "%s/%s", dir, dir_entry->d_name );
	    lstat( tmp, &stat_info );
	    if( !S_ISDIR(stat_info.st_mode) )
	        list = append_DTGStrList( list, dir_entry->d_name );
	}
	delete[] tmp;
	closedir(dp);
	return list;
}

#else

#include <windows.h>

struct DTGStrList *scan_dir( const char *dirname )
{
	struct DTGStrList *list = NULL;
	BOOL fFinished;
	HANDLE hList;
	TCHAR szDir[MAX_PATH+1];
	TCHAR szSubDir[MAX_PATH+1];
	WIN32_FIND_DATA FileData;

	sprintf( szDir, "%s\\*", dirname );
	hList = FindFirstFile( szDir, &FileData );
	if( hList == INVALID_HANDLE_VALUE )
	    return list;

	fFinished = FALSE;
	while( !fFinished )
	{
	    if( !( FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
	        list = append_DTGStrList( list, FileData.cFileName );

	    if (!FindNextFile(hList, &FileData))
	        if (GetLastError() == ERROR_NO_MORE_FILES)
	            fFinished = TRUE;
	}

	FindClose(hList);
	return list;
}

#endif
