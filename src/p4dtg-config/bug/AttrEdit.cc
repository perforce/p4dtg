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
#include <QMenu>
#include <QVBoxLayout>
#include <QStringList>
#include <QMessageBox>
#include <QRegExpValidator>
#include "AttrEdit.h"

AttrEdit::AttrEdit( QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	setWindowModality( Qt::WindowModal );
	setWindowTitle( QString( tr( "Edit Data Source Attributes" ) ) );

    	QVBoxLayout *full = new QVBoxLayout;
    	QHBoxLayout *bottom = new QHBoxLayout;
	help_btn = new QPushButton( tr( "&Help" ) );
	ok_btn = new QPushButton( tr( "&OK" ) );
	cancel_btn = new QPushButton( tr( "Cancel" ) );
	bottom->addWidget( help_btn );
	bottom->addStretch( 1 );
	bottom->addWidget( ok_btn );
	bottom->addWidget( cancel_btn );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( okPushed() ) );
	connect( cancel_btn, SIGNAL( clicked() ), 
		this, SLOT( cancelPushed() ) );

	full->addLayout( bottom, 0 );

	full->setSizeConstraint( QLayout::SetFixedSize );
	setLayout( full );
}

void AttrEdit::okPushed()
{
	QMessageBox::warning( this,
                                QString( "Attribute Validation Error" ),
                                "You have an error" );
	return;
}

void AttrEdit::cancelPushed()
{
	done( 0 );
}
