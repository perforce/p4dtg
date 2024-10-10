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
#include <QMessageBox>
#include <QInputDialog>
#include <QItemSelectionModel>
#include "SegEdit.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataSource.h>
#include <DTG-interface.h>
#include <genutils.h>

static int field_fixed( FilterSet *sets )
{
	int fixed = 0;
	for( FilterSet *s = sets; !fixed && s; s = s->next )
	    for( FilterRule *r = s->filter_list; !fixed && r; r = r->next )
	        fixed = 1;
	return fixed;
}

SegmentEdit::SegmentEdit( QString nick, DataSource *my_src, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	src = my_src;
	list = NULL;

#ifndef OS_MACOSX
	setWindowModality( Qt::WindowModal );
#endif
	setWindowTitle( QString( tr( "Edit Segments for DataSource" ) ) );

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

	QLabel *label = new QLabel( tr( "For server:" ) );
	QLineEdit *field = new QLineEdit( 
		nick + 
		QString( QUTF8( ": " ) ) + 
		QString( QUTF8( src->server ) ) + 
		QString( QUTF8( " (" ) ) + 
		QString( QUTF8( src->plugin ) ) + 
		QString( QUTF8( ")" ) )
		);
	field->setReadOnly( true );
	field->setFrame( false );
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget( label );
	hbox->addWidget( field, 1 );
	full->addLayout( hbox );

	hbox = new QHBoxLayout;
	label = new QLabel( tr( "Segment using SELECT field:" ) );
	hbox->addWidget( label );
	field_combo = new QComboBox;
	QVariant item_tag;
	int i = 0;
	const char *target;
	if( src->set_list && 
		src->set_list->filter_list && 
		src->set_list->filter_list->field )
	    target = src->set_list->filter_list->field;
	else
	    target = "";
	cur_index = -1;
	cur_field = NULL;
	for( struct DTGFieldDesc *f= src->get_fields(); f; f = f->next )
	    if( !strcasecmp( f->type, "SELECT" ) && 
		strncmp( f->name, "DTGConfig-", 10 ) &&
		strncmp( f->name, "DTGAttribute-", 13 ) )
	    {
	        item_tag.setValue( (void *)f );
	        field_combo->addItem( QString( QUTF8( f->name ) ), item_tag );
	        if( !strcasecmp( target, f->name ) )
	        {
	            cur_index = i;
	            cur_field = f;
	        }
	        i++;
	    }
	if( cur_index < 0 )
	    field_combo->setCurrentIndex( 0 );
	else
	{
	    field_combo->setCurrentIndex( cur_index );
	    field_combo->setEnabled( false );
	}
	hbox->addWidget( field_combo );
	connect( field_combo, SIGNAL( currentIndexChanged(const QString&) ),
		this, SLOT( fieldSwitched(const QString&) ) );
	full->addLayout( hbox );

	QGroupBox *box = new QGroupBox( 
		tr( "Current SELECT value to Segment mappings:" ) );
	QVBoxLayout *vbox = new QVBoxLayout;

	list = new QListWidget;
	list->setSelectionMode( QAbstractItemView::SingleSelection );
	connect( list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( listClicked(QListWidgetItem *) ) );

	vbox->addWidget( list );

	hbox = new QHBoxLayout;
	new_btn = new QPushButton( tr( "New..." ) );
	delete_btn = new QPushButton( tr( "Delete" ) );
	connect( new_btn, SIGNAL( clicked() ), this, SLOT( newPushed() ) );
	connect( delete_btn, SIGNAL( clicked() ), 
		this, SLOT( deletePushed() ) );
	hbox->addWidget( new_btn );
	hbox->addWidget( delete_btn );
	vbox->addLayout( hbox );
	box->setLayout( vbox );
	full->addWidget( box, 0 );

	label = new QLabel( tr( "For the above selected Segment:" ) );
	full->addWidget( label, 0 );

	hbox = new QHBoxLayout;

	vbox = new QVBoxLayout;
	label = new QLabel( tr( "Included options:" ) );
	vbox->addWidget( label, 0 );
	has_list = new QListWidget;
	connect( has_list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( hasClicked(QListWidgetItem *) ) );
	vbox->addWidget( has_list, 1 );
	hbox->addLayout( vbox, 0 );

	vbox = new QVBoxLayout;
	add_btn = new QToolButton;
	add_btn->setArrowType( Qt::LeftArrow );
	vbox->addWidget( add_btn, 0 );
	del_btn = new QToolButton;
	del_btn->setArrowType( Qt::RightArrow );
	vbox->addWidget( del_btn, 0 );
	connect( add_btn, SIGNAL( clicked() ), this, SLOT( addPushed() ) );
	connect( del_btn, SIGNAL( clicked() ), this, SLOT( delPushed() ) );
	hbox->addLayout( vbox, 0 );

	vbox = new QVBoxLayout;
	label = new QLabel( tr( "Unassigned options:" ) );
	vbox->addWidget( label, 0 );
	avail_list = new QListWidget;
	connect( avail_list, SIGNAL( itemPressed(QListWidgetItem *) ),
		this, SLOT( availClicked(QListWidgetItem *) ) );
	vbox->addWidget( avail_list, 1 );
	hbox->addLayout( vbox, 0 );

	full->addLayout( hbox, 0 );

	full->addLayout( bottom, 0 );

	full->setSizeConstraint( QLayout::SetFixedSize );

	setLayout( full );

	fieldSwitched( field_combo->itemText( field_combo->currentIndex() ) );
	populate_lists();
}

