
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
-- Table structure for table route
-- (legacy: routesavp)
--

CREATE TABLE IF NOT EXISTS route (
  id SERIAL PRIMARY KEY,
  method VARCHAR(32),
  event VARCHAR(64),
  matchingPattern VARCHAR(1024),
  rewriteExpression VARCHAR(1024),
  priority INT
);

--
-- Table structure for table acl
-- (aclsavp)
--

CREATE TABLE IF NOT EXISTS acl (
  id SERIAL PRIMARY KEY,
  tlsPeerName VARCHAR(64),
  address VARCHAR(64),
  mask INT,
  port INT,
  family INT,
  transport INT
);

--
-- Table structure for table config
-- (legacy configsavp)
--

CREATE TABLE IF NOT EXISTS domain (
  id SERIAL PRIMARY KEY,
  domain VARCHAR(256),
  tlsPort INT
);

--
-- Table structure for table staticreg
-- (legacy staticregsavp)
--

CREATE TABLE IF NOT EXISTS staticreg (
  id SERIAL PRIMARY KEY,
  aor VARCHAR(256),
  contact VARCHAR(1024),
  path VARCHAR(4096)
);

--
-- Table structure for table filter
-- (legacy filtersavp)
--

CREATE TABLE IF NOT EXISTS filter (
  id SERIAL PRIMARY KEY,
  condition1Header VARCHAR(256),
  condition1Regex VARCHAR(256),
  condition2Header VARCHAR(256),
  condition2Regex VARCHAR(256),
  method VARCHAR(32),
  event VARCHAR(64),
  action INT,
  actionData VARCHAR(256),
  priority INT
);

--
-- Table structure for table siloavp
-- Note:  This table contains 2 indexes
--
-- FIXME: there is no point converting this table from legacy/BDB/AVP
--        to regular columns as it can't be used in read-only mode,
--        the code needs to be changed to perform read and write
--        queries directly on the new table
--


CREATE TABLE IF NOT EXISTS siloavp (
  attr VARCHAR(255) NOT NULL,
  attr2 VARCHAR(255) NOT NULL,
  value VARCHAR(20315),
  PRIMARY KEY (attr, attr2)
);

GRANT SELECT, INSERT, UPDATE, DELETE ON users, route,
  acl, domain, staticreg, filter, siloavp TO repro;
GRANT USAGE, SELECT, UPDATE ON
  users_id_seq, route_id_seq, acl_id_seq, domain_id_seq,
  staticreg_id_seq, filter_id_seq TO repro;

--
-- until repro's code has been updated to read the new tables
-- directly, it can read them through these VIEWs which emulate the
-- attr/value columns used in the original database schema
--
-- When using these VIEWs, modifications can't be made from the repro
-- web interface and an alternative web interface or direct SQL commands
-- should be used.
--

--
-- plpgsql functions to convert values to the legacy (BDB-style) AVP views
--
-- Warning: the current implementation of these functions assumes the data
-- has been stored in little-endian byte order.  See bug #144
-- https://www.resiprocate.org/bugzilla/show_bug.cgi?id=144
--

CREATE OR REPLACE FUNCTION INT2BYTEA(x INT) RETURNS BYTEA AS'
DECLARE
  buf BYTEA;
  _x INT;
BEGIN
  _x := x;
  IF _x IS NULL
  THEN
    _x = 0;
  END IF;
  buf = INT4SEND(_x);
  RETURN SUBSTRING(buf FROM 4 FOR 1) || SUBSTRING(buf FROM 3 FOR 1);
END;'
LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION STRING2BYTEA(s VARCHAR) RETURNS BYTEA AS $$
DECLARE
  buf BYTEA;
  buf_len INT;
BEGIN
  buf_len = OCTET_LENGTH(s);
  IF buf_len = 0
  THEN
    RETURN INT2BYTEA(buf_len);
  END IF;
  RETURN INT2BYTEA(buf_len) || CONVERT_TO(s, 'utf8');
END; $$
LANGUAGE 'plpgsql';

