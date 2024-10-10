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
#include <QLineEdit>
#include <QMessageBox>
#include <QRegExpValidator>
#include "SelectMap.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataSource.h>
#include <DataMapping.h>
#include <DTG-interface.h>
extern "C" {
#include <dtg-utils.h>
}

static struct DTGFieldDesc *
find_field_desc( struct DTGFieldDesc *desc, const char *field )
{
	if( !desc )
	    return NULL;
	while( desc && strcmp( desc->name, field ) )
	    desc = desc->next;
	return desc;
}

static void clean_selects( FilterRule *filters, struct DTGFieldDesc *fields )
{
	if( !filters || !filters->field )
	    return;

	struct DTGFieldDesc *fd = 
		find_field_desc( fields, filters->field );
	if( !fd )
	    return;

	struct DTGStrList *list = NULL;
	for( FilterRule *rule = filters; 
		rule; 
		rule = rule->next )
	    list = append_DTGStrList( list, rule->pattern );
	delete_DTGStrList( fd->select_values );
	fd->select_values = list;
}

SelectMap::SelectMap( int dir, DataMapping *map, CopyRule *cr, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	my_dir = dir;
	my_cr = cr;
	my_map = map;

	scm_fields = copy_DTGFieldDesc( my_map->scm->get_fields() );
	clean_selects( my_map->scm_filters, scm_fields );
	dts_fields = copy_DTGFieldDesc( my_map->dts->get_fields() );
	clean_selects( my_map->dts_filters, dts_fields );

#ifndef OS_MACOSX
	setWindowModality( Qt::WindowModal );
#endif
	setWindowTitle( QString( tr( "Define Select Value Mapping" ) ) );

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

	QGroupBox *box = new QGroupBox( 
		tr( "Select Values for Fields" ) );


	QLabel *label1 = new QLabel( tr( "Perforce source field:" ) );
	QLineEdit *field = new QLineEdit( QUTF8( cr->scm_field ) );
	field->setReadOnly( true );
	field->setFrame( false );
	QLabel *label2 = new QLabel( tr( "Field values:" ) );
	p4val_list = new QListWidget;
	p4val_list->setSelectionMode( QAbstractItemView::SingleSelection );
	connect( p4val_list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( p4valClicked(QListWidgetItem *) ) );
	struct DTGFieldDesc *fd = find_field_desc( scm_fields, cr->scm_field );
	QListWidgetItem *line;
	if( fd )
	    for( struct DTGStrList *item = fd->select_values;
		item;
		item = item->next )
	    {
	        if( *item->value )
		    line = new QListWidgetItem( QString( QUTF8( item->value)));
	        else
		    line = new QListWidgetItem( QString( QUTF8( "<empty>" ) ) );
	        QFont font = line->font();
	        font.setBold( true );
	        if( !*item->value )
	            font.setItalic( true );
	        line->setFont( font );
	        p4val_list->addItem( line );
	    }
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget( label1 );
	hbox->addWidget( field, 1 );
	QVBoxLayout *p4vbox = new QVBoxLayout;
	p4vbox->addLayout( hbox );
	p4vbox->addWidget( label2 );
	p4vbox->addWidget( p4val_list, 1 );

	label1 = new QLabel( tr( "Defect tracking source field:" ) );
	field = new QLineEdit( QUTF8( cr->dts_field ) );
	field->setReadOnly( true );
	field->setFrame( false );
	label2 = new QLabel( tr( "Field values:" ) );
	dtval_list = new QListWidget;
	dtval_list->setSelectionMode( QAbstractItemView::SingleSelection );
	connect( dtval_list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( dtvalClicked(QListWidgetItem *) ) );
	fd = find_field_desc( dts_fields, cr->dts_field );
	if( fd )
	    for( struct DTGStrList *item = fd->select_values;
		item;
		item = item->next )
	    {
	        if( *item->value )
		    line = new QListWidgetItem( QString( QUTF8( item->value)));
	        else
		    line = new QListWidgetItem( QString( QUTF8( "<empty>" ) ) );
	        QFont font = line->font();
	        font.setBold( true );
	        if( !*item->value )
	            font.setItalic( true );
	        line->setFont( font );
	        dtval_list->addItem( line );
	    }
	hbox = new QHBoxLayout;
	hbox->addWidget( label1 );
	hbox->addWidget( field, 1 );
	QVBoxLayout *dtvbox = new QVBoxLayout;
	dtvbox->addLayout( hbox );
	dtvbox->addWidget( label2 );
	dtvbox->addWidget( dtval_list, 1 );

	hbox = new QHBoxLayout;
	switch( my_dir )
	{
	default:
	case 0: // mirror
	    left_list = dtval_list;
	    right_list = p4val_list;
	    hbox->addLayout( dtvbox, 1 );
	    map_btn = 
		new QPushButton( L_ARROW + tr( " &Map with " ) + R_ARROW );
	    hbox->addWidget( map_btn );
	    hbox->addLayout( p4vbox, 1 );
	    copy_op = L_ARROW + R_ARROW;
	    break;
	case 1: // dts -> p4
	    left_list = dtval_list;
	    right_list = p4val_list;
	    hbox->addLayout( dtvbox, 1 );
	    map_btn = 
		new QPushButton( R_ARROW + tr( " &Map to " ) + R_ARROW );
	    hbox->addWidget( map_btn );
	    hbox->addLayout( p4vbox, 1 );
	    copy_op = R_ARROW;
	    break;
	case 2: // p4 -> dts
	    left_list = p4val_list;
	    right_list = dtval_list;
	    hbox->addLayout( p4vbox, 1 );
	    map_btn = 
		new QPushButton( R_ARROW + tr( " &Map to " ) + R_ARROW );
	    hbox->addWidget( map_btn );
	    hbox->addLayout( dtvbox, 1 );
	    copy_op = R_ARROW;
	    break;
	}
	map_btn->setEnabled( false );
	connect( map_btn, SIGNAL( clicked() ), 
		this, SLOT( mapPushed() ) );

	box->setLayout( hbox );

	label1 = new QLabel( tr( "Select value mappings:" ) );
	map_list = new QListWidget;
	map_list->setSelectionMode( QAbstractItemView::SingleSelection );
	connect( map_list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( mapClicked(QListWidgetItem *) ) );
	for( CopyMap *mr = cr->mappings; mr; mr = mr->next )
	{
	    QList<QListWidgetItem *>v1;
	    QList<QListWidgetItem *>v2;
	    if( my_dir < 2 )
	    {
	        v1 = dtval_list->findItems( 
			QString( QUTF8( mr->value1 ) ), Qt::MatchFixedString );
	        v2 = p4val_list->findItems( 
			QString( QUTF8( mr->value2 ) ), Qt::MatchFixedString );
	    }
	    else
	    {
	        v1 = p4val_list->findItems( 
			QString( QUTF8( mr->value1 ) ), Qt::MatchFixedString );
	        v2 = dtval_list->findItems( 
			QString( QUTF8( mr->value2 ) ), Qt::MatchFixedString );
	    }
	    if( !v1.isEmpty() && !v2.isEmpty() )
	    {
	        for( int i=0; i < v1.size(); i++ )
	        {
	            QListWidgetItem *v = v1[i];
	            QFont font = v->font();
	            font.setBold( false );
	            v->setFont( font );
	        }
	        if( !my_dir )
	            for( int j=0; j < v2.size(); j++ )
	            {
	                QListWidgetItem *v = v2[j];
	                QFont font = v->font();
	                font.setBold( false );
	                v->setFont( font );
	            }
	    }
	    else
	        qDebug() << "fields not found";

	    QString item;
	    if( *mr->value1 )
	        item = QUTF8( mr->value1 );
	    else
	        item = QUTF8( "<empty>" );
	    item += QUTF8( " " );
	    item += copy_op;
	    item += QUTF8( " " );
	    if( *mr->value2 )
	        item += QUTF8( mr->value2 );
	    else
	        item += QUTF8( "<empty>" );

	    QStringList list;
	    list << QUTF8( mr->value1 );
	    list << QUTF8( mr->value2 );
	    if( v1.isEmpty() || v2.isEmpty() )
	        item += QUTF8( " (Error)" );
	    QVariant val( list );
	    QListWidgetItem *v = new QListWidgetItem( item );
	    v->setData( Qt::UserRole, val );
	    map_list->addItem( v );
	}
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget( label1 );
	vbox->addWidget( map_list, 1 );

	hbox = new QHBoxLayout;
	hbox->addLayout( vbox );

	vbox = new QVBoxLayout;
	del_btn = new QPushButton( tr( "&Unmap" ) );
	del_btn->setEnabled( false );
	connect( del_btn, SIGNAL( clicked() ), 
		this, SLOT( deletePushed() ) );
	delall_btn = new QPushButton( tr( "Unmap &All" ) );
	connect( delall_btn, SIGNAL( clicked() ), 
		this, SLOT( delallPushed() ) );
	vbox->addStretch( 1 );
	vbox->addWidget( del_btn );
	vbox->addStretch( 1 );
	vbox->addWidget( delall_btn );
	vbox->addStretch( 1 );
	hbox->addLayout( vbox );
	
	full->addWidget( box, 0 );
	full->addLayout( hbox, 0 );
	full->addLayout( bottom, 0 );

	setLayout( full );
}

