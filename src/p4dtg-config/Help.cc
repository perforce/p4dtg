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

#include <stdio.h>
#include <QTextBrowser>
#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "P4DTG.h"
#include <QPushButton>
#include "Help.h"

P4Help::P4Help( const char *helpdir, const char *baseurl ) : 
		QDialog( 0, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	my_x = my_y = -1;
	setWindowTitle( QString( tr( 
		"Perforce Defect Tracking Gateway Configuration Help" ) ) );
    	QVBoxLayout *full = new QVBoxLayout;
    	QHBoxLayout *bottom = new QHBoxLayout;
	QPushButton *btn = new QPushButton( tr( "&Close" ) );
	bottom->addStretch( 1 );
	bottom->addWidget( btn );
	connect( btn, SIGNAL( clicked() ), this, SLOT( closePushed() ) );

	browser = new QTextBrowser( this );
	full->addWidget( browser, 1 );
	full->addLayout( bottom );
	QUrl url( QUTF8( baseurl ) );
	browser->setSearchPaths( QStringList( QString( QUTF8( helpdir ) ) ) );
	browser->setSource( url );

	setLayout( full );
}

P4Help::~P4Help()
{
	delete browser;
}

void P4Help::showSource( const char *baseurl, QWidget *caller )
{
	QUrl url( QUTF8( baseurl ) );
	browser->setSource( url );
	raise();
	activateWindow();
	if( isHidden() )
	{
	    if( my_x < 0 )
	    {
	        resize( 550, 660 );
	        move( caller->pos() );
	        show();
	    }
	    else
	    {
	        resize( my_width, my_height );
	        show();
	        move( my_x, my_y );
	    }
	}
}

void P4Help::closePushed()
{
	my_x = pos().x();
	my_y = pos().y();
	my_width = size().width();
	my_height = size().height();

	hide();
}

