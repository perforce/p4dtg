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

#include <QDebug>
#include <QFrame>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QMenu>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDateTimeEdit>
#include <QProcess>
#include "MapTab.h"
#include "MapEdit.h"
#include "P4DTG.h"
#include "DataSource.h"
#include "DataMapping.h"
#include "Settings.h"
#include "NewMap.h"
#include "MainDialog.h"
#include <genutils.h>
#include <DTGxml.h>
extern "C" {
#include <DTG-interface.h>
#include <dtg-utils.h>
}

void MapTab::refreshList()
{
	int row = list->currentRow();
	list->clear();
	QListWidgetItem *item;
	for( DataMapping *map = global->maps; map; map = map->next )
	{
	    QString tag = QUTF8( map->id ) + QUTF8( ": " );

	    /* map->dts->nickname and map-dts->id will match but may vary */
	    /* in case, so use the plugin name when it is available       */

	    if( map->dts )
	        tag += QUTF8( map->dts->nickname );
	    else
	        tag += QUTF8( map->dts_id );
	    if( map->dts_filter )
	        tag += QUTF8( "/" ) + QUTF8( map->dts_filter );
	    tag += DBL_ARROW;
	    if( map->scm )
	        tag += QUTF8( map->scm->nickname );
	    else
	        tag = QUTF8( map->scm_id );
	    if( map->scm_filter )
	        tag += QUTF8( "/" ) + QUTF8( map->scm_filter );

	    if( map->deleted )
	        tag.append( QUTF8( " [deleted]" ) );
	    if( !map->scm )
	        tag.append( QUTF8( " [SCM Unknown]" ) );
	    if( !map->dts )
	        tag.append( QUTF8( " [DTS Unknown]" ) );
	    item = new QListWidgetItem( tag, list );
	    if( map->dirty )
	    {
	        QFont font = item->font();
	        font.setBold( true );
	        item->setFont( font );
	    }
	}
	if( row < 0 && list->count() )
	    row = 0;
	list->setCurrentRow( row );
	listClicked( list->item( row ) );
}

