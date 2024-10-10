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
#include "DTSEdit.h"
#include "AttrEdit.h"
#include "SegEdit.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataAttr.h>
#include <DataSource.h>
#include <DTGModule.h>
extern "C" {
#include <dtg-utils.h>
}

void DTSEdit::enable_ok()
{
	if( server_edit->text().isEmpty() || 
		nickname_edit->text().isEmpty() ||
		user_edit->text().isEmpty() ||
		( project_combo &&
		project_combo->currentText() == QUTF8( "*Unknown*" ) ) )
	    ok_btn->setEnabled( false );
	else
	    ok_btn->setEnabled( true );
}

DTSEdit::DTSEdit( DataSource *in_obj, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	attributes_btn = NULL;
	obj = in_obj;
	setWindowModality( Qt::WindowModal );
	int new_dts = !obj->nickname;
	if( new_dts )
	    setWindowTitle( QString( tr( "Add Defect Tracking Source" ) ) );
	else
	    setWindowTitle( QString( tr( "Edit Defect Tracking Source" ) ) );

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
	connect( cancel_btn, SIGNAL( clicked() ), this, 
		SLOT( cancelPushed() ) );
	connect( help_btn, SIGNAL( clicked() ), this, SLOT( helpPushed() ) );

	QHBoxLayout *hbox = new QHBoxLayout;
	QLabel *label = new QLabel( tr( "Name:" ) );
	nickname_edit = new QLineEdit( QUTF8( obj->nickname ) );
	if( !new_dts )
	    nickname_edit->setReadOnly( true );
	QRegExp rx( QUTF8( "[-A-Za-z0-9_]*" ) );
	nickname_edit->setValidator( new QRegExpValidator( rx, this ) );
	QWidget *widget;
	if( !obj->my_mod || new_dts )
	{
	    widget = dtstype_combo = new QComboBox();
	    struct DTGError *err = new_DTGError( NULL );
	    for( DTGModule *mod = global->plugins; mod; mod = mod->next )
	        dtstype_combo->addItem( QUTF8( mod->dt_get_name( err ) ) );
	    delete_DTGError( err );
	    connect( 
		dtstype_combo, SIGNAL( currentIndexChanged(int) ),
		this, SLOT( dtstypeChanged(int) ) );
	    dtstypeChanged(-1);
	}
	else
	{
	    QLineEdit *tmp = new QLineEdit( QUTF8( obj->plugin ) );
	    tmp->setReadOnly( true );
	    tmp->setFrame( false );
	    dtstype_combo = NULL;
	    widget = tmp;
	}
	hbox->addWidget( label, 0 );
	hbox->addWidget( nickname_edit, 0 );
	label = new QLabel( tr( "Type:" ) );
	hbox->addWidget( label, 0 );
	hbox->addWidget( widget, 1 );
	full->addLayout( hbox, 0 );

	box = new QGroupBox( tr( "Server connection details" ) );
	QGridLayout *grid = new QGridLayout;
	label = new QLabel( tr( "Server:" ) );
	server_edit = new QLineEdit( QUTF8( obj->server ) );
	grid->addWidget( label, 0, 0 );
	grid->addWidget( server_edit, 0, 1 );
	label = new QLabel( tr( "User Name:" ) );
	user_edit = new QLineEdit( QUTF8( obj->user ) );
	grid->addWidget( label, 1, 0 );
	grid->addWidget( user_edit, 1, 1 );
	label = new QLabel( tr( "Password:" ) );
	password_edit = new QLineEdit( QUTF8( obj->password ) );
	password_edit->setEchoMode( QLineEdit::PasswordEchoOnEdit );
	grid->addWidget( label, 2, 0 );
	grid->addWidget( password_edit, 2, 1 );
	check_btn = new QPushButton( tr( 
		"&Check connection and retrieve fields" ) );
	grid->addWidget( check_btn, 3, 1 );
	box->setLayout( grid );
	full->addWidget( box, 0 );

	connect( check_btn, SIGNAL( clicked() ), this, SLOT( checkPushed() ) );
	connect( nickname_edit, SIGNAL( textChanged(const QString&) ),
		this, SLOT( nickChanged(const QString&) ) );
	connect( server_edit, SIGNAL( textChanged(const QString&) ),
		this, SLOT( serverChanged(const QString&) ) );
	connect( user_edit, SIGNAL( textChanged(const QString&) ),
		this, SLOT( userChanged(const QString&) ) );
	connect( password_edit, SIGNAL( textChanged(const QString&) ),
		this, SLOT( passChanged(const QString&) ) );

	hbox = new QHBoxLayout;
	label = new QLabel( tr( "Server status:" ) );
	hbox->addWidget( label, 0 );
	status_edit = new QTextEdit( QString( tr( "Server status unknown" ) ) );
	status_edit->setReadOnly( true );
	status_edit->setFrameShadow( QFrame::Plain );
	status_edit->setFrameShape( QFrame::NoFrame );
	status_edit->setFixedHeight( 40 );
	hbox->addWidget( status_edit, 1 );
	full->addLayout( hbox, 0 );

	QHBoxLayout *pbox = new QHBoxLayout;
	label = new QLabel( tr( "For Project:" ) );
	if( new_dts )
	{
	    project_combo = new QComboBox();
	    pbox->addWidget( label, 0 );
	    pbox->addWidget( project_combo, 1 );
	    connect( 
		project_combo, SIGNAL( currentIndexChanged(const QString&) ),
		this, SLOT( projectSwitched(const QString&) ) );
	}
	else
	{
	    QLineEdit *tmp = new QLineEdit( QUTF8( obj->module ) );
	    tmp->setReadOnly( true );
	    tmp->setFrame( false );
	    project_combo = NULL;
	    pbox->addWidget( label, 0 );
	    pbox->addWidget( tmp, 1 );
	}
	full->addLayout( pbox, 0 );

	box = new QGroupBox( tr( "Reference fields" ) );
	grid = new QGridLayout;

	label = new QLabel( tr( "Modified Date field:" ) );
	moddate_combo = new QComboBox();
	moddate_combo->addItem( QUTF8( "*Unknown*" ) );
	grid->addWidget( label, 0, 0 );
	grid->addWidget( moddate_combo, 0, 1 );

	label = new QLabel( tr( "Modified By field:" ) );
	moduser_combo = new QComboBox();
	moduser_combo->addItem( QUTF8( "*Unknown*" ) );
	grid->addWidget( label, 1, 0 );
	grid->addWidget( moduser_combo, 1, 1 );

	connect( moddate_combo, SIGNAL( currentIndexChanged(const QString &) ),
		this, SLOT( moddateSwitched(const QString &) ) );
	connect( moduser_combo, SIGNAL( currentIndexChanged(const QString &) ),
		this, SLOT( moduserSwitched(const QString &) ) );

	box->setLayout( grid );
	full->addWidget( box, 0 );

	hbox = new QHBoxLayout;
	attributes_btn = 
		new QPushButton( tr( "Edit &attributes..." ) );
	hbox->addWidget( attributes_btn, 1 );
	segment_btn = new QPushButton( tr( "Edit &segments..." ) );
	hbox->addWidget( segment_btn, 1 );
	full->addLayout( hbox, 0 );
	connect( attributes_btn, SIGNAL( clicked() ), 
		this, SLOT( attributesPushed() ) );
	connect( segment_btn, SIGNAL( clicked() ), 
		this, SLOT( segmentPushed() ) );

	full->addLayout( bottom, 0 );

	if( new_dts )
	{
	    box->setEnabled( false );
	    project_combo->setEnabled( false );
	}

	full->setSizeConstraint( QLayout::SetFixedSize );
	setLayout( full );

	setup_combos();
	status_edit->clear();
	set_server_status( obj, status_edit, box );
	if( obj->error )
	    status_edit->append( QUTF8( " " ) +
				QUTF8( obj->error ) );
	if( obj->warnings )
	    status_edit->append( QUTF8( " " ) +
				QUTF8( obj->warnings ) );
	ok_btn->setEnabled( false );
	if( server_edit->text().isEmpty() || user_edit->text().isEmpty() )
	    check_btn->setEnabled( false );
	else
	    check_btn->setEnabled( true );

	attributes_btn->setEnabled( obj->get_attributes() );
}

