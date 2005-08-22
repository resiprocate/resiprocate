/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH SYS_TIME */

#include <ctype.h>
#include <string.h>
#endif

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/db_shash.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __txn_regop_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_regop_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_regop_args *argp;
	struct tm *lt;
	time_t timeval;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __txn_regop_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__txn_regop%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %ld\n", (long)argp->opcode);
	timeval = (time_t)argp->timestamp;
	lt = localtime(&timeval);
	(void)printf(
	    "\ttimestamp: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->timestamp, ctime(&timeval),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\tlocks: \n");
	__lock_list_print(dbenv, &argp->locks);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_ckp_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_ckp_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_ckp_args *argp;
	struct tm *lt;
	time_t timeval;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __txn_ckp_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__txn_ckp%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tckp_lsn: [%lu][%lu]\n",
	    (u_long)argp->ckp_lsn.file, (u_long)argp->ckp_lsn.offset);
	(void)printf("\tlast_ckp: [%lu][%lu]\n",
	    (u_long)argp->last_ckp.file, (u_long)argp->last_ckp.offset);
	timeval = (time_t)argp->timestamp;
	lt = localtime(&timeval);
	(void)printf(
	    "\ttimestamp: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->timestamp, ctime(&timeval),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\tenvid: %ld\n", (long)argp->envid);
	(void)printf("\trep_gen: %ld\n", (long)argp->rep_gen);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_child_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_child_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_child_args *argp;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __txn_child_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__txn_child%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tchild: 0x%lx\n", (u_long)argp->child);
	(void)printf("\tc_lsn: [%lu][%lu]\n",
	    (u_long)argp->c_lsn.file, (u_long)argp->c_lsn.offset);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_xa_regop_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_xa_regop_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_xa_regop_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __txn_xa_regop_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__txn_xa_regop%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\txid: ");
	for (i = 0; i < argp->xid.size; i++) {
		ch = ((u_int8_t *)argp->xid.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tformatID: %ld\n", (long)argp->formatID);
	(void)printf("\tgtrid: %u\n", argp->gtrid);
	(void)printf("\tbqual: %u\n", argp->bqual);
	(void)printf("\tbegin_lsn: [%lu][%lu]\n",
	    (u_long)argp->begin_lsn.file, (u_long)argp->begin_lsn.offset);
	(void)printf("\tlocks: \n");
	__lock_list_print(dbenv, &argp->locks);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_recycle_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_recycle_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_recycle_args *argp;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __txn_recycle_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__txn_recycle%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tmin: %u\n", argp->min);
	(void)printf("\tmax: %u\n", argp->max);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_init_print __P((DB_ENV *, int (***)(DB_ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *), size_t *));
 */
int
__txn_init_print(dbenv, dtabp, dtabsizep)
	DB_ENV *dbenv;
	int (***dtabp)__P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t *dtabsizep;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __txn_regop_print, DB___txn_regop)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __txn_ckp_print, DB___txn_ckp)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __txn_child_print, DB___txn_child)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __txn_xa_regop_print, DB___txn_xa_regop)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __txn_recycle_print, DB___txn_recycle)) != 0)
		return (ret);
	return (0);
}
