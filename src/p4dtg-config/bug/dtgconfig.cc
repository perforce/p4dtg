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

#include "MainDialog.h"
#include <QWindowsStyle>
#include <QPlastiqueStyle>
#include <QDir>
#include <QString>
#include <QSplashScreen>

#ifdef _WIN32
int __stdcall
WinMain( HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	char *lpCmdLine,
	int nCmdShow )
{
	int argc = 1;
	char *argv[2];
	argv[0] = strdup( "p4dtg-config" );
	argv[1] = lpCmdLine;
#else
int 
main( int argc, char *argv[] )
{
#endif
	    if( argv[1][1] == 'V' )
	    {
	        printf( "Rev. p4dtg-config/QTBUG\n");
	        return 0;
	    }

	QApplication app( argc, argv );
	QApplication::setStyle( new QWindowsStyle );

	MainDialog dialog;
	dialog.show();
	int result = app.exec();
	return result;
}
