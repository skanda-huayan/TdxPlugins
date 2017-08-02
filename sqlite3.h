#pragma once

typedef long long int sqlite3_int64;
typedef unsigned long long int sqlite3_uint64;
typedef int (*sqlite3_callback)(void* usrdata, int colNum, char** colVal, char** colName);
typedef struct _sqlite3 sqlite3;
typedef struct _sqlite3_stmt sqlite3_stmt;
typedef struct _Mem sqlite3_value;

#define SQLITE_OK           0
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */

int sqlite3_init_library(const char *dll);

int sqlite3_close(sqlite3*);

int sqlite3_exec(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);

sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*);

// Count The Number Of Rows Modified
int sqlite3_changes(sqlite3*);

int sqlite3_get_table(
  sqlite3 *db,          /* An open database */
  const char *zSql,     /* SQL to be evaluated */
  char ***pazResult,    /* Results of the query */
  int *pnRow,           /* Number of result rows written here */
  int *pnColumn,        /* Number of result columns written here */
  char **pzErrmsg       /* Error msg written here */
);
void sqlite3_free_table(char **result);

void sqlite3_free(void*);

// Query Progress Callbacks
void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*);

int sqlite3_open(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);

const char *sqlite3_errmsg(sqlite3*);

int sqlite3_prepare(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);

// idx start from 1; idx=[1,2,....]
int sqlite3_bind_blob(sqlite3_stmt*, int idx, const void*, int len, void(*destructor)(void*));
int sqlite3_bind_double(sqlite3_stmt*, int, double);
int sqlite3_bind_int(sqlite3_stmt*, int, int);
int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64);
int sqlite3_bind_text(sqlite3_stmt*,int,const char*,int len,void(*destructor)(void*));

int sqlite3_bind_parameter_count(sqlite3_stmt*);

int sqlite3_clear_bindings(sqlite3_stmt*);

// ?
int sqlite3_column_count(sqlite3_stmt *pStmt);

const char *sqlite3_column_name(sqlite3_stmt*, int N);

const char *sqlite3_column_origin_name(sqlite3_stmt*,int);

int sqlite3_step(sqlite3_stmt*);

//  Number of columns in a result set
int sqlite3_data_count(sqlite3_stmt *pStmt);

// Result Values From A Query
void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
double sqlite3_column_double(sqlite3_stmt*, int iCol);
int sqlite3_column_int(sqlite3_stmt*, int iCol);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);

int sqlite3_finalize(sqlite3_stmt *pStmt);

// Reset A Prepared Statement Object
int sqlite3_reset(sqlite3_stmt *pStmt);

int sqlite3_begin_transaction(sqlite3*);
void sqlite3_transaction_success();
int sqlite3_end_transaction(sqlite3*);





















