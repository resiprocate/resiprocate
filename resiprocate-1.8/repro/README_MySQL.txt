To create the repro MySQL database tables, run the create_mysql_reprodb.sql script 
against your MySQL server instance, by using one of the following two commands:

Option 1
========
mysql -u root -p < create_mysql_reprodb.sql

It will prompt for root password, alternatively you can use a different user if has "create" privileges.


Option 2
========
Login to mysql using root or a user which has 'create' privileges, then at mysql prompt run:

mysql>source create_mysql_reprodb.sql



Other Potentially Useful Commands
=================================

Start DB
sudo /sw/bin/mysqld_safe &

Remove REPRO DB with
mysqladmin -u root -p DROP repro 

Create a user called repro that can use the DB
mysql -u root -p -e "GRANT ALL PRIVILEGES ON *.* TO root@localhost WITH GRANT OPTION"
mysql -u root -p -e "GRANT ALL PRIVILEGES ON repro.* TO repro@localhost"
mysql -u root -p -e "FLUSH PRIVILEGES"

you can view permissions with 
mysqlaccess -U root -P <pwd> \* \* repro --brief

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

