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
#include <QFontDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QVBoxLayout>
#include <QDateTime>
#include <QMessageBox>
#include <DTG-platforms.h>
#include "MainDialog.h"
#include "P4Tab.h"
#include "DTSTab.h"
#include "MapTab.h"
#include "DataSource.h"
#include "DataMapping.h"
#include "DTGxml.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <genutils.h>

void MainDialog::enable()
{
	main_apply->setEnabled( true );
}

MainDialog::MainDialog(QWidget *parent )
    : QWidget( parent )
{
	setWindowTitle( QString( tr( 
		"Perforce Defect Tracking Gateway Configuration Editor" ) ) );
    	QHBoxLayout *bottom = new QHBoxLayout;
	main_help = new QPushButton( tr( "&Help" ) );
	main_ok = new QPushButton( tr( "OK" ) );
	main_ok->setDefault( true );
	main_cancel = new QPushButton( tr( "Cancel" ) );
	main_apply = new QPushButton( tr( "&Apply" ) );
	main_apply->setEnabled( false );
	bottom->addWidget( main_help );
	bottom->addStretch( 1 );
	bottom->addWidget( main_ok );
	bottom->addWidget( main_cancel );
	bottom->addWidget( main_apply );
	connect( main_ok, SIGNAL( clicked() ), this, SLOT( okPushed() ) );
	connect( main_cancel, SIGNAL( clicked() ), 
		this, SLOT( cancelPushed() ) );
	connect( main_apply, SIGNAL( clicked() ), this, SLOT( applyPushed() ) );
	connect( main_help, SIGNAL( clicked() ), this, SLOT( helpPushed() ) );


	QVBoxLayout *page = new QVBoxLayout;
	main_tabs = new QTabWidget;
	connect( main_tabs, SIGNAL( currentChanged(int) ), 
		this, SLOT( tabChanged(int) ) );

	p4_tab = new P4Tab( this );
	dts_tab = new DTSTab( this );
	map_tab = new MapTab( this );

	main_tabs->addTab( dts_tab, QString( tr( 
		"Defect &Tracking Sources" ) ) );
	main_tabs->addTab( p4_tab, QString( tr( 
		"&Perforce Server Sources" ) ) );
	main_tabs->addTab( map_tab, QString( tr( "&Gateway Mappings" ) ) );
	page->addWidget( main_tabs, 1 ); // Stretch tabs
	page->addLayout( bottom );

	setMinimumWidth( 500 );
	setLayout( page );
}

void MainDialog::tabChanged( int tab )
{
	if( tab == 2 )
	    map_tab->refreshList();
}

void
MainDialog::okPushed()
{
	if( main_apply->isEnabled() )
	{
	    char *dir = mk_string( global->root, "config", DIRSEPARATOR );
	    if ( save_config( dir,
			global->p4_srcs, global->dt_srcs,
			global->maps, NULL ) )
	    {
	        QMessageBox::critical( this,
			QString( tr( "Configuration Files" ) ),
			QString( tr( "Unable to save or delete files in " ) ) +
			QUTF8( dir ) +
			QString( tr( ".\nCheck write permissions" ) ) );
	        delete[] dir;
	        return;
	    }
	    delete[] dir;
	}
	qApp->quit();
}

void
MainDialog::applyPushed()
{
	char *dir = mk_string( global->root, "config", DIRSEPARATOR );
	if( save_config( dir,
			global->p4_srcs, global->dt_srcs,
			global->maps, NULL ) )
	{
	    QMessageBox::critical( this,
			QString( tr( "Configuration Files" ) ),
			QString( tr( "Unable to save or delete files in " ) ) +
			QUTF8( dir ) +
			QString( tr( ".\nCheck write permissions" ) ) );
	    delete[] dir;
	    return;

	    // Notes: If some of the objects are saved and others are not,
	    // it may be possible that the refcnts will be out of sync
	    // Currently not worth the redesign necessary to handle this
	    // case as they can simply restart the app to correct the issue
	}
	delete[] dir;
	main_apply->setEnabled( false );
	if( global->maps )
	{
	    DataMapping *tmp;
	    while( global->maps && global->maps->deleted )
	    {
	        tmp = global->maps;
	        global->maps = global->maps->next;
	        if( tmp->scm )
	            tmp->scm->adj_refcnt( -1, tmp->scm_filter );
	        if( tmp->dts )
	            tmp->dts->adj_refcnt( -1, tmp->dts_filter );
	        tmp->next = NULL;
	        delete tmp;
	    }
	    DataMapping *cur = global->maps;
	    while( cur )
	        if( cur->next && cur->next->deleted )
	        {
	            tmp = cur->next;
	            cur->next = cur->next->next;
	            if( tmp->scm )
	                tmp->scm->adj_refcnt( -1, tmp->scm_filter );
	            if( tmp->dts )
	                tmp->dts->adj_refcnt( -1, tmp->dts_filter );
	            tmp->next = NULL;
	            delete tmp;
	        }
	        else
	            cur = cur->next;
	}
	if( global->p4_srcs )
	{
	    DataSource *tmp;
	    while( global->p4_srcs && global->p4_srcs->deleted )
	    {
	        tmp = global->p4_srcs;
	        global->p4_srcs = global->p4_srcs->next;
	        tmp->next = NULL;
	        delete tmp;
	    }
	    DataSource *last = global->p4_srcs;
	    while( last && last->next )
	        if( last->next->deleted )
	        {
	            tmp = last->next;
	            last->next = last->next->next;
	            tmp->next = NULL;
	            delete tmp;
	        }
	        else
	            last = last->next;
	}
	if( global->dt_srcs )
	{
	    DataSource *tmp;
	    while( global->dt_srcs && global->dt_srcs->deleted )
	    {
	        tmp = global->dt_srcs;
	        global->dt_srcs = global->dt_srcs->next;
	        tmp->next = NULL;
	        delete tmp;
	    }
	    DataSource *last = global->dt_srcs;
	    while( last && last->next )
	        if( last->next->deleted )
	        {
	            tmp = last->next;
	            last->next = last->next->next;
	            tmp->next = NULL;
	            delete tmp;
	        }
	        else
	            last = last->next;
	}
	map_tab->refreshList();
	dts_tab->refreshList();
	p4_tab->refreshList();
}

void
MainDialog::cancelPushed()
{
	qApp->quit();
}

void
MainDialog::helpPushed()
{
	switch ( main_tabs->currentIndex() )
	{
	case 0: // dtstab
	    global->help->showSource( DTSTab_help, this );
	    break;
	case 1: // p4tab
	    global->help->showSource( P4Tab_help, this );
	    break;
	case 2: // maptab
	    global->help->showSource( MapTab_help, this );
	    break;
	}
}
