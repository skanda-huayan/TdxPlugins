#include "sqlite3.h"
#include <windows.h>

static void * adr[50] = {0};

int sqlite3_init_library(const char *dll) {
	HMODULE lib = LoadLibrary(dll);
	if (lib == 0) return 0;
	adr[0] = (void*)GetProcAddress(lib, "sqlite3_close");
	adr[1] = (void*)GetProcAddress(lib, "sqlite3_exec");
	adr[2] = (void*)GetProcAddress(lib, "sqlite3_last_insert_rowid");
	adr[3] = (void*)GetProcAddress(lib, "sqlite3_changes");
	adr[4] = (void*)GetProcAddress(lib, "sqlite3_get_table");
	adr[5] = (void*)GetProcAddress(lib, "sqlite3_free_table");
	adr[6] = (void*)GetProcAddress(lib, "sqlite3_free");
	adr[7] = (void*)GetProcAddress(lib, "sqlite3_progress_handler");
	adr[8] = (void*)GetProcAddress(lib, "sqlite3_open");
	adr[9] = (void*)GetProcAddress(lib, "sqlite3_errmsg");
	adr[10] = (void*)GetProcAddress(lib, "sqlite3_prepare");
	adr[11] = (void*)GetProcAddress(lib, "sqlite3_bind_blob");
	adr[12] = (void*)GetProcAddress(lib, "sqlite3_bind_double");
	adr[13] = (void*)GetProcAddress(lib, "sqlite3_bind_int");
	adr[14] = (void*)GetProcAddress(lib, "sqlite3_bind_int64");
	adr[15] = (void*)GetProcAddress(lib, "sqlite3_bind_text");
	adr[16] = (void*)GetProcAddress(lib, "sqlite3_bind_parameter_count");
	adr[17] = (void*)GetProcAddress(lib, "sqlite3_clear_bindings");
	adr[18] = (void*)GetProcAddress(lib, "sqlite3_column_count");
	adr[19] = (void*)GetProcAddress(lib, "sqlite3_column_name");
	adr[20] = (void*)GetProcAddress(lib, "sqlite3_column_origin_name");
	adr[21] = (void*)GetProcAddress(lib, "sqlite3_step");
	adr[22] = (void*)GetProcAddress(lib, "sqlite3_data_count");
	adr[23] = (void*)GetProcAddress(lib, "sqlite3_column_double");
	adr[24] = (void*)GetProcAddress(lib, "sqlite3_column_int");
	adr[25] = (void*)GetProcAddress(lib, "sqlite3_column_int64");
	adr[26] = (void*)GetProcAddress(lib, "sqlite3_column_text");
	adr[27] = (void*)GetProcAddress(lib, "sqlite3_finalize");
	adr[28] = (void*)GetProcAddress(lib, "sqlite3_reset");
	adr[29] = (void*)GetProcAddress(lib, "sqlite3_column_blob");
	adr[30] = (void*)GetProcAddress(lib, "sqlite3_column_bytes");
	
	return 1;
}

int sqlite3_close(sqlite3* db) {
	return ((int (*)(sqlite3*))adr[0])(db);
}

int sqlite3_exec(sqlite3* db,  const char *sql, int (*cb)(void*,int,char**,char**), void * dat, char **errmsg) {
	return ((int (*)(sqlite3*, const char *, int (*)(void*,int,char**,char**), void*, char**))adr[1])(db, sql,cb, dat, errmsg);
}

sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db) {
	return ((sqlite3_int64 (*)(sqlite3*))adr[2])(db);
}

int sqlite3_changes(sqlite3* db) {
	return ((int (*)(sqlite3*))adr[3])(db);
}

int sqlite3_get_table(sqlite3 *db,  const char *zSql,  char ***pazResult, int *pnRow,  int *pnColumn,  char **pzErrmsg) {
	return ((int (*)(sqlite3*,const char *,char ***, int *, int *, char**))adr[4])(db, zSql, pazResult, pnRow, pnColumn, pzErrmsg);
}

void sqlite3_free_table(char **result) {
	((void (*)(char **))adr[5])(result);
}

void sqlite3_free(void* d) {
	((void (*)(void*))adr[6])(d);
}

void sqlite3_progress_handler(sqlite3* db, int a, int(*b)(void*), void* d) {
	((void (*)(sqlite3*, int, int(*)(void*), void*))adr[7])(db, a, b, d);
}

int sqlite3_open( const char *filename, sqlite3 **ppDb) {
	return ((int (*)(const char *, sqlite3 **))adr[8])(filename, ppDb);
}

