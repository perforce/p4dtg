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

#ifndef SELECTMAP_HEADER
#define SELECTMAP_HEADER

#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QWidget>

class CopyRule;
class DataMapping;
struct DTGFieldDesc;

class SelectMap : public QDialog
{
	Q_OBJECT

    public:
	SelectMap( int kind, 
		DataMapping *map, 
		CopyRule *cr, 
		QWidget *parent = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();

	void mapPushed();
	void deletePushed();
	void delallPushed();

	void p4valClicked( QListWidgetItem *item );
	void dtvalClicked( QListWidgetItem *item );
	void mapClicked( QListWidgetItem *item );

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;

	QPushButton *map_btn;
	QPushButton *del_btn;
	QPushButton *delall_btn;

	QListWidget *p4val_list;
	QListWidget *dtval_list;
	QListWidget *map_list;

	QListWidget *left_list;
	QListWidget *right_list;

	DataMapping *my_map;
	CopyRule *my_cr;
	int my_dir;
	QString copy_op;

	struct DTGFieldDesc *scm_fields; // cached copy w/ changes for segs
	struct DTGFieldDesc *dts_fields; // cached copy w/ changes for segs
};

#endif
