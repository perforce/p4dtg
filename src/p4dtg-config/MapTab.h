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

#ifndef MAPTAB_HEADER
#define MAPTAB_HEADER

#include <QApplication>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QWidget>

class MainDialog;
class DataMapping;

class MapTab : public QWidget
{
	Q_OBJECT

    public:
	MapTab( MainDialog *my_base, QWidget *parent = 0 );

	void refreshList();

	void save_settings( DataMapping *map );

    private slots:
	void listClicked( QListWidgetItem *item );
	void rowChanged( int row );
	void editPushed();
	void newPushed();
	void deletePushed();
	void runPushed();

    private:
	QListWidget *list;
	QPushButton *edit_btn;
	QPushButton *new_btn;
	QPushButton *delete_btn;

	QLineEdit *name_edit;
	QLineEdit *dts_edit;
	QLineEdit *scm_edit;
	QLineEdit *status_edit;
	QDateTimeEdit *start_edit;
	QLineEdit *last_edit;
	QCheckBox *sync_edit;
	QPushButton *run_btn;

	MainDialog *base;
	int stop_action;
};

#endif
