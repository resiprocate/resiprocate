
DAOs in AbstractDb.hxx

# Realtime tables (not cached in RAM, RuntimeDB config)

Class: UserRecord
Table: users
- full table

Class: SiloRecord
Table: siloavp          MESSAGE silo

# Restart required to read changes

Class: ConfigRecord
Table: configsavp       domain, TLS port
ConfigStore.cxx
- added using form at top of DOMAINS page
- ReproRunner loads (mDum.addDomain) at startup

# Cached in RAM, only read after a change through web interface

Class: RouteRecord
Table: routesavp        static routes
Store: DataStore::mRouteStore
- used by monkeys/StaticRoute

Class: AclRecord
Table: aclsavp          ACLs
- can be added using form at top of ACL page
- WebAdmin modifies mAclStore in RAM

Class: StaticRegRecord
Table: staticregsavp    (AoR, Contact, Path)
- can be added using form at top of REGISTRATIONS page
- loaded into mRegistrationPersistenceManager at startup
- mRegDb

Class: FilterRecord
Table: filtersavp       filters
Store: FilterStore


