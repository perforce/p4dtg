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

#ifndef MAPEDIT_HEADER
#define MAPEDIT_HEADER

#include <QApplication>
#include <QDialog>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

class DataMapping;
class MapTab;
class CopyRule;

class MapEdit : public QDialog
{
	Q_OBJECT

    public:
	MapEdit( DataMapping *map, QWidget *parent = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();
	void p4RowChanged( int row );
	void p4Clicked( QListWidgetItem *item );
	void dtsRowChanged( int row );
	void dtsClicked( QListWidgetItem *item );
	void mapClicked( QTreeWidgetItem *item, int col );
	void p4CopyPushed();
	void p4MirrorPushed();
	void dtsCopyPushed();
	void dtsMirrorPushed();
	void treeeditPushed();
	void treeunmapPushed();
	void attributesPushed();

	int pop_tree();
	void new_child( int kind, QTreeWidgetItem *tree, CopyRule *cr );

    protected:
	friend class MapTab;
	QPushButton *ok_btn;
    private:
	QPushButton *help_btn;
	QPushButton *cancel_btn;
	QListWidget *p4_list;
	QPushButton *p4copy_btn;
	QPushButton *p4mirror_btn;
	QListWidget *dts_list;
	QPushButton *dtscopy_btn;
	QPushButton *dtsmirror_btn;
	QComboBox *filter_combo;
	QTreeWidget *mapping_tree;
	QTreeWidgetItem *mirror_tree;
	QTreeWidgetItem *p4_to_dts_tree;
	QTreeWidgetItem *dts_to_p4_tree;
	QTreeWidgetItem *fix_tree;
	QPushButton *treeedit_btn;
	QPushButton *treeunmap_btn;
	QPushButton *attributes_btn;

	DataMapping *my_map;
	int badmaps;
};

#endif
