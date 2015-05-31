To create the repro PostgreSQL database tables, run the
create_postgresql_reprodb.sql script against your PostgreSQL server
instance, by using one of the following two commands:

Option 1
========
psql -f create_postgresql_reprodb.sql repro

It will prompt for the admin (postgres) password, alternatively you can
use a different user having DDL privileges.


Option 2
========
Login to PostgreSQL using root or a user which has DDL privileges,
then at the psql prompt execute:

> \i create_postgresql_reprodb.sql



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

Database1CustomUserAuthQuery = \
   SELECT passwordHashAlt
   FROM users
   WHERE user = '$user'
     AND concat(domain, '@', domain) = '$domain'

A future version of repro will be able to use both columns during
the authentication process, to concurrently support UAs using
either authentication style.

