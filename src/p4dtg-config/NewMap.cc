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
#include <QLineEdit>
#include <QComboBox>
#include "NewMap.h"
#include "P4DTG.h"
#include "Help.h"
#include "DTGHelp.h"
#include <DataSource.h>
#include <DataMapping.h>

class SrcSet {
    public:
	DataSource *src;
	FilterSet *set;
    public:
	SrcSet( DataSource *in_src, FilterSet *in_set )
	{
	    src = in_src;
	    set = in_set;
	};
	~SrcSet() {};
};

static int
has_unmapped_segs( DataSource *src )
{
	for( FilterSet *set = src->set_list; set; set = set->next )
	    if( !set->refcnt )
	        return 1;
	return 0;
}

NewMap::NewMap( DataMapping *map, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	my_map = map;
	setWindowModality( Qt::WindowModal );
	setWindowTitle( QString( tr( "Add New Mapping" ) ) );

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

	QLabel *lab = new QLabel( tr( "Name:" ) );
	name_edit = new QLineEdit;
	QRegExp rx( QUTF8( "[-A-Za-z0-9_]*" ) );
	name_edit->setValidator( new QRegExpValidator( rx, this ) );
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget( lab );
	hbox->addWidget( name_edit );
	full->addLayout( hbox, 0 );

	QGridLayout *gbox = new QGridLayout;
	lab = new QLabel( tr( "Defect Tracking Source:" ) );
	dt_combo = new QComboBox;
	DataSource *src;
	QVariant item_tag;
	for( src = global->dt_srcs; src; src = src->next )
	    if( !src->deleted && ( !src->refcnt || has_unmapped_segs( src ) ) )
	    {
	        QString tag = QString( QUTF8( src->nickname ) ) + 
			QString( QUTF8( " : " ) ) +
	        	QString( QUTF8( src->server ) ) + 
			QString( QUTF8( " - " ) ) +
	        	QString( QUTF8( src->module ) ) +
	        	QString( QUTF8( " (" ) ) + 
			QString( QUTF8( src->plugin ) ) + 
			QString( QUTF8( ")" ) );
	        if( src->set_list )
	            tag.prepend( QUTF8( "* Select from segments of " ) );
	        item_tag.setValue( (void *)(new SrcSet( src, NULL ) ) );
	        dt_combo->addItem( tag, item_tag );
	        int cnt = 0;
	        for( FilterSet *set = src->set_list; set; set = set->next )
	        {
	            if( set->refcnt )
	                continue;

	            tag = QString( QUTF8( " -> " ) ) + 
			QString( QUTF8( src->nickname ) ) +
			QString( QUTF8( "/" ) ) + 
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
	            cnt++;
	            item_tag.setValue( (void *)(new SrcSet( src, set ) ) );
	            dt_combo->addItem( QString( tag ), item_tag );
	        }
	        /* If has segments but all are assigned, drop title entry */
	        if( src->set_list && !cnt )
	        {
	            SrcSet *src_set = 
			(SrcSet *)(dt_combo->itemData( 
				dt_combo->count() - 1 ) ).value<void*>();
	            delete src_set;
	            dt_combo->removeItem( dt_combo->count() - 1 );
	        }
	    }
	gbox->addWidget( lab, 0, 0 );
	gbox->addWidget( dt_combo, 0, 1 );
	lab = new QLabel( tr( "Perforce Server Source:" ) );
	p4_combo = new QComboBox;
	for( src = global->p4_srcs; src; src = src->next )
	    if( !src->deleted && !src->refcnt || has_unmapped_segs( src ) )
	    {
	        QString tag = QString( QUTF8( src->nickname ) ) + 
			QString( QUTF8( " : " ) ) +
	        	QString( QUTF8( src->server ) );
	        if( src->set_list )
	            tag.prepend( QUTF8( "* Select from segments of " ) );
	        item_tag.setValue( (void *)(new SrcSet( src, NULL ) ) );
	        p4_combo->addItem( tag, item_tag );
	        int cnt = 0;
	        for( FilterSet *set = src->set_list; set; set = set->next )
	        {
	            if( set->refcnt )
	                continue;

	            tag = QString( QUTF8( " -> " ) ) + 
			QString( QUTF8( src->nickname ) ) +
			QString( QUTF8( "/" ) ) + 
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
	            cnt++;
	            item_tag.setValue( (void *)(new SrcSet( src, set ) ) );
	            p4_combo->addItem( QString( tag ), item_tag );
	        }
	        /* If has segments but all are assigned, drop title entry */
	        if( src->set_list && !cnt )
	        {
	            SrcSet *src_set = 
			(SrcSet *)(p4_combo->itemData( 
				p4_combo->count() - 1 ) ).value<void*>();
	            delete src_set;
	            p4_combo->removeItem( p4_combo->count() - 1 );
	        }
	    }
	gbox->addWidget( lab, 1, 0 );
	gbox->addWidget( p4_combo, 1, 1 );
	gbox->setColumnStretch( 1, 1 );
	full->addLayout( gbox, 0 );
	
	full->addLayout( bottom, 0 );

	setLayout( full );

	connect( name_edit, SIGNAL( textChanged(const QString&) ),
		this, SLOT( nameChanged(const QString&) ) );
	connect( dt_combo, SIGNAL( currentIndexChanged(const QString &) ),
		this, SLOT( dtsrcSwitched(const QString &) ) );
	connect( p4_combo, SIGNAL( currentIndexChanged(const QString &) ),
		this, SLOT( p4srcSwitched(const QString &) ) );
}

