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
#include <QPointF>
#include <QHeaderView>
#include "MapEdit.h"
#include "AttrEdit.h"
#include "DataAttr.h"
#include "DataSource.h"
#include "DataMapping.h"
#include "P4DTG.h"
#include "Help.h"
#include "SelectMap.h"
#include "FixMap.h"
#include "PickList.h"
#include "DTGHelp.h"
#include <DTG-interface.h>

class FieldUsage {
    public:
	int type;
	int refs;
	int ro;
	FieldUsage( int in_type, int in_ro ) 
	    { type = in_type; ro = in_ro; refs = 0; };
	
};

typedef QTreeWidgetItem *QTreeWidgetItemPtr;

static bool itemSort( const QTreeWidgetItemPtr &s1, 
	const QTreeWidgetItemPtr &s2 )
{
	if( s1->text( 0 ).startsWith( QUTF8( "List of Change Numbers")))
	    if( s2->text( 0 ).startsWith( QUTF8( "List of Change Numbers")))
	        return s1->text( 0 ).toLower() < s2->text( 0 ).toLower();
	    else
	        return 0;
	else if( s2->text( 0 ).startsWith( QUTF8( "List of Change Numbers")))
	    return 1;
	return s1->text( 0 ).toLower() < s2->text( 0 ).toLower();
}

QString tagit( CopyRule *cr )
{
	QString tag;
	switch( cr->copy_type )
	{
	case CopyRule::DATE:
	    tag = QUTF8( " (date)" );
	    break;
	case CopyRule::MAP:
	    tag = QUTF8( " (map)" );
	    break;
	case CopyRule::UNMAP:
	    tag = QUTF8( " (unmap)" );
	    break;
	default:
	case CopyRule::LINE:
	case CopyRule::WORD:
	    if( cr->truncate )
	        tag = QUTF8( " (truncate)" );
	    else
	        tag = QUTF8( "" );
	    break;
	}
	return tag;
}

QString tipit( CopyRule *cr, const QString &op )
{
	QString tip;
	for( CopyMap *mr = cr->mappings; mr; mr = mr->next )
	{
	    if( *mr->value1 )
	        tip += QUTF8( mr->value1 );
	    else
	        tip += QUTF8( "<empty>" );
	    tip += QUTF8( " " );
	    tip += op;
	    tip += QUTF8( " " );
	    if( *mr->value2 )
	        tip += QUTF8( mr->value2 );
	    else
	        tip += QUTF8( "<empty>" );
	    if( mr->next )
	        tip += QUTF8( "\n" );
	}
	return tip;
}

static int mark_field( QListWidget *list, const char *field )
{
	QListWidgetItem *item = find_item( list, " : ", field );
	if( !item )
	{
	    printf( "ERROR: mark_field: Field %s not found\n", field );
	    return 1;
	}
	if( item->text() != QUTF8( "List of Change Numbers : LINE *" ) ) 
	{
	    FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	    // printf("mark(%ux):%s: %ux\n", list, field, fu );
	    if( fu->type > 0 )
	        fu->refs++;
	    QFont font = item->font();
	    font.setBold( fu->refs == 0 );
	    item->setFont( font );
	}
	return 0;
}

static int unmark_field( QListWidget *list, const char *field )
{
	QListWidgetItem *item = find_item( list, " : ", field );
	if( !item )
	{
	    printf( "ERROR: mark_field: Field %s not found\n", field );
	    return 1;
	}
	if( item->text() != QUTF8( "List of Change Numbers : LINE *" ) ) 
	{
	    FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	    // printf("unmark(%ux):%s: %ux\n", list, field, fu );
	    if( fu->type > 0 && fu->refs > 0 )
	        fu->refs--;
	    QFont font = item->font();
	    font.setBold( fu->refs == 0 );
	    item->setFont( font );
	}
	return 0;
}

QString fixtip( FixRule *fr )
{
	QString tip;
	int prev = 0;
	if( fr->change_number )
	{
	    tip += QUTF8( "include ChangeNumber" );
	    prev++;
	}
	if( fr->description )
	{
	    if( prev ) tip += QUTF8( "\n" );
	    tip += QUTF8( "include ChangeDescription" );
	    prev++;
	}
	if( fr->fixed_by )
	{
	    if( prev ) tip += QUTF8( "\n" );
	    tip += QUTF8( "include FixedBy" );
	    prev++;
	}
	if( fr->fixed_date )
	{
	    if( prev ) tip += QUTF8( "\n" );
	    tip += QUTF8( "include FixedDate" );
	    prev++;
	}
	if( fr->file_list )
	{
	    if( prev ) tip += QUTF8( "\n" );
	    tip += QUTF8( "include FileList" );
	    prev++;
	}
	if( !prev )
	    tip += QUTF8( "No details" );
	return tip;
}

