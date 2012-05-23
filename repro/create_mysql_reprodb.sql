
CREATE DATABASE repro;

USE repro;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
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

DROP TABLE IF EXISTS `routesavp`;
CREATE TABLE `routesavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `aclsavp`
--

DROP TABLE IF EXISTS `aclsavp`;
CREATE TABLE `aclsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `configsavp`
--

DROP TABLE IF EXISTS `configsavp`;
CREATE TABLE `configsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `staticregsavp`
--

DROP TABLE IF EXISTS `staticregsavp`;
CREATE TABLE `staticregsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `filtersavp`
--

DROP TABLE IF EXISTS `filtersavp`;
CREATE TABLE `filtersavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(4096),
  PRIMARY KEY (`attr`) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `siloavp`
-- Note:  This table contains 2 indexes
--

DROP TABLE IF EXISTS `siloavp`;
CREATE TABLE `siloavp` (
  `attr` VARCHAR(255) NOT NULL,
  `attr2` VARCHAR(255) NOT NULL,
  `value` VARCHAR(20315),
  PRIMARY KEY (`attr`),
  KEY `SECONDARY` (`attr2`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