static void delete_variants( QComboBox *combo )
{
	for( int i = 0; i < combo->count(); i++ )
	{
	    SrcSet *src_set = (SrcSet *)(combo->itemData( i ) ).value<void*>();
	    delete src_set;
	}
}
	
void NewMap::okPushed()
{
	if( !global->unique_mapid( name_edit->text().toUtf8().data() ) )
	{
	    QMessageBox::information( this, 
		QUTF8( "Perforce Defect Tracking Gateway" ), 
		QUTF8( "The name matches an existing mapping. Please "
		"change it to a unique value.\n" ) );
	    return;
	}
	SrcSet *p4src = 
		(SrcSet *)(p4_combo->itemData( 
			p4_combo->currentIndex() ) ).value<void*>();
	SrcSet *dtsrc = 
		(SrcSet *)(dt_combo->itemData( 
			dt_combo->currentIndex() ) ).value<void*>();

	if( p4src && dtsrc )
	{
	    my_map->id = cp_string( name_edit->text().toUtf8().data() );

	    my_map->scm_id = cp_string( p4src->src->nickname );
	    my_map->scm = p4src->src;
	    if( p4src->set )
	    {
	        my_map->scm_filter = cp_string( p4src->set->name );
	        my_map->scm_filters = p4src->set->filter_list;
	    }
	    my_map->scm->adj_refcnt( +1, my_map->scm_filter );

	    my_map->dts_id = cp_string( dtsrc->src->nickname );
	    my_map->dts = dtsrc->src;
	    if( dtsrc->set )
	    {
	        my_map->dts_filter = cp_string( dtsrc->set->name );
	        my_map->dts_filters = dtsrc->set->filter_list;
	    }
	    my_map->dts->adj_refcnt( +1, my_map->dts_filter );

	    delete_variants( p4_combo );
	    delete_variants( dt_combo );
	    done( 1 );
	}
	else
	{
	    delete_variants( p4_combo );
	    delete_variants( dt_combo );
	    done( 0 );
	}
}

void NewMap::cancelPushed()
{
	delete_variants( p4_combo );
	delete_variants( dt_combo );
	done( 0 );
}

void NewMap::helpPushed()
{
	global->help->showSource( NewMap_help, this );
}

void NewMap::p4srcSwitched( const QString & /* p4_item */ )
{
	config_btns();
}

void NewMap::dtsrcSwitched( const QString & /* dt_item */ )
{
	config_btns();
}

void NewMap::config_btns()
{
	if( name_edit->text().isEmpty() )
	{
	    ok_btn->setEnabled( false );
	    return;
	}

	FilterSet *p4_set = 
		((SrcSet *)(p4_combo->itemData( 
			p4_combo->currentIndex() ) ).value<void*>())->set;
	FilterSet *dt_set = 
		((SrcSet *)(dt_combo->itemData( 
			dt_combo->currentIndex() ) ).value<void*>())->set;
	QString p4_item = p4_combo->currentText();
	QString dt_item = dt_combo->currentText();

	if(    !p4_set && p4_item.startsWith( QString( QUTF8( "* " ) ) ) 
	    || !dt_set && dt_item.startsWith( QString( QUTF8( "* " ) ) )
		)
	    ok_btn->setEnabled( false );
	else
	    ok_btn->setEnabled( true );
}

void NewMap::nameChanged( const QString & /* item */ )
{
	config_btns();
}