int MapEdit::pop_tree()
{
	// mapping_tree->setItemsExpandable( false );
	mapping_tree->setRootIsDecorated( false );
	QTreeWidgetItem *base;
	QTreeWidgetItem *from; 

	mirror_tree = base = new QTreeWidgetItem( mapping_tree, 
		QStringList( QUTF8( "DEFECT TRACKER ") + 
		L_ARROW +
		QUTF8( " MIRRORED " ) + R_ARROW + 
		QUTF8( " PERFORCE" ) ) );
	{
	    QVariant data;
	    data.setValue( 1 );
	    base->setData( 0, Qt::UserRole, data );
	}
	mapping_tree->setItemExpanded( base, true );
	QList<QTreeWidgetItemPtr> subtree;
	CopyRule *cr;
	int marked;
	int badmaps = 0;
	for( cr = my_map->mirror_rules; cr; cr = cr->next )
	{
	    from = new QTreeWidgetItem(
			QStringList( QUTF8( cr->dts_field ) + 
			DBL_ARROW + 
			QUTF8( cr->scm_field ) + tagit( cr ) ) );
	    // printf("pop_tree1: S:%s D:%s\n", cr->scm_field, cr->dts_field );
	    marked = mark_field( p4_list, cr->scm_field );
	    marked += mark_field( dts_list, cr->dts_field );
	    if( marked )
	    {
	        badmaps += marked;
	        QFont font = from->font( 0 );
	        font.setBold( true );
	        from->setFont( 0, font );
	    }
	    mapping_tree->setItemExpanded( from, true );
	    subtree.prepend( from );
	    QVariant data;
	    data.setValue( (void *)cr );
	    from->setData( 0, Qt::UserRole, data );
	    if( cr->copy_type == CopyRule::MAP ||
		cr->copy_type == CopyRule::UNMAP )
	        from->setToolTip( 0, tipit( cr, DBL_ARROW ) );
	}
	qStableSort( subtree.begin(), subtree.end(), itemSort );
	base->addChildren( subtree );

	dts_to_p4_tree = base = new QTreeWidgetItem( mapping_tree, 
		QStringList( QString( QUTF8( "DEFECT TRACKER ")) + R_ARROW +
		QString( QUTF8( " COPIED " ) ) + R_ARROW +
		QString( QUTF8( " PERFORCE" ) ) ) );
	{
	    QVariant data;
	    data.setValue( 2 );
	    base->setData( 0, Qt::UserRole, data );
	}
	mapping_tree->setItemExpanded( base, true );
	subtree.clear();
	for( cr = my_map->dts_to_scm_rules; cr; cr = cr->next )
	{
	    from = new QTreeWidgetItem(
			QStringList( QUTF8( cr->dts_field ) + 
			QUTF8( " " ) + R_ARROW + QUTF8( " " ) + 
			QUTF8( cr->scm_field ) + tagit( cr ) ) );
	    // printf("pop_tree2: S:%s D:%s\n", cr->scm_field, cr->dts_field );
	    marked = mark_field( p4_list, cr->scm_field );
	    marked += mark_field( dts_list, cr->dts_field );
	    if( marked )
	    {
	        badmaps += marked;
	        QFont font = from->font( 0 );
	        font.setBold( true );
	        from->setFont( 0, font );
	    }
	    mapping_tree->setItemExpanded( from, true );
	    subtree.prepend( from );
	    QVariant data;
	    data.setValue( (void *)cr );
	    from->setData( 0, Qt::UserRole, data );
	    if( cr->copy_type == CopyRule::MAP ||
		cr->copy_type == CopyRule::UNMAP )
	        from->setToolTip( 0, tipit( cr, 
			QUTF8( " " ) + R_ARROW + QUTF8( " " ) ) );
	}
	qStableSort( subtree.begin(), subtree.end(), itemSort );
	base->addChildren( subtree );

	p4_to_dts_tree = base = new QTreeWidgetItem( mapping_tree, 
		QStringList( QString( QUTF8( "PERFORCE " ) ) + R_ARROW +
		QString( QUTF8( " COPIED " ) ) + R_ARROW +
		QString( QUTF8( " DEFECT TRACKER" ) ) ) );
	{
	    QVariant data;
	    data.setValue( 3 );
	    base->setData( 0, Qt::UserRole, data );
	}
	mapping_tree->setItemExpanded( base, true );
	subtree.clear();
	for( cr = my_map->scm_to_dts_rules; cr; cr = cr->next )
	{
	    from = new QTreeWidgetItem(
			QStringList( QUTF8( cr->scm_field ) + 
			QUTF8( " " ) + R_ARROW + QUTF8( " " ) + 
			QUTF8( cr->dts_field ) + tagit( cr ) ) );
	    // printf("pop_tree3: S:%s D:%s\n", cr->scm_field, cr->dts_field );
	    marked = mark_field( p4_list, cr->scm_field );
	    marked += mark_field( dts_list, cr->dts_field );
	    if( marked )
	    {
	        badmaps += marked;
	        QFont font = from->font( 0 );
	        font.setBold( true );
	        from->setFont( 0, font );
	    }
	    mapping_tree->setItemExpanded( from, true );
	    subtree.prepend( from );
	    QVariant data;
	    data.setValue( (void *)cr );
	    from->setData( 0, Qt::UserRole, data );
	    if( cr->copy_type == CopyRule::MAP ||
		cr->copy_type == CopyRule::UNMAP )
	        from->setToolTip( 0, tipit( cr, 
			QUTF8( " " ) + R_ARROW + QUTF8( " " ) ) );
	}
	qStableSort( subtree.begin(), subtree.end(), itemSort );
	base->addChildren( subtree );

	fix_tree = base = new QTreeWidgetItem( mapping_tree, 
		QStringList( QString( QUTF8( "PERFORCE FIX DETAILS " ) ) + 
		R_ARROW +
		QString( QUTF8( " COPIED " ) ) + R_ARROW +
		QString( QUTF8( " DEFECT TRACKER" ) ) ) );
	{
	    QVariant data;
	    data.setValue( 4 );
	    base->setData( 0, Qt::UserRole, data );
	}
	mapping_tree->setItemExpanded( base, true );
	subtree.clear();
	for( FixRule *fr = my_map->fix_rules; fr; fr = fr->next )
	{
	    if( fr->deleted )
		continue;
	    from = new QTreeWidgetItem(
		QStringList( QString( QUTF8( "P4Fix " ) ) + R_ARROW +
		QUTF8( " " ) + QUTF8( fr->dts_field ) +
		QString( QUTF8( " (append)" ) ) ) );
	    from->setToolTip( 0, fixtip( fr ) );
	    // printf("pop_tree4: D:%s\n", fr->dts_field );
	    marked = mark_field( dts_list, fr->dts_field );
	    if( marked )
	    {
	        badmaps += marked;
	        QFont font = from->font( 0 );
	        font.setBold( true );
	        from->setFont( 0, font );
	    }
	    mapping_tree->setItemExpanded( from, true );
	    subtree.prepend( from );
	    QVariant data;
	    data.setValue( (void *)fr );
	    from->setData( 0, Qt::UserRole, data );
	}
	qStableSort( subtree.begin(), subtree.end(), itemSort );
	base->addChildren( subtree );
	return badmaps;
}

