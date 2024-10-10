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

#ifndef DATAMAPPING_HEADER
#define DATAMAPPING_HEADER

class CopyRule;
class FilterRule;
class FixRule;
class CopyMap;
class TiXmlElement;
struct DTGFieldDesc;
class Logger;
struct DTGStrList;
class DataAttr;

class DataSource;

class DataMapping {
    public:
	int deleted;
	int dirty;
	int valid;

	char *id;
	char *oldid;
	char *scm_id;
	DataSource *scm;
	char *scm_filter;
	FilterRule *scm_filters;
	struct DTGFieldDesc *scm_filter_desc;
	char *dts_id;
	DataSource *dts;
	char *dts_filter;
	FilterRule *dts_filters;
	struct DTGFieldDesc *dts_filter_desc;
	FixRule *fix_rules;
	enum ConflictChoice { SCM, DTS, NEWER, ERROR };
	ConflictChoice mirror_conflicts;
	CopyRule *mirror_rules;
	CopyRule *scm_to_dts_rules;
	CopyRule *dts_to_scm_rules;
	DataAttr *attrs;

	int recheck_on_new_scm; // Copy from SCM uses DefectID field
	int recheck_on_new_dts; // Copy from DTS uses DefectID field

	DataMapping *next;

	struct DTGAttribute *cached_attributes;

    public:
	DataMapping();
	~DataMapping();

	DataMapping *copy();

	void save( TiXmlElement *doc );
	static DataMapping *load( TiXmlElement *me );

	static void cross_reference( DataSource *srcs, DataMapping *maps );

	int validate( int enable_write_to_readonly, Logger *log );
	struct DTGAttribute *get_attributes();
	char *validate_attribute( DataAttr *attr );

	void set_filters();

	struct DTGStrList *dts_field_references();
};

class FixRule {
    public:
	int deleted;

	char *dts_field;
	enum CopyAction { APPEND, REPLACE };
	CopyAction copy_type;
	bool file_list;
	bool change_number;
	bool description;
	bool fixed_by;
	bool fixed_date;

	FixRule *next;

    public:
	FixRule();
	~FixRule();

	void save( TiXmlElement *data_mapping );
	static FixRule *load( TiXmlElement *me );
};

class CopyRule {
    public:
	int deleted;

	char *scm_field;
	char *dts_field;
	enum CopyAction { TEXT, WORD, LINE, DATE, UNMAP, MAP };
	CopyAction copy_type;
	enum ConflictChoice { SCM, DTS };
	ConflictChoice mirror_conflicts;
	bool truncate;
	CopyMap *mappings;

	CopyRule *next;

    public:
	CopyRule();
	~CopyRule();

	void save( TiXmlElement *data_mapping );
	static CopyRule *load( TiXmlElement *me );
};

class CopyMap {
    public:
	char *value1;
	char *value2;

	CopyMap *next;

    public:
	CopyMap();
	~CopyMap();

	void save( TiXmlElement *copy_rule );
	static CopyMap *load( TiXmlElement *me );

	CopyMap *copy();
};

#endif
