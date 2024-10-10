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

#ifndef NEWMAP_HEADER
#define NEWMAP_HEADER

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QWidget>

class DataMapping;

class NewMap : public QDialog
{
	Q_OBJECT

    public:
	NewMap( DataMapping *map, QWidget *parent = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();
	void p4srcSwitched( const QString &text );
	void dtsrcSwitched( const QString &text );
	void nameChanged( const QString &text );

	void config_btns();

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;

	QLineEdit *name_edit;
	QComboBox *p4_combo;
	QComboBox *dt_combo;

	DataMapping *my_map;
};

#endif
