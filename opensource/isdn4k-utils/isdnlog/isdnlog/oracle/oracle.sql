-- run as isdn

--drop table isdn;

create table isdn
(
  sdate         date,			-- Zeitpunkt des Verbindungsaufbaues
  calling       varchar2(30),	-- Telefonnummer des Anrufers
  called        varchar2(30),	-- Telefonnummer des Gegners
  charge        number(5),		-- Gebuehreneinheiten (AOC-D)
  dir           char(1),		-- "I" incoming call, "O" outgoing call
  in_bytes      number(10),		-- Summe der uebertragenen Byte (incoming)
  out_bytes     number(10),		-- Summe der uebertragenen Byte (outgoing)
  msec          number(10),		-- Dauer der Verbindung in 1/100 Sekunden
  sec           number(8),		-- Dauer der Verbindung in Sekunden
  status        number(3),		-- Ursache nicht zustande gekommener Verbindung
  service       number(3),		-- Dienstkennung (1=Speech, 7=Data usw.)
  source        number(1),		-- 1=ISDN, 0=analog
  vrsion        varchar2(5),	-- Versionsnummer isdnlog
  factor        number(6,4),	-- Preis einer Gebuehreneinheit (0.121)
  currency      varchar2(3),	-- Waehrung (DM)
  pay           number(9,4),	-- Gebuehren
  provider      number(3),      -- Providercode
  provider_name varchar2(30),	-- Provider (Telekom)
  zone		    number(3)		-- CityCall, RegioCall, GermanCall, GlobalCall
);

grant select on isdn to public;
--create index isdn_i on isdn(connect, dir, calling, called);

quit;