--
-- legacy table: routesavp
-- new table/view: route
--
-- see:
--    repro/RouteStore.cxx buildKey   -> attr
--    repro/AbstractDb.cxx getRoute   -> value
--
CREATE VIEW routesavp AS
  SELECT
    ';' || priority || ':' || method ||
    ': ' || event || ' : ' || matchingPattern AS attr,
    ENCODE(
      int2bytea(1)
      || string2bytea(method)
      || string2bytea(event)
      || string2bytea(matchingPattern)
      || string2bytea(rewriteExpression)
      || int2bytea(priority), 'base64') AS value
  FROM route;

--
-- legacy table: aclsavp
-- new table/view: acl
--
-- see:
--    repro/AclStore.cxx buildKey    -> attr
--        tlsPeerName+":"+address+"/"+Data(mask)+":"+Data(port)+":"+Data(family)+":"+Data(transport)
--    repro/AbstractDb.cxx getAcl    -> value
--        1 tlsPeerName address 2mask 2port 2family 2transport
--
CREATE VIEW aclsavp AS
  SELECT
    tlsPeerName || ':' || address || '/' || mask || ':' || port
    || ':' || family || ':' || transport AS attr,
    ENCODE(
      int2bytea(1)
      || string2bytea(tlsPeerName)
      || string2bytea(address)
      || int2bytea(mask)
      || int2bytea(port)
      || int2bytea(family)
      || int2bytea(transport), 'base64') AS value
  FROM acl;

--
-- legacy table: configsavp
-- new table/view: domain
--
-- see:
--    repro/ConfigStore.cxx buildKey   -> attr
--        domain
--    repro/AbstractDb.cxx getConfig   -> value
--        1 domain 2tlsPort
--
CREATE VIEW configsavp AS
  SELECT
    domain.domain AS attr,
    ENCODE(
      int2bytea(1)
      || string2bytea(domain.domain)
      || int2bytea(tlsPort), 'base64') AS value
  FROM domain;

--
-- legacy table: staticregsavp
-- new table/view: staticreg
--
-- see:
--    repro/StaticRegStore.cxx buildKey  -> attr
--        aor+":"+contact
--    repro/AbstractDb.cxx getStaticReg  -> value
--        1 aor contact path
--
CREATE VIEW staticregsavp AS
  SELECT
    aor || ':' || contact AS attr,
    ENCODE(int2bytea(1)
           || string2bytea(aor)
           || string2bytea(contact)
           || string2bytea(path), 'base64') AS value
  FROM staticreg;

--
-- legacy table: filtersavp
-- new table/view: filter
--
-- see:
--    repro/FilterStore.cxx buildKey    -> attr
--        cond1Header + ":" + cond1Regex + ":" + cond2Header + ":" + cond2Regex + ":" + method + ":" + event
--    repro/AbstractDb.cxx getFilter    -> value
--        1 Condition1Header Condition1Regex Condition2Header Condition2Regex method event 2action actiondata 2order
--
CREATE VIEW filtersavp AS
  SELECT
    condition1Header || ':' || condition1Regex || ':' || condition2Header
    || ':' || condition2Regex || ':' || method || ':' || event AS attr,
    ENCODE(int2bytea(1)
      || string2bytea(condition1Header)
      || string2bytea(condition1Regex)
      || string2bytea(condition2Header)
      || string2bytea(condition2Regex)
      || string2bytea(method)
      || string2bytea(event)
      || int2bytea(action)
      || string2bytea(actionData)
      || int2bytea(priority), 'base64') AS value
  FROM filter;

--
-- legacy table: siloavp
-- new table/view: silo
--
-- FIXME: there is no point converting this table from legacy/BDB/AVP 
--        to regular columns as it can't be used in read-only mode, 
--        the code needs to be changed to perform read and write 
--        queries directly on the new table
--        HINT:  To enable inserting into the view,
--                provide an INSTEAD OF INSERT trigger or
--                an unconditional ON INSERT DO INSTEAD rule.
--        See:
--          https://wiki.postgresql.org/wiki/Updatable_views
--
-- see:
--    repro/SiloStore.cxx buildKey     -> attr
--        (UInt64)originalSendTime ":" + tid
--    repro/AbstractDb.cxx getSecondaryKey               attr2
--        destUri
--    repro/AbstractDb.cxx decodeSiloRecord -> value
--        1 destUri sourceUri 8originalSentTime tid mimeType messageBody
--

GRANT SELECT, INSERT, UPDATE, DELETE ON routesavp,
  aclsavp, configsavp, staticregsavp, filtersavp TO repro;

