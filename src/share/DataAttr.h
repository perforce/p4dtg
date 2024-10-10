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

#ifndef DATAATTR_HEADER
#define DATAATTR_HEADER

class TiXmlElement;

class DataAttr {
    public:
	char *name;
	char *value;

	DataAttr *next;

    public:
	DataAttr();
	DataAttr( const char *name, const char *value );
	~DataAttr();

	DataAttr *copy();

	void save( TiXmlElement *doc );
	static DataAttr *load( TiXmlElement *me );
};

#endif
