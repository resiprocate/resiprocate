drop table if exists Resources;

create table Resources (
       id int unsigned not null auto_increment, 
       userid int unsigned not null, 
       aor varchar(255) not null,
       forwardType char(1) not null,
       forwardDestination varchar(255),
       voicemail varchar(255),
       primary key(id),
       unique index(userid,aor));


