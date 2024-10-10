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
#include "FixMap.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataSource.h>
#include <DataMapping.h>
#include <DTG-interface.h>

FixMap::FixMap( int edit, DataMapping *map, FixRule *fr, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	my_fr = fr;
	my_map = map;

#ifndef OS_MACOSX
	setWindowModality( Qt::WindowModal );
#endif
	setWindowTitle( QString( tr( "Select Fix Details to Map" ) ) );

    	QVBoxLayout *full = new QVBoxLayout;
    	QHBoxLayout *bottom = new QHBoxLayout;
	help_btn = new QPushButton( tr( "&Help" ) );
	ok_btn = new QPushButton( tr( "OK" ) );
	ok_btn->setEnabled( !edit );
	cancel_btn = new QPushButton( tr( "Cancel" ) );
	bottom->addWidget( help_btn );
	bottom->addStretch( 1 );
	bottom->addWidget( ok_btn );
	bottom->addWidget( cancel_btn );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( okPushed() ) );
	connect( cancel_btn, SIGNAL( clicked() ), 
		this, SLOT( cancelPushed() ) );
	connect( help_btn, SIGNAL( clicked() ), this, SLOT( helpPushed() ) );

	QLabel *label = new QLabel( tr( "Target Defect tracker Field:" ) );
	QLineEdit *field = new QLineEdit( QUTF8( fr->dts_field ) );
	field->setReadOnly( true );
	field->setFrame( false );
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget( label );
	hbox->addWidget( field, 1 );

	QGroupBox *box = new QGroupBox( 
		tr( "Fix details to map" ) );
	QVBoxLayout *checks = new QVBoxLayout;

	list_check = new QCheckBox( tr( "List of changed files" ) );
	if( fr->file_list )
	    list_check->setCheckState( Qt::Checked );
	else
	    list_check->setCheckState( Qt::Unchecked );
	connect( list_check, SIGNAL( stateChanged(int) ), 
		this, SLOT( checked(int) ) );

	change_check = new QCheckBox( tr( "Change number" ) );
	if( fr->change_number )
	    change_check->setCheckState( Qt::Checked );
	else
	    change_check->setCheckState( Qt::Unchecked );
	connect( change_check, SIGNAL( stateChanged(int) ), 
		this, SLOT( checked(int) ) );

	by_check = new QCheckBox( tr( "Fixed by" ) );
	if( fr->fixed_by )
	    by_check->setCheckState( Qt::Checked );
	else
	    by_check->setCheckState( Qt::Unchecked );
	connect( by_check, SIGNAL( stateChanged(int) ), 
		this, SLOT( checked(int) ) );

	date_check = new QCheckBox( tr( "Fixed date" ) );
	if( fr->fixed_date )
	    date_check->setCheckState( Qt::Checked );
	else
	    date_check->setCheckState( Qt::Unchecked );
	connect( date_check, SIGNAL( stateChanged(int) ), 
		this, SLOT( checked(int) ) );

	desc_check = new QCheckBox( tr( "Fix description" ) );
	if( fr->description )
	    desc_check->setCheckState( Qt::Checked );
	else
	    desc_check->setCheckState( Qt::Unchecked );
	connect( desc_check, SIGNAL( stateChanged(int) ), 
		this, SLOT( checked(int) ) );

	checks->addWidget( change_check );
	checks->addWidget( by_check );
	checks->addWidget( date_check );
	checks->addWidget( desc_check );
	checks->addWidget( list_check );

	box->setLayout( checks );

	full->addLayout( hbox );
	full->addWidget( box, 0 );
	full->addLayout( bottom, 0 );

	full->setSizeConstraint( QLayout::SetFixedSize );

	setLayout( full );
}

void FixMap::okPushed()
{
	my_fr->description = desc_check->checkState() == Qt::Checked;
	my_fr->fixed_by = by_check->checkState() == Qt::Checked;
	my_fr->fixed_date = date_check->checkState() == Qt::Checked;
	my_fr->file_list = list_check->checkState() == Qt::Checked;
	my_fr->change_number = change_check->checkState() == Qt::Checked;
	done( 1 );
}

void FixMap::cancelPushed()
{
	done( 0 );
}

void FixMap::helpPushed()
{
	global->help->showSource( FixMap_help, this );
}

void FixMap::checked( int /* s */ )
{
	ok_btn->setEnabled( true );
}
