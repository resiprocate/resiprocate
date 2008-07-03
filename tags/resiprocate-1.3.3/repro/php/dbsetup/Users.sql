drop table if exists Users;

create table Users (
       id int unsigned not null auto_increment, 
       username varchar(255) not null, 
       password varchar(32) not null,
       fullname varchar(255) not null,
       domain long not null,
       email varchar(255) not null,
       state char(1) not null,
       activationDate date,
       activationCode varchar(32),
       primary key(id));