MapTab::MapTab( MainDialog *my_base, QWidget *parent )
    : QWidget( parent )
{
	base = my_base;
	QVBoxLayout *buttons = new QVBoxLayout;
	edit_btn = new QPushButton( tr( "&Edit..." ) );
	edit_btn->setToolTip( QUTF8( "Edit the selected mapping" ) );
	new_btn = new QPushButton( tr( "&New..." ) );
	new_btn->setToolTip( QUTF8( "Define a new mapping" ) );
	delete_btn = new QPushButton( tr( "&Delete" ) );
	delete_btn->setToolTip( QUTF8( "Delete the selected mapping" ) );
	buttons->addWidget( edit_btn, 0 );
	buttons->addWidget( new_btn, 0 );
	buttons->addWidget( delete_btn, 0 );
	buttons->addStretch( 1 );
	connect( edit_btn, SIGNAL( clicked() ), this, SLOT( editPushed() ) );
	connect( new_btn, SIGNAL( clicked() ), this, SLOT( newPushed() ) );
	connect( delete_btn, SIGNAL( clicked() ), 
		this, SLOT( deletePushed() ) );

    	QHBoxLayout *list_buttons = new QHBoxLayout;
	list = new QListWidget;
	list_buttons->addWidget( list, 1 ); // stretch this
	list_buttons->addLayout( buttons, 0 );
	connect( list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( listClicked(QListWidgetItem *) ) );
	connect( list, SIGNAL( currentRowChanged(int) ),
		this, SLOT( rowChanged(int) ) );

    	QVBoxLayout *full = new QVBoxLayout;
	QLabel *label = new QLabel( tr( "Defect Tracker/Perforce mappings:" ) );
	full->addWidget( label, 0 );
	full->addLayout( list_buttons, 1 ); // Stretch this

	QFrame *line = new QFrame();
	line->setFrameShape( QFrame::HLine );
	full->addWidget( line );
	
	QGridLayout *grid = new QGridLayout;
	grid->addWidget( new QLabel( QUTF8( "Name:" ) ), 0, 0 );
	name_edit = new QLineEdit( QUTF8( "insertName" ) );
	name_edit->setReadOnly( true );
	name_edit->setFrame( false );
	grid->addWidget( name_edit, 0, 1, 1, 2 );
	grid->addWidget( new QLabel( QUTF8( "Defect tracking source:" ) ), 
			1, 0 );
	dts_edit = new QLineEdit( QUTF8( "insertDTS" ) );
	dts_edit->setReadOnly( true );
	dts_edit->setFrame( false );
	grid->addWidget( dts_edit, 1, 1, 1, 2 );
	grid->addWidget( new QLabel( QUTF8( "Perforce server source:" ) ), 
			2, 0 );
	scm_edit = new QLineEdit( QUTF8( "insertSCM" ) );
	scm_edit->setReadOnly( true );
	scm_edit->setFrame( false );
	grid->addWidget( scm_edit, 2, 1, 1, 2 );
	full->addLayout( grid );
	
	QGroupBox *mbox = new QGroupBox( tr( "Using Last Modified Date" ) );
	QGridLayout *mgrid = new QGridLayout;
	mgrid->addWidget( new QLabel( QUTF8( "Replication server status:")),
			0, 0 );
	status_edit = new QLineEdit( QUTF8( "insertStatus" ) );
	status_edit->setReadOnly( true );
	status_edit->setFrame( false );
	mgrid->addWidget( status_edit, 0, 1, 1, 2 );
	mgrid->addWidget( new QLabel( QUTF8( "Start replication from:" ) ), 
			1, 0 );
	start_edit = new QDateTimeEdit(QDate::currentDate());
	start_edit->setDisplayFormat( QUTF8( "yyyy/MM/dd hh:mm:ss ap" ) );
	mgrid->addWidget( start_edit, 1, 1 );
	sync_edit = new QCheckBox( QUTF8( "Sync from start date" ) );
	mgrid->addWidget( sync_edit, 1, 2 );
	mgrid->addWidget( new QLabel( QUTF8( "Last SCM replication was:" ) ),
			2, 0 );
	last_edit = new QLineEdit;
	QDateTime now = QDateTime::currentDateTime();
	last_edit->setText( now.toString( QUTF8( "yyyy/MM/dd hh:mm:ss ap" ) ) );
	last_edit->setReadOnly( true );
	last_edit->setFrame( false );
	mgrid->addWidget( last_edit, 2, 1 );
	run_btn = new QPushButton( QUTF8( "Start &Replication" ) );
	stop_action = 0;
	mgrid->addWidget( run_btn, 2, 2 );
	connect( run_btn, SIGNAL( clicked() ), this, SLOT( runPushed() ) );
	mbox->setLayout( mgrid );
	full->addWidget( mbox );
	setLayout( full );
}

void MapTab::editPushed()
{
	int row = list->currentRow();
	int i = 0;
	DataMapping *map, *prev;
	for( prev = NULL, map = global->maps; map && i < row; map = map->next )
	{
	    prev = map; 
	    i++;
	}
	if( map && map->scm && map->dts )
	{
	    DataMapping *tmp = map->copy();
	    MapEdit *edit = new MapEdit( tmp, this );
	    if( edit->badmaps )
	    {
	        QMessageBox::information( this, 
	           QUTF8( "Perforce Defect Tracking Gateway" ),
	           QUTF8( 
		   "<p>Some of the specified mappings reference fields\n"
	           "that are not defined in the respective server. These\n"
	           "mapping are show in <b>bold</b>.</p>\n\n" ) );
	    }
	    int result = edit->exec();
	    if( result )
	    {
	        if( prev )
	            prev->next = tmp;
	        else
	            global->maps = tmp;
	        tmp->next = map->next;
	        map->next = NULL;
	        delete map;
	        tmp->dirty = 1;
	        base->enable();
	        refreshList();
	    }
	    else
	        delete tmp;
	    delete edit;
	}
}

