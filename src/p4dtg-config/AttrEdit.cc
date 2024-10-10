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
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataAttr.h>
#include <DataSource.h>
#include <DataMapping.h>
extern "C" {
#include <DTG-interface.h>
#include <dtg-utils.h>
}

class FieldWidget {
  public:
	const DTGAttribute *field;
	DataAttr *cur;
	QLineEdit *value_edit;
	QLabel *desc_label;
	QPushButton *btn;

	FieldWidget *next;

	void mark_default( QString text )
	{
	    /* Italic vs LightGray */
	    QPalette pal = value_edit->palette();
	    // QFont font = value_edit->font();

	    if( field->def && text == QUTF8( field->def ) )
	        pal.setColor( QPalette::Text, Qt::lightGray );
	    else
	        pal.setColor( QPalette::Text, Qt::black );
	    value_edit->setPalette( pal );
	};
  public:
	FieldWidget( QVBoxLayout *full, 
		const DTGAttribute *in_field, DataAttr *in_cur,
		FieldWidget *my_next = NULL )
	{
	    field = in_field;
	    cur = in_cur;
	    next = my_next;

	    QHBoxLayout *hbox = new QHBoxLayout;
	    QLabel *label = new QLabel( QUTF8( field->label ) + 
			(field->required ? QUTF8( "*" ) : QUTF8( "" )) +
						QUTF8( ":" ) );
	    value_edit = new QLineEdit( QUTF8( cur->value ) );
	    mark_default( QUTF8( cur->value ) );
	    value_edit->setObjectName( QUTF8( field->name) );
	    // QRegExp rx( "[-A-Za-z0-9_]*" );
	    // value1_edit->setValidator( new QRegExpValidator( rx, this ) );
	    btn = new QPushButton( QUTF8( "?" ) );
	    btn->setObjectName( QUTF8( field->name ) );
	    btn->setFixedWidth( 24 );
	    hbox->addWidget( label, 0 );
	    hbox->addWidget( value_edit, 1 );
	    hbox->addWidget( btn, 0 );
	    full->addLayout( hbox, 0 );
	    hbox = new QHBoxLayout;
	    desc_label = new QLabel( QUTF8( field->desc ) );
	    desc_label->setVisible( false );
	    desc_label->setWordWrap( true );
	    hbox->addWidget( desc_label, 1 );
	    full->addLayout( hbox, 0 );
            QFrame *line = new QFrame();
            line->setFrameShape( QFrame::HLine );
            full->addWidget( line );
	};

	~FieldWidget() {};

	static FieldWidget *find_widget( QString name, FieldWidget *list )
	{
	    for( FieldWidget *item = list; item; item = item->next )
	        if( name == QUTF8( item->field->name ) )
	            return item;
	    return NULL;
	};
};

AttrEdit::AttrEdit( DataSource *in_src, DataMapping *in_map, 
			DataAttr *in_attrs, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	attrs = in_attrs;
	source = in_src;
	map = in_map;
	if( source )
	    fields = source->get_attributes();
	else
	    fields = map->get_attributes();
	list = NULL;
	last_item = NULL;
#ifndef OS_MACOSX
	setWindowModality( Qt::WindowModal );
#endif
	if( source )
	    setWindowTitle( QString( tr( "Edit Data Source Attributes" ) ) );
	else
	    setWindowTitle( QString( tr( "Edit Data Mapping Attributes" ) ) );

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
	connect( help_btn, SIGNAL( clicked() ), this, SLOT( helpPushed() ) );

	QHBoxLayout *hbox;
	QLabel *label;
        QFrame *line;

        line = new QFrame();
        line->setFrameShape( QFrame::HLine );
        full->addWidget( line );

	for( const DTGAttribute *field = fields; field; field = field->next )
	{
	    DataAttr *attr;
	    for( attr = attrs;
		attr && strcmp( attr->name, field->name ); 
		attr = attr->next ) {};
	    if( !attr )
	    {
	        attr = new DataAttr();
	        attr->name = strdup( field->name );
	        attr->value = strdup( field->def ? field->def : "" );
	        attr->next = attrs->next;
	        attrs->next = attr;
	    }
	    list = new FieldWidget( full, field, attr, list );
	    connect( list->btn, SIGNAL( clicked() ), 
			this, SLOT( descPushed() ) );
	    connect( list->value_edit, SIGNAL( textChanged(const QString&) ),
			this, SLOT( valueChanged(const QString&) ) );
	    connect( list->value_edit, SIGNAL( editingFinished() ),
			this, SLOT( valueEdited() ) );
	}

	hbox = new QHBoxLayout;
	label = new QLabel( QUTF8( 
		"Light Gray: Use default value, current default shown" ) );
	QPalette pal = label->palette();
	pal.setColor( QPalette::WindowText, Qt::darkGray );
	label->setPalette( pal );
	hbox->addWidget( label, 0 );
	full->addLayout( hbox, 0 );
	hbox = new QHBoxLayout;
	label = new QLabel( QUTF8( 
		"*: Required attribute" ) );
	hbox->addWidget( label, 0 );
	full->addLayout( hbox, 0 );
        line = new QFrame();
        line->setFrameShape( QFrame::HLine );
        full->addWidget( line );
	full->addLayout( bottom, 0 );

	full->setSizeConstraint( QLayout::SetFixedSize );
	setLayout( full );

	ok_btn->setEnabled( false );
}

