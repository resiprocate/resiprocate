
Start DB
sudo /sw/bin/mysqld_safe &


Create the REPO DB
mysqladmin -u root CREATE repro 

(note can remove with)
mysqladmin -u root DROP repro 

Create a user called repro that can use the DB
mysql -u root -e "GRANT ALL PRIVILEGES ON *.* TO root@localhost WITH GRANT OPTION"
mysql -u root -e "GRANT ALL PRIVILEGES ON repro.* TO repro@localhost"
mysql -u root -e "FLUSH PRIVILEGES"

you can view permissions with 
mysqlaccess -U root \* \* repro --brief

Create the tables 
mysql -u repro REPRO -e "CREATE TABLE users (user VARCHAR(30) NOT NULL, domain VARCHAR(30), realm VARCHAR(30), passwordHash VARCHAR(40), name VARCHAR(40), email VARCHAR(50), forwardAddress VARCHAR(80), UNIQUE INDEX index1 (user, domain) )"

mysql -u repro REPRO -e "CREATE TABLE usersavp (attr VARCHAR(80) NOT NULL, value VARCHAR(255), UNIQUE INDEX index1 (attr) )"

mysql -u repro REPRO -e "CREATE TABLE routesavp (attr VARCHAR(80) NOT NULL, value VARCHAR(255), UNIQUE INDEX index1 (attr) )"

mysql -u repro REPRO -e "CREATE TABLE aclsavp (attr VARCHAR(80) NOT NULL, value VARCHAR(255), UNIQUE INDEX index1 (attr) )"

mysql -u repro REPRO -e "CREATE TABLE configsavp (attr VARCHAR(80) NOT NULL, value VARCHAR(255), UNIQUE INDEX index1 (attr) )"


(also useful side note can delete with)
mysql -u repro REPRO -e "DROP TABLE users"

Can make sure all is flushed with 
mysqladmin -u root refresh

Can add a user with something like 
mysql -u repro REPRO -e "INSERT INTO users ( user, domain ) VALUES ( 'alice', 'example.com' )"

Can view users with 
mysql -u repro REPRO -e "SELECT * FROM users" 