static int type_value( struct DTGFieldDesc *field )
{
	int val;
	switch( *field->type )
	{
	case 'W': case 'w':
	    val = 1;
	    break;
	case 'L': case 'l':
	    val = 2;
	    break;
	case 'T': case 't':
	    val = 3;
	    break;
	case 'D': case 'd':
	    val = 4;
	    break;
	case 'F': case 'f':
	    val = 5;
	    break;
	case 'S': case 's':
	    val = 6;
	    for( struct DTGStrList *opt = field->select_values; 
		opt;
		opt = opt->next )
		val += 1;
	    break;
	default:
	    val = 0;
	    break;
	}
	return val;
}

MapEdit::MapEdit( DataMapping *map, QWidget *parent )
    : QDialog( parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint )
{
	my_map = map;
	setWindowModality( Qt::WindowModal );
	setMinimumWidth( 600 );
	setMinimumHeight( 600 );
	setWindowTitle( 
		QString( QUTF8( "Edit Perforce-Defect Tracker Mapping" ) ) );

    	QVBoxLayout *full = new QVBoxLayout;
    	QHBoxLayout *bottom = new QHBoxLayout;
	QGridLayout *sources = new QGridLayout;
	sources->setColumnStretch( 1,1 );

	sources->addWidget(
		new QLabel( QString( QUTF8( "Mapping name:" ) ) ),0,0, 
			Qt::AlignLeft );
	QLineEdit *srcline = new QLineEdit();
	if( map->dts )
	{
	    srcline->setText( QUTF8( map->id ) );
	    srcline->setReadOnly( true );
	    srcline->setFrame( false );
	}
	sources->addWidget( srcline,0,1 );
	attributes_btn = new QPushButton( tr( "Edit Attributes..." ) );
	sources->addWidget( attributes_btn,0,2 );
	connect( attributes_btn, SIGNAL( clicked() ), 
		this, SLOT( attributesPushed() ) );

	sources->addWidget(
		new QLabel( QString( QUTF8( "Defect tracker source:" ) ) ),
			1, 0, 
			Qt::AlignLeft );
	srcline = new QLineEdit();
	if( map->dts )
	{
	    if( map->dts_filter )
	        srcline->setText( QUTF8( map->dts->nickname ) + 
				QUTF8( "/" ) + QUTF8( map->dts_filter ) );
	    else
	        srcline->setText( QUTF8( map->dts->nickname ) );
	    srcline->setReadOnly( true );
	    srcline->setFrame( false );
	}
	sources->addWidget( srcline,1,1,1,2 );

	sources->addWidget(
		new QLabel( QString( QUTF8( "Perforce server source:" ) ) ),
			2, 0, 
			Qt::AlignLeft );
	srcline = new QLineEdit();
	if( map->scm )
	{
	    if( map->scm_filter )
	        srcline->setText( QUTF8( map->scm->nickname ) + 
				QUTF8( "/" ) + QUTF8( map->scm_filter ) );
	    else
	        srcline->setText( QUTF8( map->scm->nickname ) );
	    srcline->setReadOnly( true );
	    srcline->setFrame( false );
	}
	sources->addWidget( srcline,2,1,1,2 );

	full->addLayout( sources );
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

	QVBoxLayout *leftbox = new QVBoxLayout;
	QVBoxLayout *rightbox = new QVBoxLayout;

	QGroupBox *p4_group = 
		new QGroupBox( tr( "Perforce fields (*: read-only)" ) );
	p4_list = new QListWidget();
	QListWidgetItem *listitem;
	struct DTGFieldDesc *field;
	FieldUsage *fu;

	listitem = new QListWidgetItem( 
			QUTF8( "List of Change Numbers : LINE *" ), 
			p4_list );
	fu = new FieldUsage( 0, 1 );
	QVariant special_val;
	special_val.setValue( (void *)fu );
	listitem->setData( Qt::UserRole, special_val );
	QFont font = listitem->font();
	font.setBold( true );
	listitem->setFont( font );

	listitem = new QListWidgetItem( 
			QUTF8( "Fix Details : TEXT (append) *"), 
			p4_list );
	fu = new FieldUsage( -1, 1 );
	QVariant special_val2;
	special_val2.setValue( (void *)fu );
	listitem->setData( Qt::UserRole, special_val2 );
	font = listitem->font();
	font.setBold( true );
	listitem->setFont( font );

	for( field = map->scm->get_fields(); field; field = field->next )
	{
	    if( strncasecmp( field->name, "DTG_", 4 ) )
	    {
	        listitem = new 
		   QListWidgetItem( QUTF8( field->name ) + 
			QUTF8( " : " ) +
			QUTF8( field->type ) +
			QUTF8( field->readonly ? " *" : "" ), p4_list );
	        QVariant data;
	        fu = new FieldUsage( type_value( field ), field->readonly );

	        // Adjust type based on segment filtering
	        if( my_map->scm_filters && my_map->scm_filters->field && 
			!strcasecmp( my_map->scm_filters->field, field->name ) )
	        {
	            fu->type = 6;
	            for( FilterRule *r = my_map->scm_filters;
				r;
				r = r->next )
	                fu->type++;
	        }

	        data.setValue( (void *)fu );
	        listitem->setData( Qt::UserRole, data );
	        QFont font = listitem->font();
	        font.setBold( true );
	        listitem->setFont( font );
	    }
	}

	connect( p4_list, SIGNAL( itemClicked(QListWidgetItem*) ), 
		this, SLOT( p4Clicked(QListWidgetItem*) ) );
	connect( p4_list, SIGNAL( currentRowChanged(int) ),
		this, SLOT( p4RowChanged(int) ) );
	QGridLayout *grid = new QGridLayout;
	grid->addWidget( p4_list, 0, 0 );
	p4_group->setLayout( grid );
	p4copy_btn = new QPushButton( tr( "&Copy to Defect tracker" ) );
	p4mirror_btn = new QPushButton( tr( "&Mirror with Defect tracker" ) );
	connect( p4copy_btn, SIGNAL( clicked() ), 
	    this, SLOT( p4CopyPushed() ) );
	connect( p4mirror_btn, SIGNAL( clicked() ), 
	    this, SLOT( p4MirrorPushed() ) );
	QHBoxLayout *p4hbox = new QHBoxLayout;
	p4hbox->addWidget( p4copy_btn );
	p4hbox->addWidget( p4mirror_btn );

	QGroupBox *dts_group = 
		new QGroupBox( tr( "Defect tracker fields (*: read-only)" ) );
	dts_list = new QListWidget();
	for( field = map->dts->get_fields(); field; field = field->next )
	{
	    listitem = new 
		QListWidgetItem( QUTF8( field->name ) + QUTF8( " : " ) +
			QUTF8( field->type ) +
			QUTF8( field->readonly ? " *" : "" ), dts_list );
	    QVariant data;
	    fu = new FieldUsage( type_value( field ), field->readonly );

	    // Adjust type based on segment filtering
	    if( my_map->dts_filters && my_map->dts_filters->field && 
		!strcasecmp( my_map->dts_filters->field, field->name ) )
	    {
	        fu->type = 6;
	        for( FilterRule *r = my_map->dts_filters;
			r;
			r = r->next )
	            fu->type++;
	    }

	    data.setValue( (void *)fu );
	    listitem->setData( Qt::UserRole, data );
	    QFont font = listitem->font();
	    font.setBold( true );
	    listitem->setFont( font );
	}
	connect( dts_list, SIGNAL( itemClicked(QListWidgetItem*) ), 
		this, SLOT( dtsClicked(QListWidgetItem*) ) );
	connect( dts_list, SIGNAL( currentRowChanged(int) ),
		this, SLOT( dtsRowChanged(int) ) );
	grid = new QGridLayout;
	grid->addWidget( dts_list, 0, 0 );
	dts_group->setLayout( grid );
	dtscopy_btn = new QPushButton( tr( "&Copy to Perforce" ) );
	dtsmirror_btn = new QPushButton( tr( "&Mirror with Perforce" ) );
	connect( dtscopy_btn, SIGNAL( clicked() ), 
	    this, SLOT( dtsCopyPushed() ) );
	connect( dtsmirror_btn, SIGNAL( clicked() ), 
	    this, SLOT( dtsMirrorPushed() ) );
	QHBoxLayout *dthbox = new QHBoxLayout;
	dthbox->addWidget( dtscopy_btn );
	dthbox->addWidget( dtsmirror_btn );

	QFrame *line = new QFrame();
	line->setFrameShape( QFrame::HLine );
	leftbox->addWidget( dts_group );
	leftbox->addLayout( dthbox );
	leftbox->addWidget( line );
	leftbox->addWidget( p4_group );
	leftbox->addLayout( p4hbox );

	mapping_tree = new QTreeWidget();
	treeedit_btn = new QPushButton( tr( "&Edit Value Mappings" ) );
	treeunmap_btn = new QPushButton( tr( "&Unmap" ) );
	mapping_tree->setColumnCount( 1 );
	mapping_tree->setHeaderLabels( QStringList(
		QString( tr( "Field mappings:" ) ) ) );
	badmaps = pop_tree();
	connect( mapping_tree, SIGNAL( itemClicked(QTreeWidgetItem*, int) ), 
		this, SLOT( mapClicked(QTreeWidgetItem*, int) ) );
	connect( treeedit_btn, SIGNAL( clicked() ), 
		this, SLOT( treeeditPushed() ) );
	connect( treeunmap_btn, SIGNAL( clicked() ), 
		this, SLOT( treeunmapPushed() ) );
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget( treeedit_btn );
	hbox->addWidget( treeunmap_btn );
	hbox->addStretch( 1 );
	// rightbox->addWidget( label );
	// rightbox->addWidget( filter_combo );
	QLabel *label = new QLabel( tr( "Field mappings:" ) );
	rightbox->addWidget( label );
	mapping_tree->header()->hide();
	rightbox->addWidget( mapping_tree );
	rightbox->addLayout( hbox );

	hbox = new QHBoxLayout;
	hbox->addLayout( leftbox );
	hbox->addSpacing( 12 );
	hbox->addLayout( rightbox );

	p4Clicked( NULL );
	dtsClicked( NULL );
	mapClicked( NULL, 0 );
	ok_btn->setEnabled( false );

	full->addLayout( hbox, 0 );
	full->addSpacing( 12 );
	full->addLayout( bottom, 0 );
	// full->setSizeConstraint( QLayout::SetFixedSize );
	setLayout( full );
}

