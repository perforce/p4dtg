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

#ifndef FIXMAP_HEADER
#define FIXMAP_HEADER

#include <QApplication>
#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QWidget>

class FixRule;
class DataMapping;

class FixMap : public QDialog
{
	Q_OBJECT

    public:
	FixMap( int edit, DataMapping *map, FixRule *fr, QWidget *parent = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();

	void checked( int s );

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;

	QCheckBox *list_check;
	QCheckBox *change_check;
	QCheckBox *by_check;
	QCheckBox *date_check;
	QCheckBox *desc_check;

	DataMapping *my_map;
	FixRule *my_fr;
};

#endif
