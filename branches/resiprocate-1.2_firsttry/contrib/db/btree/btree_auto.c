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
#include "dbinc/btree.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __bam_split_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, u_int32_t,
 * PUBLIC:     db_pgno_t, DB_LSN *, db_pgno_t, const DBT *, u_int32_t));
 */
int
__bam_split_log(dbp, txnid, ret_lsnp, flags, left, llsn, right, rlsn, indx,
    npgno, nlsn, root_pgno, pg, opflags)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t left;
	DB_LSN * llsn;
	db_pgno_t right;
	DB_LSN * rlsn;
	u_int32_t indx;
	db_pgno_t npgno;
	DB_LSN * nlsn;
	db_pgno_t root_pgno;
	const DBT *pg;
	u_int32_t opflags;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_split;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*llsn)
	    + sizeof(u_int32_t)
	    + sizeof(*rlsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*nlsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (pg == NULL ? 0 : pg->size)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)left;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (llsn != NULL)
		memcpy(bp, llsn, sizeof(*llsn));
	else
		memset(bp, 0, sizeof(*llsn));
	bp += sizeof(*llsn);

	uinttmp = (u_int32_t)right;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (rlsn != NULL)
		memcpy(bp, rlsn, sizeof(*rlsn));
	else
		memset(bp, 0, sizeof(*rlsn));
	bp += sizeof(*rlsn);

	uinttmp = (u_int32_t)indx;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)npgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (nlsn != NULL)
		memcpy(bp, nlsn, sizeof(*nlsn));
	else
		memset(bp, 0, sizeof(*nlsn));
	bp += sizeof(*nlsn);

	uinttmp = (u_int32_t)root_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (pg == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &pg->size, sizeof(pg->size));
		bp += sizeof(pg->size);
		memcpy(bp, pg->data, pg->size);
		bp += pg->size;
	}

	uinttmp = (u_int32_t)opflags;
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
		(void)__bam_split_print(dbenv,
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
 * PUBLIC: int __bam_split_read __P((DB_ENV *, void *, __bam_split_args **));
 */
int
__bam_split_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_split_args **argpp;
{
	__bam_split_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_split_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->left = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->llsn, bp,  sizeof(argp->llsn));
	bp += sizeof(argp->llsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->right = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->rlsn, bp,  sizeof(argp->rlsn));
	bp += sizeof(argp->rlsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->npgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->nlsn, bp,  sizeof(argp->nlsn));
	bp += sizeof(argp->nlsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->root_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->pg, 0, sizeof(argp->pg));
	memcpy(&argp->pg.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->pg.data = bp;
	bp += argp->pg.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->opflags = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_rsplit_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, const DBT *, db_pgno_t, db_pgno_t,
 * PUBLIC:     const DBT *, DB_LSN *));
 */
int
__bam_rsplit_log(dbp, txnid, ret_lsnp, flags, pgno, pgdbt, root_pgno, nrec, rootent,
    rootlsn)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	const DBT *pgdbt;
	db_pgno_t root_pgno;
	db_pgno_t nrec;
	const DBT *rootent;
	DB_LSN * rootlsn;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_rsplit;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (pgdbt == NULL ? 0 : pgdbt->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (rootent == NULL ? 0 : rootent->size)
	    + sizeof(*rootlsn);
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (pgdbt == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &pgdbt->size, sizeof(pgdbt->size));
		bp += sizeof(pgdbt->size);
		memcpy(bp, pgdbt->data, pgdbt->size);
		bp += pgdbt->size;
	}

	uinttmp = (u_int32_t)root_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)nrec;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (rootent == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &rootent->size, sizeof(rootent->size));
		bp += sizeof(rootent->size);
		memcpy(bp, rootent->data, rootent->size);
		bp += rootent->size;
	}

	if (rootlsn != NULL)
		memcpy(bp, rootlsn, sizeof(*rootlsn));
	else
		memset(bp, 0, sizeof(*rootlsn));
	bp += sizeof(*rootlsn);

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
		(void)__bam_rsplit_print(dbenv,
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
 * PUBLIC: int __bam_rsplit_read __P((DB_ENV *, void *, __bam_rsplit_args **));
 */
int
__bam_rsplit_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_rsplit_args **argpp;
{
	__bam_rsplit_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_rsplit_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->pgdbt, 0, sizeof(argp->pgdbt));
	memcpy(&argp->pgdbt.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->pgdbt.data = bp;
	bp += argp->pgdbt.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->root_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->nrec = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->rootent, 0, sizeof(argp->rootent));
	memcpy(&argp->rootent.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->rootent.data = bp;
	bp += argp->rootent.size;

	memcpy(&argp->rootlsn, bp,  sizeof(argp->rootlsn));
	bp += sizeof(argp->rootlsn);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_adj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, u_int32_t,
 * PUBLIC:     u_int32_t));
 */
