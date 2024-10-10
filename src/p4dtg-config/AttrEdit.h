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

#ifndef ADDREDIT_HEADER
#define ADDREDIT_HEADER

#include <QApplication>
#include <QDialog>
#include <QComboBox>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>

class DataSource;
class DataMapping;
class DataAttr;
struct DTGAttribute;
class FieldWidget;;

class AttrEdit : public QDialog
{
	Q_OBJECT

    public:
	AttrEdit( DataSource *in_src, DataMapping *in_map,
			DataAttr *in_attrs, 
			QWidget *parent  = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();

	void valueChanged( const QString &text );
	void valueEdited();
	void descPushed();

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;

	QLabel *desc1_label;
	QLineEdit *value1_edit;
	QLabel *desc2_label;
	QLineEdit *value2_edit;

	DataSource *source;
	DataMapping *map;
	const struct DTGAttribute *fields;
	DataAttr *attrs;
	FieldWidget *list;

	FieldWidget *last_item;
};

#endif