void MapEdit::attributesPushed()
{
	DataAttr *in_cur = new DataAttr();
	in_cur->name = strdup( "" );
	in_cur->value = strdup( "" );
	in_cur->next = my_map->attrs ? my_map->attrs->copy() : NULL;

	AttrEdit *edit = new AttrEdit( NULL, my_map, in_cur, this );
	int result = edit->exec();
	if( result )
	{
	    ok_btn->setEnabled( true );
	    delete my_map->attrs;
	    my_map->attrs = in_cur->next;
	    in_cur->next = NULL;
	}
	delete in_cur;
	delete edit;
}

void MapEdit::okPushed()
{
	done( 1 );
}

void MapEdit::cancelPushed()
{
	done( 0 );
}

void MapEdit::helpPushed()
{
	global->help->showSource( MapEdit_help, this );
}

void MapEdit::treeeditPushed()
{
	QTreeWidgetItem *item = mapping_tree->currentItem();
	if( item )
	{
	    int dir = mapping_tree->indexOfTopLevelItem( item->parent() );
	    if( dir != 3 )
	    {
	        CopyRule *cr = 
		  (CopyRule *)( item->data( 0, Qt::UserRole ).value<void*>() );
	        if( cr )
	        {
	            CopyMap *tmp = cr->mappings ? cr->mappings->copy() : NULL;
	            CopyMap *save = cr->mappings;
	            cr->mappings = tmp;
		    SelectMap *sm = new SelectMap( dir, my_map, cr, this );
	            int result = sm->exec();
		    if( result )
	            {
	                if( dir == 1 ) // dts->scm
	                {
	                    item->setToolTip( 0, tipit( cr,  
				QUTF8( " " ) + R_ARROW + QUTF8( " " ) ) );
	                    item->setText( 0,
				QUTF8( cr->dts_field ) + 
				QUTF8( " " ) + R_ARROW + QUTF8( " " ) +
				QUTF8( cr->scm_field ) + tagit( cr ) );
	                }
	                else if( dir == 2 ) // scm->dts
	                {
	                    item->setToolTip( 0, tipit( cr,  
				QUTF8( " " ) + R_ARROW + QUTF8( " " ) ) );
	                    item->setText( 0,
				QUTF8( cr->scm_field ) + 
				QUTF8( " " ) + R_ARROW + QUTF8( " " ) +
				QUTF8( cr->dts_field ) + tagit( cr ) );
	                }
	                else if( dir == 0 ) // mirror
	                {
	                    item->setToolTip( 0, tipit( cr, DBL_ARROW ) );
	                    item->setText( 0,
				QUTF8( cr->dts_field ) + 
				DBL_ARROW + 
				QUTF8( cr->scm_field ) + tagit( cr ) );
	                }
	                if( save )
	                    delete save;
	                ok_btn->setEnabled( true );
	            }
	            else
	            {
	                delete cr->mappings;
	                cr->mappings = save;
	            }
	            delete sm;
	        }
	    }
	    else
	    {
	        FixRule *fr = 
		  (FixRule *)( item->data( 0, Qt::UserRole ).value<void*>() );
	        if( fr )
	        {
		    FixMap *fm = new FixMap( 1, my_map, fr, this );
	            int result = fm->exec();
		    if( result )
	            {
	                item->setToolTip( 0, fixtip( fr ) );
	                ok_btn->setEnabled( true );
	            }
	            delete fm;
	        }
	    }
	}
}

