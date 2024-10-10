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

#ifndef DATASOURCE_HEADER
#define DATASOURCE_HEADER

class NewRule;
class TiXmlElement;

class DataMapping;
struct DTGFieldDesc;
struct DTGAttribute;
class DTGModule;
struct DTGStrList;
struct DTGField;
class DataAttr;
class FilterSet;
class FilterRule;

class DataSource {
    public:
	enum SourceType { SCM, DTS };
	SourceType type;
	char *nickname;
	char *oldname;
	char *plugin;
	char *server;
	char *user;
	char *password;
	char *module;
	char *moddate_field;
	char *moduser_field;
	FilterSet *set_list;
	DataAttr *attrs;
	struct DTGField *field_list;

	struct DTGFieldDesc *cached_fields;
	struct DTGStrList *cached_modules;
	struct DTGAttribute *cached_attributes;

	DataMapping *map;

	DataSource *next;

	enum ConnectStatus { UNKNOWN, FAIL, PASS, READY };
	ConnectStatus status;
	char *warnings;
	char *error;
	char *version;
	int accept_utf8;
	DTGModule *my_mod;
	int deleted;
	int dirty;
	int seg_ok;

	int refcnt;

	const struct DTGField *fields();

    public:
	DataSource(
		SourceType type = DataSource::SCM, 
		const char *plugin = 0x0, 
		DTGModule *mod = 0x0 );
	~DataSource();

	void save( TiXmlElement *doc, int full = 0 );
	static DataSource *load( TiXmlElement *me );

	DataSource *copy( int full = 0 );
	void merge( DataSource *old );
	void clear();

	void set_module( DTGModule *new_mod );

	struct DTGFieldDesc *get_fields();
	struct DTGStrList *get_modules();
	struct DTGAttribute *get_attributes();
	static void assign_plugins( DTGModule *global, DataSource *srcs );
	void check_connection();

	char *validate_attribute( DataAttr *attr );

	void adj_refcnt( int amt, const char *set );

	int has_required_fields();
};

class FilterSet {
    public:
	char *name;
	FilterRule *filter_list;
	
	FilterSet *next;

	int refcnt;

    public:
	FilterSet();
	~FilterSet();

	FilterSet *copy( int full = 0 );

	void save( TiXmlElement *source, int full = 0 );
	static FilterSet *load( TiXmlElement *me );
};

class FilterRule {
    public:
	char *field;
	char *pattern;

	FilterRule *next;

	int old;

    public:
	FilterRule();
	~FilterRule();

	struct DTGFieldDesc *extract_filter();

	void save( TiXmlElement *source );
	static FilterRule *load( TiXmlElement *me );
};

#endif
