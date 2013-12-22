
-- execute statements like those below to create the empty
-- database.  It is also necessary to create the repro
-- user and grant the necessary privileges
-- CREATE DATABASE repro;
-- USE repro;

-- Uncomment the following to have all tables re-created
-- DROP TABLE IF EXISTS `users`;
-- DROP TABLE IF EXISTS `routesavp`;
-- DROP TABLE IF EXISTS `aclsavp`;
-- DROP TABLE IF EXISTS `configsavp`;
-- DROP TABLE IF EXISTS `staticregsavp`;
-- DROP TABLE IF EXISTS `filtersavp`;
-- DROP TABLE IF EXISTS `siloavp`;


--
-- Table structure for table `users`
--
CREATE TABLE IF NOT EXISTS `users` (
  `id` INT PRIMARY KEY AUTO_INCREMENT,
  `user` VARCHAR(64) NOT NULL,
  `domain` VARCHAR(253),
  `realm` VARCHAR(253),
  `passwordHash` VARCHAR(32),
  `passwordHashAlt` VARCHAR(32),
  `name` VARCHAR(256),
  `email` VARCHAR(256),
  `forwardAddress` VARCHAR(256),
  CONSTRAINT c_user_domain UNIQUE INDEX idx_user_domain (`user`, `domain`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `routesavp`
--

CREATE TABLE IF NOT EXISTS `routesavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `aclsavp`
--

CREATE TABLE IF NOT EXISTS `aclsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `configsavp`
--

CREATE TABLE IF NOT EXISTS `configsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `staticregsavp`
--

CREATE TABLE IF NOT EXISTS `staticregsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `filtersavp`
--

CREATE TABLE IF NOT EXISTS `filtersavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `siloavp`
-- Note:  This table contains 2 indexes
--

CREATE TABLE IF NOT EXISTS `siloavp` (
  `attr` VARCHAR(255) NOT NULL,
  `attr2` VARCHAR(255) NOT NULL,
  `value` VARCHAR(20315),
  PRIMARY KEY (`attr`),
  KEY `SECONDARY` (`attr2`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