void MapEdit::treeunmapPushed()
{
	QTreeWidgetItem *item = mapping_tree->currentItem();
	if( item )
	{
	    int dir = mapping_tree->indexOfTopLevelItem( item->parent() );
	    if( dir != 3 )
	    {
	        CopyRule *cr = 
		  (CopyRule *)( item->data( 0, Qt::UserRole ).value<void*>() );
	        if( cr )
	        {
		    QTreeWidgetItem *parent = item->parent();
		    parent->takeChild( parent->indexOfChild( item ) );
		    delete item;
		    item = mapping_tree->currentItem();
		    mapClicked( item, 0 );
		    cr->deleted = 1;
	            ok_btn->setEnabled( true );
	            unmark_field( p4_list, cr->scm_field );
	            unmark_field( dts_list, cr->dts_field );
	        }
	    }
	    else
	    {
	        FixRule *fr = 
		  (FixRule *)( item->data( 0, Qt::UserRole ).value<void*>() );
	        if( fr )
	        {
		    QTreeWidgetItem *parent = item->parent();
		    parent->takeChild( parent->indexOfChild( item ) );
		    delete item;
		    item = mapping_tree->currentItem();
		    mapClicked( item, 0 );
		    fr->deleted = 1;
	            ok_btn->setEnabled( true );
	            unmark_field( dts_list, fr->dts_field );
	        }
	    }
	}
}

