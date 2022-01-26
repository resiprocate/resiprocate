#!/usr/bin/python3
# -*- coding: latin-1 -*-
#
# migrates data from MySQL (legacy schema) to PostgreSQL (legacy schema)
#
# Dependencies:
#
#    apt install python3-mysqldb python3-psycopg2
#
# To use the script:
#
# 1. create the new PostgreSQL database
#
#       createdb repro
#
# 2. enable plpgsql in the destination database
#
#       CREATE OR REPLACE LANGUAGE plpgsql;
#
# 3. create the user if necessary
#
#       createuser -D -R -S -P repro
#
# 4. load the database schema into PostgreSQL, using the legacy schema
#    from create_postgresql_reprodb.sql
#
#       psql -U repro -W repro < create_postgresql_reprodb.sql
#
# 5. update the database parameters in this script
#    (see sql_mysql_legacy and sql_pgsql_legacy below)
#
# 6. run this script
#
# 7. manually check the content of tables in the new database
#
# 8. modify repro.config to use the new database
#     (update Database* parameters)
#
# 9. restart repro
#
# 10. look in the repro web interface to verify that the data looks correct
#
# 11. at this point, you may also want to migrate from the legacy
#     format to the v2 format using the script migrate_sql_v2.py
#

import getpass
import MySQLdb
import psycopg2
import sys

sql_mysql_legacy_host = "localhost"
sql_mysql_legacy_user = "repro"
sql_mysql_legacy_db = "repro"

sql_pgsql_legacy = "dbname=repro user=repro host=localhost password="

# tables to replicate
tables = [
    "users",
    "routesavp",
    "aclsavp",
    "configsavp",
    "staticregsavp",
    "filtersavp",
    "siloavp"
];

def migrate_table(cur_src, cur_dest, table_name):
    cur_src.execute("SELECT * FROM %s" % (table_name,))
    for row in cur_src.fetchall():
        _row = list(row)
        placeholders = ",".join(["%s"]*len(row))
        insert = "INSERT INTO %s VALUES (%s)" % (table_name, placeholders)
        cur_dest.execute(insert, tuple(_row))

# Program entry point / start here
if __name__ == '__main__':

    ###### read command line, password
    if len(sys.argv) > 1:
        mysql_password = sys.argv[1]
    else:
        mysql_password = getpass.getpass("Source database (MySQL) password:")

    if len(sys.argv) > 2:
        pgsql_password = sys.argv[2]
    else:
        pgsql_password = getpass.getpass("Destination database (PostgreSQL) password:")

    conn_src = MySQLdb.connect(host=sql_mysql_legacy_host, user=sql_mysql_legacy_user, passwd=mysql_password, db=sql_mysql_legacy_db)
    conn_dest = psycopg2.connect(sql_pgsql_legacy + pgsql_password)

    cur_src = conn_src.cursor()
    cur_dest = conn_dest.cursor()

    for table_name in tables:
        migrate_table(cur_src, cur_dest, table_name)

    conn_dest.commit()
    cur_dest.close()
    conn_dest.close()

    conn_src.close()