int
__bam_adj_log(dbp, txnid, ret_lsnp, flags, pgno, lsn, indx, indx_copy, is_insert)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	u_int32_t indx_copy;
	u_int32_t is_insert;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_adj;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);

	uinttmp = (u_int32_t)indx;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)indx_copy;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)is_insert;
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
		(void)__bam_adj_print(dbenv,
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
 * PUBLIC: int __bam_adj_read __P((DB_ENV *, void *, __bam_adj_args **));
 */
int
__bam_adj_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_adj_args **argpp;
{
	__bam_adj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_adj_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->indx_copy = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->is_insert = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_cadjust_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, int32_t, u_int32_t));
 */
int
__bam_cadjust_log(dbp, txnid, ret_lsnp, flags, pgno, lsn, indx, adjust, opflags)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	int32_t adjust;
	u_int32_t opflags;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_cadjust;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);

	uinttmp = (u_int32_t)indx;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)adjust;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)opflags;
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
		(void)__bam_cadjust_print(dbenv,
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
 * PUBLIC: int __bam_cadjust_read __P((DB_ENV *, void *,
 * PUBLIC:     __bam_cadjust_args **));
 */
int
__bam_cadjust_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_cadjust_args **argpp;
{
	__bam_cadjust_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_cadjust_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->adjust = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->opflags = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_cdel_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t));
 */
int
__bam_cdel_log(dbp, txnid, ret_lsnp, flags, pgno, lsn, indx)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_cdel;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);

	uinttmp = (u_int32_t)indx;
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
		(void)__bam_cdel_print(dbenv,
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
 * PUBLIC: int __bam_cdel_read __P((DB_ENV *, void *, __bam_cdel_args **));
 */
int
__bam_cdel_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_cdel_args **argpp;
{
	__bam_cdel_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_cdel_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_repl_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, u_int32_t,
 * PUBLIC:     const DBT *, const DBT *, u_int32_t, u_int32_t));
 */
int
__bam_repl_log(dbp, txnid, ret_lsnp, flags, pgno, lsn, indx, isdeleted, orig,
    repl, prefix, suffix)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	u_int32_t isdeleted;
	const DBT *orig;
	const DBT *repl;
	u_int32_t prefix;
	u_int32_t suffix;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_repl;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (orig == NULL ? 0 : orig->size)
	    + sizeof(u_int32_t) + (repl == NULL ? 0 : repl->size)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);

	uinttmp = (u_int32_t)indx;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)isdeleted;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (orig == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &orig->size, sizeof(orig->size));
		bp += sizeof(orig->size);
		memcpy(bp, orig->data, orig->size);
		bp += orig->size;
	}

	if (repl == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &repl->size, sizeof(repl->size));
		bp += sizeof(repl->size);
		memcpy(bp, repl->data, repl->size);
		bp += repl->size;
	}

	uinttmp = (u_int32_t)prefix;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)suffix;
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
		(void)__bam_repl_print(dbenv,
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
 * PUBLIC: int __bam_repl_read __P((DB_ENV *, void *, __bam_repl_args **));
 */
int
__bam_repl_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_repl_args **argpp;
{
	__bam_repl_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_repl_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->isdeleted = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->orig, 0, sizeof(argp->orig));
	memcpy(&argp->orig.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->orig.data = bp;
	bp += argp->orig.size;

	memset(&argp->repl, 0, sizeof(argp->repl));
	memcpy(&argp->repl.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->repl.data = bp;
	bp += argp->repl.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->prefix = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->suffix = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_root_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, db_pgno_t, DB_LSN *));
 */
int
__bam_root_log(dbp, txnid, ret_lsnp, flags, meta_pgno, root_pgno, meta_lsn)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t meta_pgno;
	db_pgno_t root_pgno;
	DB_LSN * meta_lsn;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_root;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*meta_lsn);
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)meta_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)root_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (meta_lsn != NULL)
		memcpy(bp, meta_lsn, sizeof(*meta_lsn));
	else
		memset(bp, 0, sizeof(*meta_lsn));
	bp += sizeof(*meta_lsn);

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
		(void)__bam_root_print(dbenv,
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
 * PUBLIC: int __bam_root_read __P((DB_ENV *, void *, __bam_root_args **));
 */
int
__bam_root_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_root_args **argpp;
{
	__bam_root_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_root_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->root_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->meta_lsn, bp,  sizeof(argp->meta_lsn));
	bp += sizeof(argp->meta_lsn);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_curadj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_ca_mode, db_pgno_t, db_pgno_t, db_pgno_t,
 * PUBLIC:     u_int32_t, u_int32_t, u_int32_t));
 */