void DTSEdit::setup_combos()
{
	QStringList list;
	moddate_combo->clear();
	moduser_combo->clear();
	if( project_combo )
	{
	    char *saved_mod;
	    if( obj->module )
	        saved_mod = cp_string( obj->module );
	    else
	        saved_mod = NULL;
	    project_combo->clear();
	    possible_modules( obj, list );
	    if( list.isEmpty() )
	        project_combo->addItem( QUTF8( "*Unknown*" ) );
	    else
	    {
	        QStringListIterator i( list );
	        while ( i.hasNext() )
	            project_combo->addItem( i.next() );
	    }
	    list.clear();
	    if( saved_mod )
	    {
	        int which = project_combo->findText( QUTF8( saved_mod ) );
	        project_combo->setCurrentIndex( which > -1 ? which : 0 );
	    }
	}
	possible_moddate( obj, list );
	if( list.isEmpty() )
	{
	    moddate_combo->addItem( QUTF8( "*Unknown*" ) );
	    moduser_combo->addItem( QUTF8( "*Unknown*" ) );
	}
	else
	{
	    QStringListIterator i( list );
	    while ( i.hasNext() )
	        moddate_combo->addItem( i.next() );
	    list.clear();
	    possible_moduser( obj, list );
	    if( list.isEmpty() )
	    {
	        moduser_combo->addItem( QUTF8( "*Not Supported*" ) );
	        moduser_combo->setEnabled( false );
	    }
	    else
	    {
	        moduser_combo->setEnabled( true );
	        QStringListIterator i( list );
	        while ( i.hasNext() )
	            moduser_combo->addItem( i.next() );
	    }
	}
	int which = moddate_combo->findText( QUTF8( obj->moddate_field ) );
	moddate_combo->setCurrentIndex( which > -1 ? which : 0 );
	which = moduser_combo->findText( QUTF8( obj->moduser_field ) );
	moduser_combo->setCurrentIndex( which > -1 ? which : 0 );
	list.clear();
}