void MapTab::newPushed()
{
	DataSource *src;
	FilterSet *set;

	int scmcnt = 0;
	for( src = global->p4_srcs; !scmcnt && src; src = src->next )
	{
	    if( src->deleted )
	        continue;
	    if( !src->refcnt )
	        scmcnt++;
	    for( set = src->set_list; !scmcnt && set; set = set->next )
	        if( !set->refcnt )
	            scmcnt++;
	}

	int dtscnt = 0;
	for( src = global->dt_srcs; !dtscnt && src; src = src->next )
	{
	    if( src->deleted )
	        continue;
	    if( !src->refcnt )
	        dtscnt++;
	    for( set = src->set_list; !dtscnt && set; set = set->next )
	        if( !set->refcnt )
	            dtscnt++;
	}
	            
	if( !dtscnt || !scmcnt )
	{
	    char *msg;
	    if( scmcnt )
	        msg = mk_string( 
		"There are no unassigned Defect Tracking sources\n",
		"available for defining a new mapping." );
	    else if( dtscnt )
	        msg = mk_string( 
		"There are no unassigned Perforce Server sources\n",
		"available for defining a new mapping." );
	    else
	        msg = mk_string( 
		"There are no unassigned Perforce Server or Defect\n",
		"Tracking sources available for defining a new mapping." );
	    QMessageBox::information( this, 
			QUTF8( "Perforce Defect Tracking Gateway" ), 
			QUTF8( msg ) );
	    delete[] msg;
	    return;
	}

	DataMapping *map = new DataMapping;
	NewMap *newmap = new NewMap( map, this );
	int result = newmap->exec();
	if( result )
	{
	    MapEdit *edit = new MapEdit( map, this );
	    edit->ok_btn->setEnabled( true );
	    result = edit->exec();
	    if( result )
	    {
	        map->next = global->maps;
	        global->maps = map;
	        map->dirty = 1;
	        base->enable();
	        list->setCurrentItem( 0 );
	        refreshList();
	    }
	    else
	    {
	        map->scm->adj_refcnt( -1, map->scm_filter );
	        map->dts->adj_refcnt( -1, map->dts_filter );;
	        delete map;
	    }
	    delete edit;
	}
	delete newmap;
}