void SegmentEdit::okPushed()
{
	/* Purge any empty lists */
	while( src->set_list && !src->set_list->filter_list )
	{
	    FilterSet *tmp = src->set_list;
	    src->set_list = tmp->next;
	    tmp->next = NULL;
	    delete tmp;
	}
	for( FilterSet *cur = src->set_list; cur; cur = cur->next )
	{
	    /* Set the old bit on the filters */
	    for( FilterRule *r = cur->filter_list; r; r = r->next )
	        r->old = 1;
	    while( cur->next && !cur->next->filter_list )
	    {
	        FilterSet *tmp = cur->next;
	        cur->next = tmp->next;
	        tmp->next = NULL;
	        delete tmp;
	    }
	}
	done( 1 );
}

void SegmentEdit::cancelPushed()
{
	done( 0 );
}

void SegmentEdit::helpPushed()
{
	global->help->showSource( SegmentEdit_help, this );
}

void SegmentEdit::newPushed()
{
	bool ok;
	QString name = QInputDialog::getText( this, 
		QString( QUTF8( "New Segment" ) ), 
		QString( QUTF8( "Segment name:" ) ), 
		QLineEdit::Normal, 
		QString( QUTF8( "" ) ), 
		&ok );
	if( !ok || name.isEmpty() )
	    return;

	FilterSet *set = new FilterSet;
	set->name = mk_string( name.toUtf8().data() );
	if( !src->set_list )
	{
	    src->set_list = set;
	}
	else
	{
	    FilterSet *tmp;
	    for( tmp = src->set_list; tmp->next; tmp = tmp->next ) {};
	    tmp->next = set;
	}
	populate_lists();
	list->setCurrentRow( list->count() - 1 );
	has_list->clear(); // Can have no options to begin with
}

void SegmentEdit::deletePushed()
{
	int cur = list->currentRow();
	if( cur < 0 || cur >= list->count() )
	    return;
	QListWidgetItem *item = list->item( cur );
	if( !item )
	    return;

	FilterSet *set = 
		(FilterSet *)(item->data( Qt::UserRole).value<void*>());
	if( set->refcnt )
	{
	    QMessageBox::information( this, 
		QUTF8( "Perforce Defect Tracking Gateway" ), 
		QUTF8( "This segment is currently used in a mapping.\n"
		"Deleting the segment is not allowed." ) );
	    return;
	}

	if( src->set_list == set )
	    src->set_list = set->next;
	else
	{
	    FilterSet *tmp;
	    for( tmp = src->set_list; 
		tmp && tmp->next != set; 
		tmp = tmp->next ) {};
	    if( tmp )
	        tmp->next = set->next;
	    else
	        return;
	}
	set->next = NULL;
	delete set;
	populate_lists();
	ok_btn->setEnabled( true );
}