static void list_possibles( int mirror, FieldUsage *val, 
				QListWidget *list, QStringList &out )
{
	out.clear();
	int use_type = val->type;
	int num = list->count();
	for( int i=0; i < num; i++ )
	{
	    QListWidgetItem *item = list->item( i );
	    // printf("list_possibles: %s\n", qPrintable( item->text() ) );
	    FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	    if( fu->refs || fu->ro )
	        continue;
	    if( use_type == -1 )
	    {
	        if( fu->type == 3 || fu->type == 5 )
	            out << item->text() + QUTF8( " (append)" );
	        continue;
	    }
	    if( fu->type == 5 )
	        continue;
	    if( use_type == 0 )
	    {
	        if( fu->type == 3 || fu->type == 2 )
	            out << item->text();
	        continue;
	    }
	    if( fu->type == use_type )
	    {
	        if( use_type > 5 )
	            out << item->text() + QUTF8( " (map)" );
	        else
	            out << item->text();
		continue;
	    }
	    if( mirror || use_type == 4 || fu->type == 4 )
		continue;
	    if( use_type > 5 )
	    {
	        if( fu->type < 4 )
	        {
	            out << item->text();
	            continue;
	        }
	        out << item->text() + QUTF8( " (map)" );
	        continue;
	    }
	    // use_type < 4
	    if( fu->type > 5 )
	        continue;
	    if( fu->type > use_type )
	    {
	        out << item->text();
	        continue;
	    }
	    out << item->text() + QUTF8( " (truncate)" );
	}
}

static void copy_type_rule( CopyRule *cr, int from, int to )
{
	if( from == 0 )
	    from = 2;
	if( from == -1 )
	    from = 3;
	switch( to )
	{
	case 1: // Word
	    cr->copy_type = CopyRule::WORD;
	    cr->truncate = from > 1 && from < 4;
	    break;
	case 2: // Line
	    cr->copy_type = CopyRule::LINE;
	    cr->truncate = from > 2 && from < 4;
	    break;
	default: // Select, which should not occur
	case 3: // Text
	    cr->copy_type = CopyRule::TEXT;
	    cr->truncate = false;
	    break;
	case 4: // Date
	    cr->copy_type = CopyRule::DATE;
	    cr->truncate = false;
	    break;
	}
}

void MapEdit::new_child( int kind, QTreeWidgetItem *tree, CopyRule *cr )
{
	QTreeWidgetItem *child;
	switch( kind )
	{
	default:
	case 0: // dts <-> scm
	    child = new QTreeWidgetItem(
			QStringList( QUTF8( cr->dts_field ) + 
			DBL_ARROW + 
			QUTF8( cr->scm_field ) + tagit( cr ) ) );
	    if( cr->copy_type == CopyRule::MAP ||
		cr->copy_type == CopyRule::UNMAP )
	        child->setToolTip( 0, tipit( cr, DBL_ARROW ) );
	    break;
	case 1: // dts -> scm
	    child = new QTreeWidgetItem(
			QStringList( QUTF8( cr->dts_field ) + 
			QUTF8( " " ) + R_ARROW + QUTF8( " " ) + 
			QUTF8( cr->scm_field ) + tagit( cr ) ) );
	    if( cr->copy_type == CopyRule::MAP ||
		cr->copy_type == CopyRule::UNMAP )
	        child->setToolTip( 0, tipit( cr, R_ARROW ) );
	    break;
	case 2: // scm -> dts
	    child = new QTreeWidgetItem(
			QStringList( QUTF8( cr->scm_field ) + 
			QUTF8( " " ) + R_ARROW + QUTF8( " " ) + 
			QUTF8( cr->dts_field ) + tagit( cr ) ) );
	    if( cr->copy_type == CopyRule::MAP ||
		cr->copy_type == CopyRule::UNMAP )
	        child->setToolTip( 0, tipit( cr, R_ARROW ) );
	    break;
	}

	int marked = mark_field( p4_list, cr->scm_field );
	marked += mark_field( dts_list, cr->dts_field );
	QVariant data;
	data.setValue( (void *)cr );
	child->setData( 0, Qt::UserRole, data );
	tree->addChild( child );
}

char *pick_list( const char *from, 
		const char *from_field,
		const char *title,
		const char *label,
		QStringList &list,
		QWidget *parent )
{
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget( new QLabel( QUTF8( from ) ), 0 );
	QLineEdit *field = new QLineEdit();
	field->setText( QUTF8( from_field ) );
	field->setReadOnly( true );
	field->setFrame( false );
	hbox->addWidget( field, 1 );
	if( list.empty() )
	    list << QUTF8( "<No valid targets>" );
	PickList *pl = new PickList( title, hbox, label, list, parent );
	if( !pl->exec() )
	{
	    delete pl;
	    return NULL;
	}
	char *line = pl->choice;
	delete pl;
	return line;
}

