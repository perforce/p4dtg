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

#ifndef PICKLIST_HEADER
#define PICKLIST_HEADER

#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QWidget>
#include <QStringList>
#include <QLayout>

class PickList : public QDialog
{
	Q_OBJECT

    public:
	PickList( 
		const char *title, 
		QLayout *hbox,
		const char *label,
		QStringList &items, 
		QWidget *parent = 0 );

	char *choice;

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();

	void listClicked( QListWidgetItem *item );

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;

	QListWidget *item_list;
};

#endif
