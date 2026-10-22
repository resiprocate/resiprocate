#!/usr/bin/python3
# -*- coding: latin-1 -*-
#
# migrates data from legacy database to new database
#
# To use the script:
#
# 1. enable plpgsql in your existing legacy database
#
#       CREATE OR REPLACE LANGUAGE plpgsql;
#
# 2. add the plpgsql functions and VIEWs to your existing legacy database
#    (copy them from create_postgresql_reprodb.sql)
#
# 3. create the new database
#
#       createdb repro_v2
#
# 4. if the new database is on a different PostgreSQL server,
#    create the user.  Use the same password as the existing user.
#
#       createuser -D -R -S -P repro
#
# 5. enable plpgsql in your new database
#
#       CREATE OR REPLACE LANGUAGE plpgsql;
#
# 6. load the full schema in your new database
#    (use create_postgresql_reprodb_v2.sql)
#
# 7. update the database parameters in this script
#    (see sql_legacy and sql_v2)
#
# 8. run this script
#
# 9. manually check the content of tables in the new database
#
# 10. modify repro.config to use the new database
#     (update DatabaseName)
#
# 11. restart repro
#
# 12. look in the repro web interface to verify that the data looks good
#

import getpass
import psycopg2
import sys

sql_legacy = "dbname=repro user=repro host=localhost password="
sql_v2 = "dbname=repro_v2 user=repro host=localhost password="

# tables to replicate
tables = [ "users", "route", "acl", "domain", "staticreg", "filter", "siloavp" ];

def migrate_table(cur_legacy, cur_v2, table_name):
    cur_legacy.execute("SELECT * FROM %s" % (table_name,))
    for row in cur_legacy:
        _row = list(row)
        if _row[0] is None:
            placeholders = ",".join(["%s"]*(len(row)-1))
            insert = "INSERT INTO %s VALUES (nextval(\'%s_id_seq\'), %s)" % (table_name, table_name, placeholders)
            del _row[0];
        else:
            placeholders = ",".join(["%s"]*len(row))
            insert = "INSERT INTO %s VALUES (%s)" % (table_name, placeholders)
        print(insert)
        cur_v2.execute(insert, tuple(_row))

# Program entry point / start here
if __name__ == '__main__':

    ###### read command line, password
    if len(sys.argv) > 1:
        password = sys.argv[1]
    else:
        password = getpass.getpass()

    conn_legacy = psycopg2.connect(sql_legacy + password)
    conn_v2 = psycopg2.connect(sql_v2 + password)

    cur_legacy = conn_legacy.cursor()
    cur_v2 = conn_v2.cursor()

    for table_name in tables:
        migrate_table(cur_legacy, cur_v2, table_name)

    conn_v2.commit()
    cur_v2.close()
    conn_v2.close()

    conn_legacy.close()