void MapTab::deletePushed()
{
	int row = list->currentRow();
	int i = 0;
	DataMapping *map;
	for( map = global->maps; map && i < row; i++, map = map->next ) {};
	if( map )
	{
	    if( !map->deleted )
	    {
#ifdef _WIN32
	        // Check for service
	        QFile svcfile( QUTF8( global->root ) + 
			QUTF8( "config/svc-" ) + QUTF8( map->id ) );
	        if( svcfile.exists() )
	        {
	            QMessageBox::information( this, 
			QUTF8( "Replication Server" ),
			QUTF8( "Removing service: " ) + 
				QUTF8( "p4dtg-service-" ) + 
				QUTF8( map->id ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	            QStringList args;
	            args << QUTF8( "-remove" );
	            args << QUTF8( map->id );
	            if( QProcess::startDetached( QUTF8( global->root ) + 
					QUTF8( "p4dtg-service.exe" ), 
					args ) )
	                QMessageBox::information( this, 
				QUTF8( "Replication Server" ),
				QUTF8( "Removed service: " ) + 
					QUTF8( "p4dtg-service-" ) + 
				QUTF8( map->id ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	            else
	            {
	                QMessageBox::warning( this, 
				QUTF8( "Replication Server" ),
				QUTF8( "Failed to remove service: " ) + 
					QUTF8( "p4dtg-service-" ) + 
					QUTF8( map->id ),
				QMessageBox::NoButton,
				QMessageBox::NoButton,
				QMessageBox::NoButton );
	                return;
	            }
	        }
#endif
	        list->setCurrentRow( -1 );
	    }
	    map->deleted = !( map->deleted );
	    map->dirty = 1;
	    base->enable();
	    refreshList();
	}
}

extern const char *start_service( const char *service, const char *map );
extern const char *stop_service( const char *service );

void MapTab::runPushed()
{
	int row = list->currentRow();
	int i = 0;
	DataMapping *map;
	for( map = global->maps; map && i < row; i++, map = map->next ) {};
	if( !map )
	    return;

	if( stop_action )
        {
#ifdef _WIN32
	    char *unify = mk_string( "p4dtg-service-", map->id );
	    const char *errmsg = stop_service( unify );
	    do_sleep( 1 ); // Slow down a bit to allow repl to exit
	    if( !*errmsg )
	        QMessageBox::information( this, 
			QUTF8( "Replication Service" ),
			QUTF8( unify ) + QUTF8( " stopped" ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	    else
	        QMessageBox::warning( this, 
			QUTF8( "Replication Service" ),
			QUTF8( unify ) + QUTF8( " failed to stop:\n" ) +
			QUTF8( errmsg ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	    delete[] unify;
#else
	    char *stop_name = 
		mk_string( global->root, "repl/stop-", map->id );
	    FILE *fd = fopen( stop_name, "a" );
	    if( fd )
	    {
	        fclose( fd );
	        do_sleep( 1 ); // Slow down a bit to allow repl to exit
	        QMessageBox::information( this, 
			QUTF8( "Replication Server" ),
			QUTF8( "Stop requested" ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	    }
	    else
	    {
	        QMessageBox::critical( this, QUTF8( "File System Error" ),
			QUTF8( "Failed to create stop file:\n" ) +
			QUTF8( stop_name ),
			QMessageBox::Ok, QMessageBox::NoButton );
	    }
	    delete[] stop_name;
#endif
	    listClicked( list->item( row ) );
	    return;
	}

	if( map->dirty || ( map->scm && map->scm->dirty ) || 
			( map->dts && map->dts->dirty ) )
	{
	    int abort_it = QMessageBox::warning( this, 
			QUTF8( "Replication Server" ),
	QUTF8( "There are pending changes to the mapping or associated\n" ) +
	QUTF8( "sources that need to be saved prior to changing the\n" ) +
	QUTF8( "replication server settings.\n\n" ) +
	QUTF8( "Would you like to save all pending changes?" ),
			QUTF8( "Yes" ),
			QUTF8( "No" ) );
	    if( abort_it )
	        return;
	    base->applyPushed();
	}
	if( map->scm && map->scm->status != DataSource::READY )
	{
	    QMessageBox::critical( this, 
			QUTF8( "Perforce Server Configuration" ),
		QUTF8( "Please recheck the connection to the Perforce\n") +
		QUTF8( "server before invoking the Replication Server.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	else if( map->scm && map->scm_filter && !map->scm->seg_ok )
	{
	    QMessageBox::critical( this, 
			QUTF8( "Perforce Server Configuration" ),
		QUTF8( "The field DTG_MAPID is missing from the server.\n") +
		QUTF8( "This field is required for replication. See the\n") +
		QUTF8( "User Guide for more details.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	if( map->dts && !map->dts->moddate_field )
	{
	    QMessageBox::critical( this, 
			QUTF8( "Defect Tracking Server Configuration" ),
		QUTF8( "The Defect Tracking Server is missing fields\n" ) +
		QUTF8( "required by the Replication Server. See the\n" ) +
		QUTF8( "User Guide for more details.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	int allmapped = 1;
	int nummapped = 0;
	CopyRule *cr;
	for( cr = map->mirror_rules; allmapped && cr; cr = cr->next )
	    if( !cr->deleted )
	        if( cr->copy_type == CopyRule::UNMAP )
	            allmapped = 0;
	        else
	            nummapped++;
	for( cr = map->dts_to_scm_rules; allmapped && cr; cr = cr->next )
	    if( !cr->deleted )
	        if( cr->copy_type == CopyRule::UNMAP )
	            allmapped = 0;
	        else
	            nummapped++;
	for( cr = map->scm_to_dts_rules; allmapped && cr; cr = cr->next )
	    if( !cr->deleted )
	        if( cr->copy_type == CopyRule::UNMAP )
	            allmapped = 0;
	        else
	            nummapped++;
	if( !allmapped )
	{
	    QMessageBox::critical( this, 
			QUTF8( "Incomplete Select Field Mapping" ),
		QUTF8( "At least one select field mapping does not have\n") +
		QUTF8( "all possible source values mapped. Please edit\n" ) +
		QUTF8( "the mapping to fully specify all select maps.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	if( !nummapped )
	{
	    QMessageBox::critical( this, 
			QUTF8( "No Defined Mappings" ),
		QUTF8( "At least one field mapping must be defined.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	
	// Unicode check
	if( map->scm->accept_utf8 == -1 )
	{
	    QMessageBox::critical( this, 
			QUTF8( "UTF-8 Support Check" ),
		QUTF8( "Perforce Plug-in is out-of-date. Update P4DTG.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	if( map->scm->accept_utf8 != map->dts->accept_utf8 && 
		map->dts->accept_utf8 > -1 )
	{
	    QMessageBox::critical( this, 
			QUTF8( "UTF-8 Support Check" ),
		QUTF8( "Incompatible unicode servers - "
			"only 1 supports using UTF-8.\n" ),
		QMessageBox::Ok, QMessageBox::NoButton );
	        return;
	}
	
#ifdef _WIN32
	// Check for service
	QFile svcfile( QUTF8( global->root ) + 
		QUTF8( "config/svc-" ) + QUTF8( map->id ) );
	if( !svcfile.exists() )
	{
	    QMessageBox::information( this, 
			QUTF8( "Replication Server" ),
			QUTF8( "Installing service: " ) + 
				QUTF8( "p4dtg-service-" ) + 
				QUTF8( map->id ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	    QStringList args;
	    args << QUTF8( "-install" );
	    args << QUTF8( map->id );
	    if( QProcess::startDetached( QUTF8( global->root ) + 
					QUTF8( "p4dtg-service.exe" ), 
					args ) )
	        QMessageBox::information( this, 
			QUTF8( "Replication Server" ),
			QUTF8( "Installed service: " ) + 
				QUTF8( "p4dtg-service-" ) + 
				QUTF8( map->id ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	    else
	    {
	        QMessageBox::warning( this, 
			QUTF8( "Replication Server" ),
			QUTF8( "Failed to install service: " ) + 
				QUTF8( "p4dtg-service-" ) + 
				QUTF8( map->id ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	        return;
	    }
	}
#endif
	// DO IT!!!
	char *set_name = 
		mk_string( global->root, "config/set-", map->id, ".xml" );
	DTGSettings *settings;
	int loaded = load_p4dtg_settings( set_name, settings );
	if( loaded <= 0 )
	{
	    QMessageBox::critical( this, QUTF8( "File Format Error" ),
		QUTF8( "Setting file is invalid:\n" ) +
		QUTF8( set_name ),
		QMessageBox::Ok, QMessageBox::NoButton );
	    delete[] set_name;
	    return;
	}
	else if( !settings )
	{
	    settings = new DTGSettings;
	    settings->id = mk_string( map->id );
	}
	settings->starting_date->year = 
		start_edit->dateTime().date().year();
	settings->starting_date->month = 
		start_edit->dateTime().date().month();
	settings->starting_date->day = 
		start_edit->dateTime().date().day();
	settings->starting_date->hour = 
		start_edit->dateTime().time().hour();
	settings->starting_date->minute = 
		start_edit->dateTime().time().minute();
	settings->starting_date->second = 
		start_edit->dateTime().time().second();
	if( compare_DTGDate( settings->starting_date, 
				settings->last_update_scm ) < 0 )
	    set_DTGDate( settings->last_update_scm, settings->starting_date );
	if( compare_DTGDate( settings->starting_date, 
				settings->last_update_dts ) < 0 )
	    set_DTGDate( settings->last_update_dts, settings->starting_date );
	settings->force = sync_edit->checkState() != Qt::Unchecked;
	if( save_p4dtg_settings( set_name, settings ) )
	{
	    QMessageBox::critical( this,
		QUTF8( "Configuration Files" ),
		QUTF8( "Unable to save settings file:\n  " ) +
		QUTF8( set_name ) +
		QUTF8( "\nCheck write permissions" ) );
	    delete[] set_name;
	    return;
	}
	delete[] set_name;
#ifdef _WIN32
	char *unify = mk_string( "p4dtg-service-", map->id );
	const char *errmsg = start_service( unify, map->id );
	if( !*errmsg )
	{
	    do_sleep( 1 ); // Allow repl time to startup
	    QMessageBox::information( this, 
			QUTF8( "Replication Service" ),
			QUTF8( unify ) + QUTF8( " started" ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	}
	else
	    QMessageBox::warning( this, 
			QUTF8( "Replication Service" ),
			QUTF8( unify ) + QUTF8( " failed to start:\n" ) +
			QUTF8( errmsg ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	delete[] unify;
#else
	const char *exe_name = "p4dtg-repl";
	char *unify = mk_string( global->root, "p4dtg-repl" );
	if( QFile::exists( QUTF8( unify ) ) )
	{
	    QStringList args;
	    args << QUTF8( map->id );
	    args << QUTF8( global->root );
	    if( QProcess::startDetached( QUTF8( unify ), args ) )
	    {
	        do_sleep( 1 ); // Allow repl time to startup
	        QMessageBox::information( this, 
			QUTF8( "Replication Server" ),
			QUTF8( exe_name ) + QUTF8( " launched" ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	    }
	    else
	        QMessageBox::warning( this, 
			QUTF8( "Replication Server" ),
			QUTF8( exe_name ) + QUTF8( " failed to launch" ),
			QMessageBox::NoButton,
			QMessageBox::NoButton,
			QMessageBox::NoButton );
	}
	else
	    QMessageBox::warning( this, 
		QUTF8( "Replication Server" ),
		QUTF8( exe_name ) + QUTF8( " not found" ),
		QMessageBox::NoButton,
		QMessageBox::NoButton,
		QMessageBox::NoButton );
	delete[] unify;
#endif 
	listClicked( list->currentItem() );
}

void MapTab::rowChanged( int /* row */ )
{
	listClicked( list->currentItem() );
}

void MapTab::listClicked( QListWidgetItem * /*item*/ )
{
	int row = list->currentRow();
	int i = 0;
	DataMapping *map;
	for( map = global->maps; map && i < row; i++, map = map->next ) {};
	if( map && i == row && !map->deleted )
	{
	    delete_btn->setText( tr( "&Delete" ) );
	    edit_btn->setEnabled( true );
	    delete_btn->setEnabled( true );
	    if( !map->scm || !map->dts )
	        edit_btn->setEnabled( false );
	    name_edit->setText( QUTF8( map->id ) );
	    if( map->dts_filter )
	        dts_edit->setText( QUTF8( map->dts_id ) + 
			QUTF8( "/" ) + QUTF8( map->dts_filter ) );
	    else
	        dts_edit->setText( QUTF8( map->dts_id ) );
	    if( map->scm_filter )
	        scm_edit->setText( QUTF8( map->scm_id ) + 
			QUTF8( "/" ) + QUTF8( map->scm_filter ) );
	    else
	        scm_edit->setText( QUTF8( map->scm_id ) );
// Run Stop Replication
// 0   0    0      repl is not running
// 1   0    1      repl is running
// 0   1    0      repl has been stopped
// 1   1    1      waiting for repl to stop
	    QFile runfile( QUTF8( global->root ) + 
		QUTF8( "repl/run-" ) + QUTF8( map->id ) );
	    QFile stopfile( QUTF8( global->root ) + 
		QUTF8( "repl/stop-" ) + QUTF8( map->id ) );
	    QFile errfile( QUTF8( global->root ) + 
		QUTF8( "repl/err-" ) + QUTF8( map->id ) );
	    if( runfile.exists() )
	    {
	        delete_btn->setEnabled( false );
	        if( stopfile.exists() )
	        {
	            status_edit->setText( QUTF8( "Waiting for server to stop"));
	            run_btn->setText( QUTF8( "Stopping replication" ) );
	            run_btn->setEnabled( false );
	            stop_action = 1;
	            start_edit->setEnabled( false );
	            sync_edit->setEnabled( false );
	        }
	        else
	        {
	            status_edit->setText( QUTF8( "Server is running" ) );
	            run_btn->setText( QUTF8( "Stop &replication" ) );
	            run_btn->setEnabled( true );
	            stop_action = 1;
	            start_edit->setEnabled( false );
	            sync_edit->setEnabled( false );
	        }
	    }
	    else if( errfile.exists() )
	    {
	        status_edit->setText( 
			QUTF8( "Manual correction of errors required" ) );
	        run_btn->setText( QUTF8( "Replication disabled" ) );
	        run_btn->setEnabled( false );
	        stop_action = 1;
	        start_edit->setEnabled( false );
	        sync_edit->setEnabled( false );
	    }
	    else
	    {
	        if( stopfile.exists() )
	            status_edit->setText( QUTF8( "Server has stopped" ) );
	        else
	            status_edit->setText( QUTF8( "Server is not running" ) );
	        run_btn->setText( QUTF8( "Start &replication" ) );
	        run_btn->setEnabled( true );
	        stop_action = 0;
	        start_edit->setEnabled( true );
	        sync_edit->setEnabled( true );
	    }

	    char *set_name = 
		mk_string( global->root, "config/set-", map->id, ".xml" );
	    DTGSettings *settings = NULL;
	    int loaded;
	    if( !lock_file( set_name ) ) // blocks until locked
	    {
	        loaded = -1;
	        status_edit->setText( QUTF8( "Error locking setting file" ) );
	    }
	    else
	    {
	        loaded = load_p4dtg_settings( set_name, settings );
	        unlock_file( set_name );
	    }
	    if( loaded <= 0 )
	    {
	        // Invalid file
	        status_edit->setText( QUTF8( "Error with setting file" ) );
	        run_btn->setEnabled( false );
	        start_edit->setEnabled( false );
	        sync_edit->setEnabled( false );
	        last_edit->setText( QUTF8( "" ) );
	        name_edit->setText( QUTF8( "" ) );
	        dts_edit->setText( QUTF8( "" ) );
	        scm_edit->setText( QUTF8( "" ) );
	        QDateTime dt;
	        start_edit->setDateTime( dt );
	        delete[] set_name;
	        return;
	    }
	    else if( !settings )
	    {
	        settings = new DTGSettings;
	        sync_edit->setCheckState( Qt::Checked );
	        sync_edit->setEnabled( false );
	        status_edit->setText( QUTF8( "Server has never been run" ) );
	    }
	    else
	    {
	        sync_edit->setCheckState( settings->force ?
					Qt::Checked :
					Qt::Unchecked );
	        sync_edit->setEnabled( true );
	    }

	    QDate d( settings->starting_date->year, 
			settings->starting_date->month, 
			settings->starting_date->day );
	    QTime t( settings->starting_date->hour, 
			settings->starting_date->minute, 
			settings->starting_date->second );
	    QDateTime dt;
	    dt.setDate( d );
	    dt.setTime( t );
	    start_edit->setDateTime( dt );
	    QDate d2( settings->last_update_scm->year, 
			settings->last_update_scm->month, 
			settings->last_update_scm->day );
	    QTime t2( settings->last_update_scm->hour, 
			settings->last_update_scm->minute, 
			settings->last_update_scm->second );
	    delete settings;
	    dt.setDate( d2 );
	    dt.setTime( t2 );
	    last_edit->setText( dt.toString( QUTF8( "yyyy/MM/dd hh:mm:ss ap")));

	    delete[] set_name;
	}
	else
	{
	    run_btn->setEnabled( false );
	    start_edit->setEnabled( false );
	    sync_edit->setEnabled( false );
	    last_edit->setText( QUTF8( "" ) );
	    name_edit->setText( QUTF8( "" ) );
	    dts_edit->setText( QUTF8( "" ) );
	    scm_edit->setText( QUTF8( "" ) );
	    QDateTime dt;
	    start_edit->setDateTime( dt );

	    if( map && map->deleted )
	    {
	        status_edit->setText( QUTF8( "Deleted mapping specified" ) );
	        delete_btn->setText( tr( "Un&delete" ) );
	        delete_btn->setEnabled( true );
	        edit_btn->setEnabled( false );
	    }
	    else
	    {
	        status_edit->setText( QUTF8( "No mapping specified" ) );
	        delete_btn->setText( tr( "&Delete" ) );
	        delete_btn->setEnabled( false );
	        edit_btn->setEnabled( false );
	    }

	}
}
