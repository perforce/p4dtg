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

#ifndef P4DTG_HEADER
#define P4DTG_HEADER

#define QUTF8( str ) (QString::fromUtf8( str ))

#ifdef _WIN32
#define R_ARROW QUTF8("->")
#define L_ARROW QUTF8("<-")
#define DBL_ARROW (QUTF8(" ") + L_ARROW + R_ARROW + QUTF8(" "))
#else
#define R_ARROW QUTF8("->")
#define L_ARROW QUTF8("<-")
#define DBL_ARROW (QUTF8(" ") + L_ARROW + R_ARROW + QUTF8(" "))
#endif

#define SAFEGET(obj, attr) ((obj && obj->attr) ? obj->attr : "")

#define SAFESET(obj, attr, val) \
  { if( obj ) \
    { \
      if( obj->attr ) \
        delete[] obj->attr; \
      obj->attr = cp_string( val ); \
    }\
  }

class QListWidget;
class QListWidgetItem;
class DTGModule;
class QStringList;
class QTextEdit;
class QGroupBox;
class DataSource;
class DataMapping;
class P4Help;

class P4DTG {
    public:
	P4DTG( const char *use_root );
	~P4DTG();

	char *root;

	DTGModule *plugins;
	DTGModule *p4plugin;

	DataSource *p4_srcs;
	DataSource *dt_srcs;
	DataMapping *maps;

	P4Help *help;

	void about();
	void cache_plugins();
	void setup_help();
	void add_sources( DataSource *srcs );
	void add_maps( DataMapping *maps );
	void replace_src( DataSource *old, DataSource *cur );
	
	int unique_nickname( const char *name );
	int unique_mapid( const char *name );
};

extern P4DTG *global;

extern void possible_modules( DataSource *src, QStringList &list );
extern void possible_defectid( DataSource *src, QStringList &list );
extern void possible_moddate( DataSource *src, QStringList &list );
extern void possible_moduser( DataSource *src, QStringList &list );
extern QListWidgetItem *find_item( QListWidget *list, 
	const char *suffix, const char *field );
extern char *cp_string( const char *str );

extern void set_server_status( 
			DataSource *obj, QTextEdit *status, QGroupBox *box );

#endif
