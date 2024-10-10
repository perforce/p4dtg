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

#ifndef SETTINGS_HEADER
#define SETTINGS_HEADER

struct DTGDate;
class TiXmlElement;

class DTGSettings {
    public:
	char *id;
	char *oldid;
	DTGDate *starting_date;
	DTGDate *last_update;
	DTGDate *last_update_dts;
	DTGDate *last_update_scm;
	char *notify_email;
	char *from_address;
	bool force;

	DTGSettings *next;

	int deleted;
	int dirty;

    public:
	DTGSettings();
	~DTGSettings();

	void save( TiXmlElement *doc );
	static DTGSettings *load( TiXmlElement *me );
};

#endif
