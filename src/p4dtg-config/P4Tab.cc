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
#include <QMessageBox>
#include <QVBoxLayout>
#include "P4Tab.h"
#include "P4Edit.h"
#include "P4DTG.h"
#include "MainDialog.h"
#include "ListSource.h"
#include <DataSource.h>

void P4Tab::refreshList()
{
	int row = list->currentRow();
	list->clear();
	QListWidgetItem *item, *subitem;
	QVariant item_tag;
	DataSource *prev = NULL;
	for( DataSource *src = global->p4_srcs; src; src = src->next )
	{
	    QString tag = QString( QUTF8( src->nickname ) ) + 
			QString( QUTF8( " : " ) ) +
			QString( QUTF8( src->server ) );
	    if( src->deleted )
	        tag.append( QUTF8( " [deleted]" ) );
	    item = new QListWidgetItem( tag, list );
	    item_tag.setValue( (void *)(new ListSource( src, NULL, prev ) ) );
	    item->setData( Qt::UserRole, item_tag );
	    if( src->dirty )
	    {
	        QFont font = item->font();
	        font.setBold( true );
	        item->setFont( font );
	    }

	    for( FilterSet *set = src->set_list; set; set = set->next )
	    {
	        tag = QString( QUTF8( "  -> " ) ) + 
			QString( QUTF8( set->name ) ) + 
			QString( QUTF8( ": " ) ) + 
			QString( QUTF8( set->filter_list->field ) ) + 
			QString( QUTF8( "=" ) );
	        for( FilterRule *rule = set->filter_list; 
			rule; 
			rule = rule->next )
	        {
	            tag += QString( QUTF8( rule->pattern ) );
	            if( rule->next )
	                tag += QString( QUTF8( ", " ) );
	        }

	        subitem = new QListWidgetItem( tag, list );
	        item_tag.setValue( 
			(void *)(new ListSource( src, set, prev, item ) ) );
	        subitem->setData( Qt::UserRole, item_tag );
	        if( src->dirty )
	        {
	            QFont font = subitem->font();
	            font.setBold( true );
	            subitem->setFont( font );
	        }
	    }

	    prev = src;
	}
	list->setCurrentRow( row );
	listClicked( list->item( row ) );
}

