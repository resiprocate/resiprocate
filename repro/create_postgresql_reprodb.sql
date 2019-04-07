
-- Login as the postgres admin user (normally called postgres)
--
--    createdb repro
--    createuser -D -R -S -P repro
--
-- and set up network access to the repro database through
-- /etc/postgresql/9.1/main/pg_hba.conf or equivalent

-- Must do this as the database owner/superuser
-- CREATE OR REPLACE LANGUAGE plpgsql;

-- Uncomment the following to have all tables re-created
-- DROP TABLE IF EXISTS users;
-- DROP TABLE IF EXISTS tlsPeerIdentity;
-- DROP TABLE IF EXISTS routesavp;
-- DROP TABLE IF EXISTS aclsavp;
-- DROP TABLE IF EXISTS configsavp;
-- DROP TABLE IF EXISTS staticregsavp;
-- DROP TABLE IF EXISTS filtersavp;
-- DROP TABLE IF EXISTS siloavp;


--
-- Table structure for table users
--
CREATE TABLE IF NOT EXISTS users (
  id SERIAL PRIMARY KEY,
  username VARCHAR(64) NOT NULL,
  domain VARCHAR(253),
  realm VARCHAR(253),
  passwordHash VARCHAR(32),
  passwordHashAlt VARCHAR(32),
  name VARCHAR(256),
  email VARCHAR(256),
  forwardAddress VARCHAR(256)
);

CREATE UNIQUE INDEX idx_user_domain ON users (username, domain);

--
-- Table structure for table tlsPeerIdentity
--
CREATE TABLE IF NOT EXISTS tlsPeerIdentity (
  id SERIAL PRIMARY KEY,
  peerName VARCHAR(253) NOT NULL,
  authorizedIdentity VARCHAR(253) NOT NULL
);

--
-- Table structure for table routesavp
--

CREATE TABLE IF NOT EXISTS routesavp (
  attr VARCHAR(255) PRIMARY KEY,
  value VARCHAR(4096)
);

--
-- Table structure for table aclsavp
--

CREATE TABLE IF NOT EXISTS aclsavp (
  attr VARCHAR(255) PRIMARY KEY,
  value VARCHAR(4096)
);

--
-- Table structure for table configsavp
--

CREATE TABLE IF NOT EXISTS configsavp (
  attr VARCHAR(255) PRIMARY KEY,
  value VARCHAR(4096)
);

--
-- Table structure for table staticregsavp
--

CREATE TABLE IF NOT EXISTS staticregsavp (
  attr VARCHAR(255) PRIMARY KEY,
  value VARCHAR(4096)
);

--
-- Table structure for table filtersavp
--

CREATE TABLE IF NOT EXISTS filtersavp (
  attr VARCHAR(255) PRIMARY KEY,
  value VARCHAR(4096)
);

--
-- Table structure for table siloavp
-- Note:  This table contains 2 indexes
--

CREATE TABLE IF NOT EXISTS siloavp (
  attr VARCHAR(255) NOT NULL,
  attr2 VARCHAR(255) NOT NULL,
  value VARCHAR(20315),
  PRIMARY KEY (attr, attr2)
);

GRANT SELECT, INSERT, UPDATE, DELETE ON users, routesavp,
  aclsavp, configsavp, staticregsavp, filtersavp, siloavp TO repro;
GRANT USAGE, SELECT, UPDATE ON users_id_seq TO repro;

--
-- plpgsql functions to extract values from the legacy (BDB-style) AVP tables
--
-- Warning: the current implementation of these functions assumes the data
-- has been stored in little-endian byte order.  See bug #144
-- https://www.resiprocate.org/bugzilla/show_bug.cgi?id=144
--

CREATE OR REPLACE FUNCTION BYTEA2INT(buf BYTEA,
    INOUT pos INTEGER,
    OUT val INTEGER) AS $$
DECLARE
BEGIN
  val := (GET_BYTE(buf, pos) << 8) + GET_BYTE(buf, pos-1);
  pos := pos + 2;