const char *sqlite3_errmsg(sqlite3* db) {
	return ((const char * (*)(sqlite3*))adr[9])(db);
}

int sqlite3_prepare( sqlite3 *db,  const char *zSql,  int nByte, sqlite3_stmt **ppStmt, const char **pzTail) {
	return ((int (*)( sqlite3 *,  const char *,  int , sqlite3_stmt **, const char ** ))adr[10])(db, zSql, nByte,ppStmt,  pzTail);
}

int sqlite3_bind_blob(sqlite3_stmt* a, int b , const void* c , int n, void(*d)(void*)) {
	return ((int (*)(sqlite3_stmt*, int, const void*, int, void(*)(void*)))adr[11])(a, b, c, n, d);
}
int sqlite3_bind_double(sqlite3_stmt* a, int b, double c) {
	return ((int (*)(sqlite3_stmt*, int, double))adr[12])(a, b, c);
}
int sqlite3_bind_int(sqlite3_stmt* a, int b, int c) {
	return ((int (*)(sqlite3_stmt*, int, int))adr[13])(a, b, c);
}
int sqlite3_bind_int64(sqlite3_stmt* a, int b, sqlite3_int64 c) {
	return ((int (*)(sqlite3_stmt*, int, sqlite3_int64))adr[14])(a, b, c);
}
int sqlite3_bind_text(sqlite3_stmt* a,int b ,const char* c,int n,void(*d)(void*)) {
	return ((int (*)(sqlite3_stmt*, int, const char*,int,void(*)(void*)))adr[15])(a, b, c, n, d);
}

int sqlite3_bind_parameter_count(sqlite3_stmt* a) {
	return ((int (*)(sqlite3_stmt*))adr[16])(a);
}

int sqlite3_clear_bindings(sqlite3_stmt* a) {
	return ((int (*)(sqlite3_stmt*))adr[17])(a);
}

// ?
int sqlite3_column_count(sqlite3_stmt *a) {
	return ((int (*)(sqlite3_stmt*))adr[18])(a);
}

const char *sqlite3_column_name(sqlite3_stmt* a, int b) {
	return ((const char * (*)(sqlite3_stmt*, int))adr[19])(a, b);
}

const char *sqlite3_column_origin_name(sqlite3_stmt* a,int b) {
	return ((const char * (*)(sqlite3_stmt*, int))adr[20])(a, b);
}

int sqlite3_step(sqlite3_stmt* a) {
	return ((int (*)(sqlite3_stmt*))adr[21])(a);
}

//  Number of columns in a result set
int sqlite3_data_count(sqlite3_stmt *a) {
	return ((int (*)(sqlite3_stmt*))adr[22])(a);
}

void *sqlite3_column_blob(sqlite3_stmt* a, int iCol) {
	return ((void* (*)(sqlite3_stmt*, int))adr[29])(a, iCol);
}

int sqlite3_column_bytes(sqlite3_stmt* a, int iCol) {
	return ((int (*)(sqlite3_stmt*, int))adr[30])(a, iCol);
}

// Result Values From A Query
double sqlite3_column_double(sqlite3_stmt* a, int iCol) {
	return ((double (*)(sqlite3_stmt*, int))adr[23])(a, iCol);
}
int sqlite3_column_int(sqlite3_stmt* a, int iCol) {
	return ((int (*)(sqlite3_stmt*, int))adr[24])(a, iCol);
}
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* a, int iCol) {
	return ((sqlite3_int64 (*)(sqlite3_stmt*, int))adr[25])(a, iCol);
}
const unsigned char *sqlite3_column_text(sqlite3_stmt* a, int iCol) {
	return ((const unsigned char * (*)(sqlite3_stmt*, int))adr[26])(a, iCol);
}

int sqlite3_finalize(sqlite3_stmt *a) {
	return ((int (*)(sqlite3_stmt*))adr[27])(a);
}

// Reset A Prepared Statement Object
int sqlite3_reset(sqlite3_stmt *a) {
	return ((int (*)(sqlite3_stmt*))adr[28])(a);
}

static int trans_ok = 0;
int sqlite3_begin_transaction(sqlite3* db) {
	trans_ok = 0;
	return sqlite3_exec(db, "begin transaction", NULL, NULL, NULL);
}

void sqlite3_transaction_success() {
	trans_ok = 1;
}

int sqlite3_end_transaction(sqlite3* db) {
	if (trans_ok == 1) {
		trans_ok = 0;
		return sqlite3_exec (db ,"commit transaction", NULL , NULL , NULL);
	}
	return  sqlite3_exec(db, "rollback transaction", NULL , NULL , NULL);
}