void SelectMap::okPushed()
{
	int all_mapped = 1;
	int row = 0;
	QListWidgetItem *item;
	while( (item = left_list->item( row++ ) ) )
	    if( item->font().bold() )
	    {
	        all_mapped = 0;
	        break;
	    }
	if( all_mapped )
	{
	    my_cr->copy_type = CopyRule::MAP;
	    if( scm_fields ) delete_DTGFieldDesc( scm_fields );
	    if( dts_fields ) delete_DTGFieldDesc( dts_fields );
	    done( 1 );
	    return;
	}
	my_cr->copy_type = CopyRule::UNMAP;

	int abort_it = QMessageBox::warning( this,
		QString( QUTF8( "Select Field Mapping" ) ),
		QString( QUTF8( 
		"All fields in the left column must be mapped\n"
		"prior to starting the Replication engine.\n\n"
		"Would you like to continue mapping?" ) ),
		QString( QUTF8( "Yes" ) ),
		QString( QUTF8( "No" ) ) );
	if( !abort_it )
	    return;

	if( scm_fields ) delete_DTGFieldDesc( scm_fields );
	if( dts_fields ) delete_DTGFieldDesc( dts_fields );
	done( 1 );
}

void SelectMap::cancelPushed()
{
	if( scm_fields ) delete_DTGFieldDesc( scm_fields );
	if( dts_fields ) delete_DTGFieldDesc( dts_fields );
	done( 0 );
}