END;$$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION BYTEA2TIMESTAMP(buf BYTEA,
    INOUT pos INTEGER,
    OUT val TIMESTAMP) AS $$
DECLARE
  val_sec DOUBLE PRECISION;
BEGIN
  val_sec := (GET_BYTE(buf, pos+6) << 56)
             + (GET_BYTE(buf, pos+5) << 48)
             + (GET_BYTE(buf, pos+4) << 40)
             + (GET_BYTE(buf, pos+3) << 32)
             + (GET_BYTE(buf, pos+2) << 24)
             + (GET_BYTE(buf, pos+1) << 16)
             + (GET_BYTE(buf, pos) << 8)
             + GET_BYTE(buf, pos-1);
  val := TO_TIMESTAMP(val_sec);
  pos := pos + 8;
END;$$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION BYTEA2STRING(buf BYTEA,
    INOUT pos INTEGER,
    OUT sval VARCHAR) AS $$
DECLARE
  rec RECORD;
  slen INTEGER;
BEGIN
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, slen;
  sval := CONVERT_FROM(SUBSTRING(buf FROM pos FOR slen), 'utf8');
  pos := pos + slen;
END; $$
LANGUAGE 'plpgsql';

--
-- legacy table: routesavp
-- new table/view: route
--
CREATE OR REPLACE FUNCTION route_record(value VARCHAR,
    OUT method VARCHAR,
    OUT event VARCHAR,
    OUT matchingPattern VARCHAR,
    OUT rewriteExpression VARCHAR,
    OUT priority INT) AS $$
DECLARE
  pos INTEGER;
  buf BYTEA;
  version INTEGER;
BEGIN
  pos := 1;
  buf := DECODE(value, 'base64');
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, version;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, method;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, event;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, matchingPattern;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, rewriteExpression;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, priority;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION get_route_table() RETURNS TABLE (
  method VARCHAR(32),
  event VARCHAR(64),
  matchingPattern VARCHAR(1024),
  rewriteExpression VARCHAR(1024),
  priority INT)
AS $$
BEGIN
  RETURN QUERY SELECT (route_record(value)).* FROM routesavp;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE VIEW route AS
  SELECT NULL::INT AS id, * FROM get_route_table();

--
-- legacy table: aclsavp
-- new table/view: acl
--
CREATE OR REPLACE FUNCTION acl_record(value VARCHAR,
    OUT tlsPeerName VARCHAR,
    OUT address VARCHAR,
    OUT mask INT,
    OUT port INT,
    OUT family INT,
    OUT transport INT) AS $$
DECLARE
  pos INTEGER;
  buf BYTEA;
  version INTEGER;
BEGIN
  pos := 1;
  buf := DECODE(value, 'base64');
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, version;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, tlsPeerName;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, address;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, mask;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, port;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, family;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, transport;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION get_acl_table() RETURNS TABLE (
  tlsPeerName VARCHAR(64),
  address VARCHAR(64),
  mask INT,
  port INT,
  family INT,
  transport INT)
AS $$
BEGIN
  RETURN QUERY SELECT (acl_record(value)).* FROM aclsavp;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE VIEW acl AS
  SELECT NULL::INT AS id, * FROM get_acl_table();

--
-- legacy table: configsavp
-- new table/view: domain
--
CREATE OR REPLACE FUNCTION config_record(value VARCHAR,
    OUT domain VARCHAR, OUT tlsPort INT) AS $$
DECLARE
  pos INTEGER;
  buf BYTEA;
  version INTEGER;
BEGIN
  pos := 1;
  buf := DECODE(value, 'base64');
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, version;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, domain;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, tlsPort;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION get_config_table() RETURNS TABLE (
  domain VARCHAR(256),
  tlsPort INT)
AS $$
BEGIN
  RETURN QUERY SELECT (config_record(value)).* FROM configsavp;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE VIEW domain AS
  SELECT NULL::INT AS id, * FROM get_config_table();