P4Tab::P4Tab( MainDialog *my_base, QWidget *parent )
    : QWidget( parent )
{
	base = my_base;
	QVBoxLayout *buttons = new QVBoxLayout;
	edit_btn = new QPushButton( tr( "&Edit..." ) );
	edit_btn->setToolTip( QString( 
		tr( "Edit the selected Perforce Server Source" ) ) );
	new_btn = new QPushButton( tr( "&New..." ) );
	new_btn->setToolTip( QString( 
		tr( "Define a new Perforce Server Source" ) ) );
	delete_btn = new QPushButton( tr( "&Delete" ) );
	delete_btn->setToolTip( QString( 
		tr( "Delete the selected Perforce Server Source" ) ) );
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
	list->setSelectionMode( QAbstractItemView::SingleSelection );
	list_buttons->addWidget( list, 1 ); // stretch this
	list_buttons->addLayout( buttons, 0 );
	connect( list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( listClicked(QListWidgetItem *) ) );
	connect( list, SIGNAL( currentRowChanged(int) ),
		this, SLOT( rowChanged(int) ) );

    	QVBoxLayout *full = new QVBoxLayout;
	QLabel *label = new QLabel( tr( "Perforce server sources:" ) );
	full->addWidget( label, 0 );
	full->addLayout( list_buttons, 1 ); // Stretch this

	QFrame *line = new QFrame();
	line->setFrameShape( QFrame::HLine );
	full->addWidget( line );

	QHBoxLayout *hbox = new QHBoxLayout;
	label = new QLabel( tr( "Name:" ) );
	nickname_edit = new QLineEdit( QString( QUTF8( "Beta Toolbox" ) ) );;
	nickname_edit->setReadOnly( true );
	nickname_edit->setFrame( false );
	hbox->addWidget( label, 0 );
	hbox->addWidget( nickname_edit, 0 );
	hbox->addStretch( 1 );
	full->addLayout( hbox, 0 );

	QGroupBox *box = new QGroupBox( tr( "Server connection details" ) );
	QGridLayout *grid = new QGridLayout;
	label = new QLabel( tr( "Server:" ) );
	server_edit = new QLineEdit( QString( QUTF8( "computer:2345" ) ) );
	server_edit->setReadOnly( true );
	server_edit->setFrame( false );
	grid->addWidget( label, 0, 0 );
	grid->addWidget( server_edit, 0, 1 );
	label = new QLabel( tr( "User name:" ) );
	user_edit = new QLineEdit( QString( QUTF8( "admin" ) ) );
	user_edit->setReadOnly( true );
	user_edit->setFrame( false );
	grid->addWidget( label, 1, 0 );
	grid->addWidget( user_edit, 1, 1 );
	label = new QLabel( tr( "Password:" ) );
	password_edit = new QLineEdit( QString( QUTF8( "" ) ) );
	password_edit->setEchoMode( QLineEdit::PasswordEchoOnEdit );
	password_edit->setReadOnly( true );
	password_edit->setFrame( false );
	grid->addWidget( label, 2, 0 );
	grid->addWidget( password_edit, 2, 1 );
	box->setLayout( grid );
	full->addWidget( box, 0 );

	hbox = new QHBoxLayout;
	label = new QLabel( tr( "Server status:" ) );
	hbox->addWidget( label, 0 );
 	status_edit = new QTextEdit( QString( QUTF8( 
		"This server IS current configured for use with P4DTG" ) ) );
	status_edit->setReadOnly( true );
	status_edit->setFrameShadow( QFrame::Plain );
	status_edit->setFrameShape( QFrame::NoFrame );
	status_edit->setFixedHeight( 40 );
	hbox->addWidget( status_edit, 1 );
	full->addLayout( hbox, 0 );
 
	box = new QGroupBox( tr( "Reference fields" ) );
	grid = new QGridLayout;
	label = new QLabel( tr( "Modified Date field:" ) );
	moddate_edit = new QLineEdit( QString( QUTF8( "ModifiedDate" ) ) );
	moddate_edit->setReadOnly( true );
	moddate_edit->setFrame( false );
	grid->addWidget( label, 0, 0 );
	grid->addWidget( moddate_edit, 0, 1 );

	label = new QLabel( tr( "Modified By field:" ) );
	moduser_edit = new QLineEdit( QString( QUTF8( "ModifiedBy" ) ) );
	moduser_edit->setReadOnly( true );
	moduser_edit->setFrame( false );
	grid->addWidget( label, 1, 0 );
	grid->addWidget( moduser_edit, 1, 1 );
	box->setLayout( grid );
	full->addWidget( box, 0 );

	displayObject( NULL );
	refreshList();
	edit_btn->setEnabled( false );
	delete_btn->setEnabled( false );

	setLayout( full );

	if( list->count() )
	{
	    list->setCurrentRow( 0 );
	    listClicked( list->item( 0 ) );
	}
}

void P4Tab::displayObject( DataSource *src )
{
	if( src != NULL )
	{
	    nickname_edit->setText( QUTF8( SAFEGET(src, nickname) ) );
	    server_edit->setText( QUTF8( SAFEGET(src, server) ) );
	    user_edit->setText( QUTF8( SAFEGET(src, user) ) );
	    password_edit->setText( QUTF8( SAFEGET(src, password) ) );
	    status_edit->clear();
	    set_server_status( src, status_edit, NULL );
	    if( src->error )
	        status_edit->append( QString( QUTF8( " " ) ) +
				QString( QUTF8( src->error ) ) );
	    if( src->warnings )
	        status_edit->append( QString( QUTF8( " " ) ) +
				QString( QUTF8( src->warnings ) ) );
	    moddate_edit->setText( QUTF8( SAFEGET(src, moddate_field) ) );
	    moduser_edit->setText( QUTF8( SAFEGET(src, moduser_field) ) );
	    edit_btn->setEnabled( true );
	    delete_btn->setEnabled( true );
	}
	else
	{
	    nickname_edit->setText( QUTF8( "" ) );
	    server_edit->setText( QUTF8( "" ) );
	    user_edit->setText( QUTF8( "" ) );
	    password_edit->setText( QUTF8( "" ) );
	    status_edit->clear();
	    status_edit->append( 
			QString( tr( "No server is defined" ) ) );
	    moddate_edit->setText( QUTF8( "" ) );
	    moduser_edit->setText( QUTF8( "" ) );
	    edit_btn->setEnabled( false );
	    delete_btn->setEnabled( false );
	}
}

