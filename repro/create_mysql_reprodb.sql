
CREATE DATABASE repro;

USE repro;

--
-- Table structure for table `reprousers`
--

DROP TABLE IF EXISTS `reprousers`;
CREATE TABLE `reprousers` (
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
-- Table structure for table `reprousersavp`
--

DROP TABLE IF EXISTS `reprousersavp`;
CREATE TABLE `reprousersavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `reproroutesavp`
--

DROP TABLE IF EXISTS `reproroutesavp`;
CREATE TABLE `reproroutesavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `reproaclsavp`
--

DROP TABLE IF EXISTS `reproaclsavp`;
CREATE TABLE `reproaclsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Table structure for table `reproconfigsavp`
--

DROP TABLE IF EXISTS `reproconfigsavp`;
CREATE TABLE `reproconfigsavp` (
  `attr` VARCHAR(255) NOT NULL,
  `value` VARCHAR(1024),
  PRIMARY KEY (attr) 
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