void AttrEdit::okPushed()
{
	FieldWidget *item;
	QString error_msg;
	for( item = list; item; item = item->next )
	{
	    if( item->field->required && item->value_edit->text().isEmpty() )
	    {
	        error_msg += QUTF8( "You must set a value for " )+ 
				QUTF8( item->field->name ) +
				QUTF8( "\n" );
	        item->value_edit->setFocus( Qt::OtherFocusReason );
	    }
	    else
	    {
	        DataAttr field( item->field->name, 
				item->value_edit->text().toUtf8().data() );
	        char *err;
	        if( source ) 
	            err = source->validate_attribute( &field );
	        else
	            err = map->validate_attribute( &field );
	        if( err )
	        {
	            error_msg += QUTF8( err ) + QUTF8( "\n" );
	            delete[] err;
	            item->value_edit->setFocus( Qt::OtherFocusReason );
	        }
	    }
	}

	if( error_msg.length() )
	{
	    QMessageBox::warning( this,
                                QUTF8( "Attribute Validation Error" ),
                                error_msg );
	    return;
	}

	for( item = list; item; item = item->next )
	{
	    SAFESET(item->cur, value, 
			item->value_edit->text().toUtf8().data() );
	    if( item->field->def && 
		!strcmp( item->field->def, item->cur->value ) )
	    {
	        /* Mark all DEFAULT values as NULL for later removal */
	        delete item->cur->value;
	        item->cur->value = NULL;
	    }
	}

	/* Now remove the ones which have NULL values, ie use default */
	DataAttr *attr = attrs;
	while( attr )
	{
	    if( attr->next && !attr->next->value )
	    {
	        DataAttr *tmp = attr->next;
	        attr->next = attr->next->next;
	        tmp->next = NULL;
	        delete tmp;
	    }
	    else
	        attr = attr->next;
	}
	done( 1 );
}

void AttrEdit::cancelPushed()
{
	done( 0 );
}

void AttrEdit::helpPushed()
{
	global->help->showSource( AttrEdit_help, this );
}

void AttrEdit::descPushed()
{
	QWidget *cur = QApplication::focusWidget();
	if( !cur ) return;
	FieldWidget *item = FieldWidget::find_widget( cur->objectName(), list );
	if( !item ) return;
	item->desc_label->setVisible( !item->desc_label->isVisible() );
}

void AttrEdit::valueChanged( const QString &text )
{
	QWidget *cur = QApplication::focusWidget();
	if( !cur ) return;
	FieldWidget *item = FieldWidget::find_widget( cur->objectName(), list );
	if( !item ) return;
	last_item = item;

	ok_btn->setEnabled( true );
	item->mark_default( text );
}

void AttrEdit::valueEdited()
{
	if( !last_item ) return;
	if( !last_item->value_edit->isModified() ) return;
	
	const char *cur_value = last_item->value_edit->text().toUtf8().data();

	/* If blank and a default exists, replace w/ default */
	if( !*cur_value && last_item->field->def )
	{
	    FieldWidget *saved = last_item;
	    /* This will call valueChanged which could change last_item */
	    last_item->value_edit->setText( QUTF8( last_item->field->def ) );
	    last_item = saved;
	    last_item->mark_default( QUTF8( last_item->field->def ) );
	    return;
	}

	DataAttr field( last_item->field->name, cur_value );
	char *err;
	if( source )
	    err = source->validate_attribute( &field );
	else
	    err = map->validate_attribute( &field );
	last_item->value_edit->setModified( false );
	if( err )
	{
            QMessageBox::warning( this,
                                QUTF8( "Attribute Validation Error" ),
                                QUTF8( err ) );
	    last_item->value_edit->setFocus( Qt::OtherFocusReason );
	}
	delete err;
}