void P4Tab::editPushed()
{
	ListSource *ls = ListSource::find_source( list );
	DataSource *prev = ls ? ls->prev : NULL;
	DataSource *src = ls ? ls->src : NULL;
	if( src )
	{
	    DataSource *tmp = src->copy( 1 );
	    P4Edit *edit = new P4Edit( tmp, this );
	    int result = edit->exec();
	    if( result )
	    {
	        if( prev )
	            prev->next = tmp;
	        else
	            global->p4_srcs = tmp;
	        tmp->next = src->next;
	        src->next = NULL;
	        global->replace_src( src, tmp );
	        delete src;
	        tmp->dirty = 1;
	        base->enable();
	        refreshList();
	    }
	    else
	        delete tmp;
	    delete edit;
	}
}

void P4Tab::newPushed()
{
	DataSource *src = new DataSource( 
		DataSource::SCM, 
		"Perforce Jobs", 
		global->p4plugin );
	P4Edit *edit = new P4Edit( src, this );
	int result = edit->exec();
	if( result )
	{
	    src->next = global->p4_srcs;
	    global->p4_srcs = src;
	    src->dirty = 1;
	    base->enable();
	    refreshList();
	}
	else
	    delete src;
	delete edit;
}

void P4Tab::deletePushed()
{
	ListSource *ls = ListSource::find_source( list );
	DataSource *src = ls ? ls->src : NULL;
	if( src )
	{
	    if( src->deleted )
	    {
	        src->deleted = 0;
	        src->dirty = 1;
	    }
	    else if( src->refcnt )
	    {
	        QMessageBox::information( this, 
			QUTF8( "Perforce Defect Tracking Gateway" ), 
	QUTF8( "This source cannot be deleted since it is currently\n"
	"used by one of the Gateway Mappings.\n" ) );
	    }
	    else
	    {
	        src->deleted = 1;
	        list->setCurrentRow( -1 );
	        src->dirty = 1;
	    }
	    base->enable();
	    refreshList();
	}
}

void P4Tab::rowChanged( int /* row */ )
{
	listClicked( list->currentItem() );
}

void P4Tab::listClicked( QListWidgetItem * /* item */ )
{
	ListSource *ls = ListSource::find_source( list );
	DataSource *src = NULL;
	if( ls )
	{
	    /* SubItems are redirects to Parent */
	    if( ls->parent )
	    {
	        list->setCurrentItem( ls->parent );
	        return;
	    }
	    src = ls->src;
	}
	if( src )
	{
	    displayObject( src );
	    if( src->deleted )
	    {
	        delete_btn->setText( tr( "Un&delete" ) );
	        edit_btn->setEnabled( false );
	    }
	    else
	    {
	        delete_btn->setText( tr( "&Delete" ) );
	        edit_btn->setEnabled( true );
	    }
	    QList<QListWidgetItem *>items = list->selectedItems();
	    const QListWidgetItem *selected = 
			items.isEmpty() ? NULL : items.first();
	    const QListWidgetItem *current = list->currentItem();
	    delete_btn->setEnabled( selected == current );
	}
	else
	{
	    displayObject( NULL );
	    delete_btn->setText( tr( "&Delete" ) );
	    delete_btn->setEnabled( false );
	    edit_btn->setEnabled( false );
	}
}