void DTSEdit::okPushed()
{
	if( !obj->nickname && 
	    !global->unique_nickname( nickname_edit->text().toUtf8().data() ) )
	{
	    QMessageBox::information( this, 
		QUTF8( "Perforce Defect Tracking Gateway" ), 
		QUTF8( "The name matches an existing DataSource. Please " ) +
		QUTF8( "change it to a unique value.\n" ) );
	    return;
	}
	if( !obj->oldname )
	    obj->oldname = cp_string( obj->nickname );
	SAFESET(obj, nickname, nickname_edit->text().toUtf8().data() );
	SAFESET(obj, server, server_edit->text().toUtf8().data() );
	SAFESET(obj, user, user_edit->text().toUtf8().data() );
	SAFESET(obj, password, password_edit->text().toUtf8().data() );
	if( moddate_combo->currentText() != QUTF8( "*Unknown*" ) )
	    SAFESET(obj, moddate_field, 
		moddate_combo->currentText().toUtf8().data() );
	if( moduser_combo->currentText() != QUTF8( "*Unknown*" ) )
	    SAFESET(obj, moduser_field, 
		moduser_combo->currentText().toUtf8().data() );
	if( project_combo && 
		project_combo->currentText() != QUTF8( "*Unknown*" ) )
	    SAFESET(obj, module, project_combo->currentText().toUtf8().data() );
	done( 1 );
}

void DTSEdit::cancelPushed()
{
	done( 0 );
}

void DTSEdit::helpPushed()
{
	global->help->showSource( DTSEdit_help, this );
}

void DTSEdit::attributesPushed()
{
	DataAttr *in_cur = new DataAttr();
	in_cur->name = strdup( "" );
	in_cur->value = strdup( "" );
	in_cur->next = obj->attrs ? obj->attrs->copy() : NULL;

	AttrEdit *edit = new AttrEdit( obj, NULL, in_cur, this );
	int result = edit->exec();
	if( result )
	{
	    enable_ok();
	    delete obj->attrs;
	    obj->attrs = in_cur->next;
	    in_cur->next = NULL;
	    int no_refresh = QMessageBox::warning( this,
		QUTF8( "Updated Data Source" ),
		QUTF8( "There were changes made to the attributes.\n" ) +
		QUTF8( "\n" ) +
		QUTF8( "Would you like to recheck the connection?" ),
		QUTF8( "Yes" ),
		QUTF8( "No" ) );
	    if( !no_refresh )
	        checkPushed();
	}
	delete in_cur;
	delete edit;
}

