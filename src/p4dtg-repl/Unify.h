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

#ifndef UNIFY_HEADER
#define UNIFY_HEADER

class DataMapping;
class DataSource;
class DTGModule;
class CopyRule;
class FixRule;
class Logger;
struct DTGDate;
struct DTGStrList;
struct DTGField;
struct DTGSettings;

class Unify {
    protected:
	DataMapping *map;
	
	DTGSettings *set;
	struct DTGDate *since_scm; 
	struct DTGDate *since_dts; 

	DataSource *scm;
	DTGModule *scm_mod;
	void *scm_dtID;
	void *scm_projID;
	int scm_dirty;

	DataSource *dts;
	DTGModule *dts_mod;
	void *dts_dtID;
	void *dts_projID;
	int dts_dirty;
	int report_id;

	void get_project_id( DataSource *src, void *&dtID, void *&projID );
	int fail_on_read_err( struct DTGError *err, const char *type,
				const char *id, const char *field );

	char *cur_scm;
	char *cur_dts;

	int query_cnt;
	int abort_run;
	int force_exit;

	struct DTGStrList *scm_recheck;
	struct DTGField *scm_failed;

    public:
	Logger *log;
	char *stop_file;
	char *run_file;
	char *err_file;

    public:
	Unify( DataMapping *my_map, Logger *my_log );
	~Unify();

	void *get_scmID() { return scm_dtID; }
	void *get_dtsID() { return dts_dtID; }

	char *convert( DTGModule *from, const char *new_val, 
			CopyRule *cr, DTGModule *to, int rev = 0 );
	void log_fatal( int last_chance, const char *msg );
	void fail_scm( struct DTGField *pair );
	void process_scm_defect( const char *defect, int last_chance = 0 );
	void process_dts_defect( const char *defect, int last_chance = 0 );
	int server_offline( DTGModule * mod, void* id );
	int stop_exists();
	int unify( DTGSettings *set );

	void unify_defects( int scm_stat, void *scm_defect, 
			int dts_stat, void *dts_defect,
			struct DTGStrList *add, struct DTGStrList *del );
	
	char *update_fix_record( const char *id, void *scm_id,
		struct DTGStrList *&add_fixes, 
		struct DTGStrList *&del_fixes );
	char *format_fix( FixRule *fr, char *fixid );

	int reset_servers();
	int reset_scm();
	int reset_dts();
};

#endif
