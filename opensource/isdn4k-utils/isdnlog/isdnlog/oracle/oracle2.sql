-- run as isdn

--drop table isdn_zone;

create table isdn_zone
(
  zone number(3),		-- Zone
  name varchar2(30),	-- Name
  primary key(zone)
);

grant select on isdn_zone to public;

insert into isdn_zone(zone, name) values(0, 'S0-Bus');
insert into isdn_zone(zone, name) values(1, 'CityCall');
insert into isdn_zone(zone, name) values(2, 'RegioCall');
insert into isdn_zone(zone, name) values(3, 'GermanCall');
insert into isdn_zone(zone, name) values(4, 'C-Netz');
insert into isdn_zone(zone, name) values(5, 'C-Mobilbox');
insert into isdn_zone(zone, name) values(6, 'D1-Netz');
insert into isdn_zone(zone, name) values(7, 'D2-Netz');
insert into isdn_zone(zone, name) values(8, 'E-plus-Netz');
insert into isdn_zone(zone, name) values(9, 'E2-Netz');
insert into isdn_zone(zone, name) values(10, 'Euro City');
insert into isdn_zone(zone, name) values(11, 'Euro 1');
insert into isdn_zone(zone, name) values(12, 'Euro 2');
insert into isdn_zone(zone, name) values(13, 'Welt 1');
insert into isdn_zone(zone, name) values(14, 'Welt 2');
insert into isdn_zone(zone, name) values(15, 'Welt 3');
insert into isdn_zone(zone, name) values(16, 'Welt 4');
insert into isdn_zone(zone, name) values(17, 'Internet');

commit;

quit;
