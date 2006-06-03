/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

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
#include "dbinc/txn.h"
#include "dbinc/fop.h"

/*
 * PUBLIC: int __fop_create_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, u_int32_t, u_int32_t));
 */
int
__fop_create_log(dbenv, txnid, ret_lsnp, flags,
    name, appname, mode)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	u_int32_t appname;
	u_int32_t mode;
{
	DBT logrec;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rectype = DB___fop_create;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnid == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnid == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnid->kids) != NULL &&
		    (ret = __txn_activekids(dbenv, rectype, txnid)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		*/
		DB_SET_BEGIN_LSNP(txnid, &rlsnp);
		txn_num = txnid->txnid;
		lsnp = &txnid->last_lsn;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t);
	if (CRYPTO_ON(dbenv)) {
		npad =
		    ((DB_CIPHER *)dbenv->crypto_handle)->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnid == NULL) {
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(dbenv,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0) {
			__os_free(dbenv, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);

	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);

	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	if (name == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &name->size, sizeof(name->size));
		bp += sizeof(name->size);
		memcpy(bp, name->data, name->size);
		bp += name->size;
	}

	uinttmp = (u_int32_t)appname;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)mode;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnid == NULL) {
		if ((ret = __log_put(dbenv, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnid != NULL) {
			txnid->last_lsn = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		memcpy(logrec.data, &rectype, sizeof(rectype));

		ret = __log_put(dbenv,
		    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#else
		ret = 0;
#endif
		STAILQ_INSERT_HEAD(&txnid->logs, lr, links);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__fop_create_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, NULL, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(dbenv, logrec.data);
#else
	if (is_durable || txnid == NULL)
		__os_free(dbenv, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __fop_create_read __P((DB_ENV *, void *, __fop_create_args **));
 */
int
__fop_create_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__fop_create_args **argpp;
{
	__fop_create_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__fop_create_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memset(&argp->name, 0, sizeof(argp->name));
	memcpy(&argp->name.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->appname = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->mode = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __fop_remove_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, u_int32_t));
 */
int
__fop_remove_log(dbenv, txnid, ret_lsnp, flags,
    name, fid, appname)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	const DBT *fid;
	u_int32_t appname;
{
	DBT logrec;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rectype = DB___fop_remove;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnid == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnid == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnid->kids) != NULL &&
		    (ret = __txn_activekids(dbenv, rectype, txnid)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		*/
		DB_SET_BEGIN_LSNP(txnid, &rlsnp);
		txn_num = txnid->txnid;
		lsnp = &txnid->last_lsn;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t) + (fid == NULL ? 0 : fid->size)
	    + sizeof(u_int32_t);
	if (CRYPTO_ON(dbenv)) {
		npad =
		    ((DB_CIPHER *)dbenv->crypto_handle)->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnid == NULL) {
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(dbenv,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0) {
			__os_free(dbenv, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);

	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);

	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	if (name == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &name->size, sizeof(name->size));
		bp += sizeof(name->size);
		memcpy(bp, name->data, name->size);
		bp += name->size;
	}

	if (fid == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &fid->size, sizeof(fid->size));
		bp += sizeof(fid->size);
		memcpy(bp, fid->data, fid->size);
		bp += fid->size;
	}

	uinttmp = (u_int32_t)appname;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnid == NULL) {
		if ((ret = __log_put(dbenv, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnid != NULL) {
			txnid->last_lsn = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		memcpy(logrec.data, &rectype, sizeof(rectype));

		ret = __log_put(dbenv,
		    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#else
		ret = 0;
#endif
		STAILQ_INSERT_HEAD(&txnid->logs, lr, links);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__fop_remove_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, NULL, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(dbenv, logrec.data);
#else
	if (is_durable || txnid == NULL)
		__os_free(dbenv, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __fop_remove_read __P((DB_ENV *, void *, __fop_remove_args **));
 */
int
__fop_remove_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__fop_remove_args **argpp;
{
	__fop_remove_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__fop_remove_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memset(&argp->name, 0, sizeof(argp->name));
	memcpy(&argp->name.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memset(&argp->fid, 0, sizeof(argp->fid));
	memcpy(&argp->fid.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->fid.data = bp;
	bp += argp->fid.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->appname = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __fop_write_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, u_int32_t, u_int32_t, db_pgno_t,
 * PUBLIC:     u_int32_t, const DBT *, u_int32_t));
 */
int
__fop_write_log(dbenv, txnid, ret_lsnp, flags,
    name, appname, pgsize, pageno, offset, page,
    flag)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	u_int32_t appname;
	u_int32_t pgsize;
	db_pgno_t pageno;
	u_int32_t offset;
	const DBT *page;
	u_int32_t flag;
{
	DBT logrec;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rectype = DB___fop_write;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnid == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnid == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnid->kids) != NULL &&
		    (ret = __txn_activekids(dbenv, rectype, txnid)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		*/
		DB_SET_BEGIN_LSNP(txnid, &rlsnp);
		txn_num = txnid->txnid;
		lsnp = &txnid->last_lsn;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (page == NULL ? 0 : page->size)
	    + sizeof(u_int32_t);
	if (CRYPTO_ON(dbenv)) {
		npad =
		    ((DB_CIPHER *)dbenv->crypto_handle)->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnid == NULL) {
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(dbenv,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0) {
			__os_free(dbenv, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);

	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);

	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	if (name == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &name->size, sizeof(name->size));
		bp += sizeof(name->size);
		memcpy(bp, name->data, name->size);
		bp += name->size;
	}

	uinttmp = (u_int32_t)appname;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgsize;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pageno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)offset;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (page == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &page->size, sizeof(page->size));
		bp += sizeof(page->size);
		memcpy(bp, page->data, page->size);
		bp += page->size;
	}

	uinttmp = (u_int32_t)flag;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnid == NULL) {
		if ((ret = __log_put(dbenv, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnid != NULL) {
			txnid->last_lsn = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		memcpy(logrec.data, &rectype, sizeof(rectype));

		ret = __log_put(dbenv,
		    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#else
		ret = 0;
#endif
		STAILQ_INSERT_HEAD(&txnid->logs, lr, links);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__fop_write_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, NULL, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(dbenv, logrec.data);
#else
	if (is_durable || txnid == NULL)
		__os_free(dbenv, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __fop_write_read __P((DB_ENV *, void *, __fop_write_args **));
 */
int
__fop_write_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__fop_write_args **argpp;
{
	__fop_write_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__fop_write_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memset(&argp->name, 0, sizeof(argp->name));
	memcpy(&argp->name.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->appname = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgsize = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pageno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->offset = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->page, 0, sizeof(argp->page));
	memcpy(&argp->page.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->page.data = bp;
	bp += argp->page.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->flag = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __fop_rename_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, const DBT *, u_int32_t));
 */
int
__fop_rename_log(dbenv, txnid, ret_lsnp, flags,
    oldname, newname, fileid, appname)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *oldname;
	const DBT *newname;
	const DBT *fileid;
	u_int32_t appname;
{
	DBT logrec;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rectype = DB___fop_rename;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnid == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnid == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnid->kids) != NULL &&
		    (ret = __txn_activekids(dbenv, rectype, txnid)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		*/
		DB_SET_BEGIN_LSNP(txnid, &rlsnp);
		txn_num = txnid->txnid;
		lsnp = &txnid->last_lsn;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (oldname == NULL ? 0 : oldname->size)
	    + sizeof(u_int32_t) + (newname == NULL ? 0 : newname->size)
	    + sizeof(u_int32_t) + (fileid == NULL ? 0 : fileid->size)
	    + sizeof(u_int32_t);
	if (CRYPTO_ON(dbenv)) {
		npad =
		    ((DB_CIPHER *)dbenv->crypto_handle)->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnid == NULL) {
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(dbenv,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0) {
			__os_free(dbenv, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);

	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);

	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	if (oldname == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &oldname->size, sizeof(oldname->size));
		bp += sizeof(oldname->size);
		memcpy(bp, oldname->data, oldname->size);
		bp += oldname->size;
	}

	if (newname == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &newname->size, sizeof(newname->size));
		bp += sizeof(newname->size);
		memcpy(bp, newname->data, newname->size);
		bp += newname->size;
	}

	if (fileid == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &fileid->size, sizeof(fileid->size));
		bp += sizeof(fileid->size);
		memcpy(bp, fileid->data, fileid->size);
		bp += fileid->size;
	}

	uinttmp = (u_int32_t)appname;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnid == NULL) {
		if ((ret = __log_put(dbenv, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnid != NULL) {
			txnid->last_lsn = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		memcpy(logrec.data, &rectype, sizeof(rectype));

		ret = __log_put(dbenv,
		    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#else
		ret = 0;
#endif
		STAILQ_INSERT_HEAD(&txnid->logs, lr, links);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__fop_rename_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, NULL, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(dbenv, logrec.data);
#else
	if (is_durable || txnid == NULL)
		__os_free(dbenv, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __fop_rename_read __P((DB_ENV *, void *, __fop_rename_args **));
 */
int
__fop_rename_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__fop_rename_args **argpp;
{
	__fop_rename_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__fop_rename_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memset(&argp->oldname, 0, sizeof(argp->oldname));
	memcpy(&argp->oldname.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->oldname.data = bp;
	bp += argp->oldname.size;

	memset(&argp->newname, 0, sizeof(argp->newname));
	memcpy(&argp->newname.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->newname.data = bp;
	bp += argp->newname.size;

	memset(&argp->fileid, 0, sizeof(argp->fileid));
	memcpy(&argp->fileid.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->fileid.data = bp;
	bp += argp->fileid.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->appname = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __fop_file_remove_log __P((DB_ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, u_int32_t, const DBT *, const DBT *, const DBT *,
 * PUBLIC:     u_int32_t, u_int32_t));
 */
int
__fop_file_remove_log(dbenv, txnid, ret_lsnp, flags,
    real_fid, tmp_fid, name, appname, child)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *real_fid;
	const DBT *tmp_fid;
	const DBT *name;
	u_int32_t appname;
	u_int32_t child;
{
	DBT logrec;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rectype = DB___fop_file_remove;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnid == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnid == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnid->kids) != NULL &&
		    (ret = __txn_activekids(dbenv, rectype, txnid)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		*/
		DB_SET_BEGIN_LSNP(txnid, &rlsnp);
		txn_num = txnid->txnid;
		lsnp = &txnid->last_lsn;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (real_fid == NULL ? 0 : real_fid->size)
	    + sizeof(u_int32_t) + (tmp_fid == NULL ? 0 : tmp_fid->size)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t);
	if (CRYPTO_ON(dbenv)) {
		npad =
		    ((DB_CIPHER *)dbenv->crypto_handle)->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnid == NULL) {
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(dbenv,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(dbenv, logrec.size, &logrec.data)) != 0) {
			__os_free(dbenv, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);

	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);

	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	if (real_fid == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &real_fid->size, sizeof(real_fid->size));
		bp += sizeof(real_fid->size);
		memcpy(bp, real_fid->data, real_fid->size);
		bp += real_fid->size;
	}

	if (tmp_fid == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &tmp_fid->size, sizeof(tmp_fid->size));
		bp += sizeof(tmp_fid->size);
		memcpy(bp, tmp_fid->data, tmp_fid->size);
		bp += tmp_fid->size;
	}

	if (name == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &name->size, sizeof(name->size));
		bp += sizeof(name->size);
		memcpy(bp, name->data, name->size);
		bp += name->size;
	}

	uinttmp = (u_int32_t)appname;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)child;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnid == NULL) {
		if ((ret = __log_put(dbenv, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnid != NULL) {
			txnid->last_lsn = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		memcpy(logrec.data, &rectype, sizeof(rectype));

		ret = __log_put(dbenv,
		    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#else
		ret = 0;
#endif
		STAILQ_INSERT_HEAD(&txnid->logs, lr, links);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__fop_file_remove_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, NULL, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(dbenv, logrec.data);
#else
	if (is_durable || txnid == NULL)
		__os_free(dbenv, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __fop_file_remove_read __P((DB_ENV *, void *,
 * PUBLIC:     __fop_file_remove_args **));
 */
int
__fop_file_remove_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__fop_file_remove_args **argpp;
{
	__fop_file_remove_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__fop_file_remove_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memset(&argp->real_fid, 0, sizeof(argp->real_fid));
	memcpy(&argp->real_fid.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->real_fid.data = bp;
	bp += argp->real_fid.size;

	memset(&argp->tmp_fid, 0, sizeof(argp->tmp_fid));
	memcpy(&argp->tmp_fid.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->tmp_fid.data = bp;
	bp += argp->tmp_fid.size;

	memset(&argp->name, 0, sizeof(argp->name));
	memcpy(&argp->name.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->appname = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->child = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __fop_init_recover __P((DB_ENV *, int (***)(DB_ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *), size_t *));
 */
int
__fop_init_recover(dbenv, dtabp, dtabsizep)
	DB_ENV *dbenv;
	int (***dtabp)__P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t *dtabsizep;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __fop_create_recover, DB___fop_create)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __fop_remove_recover, DB___fop_remove)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __fop_write_recover, DB___fop_write)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __fop_rename_recover, DB___fop_rename)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __fop_file_remove_recover, DB___fop_file_remove)) != 0)
		return (ret);
	return (0);
}
