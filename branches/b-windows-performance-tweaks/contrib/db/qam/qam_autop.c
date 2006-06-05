/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#ifdef HAVE_QUEUE
#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <ctype.h>
#include <string.h>
#endif

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __qam_incfirst_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_incfirst_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_incfirst_args *argp;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __qam_incfirst_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__qam_incfirst%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\tmeta_pgno: %lu\n", (u_long)argp->meta_pgno);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_mvptr_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_mvptr_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_mvptr_args *argp;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __qam_mvptr_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__qam_mvptr%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\told_first: %lu\n", (u_long)argp->old_first);
	(void)printf("\tnew_first: %lu\n", (u_long)argp->new_first);
	(void)printf("\told_cur: %lu\n", (u_long)argp->old_cur);
	(void)printf("\tnew_cur: %lu\n", (u_long)argp->new_cur);
	(void)printf("\tmetalsn: [%lu][%lu]\n",
	    (u_long)argp->metalsn.file, (u_long)argp->metalsn.offset);
	(void)printf("\tmeta_pgno: %lu\n", (u_long)argp->meta_pgno);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_del_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_del_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_del_args *argp;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __qam_del_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__qam_del%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_add_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_add_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_add_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __qam_add_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__qam_add%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\tdata: ");
	for (i = 0; i < argp->data.size; i++) {
		ch = ((u_int8_t *)argp->data.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tvflag: %lu\n", (u_long)argp->vflag);
	(void)printf("\tolddata: ");
	for (i = 0; i < argp->olddata.size; i++) {
		ch = ((u_int8_t *)argp->olddata.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_delext_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_delext_print(dbenv, dbtp, lsnp, notused2, notused3)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_delext_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_ABORT;
	notused3 = NULL;

	if ((ret = __qam_delext_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
	    "[%lu][%lu]__qam_delext%s: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\tdata: ");
	for (i = 0; i < argp->data.size; i++) {
		ch = ((u_int8_t *)argp->data.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	__os_free(dbenv, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_init_print __P((DB_ENV *, int (***)(DB_ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *), size_t *));
 */
int
__qam_init_print(dbenv, dtabp, dtabsizep)
	DB_ENV *dbenv;
	int (***dtabp)__P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t *dtabsizep;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __qam_incfirst_print, DB___qam_incfirst)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __qam_mvptr_print, DB___qam_mvptr)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __qam_del_print, DB___qam_del)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __qam_add_print, DB___qam_add)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __qam_delext_print, DB___qam_delext)) != 0)
		return (ret);
	return (0);
}
#endif /* HAVE_QUEUE */
