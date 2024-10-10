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

#include "ListSource.h"
#include <QDir>
#include <QStringList>
#include <QListWidget>
#include <QListWidgetItem>
#include <DataSource.h>

ListSource *ListSource::find_source( QListWidget *list )
{
	if( !list )
	    return NULL;

	QListWidgetItem *item = list->currentItem();
	if( !item )
	    return NULL;

	ListSource *src = 
		(ListSource *)(item->data( Qt::UserRole).value<void*>());

	return src;
}