void SegmentEdit::listClicked( QListWidgetItem * item )
{
	FilterSet *set = 
		(FilterSet *)(item->data( Qt::UserRole).value<void*>());
	if( !set )
	    return;
	struct DTGFieldDesc *field = 
		(struct DTGFieldDesc *) (field_combo->itemData( 
			field_combo->currentIndex() ) ).value<void*>();
	if( !field )
	    return;

	has_list->clear();

	for( FilterRule *r = set->filter_list; r; r = r->next )
	{
	    QListWidgetItem *i = new QListWidgetItem( QUTF8( r->pattern ) );
	    QFont f = i->font();
	    f.setBold( !r->old );
	    i->setFont( f );
	    has_list->addItem( i );
	}
	config_btns();
}

void SegmentEdit::populate_lists()
{
	struct DTGFieldDesc *field = 
		(struct DTGFieldDesc *) (field_combo->itemData( 
			field_combo->currentIndex() ) ).value<void*>();
	if( !field )
	{
	    list->clear();
	    avail_list->clear();
	    has_list->clear();
	    config_btns();
	    return;
	}

	list->clear();
	int x = 0;
	int cur = list->currentRow();
	if( cur < 0 )
	    cur = 0;
	FilterSet *cur_set = src->set_list;
	for( FilterSet *s = src->set_list; s; x++, s = s->next )
	{
	    QString tag = QString( QUTF8( s->name ) ) + QString( QUTF8( ": "));
	    for( FilterRule *r = s->filter_list; r; r = r->next )
	    {
	        tag += QString( QUTF8( r->pattern ) );
	        if( r->next )
	            tag += QString( QUTF8( ", " ) );
	    }
	    QListWidgetItem *item = new QListWidgetItem( tag, list );
	    QVariant item_tag;
	    item_tag.setValue( (void *)s );
	    item->setData( Qt::UserRole, item_tag );
	    if( x == cur )
	        cur_set = s;
	}
	list->setCurrentRow( cur );

	avail_list->clear();
	for( struct DTGStrList *i = field->select_values; i; i = i->next )
	    avail_list->addItem( new QListWidgetItem( QUTF8( i->value ) ) );

	has_list->clear();
	if( cur_set )
	    for( FilterRule *r = cur_set->filter_list; r; r = r->next )
	    {
	        QListWidgetItem *i = new QListWidgetItem( QUTF8( r->pattern ) );
	        QFont f = i->font();
	        f.setBold( !r->old );
	        i->setFont( f );
	        has_list->addItem( i );
	    }

	for( FilterSet *s = src->set_list; s; s = s->next )
	    for( FilterRule *r = s->filter_list; r; r = r->next )
	    {
	        QList<QListWidgetItem*> n = avail_list->findItems( 
				QString( QUTF8( r->pattern ) ), 
				Qt::MatchFixedString );
	        if( n.isEmpty() )
	            continue;
	        avail_list->removeItemWidget( n.first() );
	        delete n.first();
	    }
	config_btns();
}

void SegmentEdit::hasClicked( QListWidgetItem * /* item */ )
{
	config_btns();
}

void SegmentEdit::availClicked( QListWidgetItem * /* item */ )
{
	config_btns();
}

