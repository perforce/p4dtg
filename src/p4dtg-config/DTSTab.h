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

#ifndef DTSTAB_HEADER
#define DTSTAB_HEADER

#include <QApplication>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>

class DataSource;
class MainDialog;

class DTSTab : public QWidget
{
	Q_OBJECT

    public:
	DTSTab( MainDialog *my_base, QWidget *parent = 0 );

	void refreshList();

    private slots:
	void listClicked( QListWidgetItem *item );
	void editPushed();
	void newPushed();
	void deletePushed();
	void rowChanged( int row );

	void displayObject( DataSource *src );

    private:
	QListWidget *list;
	QPushButton *edit_btn;
	QPushButton *new_btn;
	QPushButton *delete_btn;
	QLineEdit *nickname_edit;
	QLineEdit *dtstype_edit;
	QLineEdit *server_edit;
	QLineEdit *user_edit;
	QLineEdit *password_edit;
	QTextEdit *status_edit;
	QLineEdit *module_edit;
	QLineEdit *moddate_edit;
	QLineEdit *moduser_edit;

	MainDialog *base;
};

#endif