--
-- legacy table: staticregsavp
-- new table/view: staticreg
--
CREATE OR REPLACE FUNCTION staticreg_record(value VARCHAR,
    OUT aor VARCHAR,
    OUT contact VARCHAR,
    OUT path VARCHAR) AS $$
DECLARE
  pos INTEGER;
  buf BYTEA;
  version INTEGER;
BEGIN
  pos := 1;
  buf := DECODE(value, 'base64');
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, version;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, aor;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, contact;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, path;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION get_staticreg_table() RETURNS TABLE (
  aor VARCHAR(256),
  contact VARCHAR(1024),
  path VARCHAR(4096))
AS $$
BEGIN
  RETURN QUERY SELECT (staticreg_record(value)).* FROM staticregsavp;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE VIEW staticreg AS
  SELECT NULL::INT AS id, * FROM get_staticreg_table();

--
-- legacy table: filtersavp
-- new table/view: filter
--
CREATE OR REPLACE FUNCTION filter_record(value VARCHAR,
    OUT condition1Header VARCHAR,
    OUT condition1Regex VARCHAR,
    OUT condition2Header VARCHAR,
    OUT condition2Regex VARCHAR,
    OUT method VARCHAR,
    OUT event VARCHAR,
    OUT action INT,
    OUT actionData VARCHAR,
    OUT priority INT) AS $$
DECLARE
  pos INTEGER;
  buf BYTEA;
  version INTEGER;
BEGIN
  pos := 1;
  buf := DECODE(value, 'base64');
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, version;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, condition1Header;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, condition1Regex;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, condition2Header;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, condition2Regex;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, method;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, event;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, action;
  SELECT * FROM BYTEA2VARCHAR(buf, pos) INTO pos, actionData;
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, priority;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION get_filter_table() RETURNS TABLE (
  condition1Header VARCHAR(256),
  condition1Regex VARCHAR(256),
  condition2Header VARCHAR(256),
  condition2Regex VARCHAR(256),
  method VARCHAR(32),
  event VARCHAR(64),
  action INT,
  actionData VARCHAR(256),
  priority INT)
AS $$
BEGIN
  RETURN QUERY SELECT (filter_record(value)).* FROM filtersavp;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE VIEW filter AS
  SELECT NULL::INT AS id, * FROM get_filter_table();

--
-- legacy table: siloavp
-- new table/view: silo
--
CREATE OR REPLACE FUNCTION silo_record(value VARCHAR,
    OUT destUri VARCHAR,
    OUT sourceUri VARCHAR,
    OUT originalSentTime TIMESTAMP,
    OUT tid VARCHAR,
    OUT mimeType VARCHAR,
    OUT messageBody VARCHAR) AS $$
DECLARE
  pos INTEGER;
  buf BYTEA;
  version INTEGER;
BEGIN
  pos := 1;
  buf := DECODE(value, 'base64');
  SELECT * FROM BYTEA2INT(buf, pos) INTO pos, version;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, destUri;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, sourceUri;
  SELECT * FROM BYTEA2TIMESTAMP(buf, pos) INTO pos, originalSentTime;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, tid;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, mimeType;
  SELECT * FROM BYTEA2STRING(buf, pos) INTO pos, messageBody;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION get_silo_table() RETURNS TABLE (
  destUri VARCHAR,
  sourceUri VARCHAR,
  originalSentTime TIMESTAMP,
  tid VARCHAR,
  mimeType VARCHAR,
  messageBody VARCHAR)
AS $$
BEGIN
  RETURN QUERY SELECT (silo_record(value)).* FROM siloavp;
END; $$
LANGUAGE 'plpgsql';

CREATE OR REPLACE VIEW silo AS
  SELECT NULL::INT AS id, * FROM get_silo_table();

GRANT SELECT ON route, acl, domain, staticreg, filter, silo TO repro;

