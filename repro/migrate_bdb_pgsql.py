#!/usr/bin/python3
# -*- coding: latin-1 -*-
#
# migrates data from BerkeleyDB (legacy *.db files) to
# PostgreSQL (legacy or v2 schema)
#
# FIXME: doesn't yet support the users table or silo table
#
# Dependencies:
#
#    apt install python3-psycopg2 python3-bsddb3
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
# 4. load the database schema into PostgreSQL, using either:
#    the legacy schema from:  create_postgresql_reprodb.sql     or
#    the new schema from:     create_postgresql_reprodb_v2.sql
#
#    e.g. for new (v2) schema:
#
#       psql -U repro -W repro < create_postgresql_reprodb_v2.sql
#
# 5. set the mode parameter below to match the schema you chose
#
# 6. update the database connection parameters in this script
#    (search for dbname below) for connecting to your database
#
# 7. run this script
#
# 8. manually check the content of tables in the new database
#
# 9. modify repro.config to use the new database
#     (update Database* parameters)
#
# 10. restart repro
#
# 11. look in the repro web interface to verify that the data looks correct
#


import array
import base64
import bsddb3
import getpass
import psycopg2
import sys

# where the *.db files are located
default_path = "/var/lib/repro"

# schema of the destination database
#mode = "legacy"
mode = "v2"

# PostgreSQL connection string for destination database
sql = "dbname=repro user=repro host=localhost password="

# describes the data in the source tables that will be migrated
# FIXME: doesn't migrate the users table
table_names = {
    "config" : "2s2",
    "route" : "2ssss2",
    "acl" : "2ss2222",
    "staticreg" : "2sss",
    "filter" : "2ssssss2s2"
    # "silo" : "2sstsss"    # FIXME: need to support timestamps (t)
}

def legacy_name(table_name):
    return "%ssavp" % (table_name, )

def get_short(pos, value):
    return value[pos] + (value[pos+1] << 8)

def get_values(fmt, value):
    _value = array.array('b', value)
    values = list()
    p = 0
    for i in fmt:
        if i == "2":
            v = get_short(p, _value)
            p = p + 2
            values.append(v)
        if i == "s":
            slen = get_short(p, _value)
            p = p + 2
            v = value[p:p+slen]
            p = p + slen
            values.append(v) 
    return values

def load_table(path, table_name, fmt, cur):
    db_name = "%s/repro_%s.db" % (path, table_name,)
    db = bsddb.btopen(db_name, "r")

    for attr in db.keys():
        row = list()
        value = db[attr]
        if mode == "legacy":
            row.append(attr)
            row.append(base64.b64encode(value))
            placeholders = ",".join(["%s"]*2)
            insert = "INSERT INTO %s VALUES (%s)" % (legacy_name(table_name), placeholders)
        else:
            row = get_values(fmt, value)
            del row[0]
            placeholders = ",".join(["%s"]*len(row))
            dest_table_name = table_name
            if dest_table_name == "config":
                dest_table_name = "domain"
            insert = "INSERT INTO %s VALUES (nextval(\'%s_id_seq\'), %s)" % (dest_table_name, dest_table_name, placeholders)
        cur.execute(insert, tuple(row))

    db.close()


# Program entry point / start here
if __name__ == '__main__':

    ###### read command line, password
    if len(sys.argv) > 1:
        password = sys.argv[1]
    else:
        password = getpass.getpass("Destination database password:")

    conn = psycopg2.connect(sql + password)
    cur = conn.cursor()

    for table_name in table_names.keys():
        fmt = table_names[table_name]
        load_table(default_path, table_name, fmt, cur)

    conn.commit()
    cur.close()
    conn.close()

