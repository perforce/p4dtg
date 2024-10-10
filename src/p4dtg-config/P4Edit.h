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

#ifndef P4EDIT_HEADER
#define P4EDIT_HEADER

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

class P4Edit : public QDialog
{
	Q_OBJECT

    public:
	P4Edit( DataSource *obj, QWidget *parent = 0 );

    private slots:
	void okPushed();
	void cancelPushed();
	void helpPushed();
	void checkPushed();
	void segmentPushed();
	void attributesPushed();
	void moddateSwitched( const QString &text );
	void moduserSwitched( const QString &text );

	void nickChanged( const QString &text );
	void serverChanged( const QString &text );
	void userChanged( const QString &text );
	void passChanged( const QString &text );

	void setup_combos();
	void enable_ok();

    private:
	QPushButton *help_btn;
	QPushButton *ok_btn;
	QPushButton *cancel_btn;
	QLineEdit *nickname_edit;
	QLineEdit *server_edit;
	QLineEdit *user_edit;
	QLineEdit *password_edit;
	QPushButton *check_btn;
	QPushButton *segment_btn;
	QPushButton *attributes_btn;
	QTextEdit *status_edit;
	QComboBox *moddate_combo;
	QComboBox *moduser_combo;

	QGroupBox *box;

	QString nick;
	QString server;
	QString user;
	QString pass;

	DataSource *obj;
};

#endif