void MapEdit::p4CopyPushed()
{
	QListWidgetItem *item = p4_list->currentItem();
	if( !item )
	    return;
	FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	if( !fu->refs )
	{
	    QStringList list;
	    list_possibles( 0, fu, dts_list, list );
	    char *line = 
		pick_list( "From Perforce field:", item->text().toUtf8().data(),
			"Copy Perforce to Defect tracker", 
			"Select target DTS field:",
			list, this );
	    if( line )
	    {
	        CopyRule *cr = new CopyRule;
	        cr->scm_field = cp_string( item->text().toUtf8().data() );
	        char *end = strstr( cr->scm_field, " : " );
	        if( end )
	            *end = '\0';
	        // cr->dts_field = cp_string( line->text().toUtf8().data() );
	        cr->dts_field = line;
	        end = strstr( cr->dts_field, " : " );
	        if( end )
	            *end = '\0';
	        QListWidgetItem *toitem = 
			find_item( dts_list, " : ", cr->dts_field );
	        FieldUsage *tofu = toitem ?
		  (FieldUsage *)(toitem->data( Qt::UserRole ).value<void*>()) :
	          NULL;
	        if( !tofu )
	        {
	            delete cr;
	            return;
	        }
	        if( tofu->type > 5 && fu->type > 5 )
	        {
	            cr->copy_type = CopyRule::UNMAP;
		    SelectMap *sm = new SelectMap( 2, my_map, cr, this );
	            if( !sm->exec() )
	            {
	                delete sm;
	                delete cr;
		        p4Clicked( item );
	                return;
	            }
	            cr->next = my_map->scm_to_dts_rules;
	            my_map->scm_to_dts_rules = cr;
	            new_child( 2, p4_to_dts_tree, cr );
	        }
	        else if( fu->type >= 0 )
	        {
	            copy_type_rule( cr, fu->type, tofu->type );
	            cr->next = my_map->scm_to_dts_rules;
	            my_map->scm_to_dts_rules = cr;
	            new_child( 2, p4_to_dts_tree, cr );
	        }
	        else // Fix Rule
	        {
	            FixRule *fr = new FixRule;
	            fr->copy_type = FixRule::APPEND;
	            fr->change_number = true;
	            fr->fixed_by = true;
	            fr->fixed_date = true;
	            fr->dts_field = cr->dts_field;
	            cr->dts_field = NULL;
	            delete cr;
		    FixMap *fm = new FixMap( 0, my_map, fr, this );
	            if( !fm->exec() )
	            {
	                delete fm;
	                delete fr;
		        p4Clicked( item );
	                return;
	            }
	            fr->next = my_map->fix_rules;
	            my_map->fix_rules = fr;

	            QTreeWidgetItem *from = new QTreeWidgetItem(
		        QStringList( QString( QUTF8( "P4Fix " ) ) + R_ARROW +
		        QUTF8( " " ) + QUTF8( fr->dts_field ) +
		        QString( QUTF8( " (append)" ) ) ) );
	            from->setToolTip( 0, fixtip( fr ) );
	            if( mark_field( dts_list, fr->dts_field ) )
	            {
	                QFont font = from->font( 0 );
	                font.setBold( true );
	                from->setFont( 0, font );
	            }
	            mapping_tree->setItemExpanded( from, true );
	            fix_tree->addChild( from );
	            QVariant data;
	            data.setValue( (void *)fr );
	            from->setData( 0, Qt::UserRole, data );
	        }
	        ok_btn->setEnabled( true );
		p4Clicked( item );
	    }
	}
}

void MapEdit::p4MirrorPushed()
{
	QListWidgetItem *item = p4_list->currentItem();
	if( !item )
	    return;
	FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	if( fu->type > 0 )
	{
	    QStringList list;
	    list_possibles( 1, fu, dts_list, list );
	    char *line = 
		pick_list( "Mirror Perforce field:", 
			item->text().toUtf8().data(),
			"Mirror Perforce with Defect tracker", 
			"Select matching Defect tracker field:",
			list, this );
	    if( line )
	    {
	        CopyRule *cr = new CopyRule;
	        cr->scm_field = cp_string( item->text().toUtf8().data() );
	        char *end = strstr( cr->scm_field, " : " );
	        if( end )
	            *end = '\0';
	        cr->dts_field = line;
	        end = strstr( cr->dts_field, " : " );
	        if( end )
	            *end = '\0';
	        QListWidgetItem *toitem = 
			find_item( dts_list, " : ", cr->dts_field );
	        FieldUsage *tofu = toitem ?
		  (FieldUsage *)(toitem->data( Qt::UserRole ).value<void*>()) :
	          NULL;
	        if( !tofu )
	        {
	            delete cr;
	            return;
	        }
	        if( tofu->type > 5 && fu->type > 5 )
	        {
	            cr->copy_type = CopyRule::UNMAP;
		    SelectMap *sm = new SelectMap( 0, my_map, cr, this );
	            if( !sm->exec() )
	            {
	                delete sm;
	                delete cr;
		        p4Clicked( item );
	                return;
	            }
	        }
	        else
	            copy_type_rule( cr, fu->type, tofu->type );
	        cr->next = my_map->mirror_rules;
	        my_map->mirror_rules = cr;
	        new_child( 0, mirror_tree, cr );
	        ok_btn->setEnabled( true );
		p4Clicked( item );
	    }
	}
}

void MapEdit::dtsCopyPushed()
{
	QListWidgetItem *item = dts_list->currentItem();
	if( !item )
	    return;
	FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	if( fu->type > 0 )
	{
	    QStringList list;
	    list_possibles( 0, fu, p4_list, list );
	    char *line = 
		pick_list( "From Defect tracker field:", 
			item->text().toUtf8().data(),
			"Copy Defect tracker to Perforce", 
			"Select target Perforce field:",
			list, this );
	    if( line )
	    {
	        CopyRule *cr = new CopyRule;
	        cr->dts_field = cp_string( item->text().toUtf8().data() );
	        char *end = strstr( cr->dts_field, " : " );
	        if( end )
	            *end = '\0';
	        cr->scm_field = line;
	        end = strstr( cr->scm_field, " : " );
	        if( end )
	            *end = '\0';
	        QListWidgetItem *toitem = 
			find_item( p4_list, " : ", cr->scm_field );
	        FieldUsage *tofu = toitem ?
		  (FieldUsage *)(toitem->data( Qt::UserRole ).value<void*>()) :
	          NULL;
	        if( !tofu )
	        {
	            delete cr;
	            return;
	        }
	        if( tofu->type > 5 && fu->type > 5 )
	        {
	            cr->copy_type = CopyRule::UNMAP;
		    SelectMap *sm = new SelectMap( 1, my_map, cr, this );
	            if( !sm->exec() )
	            {
	                delete sm;
	                delete cr;
		        dtsClicked( item );
	                return;
	            }
	        }
	        else
	            copy_type_rule( cr, fu->type, tofu->type );
	        cr->next = my_map->dts_to_scm_rules;
	        my_map->dts_to_scm_rules = cr;
	        new_child( 1, dts_to_p4_tree, cr );
	        ok_btn->setEnabled( true );
		dtsClicked( item );
	    }
	}
}

