
NOTE: PostgreSQL support is currently more advanced than MySQL
support.  For new installations, we recommend you consider using
PostgreSQL and see README_PostgreSQL.txt for instructions.

In the examples below, the superuser (with permission to create
databases and users) is the username root and the database name
is repro.

1. create a database:

mysqladmin -u root -p create repro

2. create a user and flush privileges:

mysql -u root -p -e "GRANT ALL PRIVILEGES ON *.* TO root@localhost WITH GRANT OPTION"
mysql -u root -p -e "GRANT ALL PRIVILEGES ON repro.* TO repro@localhost"
mysql -u root -p -e "FLUSH PRIVILEGES"

3. load the schema:

mysql -u root -p repro < create_mysql_reprodb.sql

Other Potentially Useful Commands
=================================

Start DB
sudo /sw/bin/mysqld_safe &

Remove REPRO DB with
mysqladmin -u root -p DROP repro 

you can view permissions with 
mysqlaccess -U root -p \* \* repro --brief

Can make sure all is flushed with 
mysqladmin -u root -p refresh


passwordHashAlt
===============

The schema contains a passwordHashAlt column

This is much the same as the ha1b column used by Kamailio

  passwordHash =    h(A1) = md5(user:realm:password)

  passwordHashAlt = h(A1) = md5(user@domain:realm:password)

repro will update the value in this column each time a user is added
or their password is changed.  However, it is not currently consulted
as part of the authentication process.

To explicitly use this column instead of the regular passwordHash
column, use this custom auth query:

MySQLCustomUserAuthQuery = \
   SELECT passwordHashAlt
   FROM users
   WHERE user = '$user'
     AND concat(domain, '@', domain) = '$domain'

A future version of repro will be able to use both columns during
the authentication process, to concurrently support UAs using
either authentication style.

