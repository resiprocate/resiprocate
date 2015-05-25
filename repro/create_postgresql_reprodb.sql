
-- Login as the postgres admin user (normally called postgres)
--
--    createdb repro
--    createuser -D -R -S -P repro
--
-- and set up network access to the repro database through
-- /etc/postgresql/9.1/main/pg_hba.conf or equivalent


-- Uncomment the following to have all tables re-created
-- DROP TABLE IF EXISTS users;
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

