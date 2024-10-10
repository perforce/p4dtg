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

#ifndef SEGEDIT_HEADER
#define SEGEDIT_HEADER

#include <QApplication>
#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QListWidget>
#include <QWidget>

class DataSource;
struct DTGFieldDesc;

class SegmentEdit : public QDialog
{
	Q_OBJECT

    public:
	SegmentEdit( QString nick, DataSource *src, QWidget *parent = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();

	void fieldSwitched( const QString &text );

	void listClicked( QListWidgetItem *item );
	void newPushed();
	void deletePushed();

	void hasClicked( QListWidgetItem *item );
	void availClicked( QListWidgetItem *item );
	void addPushed();
	void delPushed();

	void populate_lists();
	void config_btns();

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;

	QComboBox *field_combo;

	QListWidget *list;
	QPushButton *new_btn;
	QPushButton *delete_btn;

	QListWidget *has_list;
	QListWidget *avail_list;
	QToolButton *add_btn;
	QToolButton *del_btn;

	DataSource *src;

	int cur_index;
	struct DTGFieldDesc *cur_field;
};

#endif
