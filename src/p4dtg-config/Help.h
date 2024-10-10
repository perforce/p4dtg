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

#ifndef P4HELP_HEADER
#define P4HELP_HEADER

#include <QApplication>
#include <QDialog>
#include <QWidget>

class QTextBrowser;

class P4Help : public QDialog
{
	Q_OBJECT

    public:
	P4Help( const char *helpdir, const char *url );
	virtual ~P4Help();

	void showSource( const char *baseurl, QWidget *caller );

    private slots:
	void closePushed();

    private:
	QTextBrowser *browser;

	int my_x;
	int my_y;
	int my_width;
	int my_height;
};

#endif
