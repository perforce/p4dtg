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
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include "PickList.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataSource.h>
#include <DataMapping.h>
#include <DTG-interface.h>

PickList::PickList( const char *title, 
		QLayout *hbox,
		const char *label,
		QStringList &items, 
		QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
#ifndef OS_MACOSX
	setWindowModality( Qt::WindowModal );
#endif
	setWindowTitle( QString( QUTF8( title ) ) );

    	QVBoxLayout *full = new QVBoxLayout;
    	QHBoxLayout *bottom = new QHBoxLayout;
	help_btn = new QPushButton( tr( "&Help" ) );
	ok_btn = new QPushButton( tr( "OK" ) );
	ok_btn->setEnabled( false );
	cancel_btn = new QPushButton( tr( "Cancel" ) );
	bottom->addWidget( help_btn );
	bottom->addStretch( 1 );
	bottom->addWidget( ok_btn );
	bottom->addWidget( cancel_btn );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( okPushed() ) );
	connect( cancel_btn, SIGNAL( clicked() ), 
		this, SLOT( cancelPushed() ) );
	connect( help_btn, SIGNAL( clicked() ), this, SLOT( helpPushed() ) );

	item_list = new QListWidget;
	item_list->addItems( items );
	connect( item_list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( listClicked(QListWidgetItem *) ) );

	full->addLayout( hbox );
	full->addWidget( new QLabel( QUTF8( label ) ), 0 );
	full->addWidget( item_list, 0 );
	full->addLayout( bottom, 0 );

	full->setSizeConstraint( QLayout::SetFixedSize );

	setLayout( full );
}

void PickList::okPushed()
{
	choice = cp_string( item_list->currentItem()->text().toUtf8().data() );
	done( 1 );
}

void PickList::cancelPushed()
{
	done( 0 );
}

void PickList::helpPushed()
{
	global->help->showSource( PickList_help, this );
}

void PickList::listClicked( QListWidgetItem * /* item */ )
{
	const char *val = item_list->currentItem()->text().toUtf8().data();
	if( *val != '<' )
	    ok_btn->setEnabled( true );
}
