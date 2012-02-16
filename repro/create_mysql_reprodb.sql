
CREATE DATABASE repro;

USE repro;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `user` VARCHAR(64) NOT NULL,
  `domain` VARCHAR(253),
  `realm` VARCHAR(253),
  `passwordHash` VARCHAR(32),
  `name` VARCHAR(256),
  `email` VARCHAR(256),
  `forwardAddress` VARCHAR(256),
  PRIMARY KEY (`user`, `domain`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


--
-- Table structure for table `routesavp`
--

DROP TABLE IF EXISTS `routesavp`;
CREATE TABLE `routesavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `aclsavp`
--

DROP TABLE IF EXISTS `aclsavp`;
CREATE TABLE `aclsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `configsavp`
--

DROP TABLE IF EXISTS `configsavp`;
CREATE TABLE `configsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