int
__bam_curadj_log(dbp, txnid, ret_lsnp, flags, mode, from_pgno, to_pgno, left_pgno, first_indx,
    from_indx, to_indx)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_ca_mode mode;
	db_pgno_t from_pgno;
	db_pgno_t to_pgno;
	db_pgno_t left_pgno;
	u_int32_t first_indx;
	u_int32_t from_indx;
	u_int32_t to_indx;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_curadj;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)mode;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)from_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)to_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)left_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)first_indx;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)from_indx;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)to_indx;
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
		(void)__bam_curadj_print(dbenv,
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
 * PUBLIC: int __bam_curadj_read __P((DB_ENV *, void *, __bam_curadj_args **));
 */
int
__bam_curadj_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_curadj_args **argpp;
{
	__bam_curadj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_curadj_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->mode = (db_ca_mode)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->from_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->to_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->left_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->first_indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->from_indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->to_indx = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_rcuradj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, ca_recno_arg, db_pgno_t, db_recno_t, u_int32_t));
 */
int
__bam_rcuradj_log(dbp, txnid, ret_lsnp, flags, mode, root, recno, order)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	ca_recno_arg mode;
	db_pgno_t root;
	db_recno_t recno;
	u_int32_t order;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_rcuradj;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)mode;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)root;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)recno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)order;
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
		(void)__bam_rcuradj_print(dbenv,
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
 * PUBLIC: int __bam_rcuradj_read __P((DB_ENV *, void *,
 * PUBLIC:     __bam_rcuradj_args **));
 */
int
__bam_rcuradj_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_rcuradj_args **argpp;
{
	__bam_rcuradj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_rcuradj_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->mode = (ca_recno_arg)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->root = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->recno = (db_recno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->order = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_relink_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, db_pgno_t,
 * PUBLIC:     DB_LSN *));
 */
int
__bam_relink_log(dbp, txnid, ret_lsnp, flags, pgno, lsn, prev, lsn_prev, next,
    lsn_next)
	DB *dbp;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	db_pgno_t prev;
	DB_LSN * lsn_prev;
	db_pgno_t next;
	DB_LSN * lsn_next;
{
	DBT logrec;
	DB_ENV *dbenv;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	dbenv = dbp->dbenv;
	COMPQUIET(lr, NULL);

	rectype = DB___bam_relink;
	npad = 0;
	rlsnp = ret_lsnp;

	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
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

	DB_ASSERT(dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn_prev)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn_next);
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);

	uinttmp = (u_int32_t)prev;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn_prev != NULL)
		memcpy(bp, lsn_prev, sizeof(*lsn_prev));
	else
		memset(bp, 0, sizeof(*lsn_prev));
	bp += sizeof(*lsn_prev);

	uinttmp = (u_int32_t)next;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (lsn_next != NULL)
		memcpy(bp, lsn_next, sizeof(*lsn_next));
	else
		memset(bp, 0, sizeof(*lsn_next));
	bp += sizeof(*lsn_next);

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
		(void)__bam_relink_print(dbenv,
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
 * PUBLIC: int __bam_relink_read __P((DB_ENV *, void *, __bam_relink_args **));
 */
int
__bam_relink_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__bam_relink_args **argpp;
{
	__bam_relink_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__bam_relink_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnid = (DB_TXN *)&argp[1];

	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);

	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);

	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->prev = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn_prev, bp,  sizeof(argp->lsn_prev));
	bp += sizeof(argp->lsn_prev);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&argp->lsn_next, bp,  sizeof(argp->lsn_next));
	bp += sizeof(argp->lsn_next);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __bam_init_recover __P((DB_ENV *, int (***)(DB_ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *), size_t *));
 */
int
__bam_init_recover(dbenv, dtabp, dtabsizep)
	DB_ENV *dbenv;
	int (***dtabp)__P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t *dtabsizep;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_split_recover, DB___bam_split)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_rsplit_recover, DB___bam_rsplit)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_adj_recover, DB___bam_adj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_cadjust_recover, DB___bam_cadjust)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_cdel_recover, DB___bam_cdel)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_repl_recover, DB___bam_repl)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_root_recover, DB___bam_root)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_curadj_recover, DB___bam_curadj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_rcuradj_recover, DB___bam_rcuradj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __bam_relink_recover, DB___bam_relink)) != 0)
		return (ret);
	return (0);
}