void SelectMap::helpPushed()
{
	global->help->showSource( SelectMap_help, this );
}

void SelectMap::mapPushed()
{
	QListWidgetItem *dt = dtval_list->currentItem();
	QListWidgetItem *p4 = p4val_list->currentItem();
	if( dt && p4 )
	{
	    CopyMap *nm = new CopyMap;
	    QList<QListWidgetItem *>v1;
	    QList<QListWidgetItem *>v2;
	    if( my_dir < 2 ) 
	    {
	        v1 = dtval_list->findItems( dt->text(), Qt::MatchFixedString );
	        v2 = p4val_list->findItems( p4->text(), Qt::MatchFixedString );
	    }
	    else
	    {
	        v1 = p4val_list->findItems( p4->text(), Qt::MatchFixedString );
	        v2 = dtval_list->findItems( dt->text(), Qt::MatchFixedString );
	    }
	    int i;
	    QListWidgetItem *v;
	    QFont font;
	    for( i=0; i < v1.size(); i++ )
	    {
	        v = v1[i];
	        font = v->font();
	        font.setBold( false );
	        v->setFont( font );
	        if( !nm->value1 )
	            if( font.italic() )
	                nm->value1 = cp_string( "" );
	            else
	                nm->value1 = cp_string( v->text().toUtf8().data() );
	    }
	    for( i=0; i < v2.size(); i++ )
	    {
	        v = v2[i];
	        font = v->font();
	        if( !my_dir )
	        {
	            font.setBold( false );
	            v->setFont( font );
	        }
	        if( !nm->value2 )
	            if( font.italic() )
	                nm->value2 = cp_string( "" );
	            else
	                nm->value2 = cp_string( v->text().toUtf8().data() );
	    }

	    nm->next = my_cr->mappings;
	    my_cr->mappings = nm;

	    QString item;
	    if( *nm->value1 )
	        item = QUTF8( nm->value1 );
	    else
	        item = QUTF8( "<empty>" );
	    item += QUTF8( " " );
	    item += copy_op;
	    item += QUTF8( " " );
	    if( *nm->value2 )
	        item += QUTF8( nm->value2 );
	    else
	        item += QUTF8( "<empty>" );
	    QStringList list;
	    list << QUTF8( nm->value1 );
	    list << QUTF8( nm->value2 );
	    QVariant val( list );
	    v = new QListWidgetItem( item );
	    v->setData( Qt::UserRole, val );
	    map_list->addItem( v );
	    ok_btn->setEnabled( true );
	    p4valClicked( p4 );
	}
}

