/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#ifndef NO_SYSTEM_INCLUDES
#include <stdlib.h>
#include <string.h>
#endif

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_shash.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __rep_update_buf __P((u_int8_t *, size_t, size_t *,
 * PUBLIC:     DB_LSN *, int));
 */
int
__rep_update_buf(buf, max, lenp,
    first_lsn, num_files)
	u_int8_t *buf;
	size_t max, *lenp;
	DB_LSN * first_lsn;
	int num_files;
{
	u_int32_t uinttmp;
	u_int8_t *endbuf;
	u_int8_t *bp;
	int ret;

	ret = 0;

	bp = buf;
	endbuf = bp + max;

	if (bp + sizeof(*first_lsn) > endbuf)
		return (ENOMEM);
	if (first_lsn != NULL)
		memcpy(bp, first_lsn, sizeof(*first_lsn));
	else
		memset(bp, 0, sizeof(*first_lsn));
	bp += sizeof(*first_lsn);

	uinttmp = (u_int32_t)num_files;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	*lenp = (u_int32_t)(bp - buf);

	return (ret);
}

/*
 * PUBLIC: int __rep_update_read __P((DB_ENV *, void *, void **,
 * PUBLIC:     __rep_update_args **));
 */
int
__rep_update_read(dbenv, recbuf, nextp, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	void **nextp;
	__rep_update_args **argpp;
{
	__rep_update_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__rep_update_args), &argp)) != 0)
		return (ret);
	bp = recbuf;
	memcpy(&argp->first_lsn, bp,  sizeof(argp->first_lsn));
	bp += sizeof(argp->first_lsn);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->num_files = (int)uinttmp;
	bp += sizeof(uinttmp);

	*nextp = bp;
	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int __rep_fileinfo_buf __P((u_int8_t *, size_t, size_t *,
 * PUBLIC:     size_t, db_pgno_t, db_pgno_t, int, int32_t, u_int32_t,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *));
 */
int
__rep_fileinfo_buf(buf, max, lenp,
    pgsize, pgno, max_pgno, filenum, id, type,
    flags, uid, info)
	u_int8_t *buf;
	size_t max, *lenp;
	size_t pgsize;
	db_pgno_t pgno;
	db_pgno_t max_pgno;
	int filenum;
	int32_t id;
	u_int32_t type;
	u_int32_t flags;
	const DBT *uid;
	const DBT *info;
{
	u_int32_t zero;
	u_int32_t uinttmp;
	u_int8_t *endbuf;
	u_int8_t *bp;
	int ret;

	ret = 0;

	bp = buf;
	endbuf = bp + max;

	uinttmp = (u_int32_t)pgsize;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)max_pgno;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)filenum;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)id;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)type;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)flags;
	if (bp + sizeof(uinttmp) > endbuf)
		return (ENOMEM);
	memcpy(bp, &uinttmp, sizeof(uinttmp));
	bp += sizeof(uinttmp);

	if (uid == NULL) {
		zero = 0;
		if (bp + sizeof(u_int32_t) > endbuf)
			return (ENOMEM);
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		if (bp + sizeof(uid->size) > endbuf)
			return (ENOMEM);
		memcpy(bp, &uid->size, sizeof(uid->size));
		bp += sizeof(uid->size);
		if (bp + uid->size > endbuf)
			return (ENOMEM);
		memcpy(bp, uid->data, uid->size);
		bp += uid->size;
	}

	if (info == NULL) {
		zero = 0;
		if (bp + sizeof(u_int32_t) > endbuf)
			return (ENOMEM);
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		if (bp + sizeof(info->size) > endbuf)
			return (ENOMEM);
		memcpy(bp, &info->size, sizeof(info->size));
		bp += sizeof(info->size);
		if (bp + info->size > endbuf)
			return (ENOMEM);
		memcpy(bp, info->data, info->size);
		bp += info->size;
	}

	*lenp = (u_int32_t)(bp - buf);

	return (ret);
}

/*
 * PUBLIC: int __rep_fileinfo_read __P((DB_ENV *, void *, void **,
 * PUBLIC:     __rep_fileinfo_args **));
 */
int
__rep_fileinfo_read(dbenv, recbuf, nextp, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	void **nextp;
	__rep_fileinfo_args **argpp;
{
	__rep_fileinfo_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(dbenv,
	    sizeof(__rep_fileinfo_args), &argp)) != 0)
		return (ret);
	bp = recbuf;
	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgsize = (size_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->max_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->filenum = (int)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->id = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->type = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memcpy(&uinttmp, bp, sizeof(uinttmp));
	argp->flags = (u_int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->uid, 0, sizeof(argp->uid));
	memcpy(&argp->uid.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->uid.data = bp;
	bp += argp->uid.size;

	memset(&argp->info, 0, sizeof(argp->info));
	memcpy(&argp->info.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->info.data = bp;
	bp += argp->info.size;

	*nextp = bp;
	*argpp = argp;
	return (0);
}

