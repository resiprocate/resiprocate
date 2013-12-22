
-- As well as doing this, please check that you have all the tables
-- listed in the file create_mysql_reprodb.sql

-- Upgrade from version 2 schema to version 3
ALTER TABLE `users` 
   ADD COLUMN `id` INT NULL AUTO_INCREMENT  FIRST , 
   ADD COLUMN `passwordHashAlt` VARCHAR(32) NULL DEFAULT NULL  AFTER `passwordHash` , 
   CHANGE COLUMN `domain` `domain` VARCHAR(253) NULL DEFAULT NULL  
, DROP PRIMARY KEY 
, ADD PRIMARY KEY (`id`) 
, ADD UNIQUE INDEX `idx_user_domain` (`user` ASC, `domain` ASC) ;