void MapEdit::dtsMirrorPushed()
{
	QListWidgetItem *item = dts_list->currentItem();
	if( !item )
	    return;
	FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	if( fu->type > 0 )
	{
	    QStringList list;
	    list_possibles( 1, fu, p4_list, list );
	    char *line = 
		pick_list( "Mirror Defect tracker field:", 
			item->text().toUtf8().data(),
			"Mirror Defect tracker with Perforce", 
			"Select matching Perforce field:",
			list, this );
	    if( line )
	    {
	        CopyRule *cr = new CopyRule;
	        cr->dts_field = cp_string( item->text().toUtf8().data() );
	        char *end = strstr( cr->dts_field, " : " );
	        if( end )
	            *end = '\0';
	        cr->scm_field = line;
	        end = strstr( cr->scm_field, " : " );
	        if( end )
	            *end = '\0';
	        QListWidgetItem *toitem = 
			find_item( p4_list, " : ", cr->scm_field );
	        FieldUsage *tofu = toitem ?
		  (FieldUsage *)(toitem->data( Qt::UserRole ).value<void*>()) :
	          NULL;
	        if( !tofu )
	        {
	            delete cr;
	            return;
	        }
	        if( tofu->type > 5 && fu->type > 5 )
	        {
	            cr->copy_type = CopyRule::UNMAP;
		    SelectMap *sm = new SelectMap( 0, my_map, cr, this );
	            if( !sm->exec() )
	            {
	                delete sm;
	                delete cr;
		        dtsClicked( item );
	                return;
		    }
	        }
	        else
	            copy_type_rule( cr, fu->type, tofu->type );
	        cr->next = my_map->mirror_rules;
	        my_map->mirror_rules = cr;
	        new_child( 0, mirror_tree, cr );
	        ok_btn->setEnabled( true );
		dtsClicked( item );
	    }
	}
}

void MapEdit::p4RowChanged( int row )
{
	p4Clicked( p4_list->item( row ) );
}

void MapEdit::p4Clicked( QListWidgetItem *item )
{
	if( item )
	{
	    FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	    dtsClicked( NULL );
	    mapClicked( NULL, 0 );
	    p4copy_btn->setEnabled( !fu->refs ? true : false );
	    p4mirror_btn->setEnabled( !fu->refs && !fu->ro ? true : false );
	}
	else
	{
	    QListWidgetItem *other = p4_list->currentItem();
	    if( other )
	        p4_list->setItemSelected( other, false );
	    p4copy_btn->setEnabled( false );
	    p4mirror_btn->setEnabled( false );
	}
}

void MapEdit::dtsRowChanged( int row )
{
	dtsClicked( dts_list->item( row ) );
}

void MapEdit::dtsClicked( QListWidgetItem *item )
{
	if( item )
	{
	    FieldUsage *fu = 
		(FieldUsage *)( item->data( Qt::UserRole ).value<void*>() );
	    p4Clicked( NULL );
	    mapClicked( NULL, 0 );
	    dtscopy_btn->setEnabled( !fu->refs ? true : false );
	    dtsmirror_btn->setEnabled( !fu->refs && !fu->ro ? true : false );
	}
	else
	{
	    QListWidgetItem *other = dts_list->currentItem();
	    if( other )
	        dts_list->setItemSelected( other, false );
	    dtscopy_btn->setEnabled( false );
	    dtsmirror_btn->setEnabled( false );
	}
}

void MapEdit::mapClicked( QTreeWidgetItem *item, int /* col */ )
{
	if( item && item->parent() )
	{
	    int dir = item->parent()->data( 0, Qt::UserRole ).toInt();
	    if( dir != 4 )
	    {
	        CopyRule *cr = 
		  (CopyRule *)( item->data( 0, Qt::UserRole ).value<void*>() );
	        if( cr )
	        {
	            treeedit_btn->setEnabled( cr->copy_type == CopyRule::MAP ||
			cr->copy_type == CopyRule::UNMAP );
	            treeunmap_btn->setEnabled( true );
	        }
	        else
	        {
	            treeedit_btn->setEnabled( false );
	            treeunmap_btn->setEnabled( false );
	        }
	    }
	    else
	    {
	        FixRule *fr = 
		  (FixRule *)( item->data( 0, Qt::UserRole ).value<void*>() );
	        if( fr )
	        {
	            treeedit_btn->setEnabled( true );
	            treeunmap_btn->setEnabled( true );
	        }
	        else
	        {
	            treeedit_btn->setEnabled( false );
	            treeunmap_btn->setEnabled( false );
	        }
	    }
	    p4Clicked( NULL );
	    dtsClicked( NULL );
	}
	else
	{
	    QTreeWidgetItem *other = mapping_tree->currentItem();
	    if( other )
	        mapping_tree->setItemSelected( other, false );
	    treeedit_btn->setEnabled( false );
	    treeunmap_btn->setEnabled( false );
	}
}