void SegmentEdit::addPushed()
{
	QListWidgetItem *item = 
		avail_list->takeItem( avail_list->currentRow() );
	QListWidgetItem *base = list->currentItem();
	if( item && base )
	{
	    FilterSet *set = 
		(FilterSet *)(base->data( Qt::UserRole).value<void*>());
	    QFont f = item->font();
	    f.setBold( true );
	    item->setFont( f );
	    has_list->addItem( item );
	    QString tag = base->text();
	    if( tag.endsWith( QString( QUTF8( ": " ) ) ) )
	        tag += QString( QUTF8( "" ) ) + item->text();
	    else
	        tag += QString( QUTF8( ", " ) ) + item->text();
	    base->setText( tag );

	    FilterRule *rule = new FilterRule;
	    rule->field = mk_string( cur_field->name );
	    rule->pattern = mk_string( item->text().toUtf8().data() );
	    rule->old = 0;
	    if( set->filter_list )
	    {
	        FilterRule *tmp;
	        for( tmp = set->filter_list; tmp->next; tmp = tmp->next ) {};
	        tmp->next = rule;
	    }
	    else
	        set->filter_list = rule;
	    ok_btn->setEnabled( true );
	    config_btns();
	}
}

void SegmentEdit::delPushed()
{
	QListWidgetItem *item = has_list->currentItem();
	QListWidgetItem *base = list->currentItem();
	if( item && base )
	{
	    FilterSet *set = 
		(FilterSet *)(base->data( Qt::UserRole).value<void*>());
	    if( set->refcnt && !item->font().bold() )
	    {
	        QMessageBox::information( this, 
			QUTF8( "Perforce Defect Tracking Gateway" ), 
			QUTF8( "This segment is currently used in a mapping.\n"
			"Removing options is not allowed." ) );
	        return;
	    }
	    item = has_list->takeItem( has_list->currentRow() );
	    QFont f = item->font();
	    f.setBold( false );
	    item->setFont( f );
	    avail_list->addItem( item );
	    QString tag = QString( QUTF8( set->name ) ) + 
						QString( QUTF8( ": " ) );
	    if( QString( QUTF8( set->filter_list->pattern ) ) == item->text() )
	    {
	        FilterRule *tmp = set->filter_list;
	        set->filter_list = set->filter_list->next;
	        tmp->next = NULL;
	        delete tmp;
	    }
	    for( FilterRule *r = set->filter_list; r; r = r->next )
	    {
	        tag += QString( QUTF8( r->pattern ) );
	        if( r->next && item->text() == QUTF8( r->next->pattern ) )
	        {
	            FilterRule *tmp = r->next;
	            r->next = r->next->next;
	            tmp->next = NULL;
	            delete tmp;
	        }
	        if( r->next )
	            tag += QString( QUTF8( ", " ) );
	    }
	    base->setText( tag );
	    ok_btn->setEnabled( true );
	    config_btns();
	}
}

void SegmentEdit::config_btns()
{
	delete_btn->setEnabled( list->count() && list->currentRow() >= 0 );
	new_btn->setEnabled( avail_list->count() );
	add_btn->setEnabled( list->count() && 
				list->currentRow() >= 0 &&
				avail_list->count() &&
				avail_list->currentRow() >= 0 );
	del_btn->setEnabled( has_list->currentRow() >= 0 );
	field_combo->setEnabled( !field_fixed( src->set_list ) );
}

void SegmentEdit::fieldSwitched( const QString & /* text */ )
{
	struct DTGFieldDesc *field = 
		(struct DTGFieldDesc *) (field_combo->itemData( 
			field_combo->currentIndex() ) ).value<void*>();
	if( !field )
	    return;

	if( cur_index >= 0 && src->set_list &&
		src->set_list->filter_list &&
		src->set_list->filter_list->field &&
	    strcasecmp( src->set_list->filter_list->field, field->name ) )
	{
	    QMessageBox::warning( this,
		QString( QUTF8( "Segment Definition Error" ) ),
		QUTF8( "Cannot change SELECT field with mapped segments" ) );
	    field_combo->setCurrentIndex( cur_index );
	    return;
	}

	cur_index = field_combo->currentIndex();
	cur_field = field;
	populate_lists();
}
