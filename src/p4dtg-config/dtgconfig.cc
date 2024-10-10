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

#ifndef _WIN32
#include <unistd.h>
#endif

#include <sys/stat.h>
#include "DataSource.h"
#include "DataMapping.h"
#include "Settings.h"
#include "DTGxml.h"
#include "MainDialog.h"
#include "P4DTG.h"
#include <QDir>
#include <QString>
#include <QSplashScreen>
#include <QMessageBox>
#include "Help.h"
#include <genutils.h>
#include <DTG-platforms.h>

P4DTG *global = NULL;

static int writeable_directory( const char *dir )
{
	char *tmp = mk_string( dir, DIRSEPARATOR, "abc123.tmp" );
	unlink( tmp );
	FILE *f = fopen( tmp, "w" );
	if( f )
	{
	    fclose( f );
	    unlink( tmp );
	}
	delete[] tmp;
	return (f != NULL);
}

#ifdef _WIN32
int __stdcall
WinMain( HINSTANCE /* hInstance */,
	HINSTANCE /* hPrevInstance */,
	char *lpCmdLine,
	int /* nCmdShow */ )
{
	int argc = lpCmdLine ? 2 : 1;
	char *argv[2];
	argv[0] = strdup( "p4dtg-config" );
	argv[1] = lpCmdLine;
	if( argc == 2 && argv[1] && argv[1][0] == '-' )
	    if( argv[1][1] == 'h' )
	    {
	        char *msg = mk_string(
	            "Usage: ", argv[0], " {DTG_ROOT}\n",
	            "       DTG_ROOT defaults to current directory\n" );
	        QApplication app( argc, argv );
	        QMessageBox msgBox;
	        msgBox.setText( QUTF8( msg ) );
	        msgBox.exec();
	        delete[] msg;
	        return 0;
	    }
	    else if( argv[1][1] == 'V' )
	    {
	        char *rev_msg = mk_string( "Rev. p4dtg-config ",
					ID_OS, "/",
					ID_REL, "/",
					ID_PATCH );
	        char *date_msg = mk_string(  ID_Y, " ",
					ID_M, " ",
					ID_D );
	        char *msg = mk_string( rev_msg, "\n", date_msg );
	        delete[] rev_msg;
	        delete[] date_msg;
	        QApplication app( argc, argv );
	        QMessageBox msgBox;
	        msgBox.setText( QUTF8( msg ) );
	        msgBox.exec();
	        delete[] msg;
	        return 0;
	    }
#else
int
main( int argc, char *argv[] )
{
	if( argc == 2 && argv[1][0] == '-' )
	    if( argv[1][1] == 'h' )
	    {
	        printf( "Usage: %s {DTG_ROOT}\n", argv[0] );
	        printf( "       DTG_ROOT defaults to current directory\n" );
	        return 0;
	    }
	    else if( argv[1][1] == 'V' )
	    {
	        printf( "Rev. p4dtg-config/%s/%s/%s\n",
		        ID_OS, ID_REL, ID_PATCH );
	        printf( "%s %s %s\n", ID_Y, ID_M, ID_D );
	        return 0;
	    }
#endif

	QApplication app( argc, argv );
	global = new P4DTG( argv[1] );

	DataSource *sources = NULL;
	DataMapping *mappings = NULL;
	DTGSettings *settings = NULL;
	char *tmp = mk_string( global->root, "config", DIRSEPARATOR );
	QPixmap pixmap( QUTF8( "p4dtg-config.png" ) );
	QSplashScreen *splash =
		new QSplashScreen( pixmap, Qt::WindowStaysOnTopHint );
	splash->show();
	Qt::Alignment spot = Qt::AlignHCenter | Qt::AlignBottom;
	splash->showMessage( QUTF8( "Loading configuration..." ), spot );
	load_config( tmp, sources, mappings, settings );
	delete[] tmp;
	splash->showMessage( QUTF8( "Loaded configuration" ), spot );
	app.processEvents();
	DataMapping::cross_reference( sources, mappings );
	splash->showMessage( QUTF8( "Loading plugins..." ), spot );
	app.processEvents();
	global->cache_plugins();
	splash->showMessage( QUTF8( "Loaded plugins" ), spot );
	app.processEvents();
	DataSource::assign_plugins( global->plugins, sources );
	splash->showMessage( QUTF8( "Checking connections..." ), spot );
	app.processEvents();
	for( DataSource *src = sources; src; src = src->next )
	{
	    // Must call check_connection for each source
	    app.processEvents();
	    src->check_connection();
	}
	splash->showMessage( QUTF8( "Launching configuration editor" ), spot );
	app.processEvents();
	global->add_sources( sources );
	global->add_maps( mappings );
	global->setup_help();

	/* Check for required subdirectories */
	struct stat buf;
	QString msg;
	if( stat( "plugins", &buf ) )
	    msg += QString( QUTF8( "\nMissing 'plugins' subdirectory" ) );
	if( stat( "config", &buf ) )
	    msg += QString( QUTF8( "\nMissing 'config' subdirectory" ) );
	if( !writeable_directory( "config" ) )
	    msg += QString( QUTF8( "\n'config' subdirectory is not writable"));
	if( stat( "repl", &buf ) )
	    msg += QString( QUTF8( "\nMissing 'repl' subdirectory" ) );
	if( !writeable_directory( "repl" ) )
	    msg += QString( QUTF8( "\n'repl' subdirectory is not writable" ) );
	if( stat( "help", &buf ) )
	    msg += QString( QUTF8( "\nMissing 'help' subdirectory" ) );
	if( !writeable_directory( "help" ) )
	    msg += QString( QUTF8( "\n'help' subdirectory is not writable" ) );
	if( msg.length() )
	{
	    fprintf( stderr, "%s\n", qPrintable( msg ) );
	    splash->showMessage( msg, spot );
	    app.processEvents();
	    do_sleep( 5 );
	    delete splash;
	    delete global;
	    return 0;
	}

	MainDialog dialog;
	dialog.show();
	delete splash;
	int result = app.exec();
	delete global;
	return result;
}
