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

/*
 * PUBLIC: int __dbreg_register_log __P((DB_ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, u_int32_t, u_int32_t, const DBT *, const DBT *,
 * PUBLIC:     int32_t, DBTYPE, db_pgno_t, u_int32_t));
 */
int
__dbreg_register_log(dbenv, txnid, ret_lsnp, flags,
    opcode, name, uid, fileid, ftype, meta_pgno,
    id)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	const DBT *name;
	const DBT *uid;
	int32_t fileid;
	DBTYPE ftype;
	db_pgno_t meta_pgno;
	u_int32_t id;
{
	DBT logrec;
	DB_TXNLOGREC *lr;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rectype = DB___dbreg_register;
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
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t) + (uid == NULL ? 0 : uid->size)
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

	uinttmp = (u_int32_t)opcode;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

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

	if (uid == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &uid->size, sizeof(uid->size));
		bp += sizeof(uid->size);
		memcpy(bp, uid->data, uid->size);
		bp += uid->size;
	}

	uinttmp = (u_int32_t)fileid;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)ftype;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)meta_pgno;
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)id;
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
		(void)__dbreg_register_print(dbenv,
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
 * PUBLIC: int __dbreg_register_read __P((DB_ENV *, void *,
 * PUBLIC:     __dbreg_register_args **));
 */
int
__dbreg_register_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	__dbreg_register_args **argpp;
{
	__dbreg_register_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__dbreg_register_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->opcode = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->name, 0, sizeof(argp->name));
	memcpy(&argp->name.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memset(&argp->uid, 0, sizeof(argp->uid));
	memcpy(&argp->uid.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->uid.data = bp;
	bp += argp->uid.size;

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->ftype = (DBTYPE)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->id = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __dbreg_init_recover __P((DB_ENV *, int (***)(DB_ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *), size_t *));
 */
int
__dbreg_init_recover(dbenv, dtabp, dtabsizep)
	DB_ENV *dbenv;
	int (***dtabp)__P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
	size_t *dtabsizep;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp, dtabsizep,
	    __dbreg_register_recover, DB___dbreg_register)) != 0)
		return (ret);
	return (0);
}