void SelectMap::deletePushed()
{
	int num = map_list->count();
	QListWidgetItem *item = map_list->currentItem();
	QStringList list = item->data( Qt::UserRole ).toStringList();
	map_list->takeItem( map_list->row( item ) );
	CopyMap *cm;
	if( my_cr->mappings && 
	    list[0] == QString( QUTF8( my_cr->mappings->value1 ) ) &&
	    list[1] == QString( QUTF8( my_cr->mappings->value2 ) ) )
	{
	    cm = my_cr->mappings;
	    my_cr->mappings = cm->next;
	    cm->next = NULL;
	    delete cm;
	}
	else
	{
	    for( cm = my_cr->mappings; cm && cm->next; cm = cm->next )
	        if( list[0] == QString( QUTF8( cm->next->value1 ) ) && 
		    list[1] == QString( QUTF8( cm->next->value2 ) ) )
	            break;
	    if( cm && cm->next )
	    {
	        CopyMap *tmp = cm->next;
	        cm->next = cm->next->next;
	        tmp->next = NULL;
	        delete tmp;
	    }
	    else
	        qDebug() << "Oops, not found";
	}
	QList<QListWidgetItem *>value = 
		// left_list->findItems( list[0].toUtf8().data(), 
		left_list->findItems( list[0],
					Qt::MatchFixedString );
	if( !value.isEmpty() )
	    for( int i=0; i < value.size(); i++ )
	    {
	        QListWidgetItem *v = value[i];
	        QFont font = v->font();
	        font.setBold( true );
	        v->setFont( font );
	    }
	value.clear();
	value = 
	    //right_list->findItems( list[1].toUtf8().data(), 
	    right_list->findItems( list[1],
					Qt::MatchFixedString );
	if( !value.isEmpty() )
	    for( int i=0; i < value.size(); i++ )
	    {
	        QListWidgetItem *v = value[i];
	        QFont font = v->font();
	        font.setBold( true );
	        v->setFont( font );
	    }
	ok_btn->setEnabled( true );
	if( num < 2 )
	    del_btn->setEnabled( false );
}

void SelectMap::delallPushed()
{
	map_list->clear();
	if( my_cr->mappings )
	    delete my_cr->mappings;
	my_cr->mappings = NULL;
	ok_btn->setEnabled( true );
	del_btn->setEnabled( false );

	int row = 0;
	QListWidgetItem *item;
	while( (item = left_list->item( row++ ) ) )
	{
	    QFont font = item->font();
	    font.setBold( true );
	    item->setFont( font );
	}
	row = 0;
	while( (item = right_list->item( row++ ) ) )
	{
	    QFont font = item->font();
	    font.setBold( true );
	    item->setFont( font );
	}

}

void SelectMap::p4valClicked( QListWidgetItem *item )
{
	QListWidgetItem *other = dtval_list->currentItem();
	if( other && other->font().bold() && item->font().bold() )
	{
	    map_btn->setEnabled( true );
	}
	else
	    map_btn->setEnabled( false );
}

void SelectMap::dtvalClicked( QListWidgetItem *item )
{
	QListWidgetItem *other = p4val_list->currentItem();
	if( other && other->font().bold() && item->font().bold() )
	{
	    map_btn->setEnabled( true );
	}
	else
	    map_btn->setEnabled( false );
}

void SelectMap::mapClicked( QListWidgetItem *item )
{
	if( item )
	    del_btn->setEnabled( true );
	else
	    del_btn->setEnabled( false );
}
