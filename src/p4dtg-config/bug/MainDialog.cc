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
#include "MainDialog.h"
#include "AttrEdit.h"

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

	QVBoxLayout *page = new QVBoxLayout;
	page->addLayout( bottom );

	setMinimumWidth( 500 );
	setLayout( page );
}

void
MainDialog::okPushed()
{
	qApp->quit();
}

void
MainDialog::applyPushed()
{
}

void
MainDialog::cancelPushed()
{
	qApp->quit();
}
