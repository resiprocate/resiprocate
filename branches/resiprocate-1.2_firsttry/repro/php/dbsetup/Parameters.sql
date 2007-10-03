drop table if exists Parameters;

create table Parameters (
       parameter varchar(20) not null, 
       value varchar(255) not null);

insert into Parameters(parameter,value) values('salt','Pilot#$Floss');