void DTSEdit::segmentPushed()
{
	if( obj->status != DataSource::PASS &&
	    obj->status != DataSource::READY )
	{
	    QMessageBox::information( this, 
		QUTF8( "Perforce Defect Tracking Gateway" ), 
		QUTF8( "A valid connection to the Defect Tracking server " ) +
		QUTF8( "is required before editing segments.\n" ) );
	    return;
	}
	if( obj->refcnt && !obj->set_list )
	{
	    QMessageBox::information( this,
	        QUTF8( "Perforce Defect Tracking Gateway" ),
	        QUTF8( "This source cannot be segmented since it is\n" ) +
	        QUTF8( "currently used by one of the Gateway Mappings.\n" ) );
	    return;
	}

	FilterSet *saved = obj->set_list ? obj->set_list->copy( 1 ) : NULL;
	SegmentEdit *edit = new SegmentEdit( nickname_edit->text(), obj, this );
	int result = edit->exec();
	if( result )
	{
	    enable_ok();
	    delete saved;
	}
	else
	{
	    delete obj->set_list;
	    obj->set_list = saved;
	}
	delete edit;
}

void DTSEdit::checkPushed()
{
	enable_ok();
	SAFESET(obj, server, server_edit->text().toUtf8().data() );
	SAFESET(obj, user, user_edit->text().toUtf8().data() );
	SAFESET(obj, password, password_edit->text().toUtf8().data() );
	if( project_combo && 
		project_combo->currentText() != QUTF8( "*Unknown*" ) )
	    SAFESET(obj, module, project_combo->currentText().toUtf8().data() );
	obj->check_connection();
	status_edit->clear();
	setup_combos();
	set_server_status( obj, status_edit, box );
	switch( obj->status )
	{
	default:
	case DataSource::UNKNOWN:
	case DataSource::FAIL: 
	    if( project_combo ) project_combo->setEnabled( false );
	    break;
	case DataSource::PASS:
	case DataSource::READY:
	    if( project_combo ) project_combo->setEnabled( true );
	    break;
	}
	if( obj->error || obj->warnings )
	{
	    if( obj->error )
	        status_edit->append( QUTF8( " " ) +
				QUTF8( obj->error ) );
	    if( obj->warnings )
	        status_edit->append( QUTF8( " " ) +
				QUTF8( obj->warnings ) );
	    QMessageBox::information( this, 
		QUTF8( "Perforce Defect Tracking Gateway" ),
		status_edit->toPlainText() );
	}
}

void DTSEdit::moddateSwitched( const QString & /* text */ )
{
	enable_ok();
}

void DTSEdit::moduserSwitched( const QString & /* text */ )
{
	enable_ok();
}

void DTSEdit::projectSwitched( const QString &text )
{
	if( text != QUTF8( "*Unknown*" ) && !text.isEmpty() )
	{
	    SAFESET(obj, module, text.toUtf8().data() );
	    obj->check_connection();
	}
	enable_ok();
}

void DTSEdit::dtstypeChanged( int i )
{
	DTGModule *mod;
	int first_time = 0;
	if( i < 0 )
	{
	    i = 0;
	    first_time = 1;
	}
	for( mod = global->plugins; mod && i > 0; i--, mod = mod->next ) {};
	if( mod )
	{
	    struct DTGError *err = new_DTGError( NULL );
	    SAFESET(obj, plugin, mod->dt_get_name( err ) );
	    delete_DTGError( err );
	    obj->set_module( mod );
	    if( !first_time )
	        setup_combos();
	}
	
	if( attributes_btn )
	    attributes_btn->setEnabled( obj->get_attributes() );
}

void DTSEdit::nickChanged( const QString &text )
{
	if( !nickname_edit->text().isEmpty() && text != nick )
	    nick = nickname_edit->text();
	enable_ok();
}

void DTSEdit::serverChanged( const QString &text )
{
	if( !server_edit->text().isEmpty() && text != server )
	    server = server_edit->text();
	enable_ok();
	if( server_edit->text().isEmpty() || user_edit->text().isEmpty() )
	    check_btn->setEnabled( false );
	else
	    check_btn->setEnabled( true );
}

void DTSEdit::userChanged( const QString &text )
{
	if( !user_edit->text().isEmpty() && text != user )
	    user = user_edit->text();
	enable_ok();
	if( server_edit->text().isEmpty() || user_edit->text().isEmpty() )
	    check_btn->setEnabled( false );
	else
	    check_btn->setEnabled( true );
}

void DTSEdit::passChanged( const QString &text )
{
	if( text != pass )
	    pass = password_edit->text();
	enable_ok();
}
