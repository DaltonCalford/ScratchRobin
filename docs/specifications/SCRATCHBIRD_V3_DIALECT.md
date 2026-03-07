# ScratchBird V3 Authorized SQL Dialect Guide – Canonical EBNF

> Scope: This authorized guide defines the canonical EBNF surface syntax (parser entry points / statement families) for ScratchBird V3, with:
> 
> - Style normalizations applied (canonical spellings / shapes)
> - Previously identified lifecycle gaps filled (EVENT, PUBLICATION/SUBSCRIPTION ALTER+SHOW)
> 
> Notes:
> 
> - This is **surface EBNF**. Expression grammar is provided at a practical level (enough to define statement shapes) rather than attempting to fully encode SQL operator precedence.
> - Keywords are **not** assumed reserved globally; the parser is context-sensitive. The EBNF uses keyword tokens for readability.
> - Legacy alias compatibility syntax is excluded from canonical grammar.

## Implementation Status Snapshot (`2026-03-04`)

- Canonical V3 dialect style is fixed to explicit grammar forms (for example `EXTRACT(<field> FROM <expr>)`), and legacy compatibility aliases are removed from authorized syntax.
- Recursive schema/search-path/current-schema semantics and explicit name-resolution rules are encoded in this guide as canonical behavior.
- Object-family and parent-scoped uniqueness rules are encoded:
  - schema-scoped uniqueness by family,
  - parent-required uniqueness for `INDEX`/`TRIGGER`,
  - database-scoped uniqueness for `FILESPACE`,
  - global uniqueness/identity semantics for `DOMAIN`.
- `SHOW CURRENT SCHEMA`, `SHOW SEARCH PATH`, and `SHOW SCHEMA SEARCH PATH` are part of the authorized dialect surface.
- PostgreSQL upstream bounded harness status now includes dated closure of the `test_setup` duplicate-function blocker (`fipshash` second create) and inherited-column notice parity:
  - direct isolated `pg_regress` execution (`test_setup`) passes (`1/1`) with durable artifacts under `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_024216_single_test_setup/upstream`,
  - evidence recorded at `ScratchBird/tests/compatibility/results/emulation/epfc-026-postgresql-upstream-harness-evidence-20260304T024216Z.md`.
- JDBC promotion and parser-adjacent execution alignment is closed with dated PASS evidence in `LD-013`:
  - `local_work/docs/specifications/work/implementation_tracks/sql_language_workpack/evidence/LD-013/P21_JDBC_GATE_05_COMPLETION_CHECKLIST_2026-03-03.md`
  - `local_work/docs/specifications/work/implementation_tracks/sql_language_workpack/evidence/LD-013/P21_JDBC_GATE_ROLLUP.md`
  - `local_work/docs/specifications/work/implementation_tracks/sql_language_workpack/evidence/LD-013/P21_JDBC_GATE_06_EXECUTION_CHECKLIST_2026-03-03.md`
  - `local_work/docs/specifications/work/implementation_tracks/sql_language_workpack/evidence/LD-013/G06_command_log_04_20260303.md`
- `nativeSQL` conversion-vs-execution parity is validated in integration coverage (`SBNativeSQLParityTest`).
- Phase-6 profile-gated DML parser feature-key wiring is now present for:
  - `F_DML_MYSQL_ON_DUPLICATE_KEY`,
  - `F_DML_UPDATE_ORDER_LIMIT`,
  - `F_DML_DELETE_ORDER_LIMIT`,
  - `F_DML_WRITABLE_CTE`,
  - `F_DML_MERGE_NOT_MATCHED_BY_SOURCE`,
  with deterministic disabled-path (`PRS_0503`) parser coverage (targeted gate test run passed on `2026-03-03`).
- Replication-family execution closure (`2026-03-03`) now includes explicit runtime handlers for:
  - `ALTER PUBLICATION` rename/owner transfer, publish flag updates, and partition-root option toggles.
  - `ALTER SUBSCRIPTION` rename/owner transfer, enable/disable, slot-name set/clear, and option toggles (`SYNC_COMMIT`, `COPY_DATA`, `CREATE_SLOT`, `REFRESH_ON_START`).
  - Targeted post-change verification: `ctest -R "PostgreSQLParserTest|CompatibilityPostgreSQL" --output-on-failure` -> `164/164 PASS`; evidence log `ScratchBird/tests/compatibility/results/emulation/ctest-postgresql-parser-compat-20260303T222905Z.log`.
- Architecture rule in effect:
  - `v3(native)` is the canonical semantic surface.
  - emulated parsers are intentionally separate engine-dialect translators that map source-engine SQL to internal methods while preserving engine-equivalent behavior contracts.
  - no capability may exist only in an emulated parser; canonical `v3(native)` must provide an equivalent functional route in v3 dialect form.
- Emulation security rule in effect (`2026-03-04`):
  - each emulated engine (`firebird`, `mysql`, `postgresql`) has an independent security policy profile.
  - shared adapter infrastructure is allowed; shared policy behavior across emulated engines is not allowed by default.
  - EMU-GATE-03 closure now requires engine-specific policy artifacts and evidence.
  - policy profile index: `local_work/docs/planning/EMULATED_ENGINE_SECURITY_POLICY_PROFILES_2026-03-04.md`.
- Runtime implementation status (`2026-03-04`):
  - PostgreSQL adapter auth-method selection now enforces PostgreSQL-lane policy method set (`PASSWORD`, `MD5`, `SCRAM-SHA-256`) with deterministic fallback.
  - MySQL adapter auth-response validation now enforces MySQL-lane plugin allowlist (`mysql_native_password`, `caching_sha2_password`).
  - Firebird adapter connect handshake now uses explicit Firebird-lane authentication-policy gate and plugin constant routing.
  - Firebird adapter now deterministically rejects unsupported configured auth methods at connect-time under Firebird emulation policy rules.
  - PostgreSQL adapter now deterministically rejects unsupported configured auth methods at startup with policy-scoped failure (`0A000`).
  - MySQL adapter now carries deterministic policy handling for `sha256_password` (explicit verifier-path deny until dedicated flow support) and `auth_socket` (host-identity verifier rule).
  - Targeted policy-auth boundary unit run passed on `2026-03-04` (`4/4` tests); evidence log:
    `ScratchBird/tests/compatibility/results/emulation/emu-gate03-policy-auth-targeted-20260304T141336Z.log`.
  - Expanded provider-family policy assertions passed on `2026-03-04` (`8/8` tests); evidence log:
    `ScratchBird/tests/compatibility/results/emulation/emu-gate03-auth-policy-provider-targeted-20260304T141648Z.log`.
  - Targeted compatibility suites for emulated lanes passed on `2026-03-04` (`CompatibilityMySQL`, `CompatibilityPostgreSQL`, `CompatibilityFirebird`; setup/teardown included, `5/5` CTest pass); evidence log:
    `ScratchBird/tests/compatibility/results/emulation/emu-gate03-compatibility-targeted-20260304T141834Z.log`.
  - Deferred-provider and fallback-policy assertion suite passed on `2026-03-04` (`13/13` targeted tests) including PostgreSQL peer/sasl rejects, MySQL unknown-plugin verifier reject path, and Firebird SCRAM-512 policy-allow handshake path; evidence log:
    `ScratchBird/tests/compatibility/results/emulation/emu-gate03-auth-policy-deferred-provider-targeted-20260304T142524Z.log`.
- Emulated Firebird parser status (`2026-03-03`) now includes explicit top-level support for `DECLARE FILTER` and `DECLARE EXTERNAL FUNCTION`, while non-server top-level forms (`DECLARE VARIABLE`, top-level `BEGIN ... END`) remain deterministic rejects by policy.
- Emulated Firebird parser status (`2026-03-03`) also includes explicit object-family handlers for `ALTER ROLE`, `ALTER FUNCTION` variants (including `ALTER EXTERNAL FUNCTION`), `CREATE/DROP COLLATION`, `CREATE/DROP SHADOW` (simulation-mapped), `CREATE/DROP MAPPING`, `CREATE/ALTER/DROP USER`, and `CREATE/ALTER/DROP SCHEMA`; parser-surface closure is complete for the currently tracked `EPFC-017` row set.
- Emulated Firebird parser status (`2026-03-03`) includes explicit managed-session `SET` support for `SET DEBUG OPTION`, `SET DECFLOAT ROUND`, `SET DECFLOAT TRAPS`, `SET BIND`, `SET SEARCH_PATH`, `SET OPTIMIZE`, `SET TIME ZONE`, `SET SESSION IDLE TIMEOUT`, and `SET STATEMENT TIMEOUT`; legacy `SET TERM` remains intentionally rejected as a client-only command.
- Cross-engine emulation status (`2026-03-03`) now includes a shared file-level simulation adapter contract for `CREATE/DROP DATABASE` and related file-oriented operations, enforcing parser-side simulation mapping and engine-profile response adaptation without direct filesystem orchestration.
- Cross-engine result-shape status (`2026-03-03`) now includes:
  - MySQL protocol column-definition metadata normalization in `ScratchBird/src/ipc/external_agents/mysql_parser_agent.cpp`.
  - PostgreSQL protocol `RowDescription`/`DataRow` response encoding and command-tag normalization in `ScratchBird/src/ipc/external_agents/postgresql_parser_agent.cpp`.
  - Firebird protocol status-vector normalization and deterministic `op_sql_response`/`op_fetch_response` translation in `ScratchBird/src/ipc/external_agents/firebird_parser_agent.cpp`.
- Cross-engine diagnostics status (`2026-03-03`) now includes deterministic SQLSTATE/error translation hooks per emulated protocol lane (MySQL errno+SQLSTATE, PostgreSQL ErrorResponse field `C`, Firebird GDS/status-vector extraction), with mapping authority documented in `local_work/docs/planning/EMULATED_ERROR_SQLSTATE_TRANSLATION_MATRIX_2026-03-03.md`.
- Cross-engine boundary-suite status (`2026-03-03`) now includes unified emulation boundary accept/reject unit packs in `ScratchBird/tests/unit/test_emulated_parser_boundary_contracts.cpp`, with pack inventory documented in `local_work/docs/planning/EMULATED_BOUNDARY_TEST_PACKS_2026-03-03.md`.
- Canonical DML equivalents for emulated compatibility families are explicit:
  - upsert functionality routes through `ON CONFLICT ... DO UPDATE`,
  - bounded mutation functionality routes through canonical key-subquery mutation forms,
  - merge source-absence mutation routes through `MERGE ... WHEN NOT MATCHED BY SOURCE ...`.
- Targeted canonical-equivalent parser verification completed (`2026-03-03`):
  - `/home/dcalford/CliWork/ScratchBird/build/tests/scratchbird_tests --gtest_filter=ParserV3NativeExtensionSurfaceTest.ParsesCanonicalDmlEquivalentsForCompatibilityFamilies`
  - Result: `PASS` (`1/1` tests, exit code `0`).
- Targeted native parser closure verification completed (`2026-03-03`):
  - `/home/dcalford/CliWork/ScratchBird/build/tests/scratchbird_tests --gtest_filter=ParserV3CanonicalRejectionsTest.*:ParserV3NativeExtensionSurfaceTest.RejectsRemovedLegacyFunctionAndViewRoots:QueryCompilerV3Test.CanonicalFunctionDispatchUsesDedicatedOpcodes:QueryCompilerV3Test.ExecuteCanonicalExtractMonthFromCurrentDate:QueryCompilerV3Test.CompileExtractUnknownFieldUsesDeterministicErrorCode:QueryCompilerV3Test.ExtractElementFieldNotValidForTypeUsesDeterministicErrorCode:QueryCompilerV3Test.ExtractElementConstraintViolationUsesDeterministicErrorCode:QueryCompilerV3Test.ExtractElementSupportsTimeWithTimeZoneType:QueryCompilerV3Test.ExtractElementSupportsTimestampWithTimeZoneType`
  - Result: `PASS` (`10/10` tests, exit code `0`).
- Runtime dispatch closure status (`2026-03-04`) for PostgreSQL parity blockers:
  - `SBLR3_ALTER_TABLESPACE` is now routed through v3 DDL mutation dispatch in `src/sblr/executor.cpp`, and `AlterTablespaceStmt` now emits explicit per-action `alterations` payload entries in `src/parser/v3_emitter.cpp` (instead of placeholder options-only payload).
  - `SBLR3_DROP_SEQUENCE`, `SBLR3_DROP_USER`, and `SBLR3_DROP_GROUP` are now routed through v3 DDL mutation dispatch in `src/sblr/executor.cpp` (no unknown-opcode dispatch reject path for these families).
  - `DropRoleStmt`, `DropUserStmt`, and `DropGroupStmt` now propagate `IF EXISTS`/`CASCADE` flags in `src/parser/v3_emitter.cpp` so canonical drop intent reaches runtime.
- PostgreSQL bounded upstream compatibility recheck (`2026-03-04`, run id `20260304_123210`):
  - targeted lane: `SCRATCHBIRD_PG_REGRESS_TESTS='object_address tablespace'`.
  - `object_address`: parser closure improved to remove prior parse-error class (no `Parse error` entries in run artifact).
  - `tablespace`: still broad-fail with many parser/runtime compatibility deltas; active remediation remains in PostgreSQL emulation lane.
  - evidence artifacts:
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_123210/upstream/pg_regress.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_123210/upstream/regression.diffs`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_123210/upstream/results/object_address.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_123210/upstream/results/tablespace.out`
- PostgreSQL bounded upstream parser closure recheck (`2026-03-04`, run id `20260304_125332`):
  - targeted lane: `SCRATCHBIRD_PG_REGRESS_TESTS='object_address tablespace'`.
  - parser status: `0` `Parse error` entries in both result artifacts:
    - `.../results/object_address.out`
    - `.../results/tablespace.out`
  - remaining failures are runtime/semantic parity gaps (catalog object availability, tablespace behavior, and emulation-result shape mismatches), not parser syntax rejection.
  - evidence artifacts:
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_125332/upstream/pg_regress.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_125332/upstream/regression.diffs`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_125332/upstream/results/object_address.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_125332/upstream/results/tablespace.out`
- PostgreSQL runtime parity increment (`2026-03-04`):
  - v3 `CREATE TABLESPACE` runtime bridge now defaults `autoextend_enabled` to `true` and honors explicit payload overrides (`autoextend_enabled`, `autoextend_size_mb`, `max_size_mb`, `prealloc_pages`) in `ScratchBird/src/sblr/executor.cpp`.
  - runtime tablespace alias normalization now treats `pg_default` as default tablespace and deterministically rejects `pg_global` for non-shared relations with PostgreSQL-style message in create-table/create-index/v3 mutation resolution paths (`ScratchBird/src/sblr/executor.cpp`).
  - PostgreSQL wire auth-path blocker for upstream harnesses is closed:
    - `psql` direct probe now authenticates and executes (`select 1`) against `sb_listener_pg` on `127.0.0.1:16432`.
    - bounded upstream run `20260304_134157` executes `object_address/tablespace` statements end-to-end (runtime diffs remain, but no immediate auth gate failure).
    - implementation touchpoint: `ScratchBird/src/protocol/adapters/postgresql_adapter.cpp` (SCRAM client-first normalization + temporary PostgreSQL-lane auth fallback for harness reachability while SCRAM parity work remains open).
  - direct PG-lane runtime probe evidence recorded at:
    - `local_work/Update_Parser_Work/postgresql_runtime_parity_probe_2026-03-04.md`
  - bounded upstream rerun `20260304_131027` recorded the prior auth blocker state (pre-fix):
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_131027/upstream/pg_regress.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_131027/upstream/results/object_address.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_131027/upstream/results/tablespace.out`
  - latest bounded upstream evidence after auth-path closure:
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_134157/upstream/pg_regress.out`
    - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_134157/upstream/regression.diffs`
  - latest bounded upstream object-address lane evidence after table-function bridge closure (`20260304_140258`):
    - `FROM pg_get_object_address(...)` no longer fails relation resolution (`Table or view not found`) and no longer fails table-ref validation (`V3 SELECT FROM requires table reference`); runtime now executes table-function source and emits shaped rowset columns (`classid`, `objid`, `objsubid`).
    - semantic parity remains open (expected PostgreSQL error/result semantics still differ in `object_address` suite).
  - implementation increment verified (`2026-03-04`, bounded lane `20260304_143625`):
    - `PG_GET_OBJECT_ADDRESS` scalar path now validates and parses object type/name/arg inputs (including PostgreSQL-style error text for: unrecognized type, unsupported type, null list entries, empty table name list, and access-method operator/function not-found cases).
    - `FROM PG_GET_OBJECT_ADDRESS(...)` now consumes scalar resolver output instead of a fixed placeholder row, so both scalar/table-function execution paths share the same error gate and address-shaping behavior.
    - bounded upstream evidence:
      - relation-resolution/table-ref reject classes remain closed for function table source execution.
      - targeted object-address semantic failures now surface through runtime with expected message cores, but still wrapped as `Execution error: ...`.
      - remaining open parity gaps in this lane include:
        - roundtrip query failure at `ERROR:  Execution error: unrecognized object type ""`
        - PL/pgSQL DO-block execution gate: `FOR EXECUTE requires precompiled SBLR; SQL execution in engine is disabled`
    - artifacts:
      - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_143625/upstream/pg_regress.out`
      - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_143625/upstream/regression.diffs`
      - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_143625/upstream/results/object_address.out`
      - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_140258/upstream/pg_regress.out`
      - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_140258/upstream/regression.diffs`
      - `ScratchBird/tests/compatibility/postgresql/results/ctest/20260304_140258/upstream/results/object_address.out`
- Targeted unit-test additions for the above closure (`2026-03-04`):
  - `ParserV3IndexManagementTest.ParsesAlterTablespaceActions`
  - `SBLRVNextExecutorDispatchContractTest.AlterTablespaceOpcodeRoutesWithoutUnknownOpcodeReject`
  - `SBLRVNextExecutorDispatchContractTest.DropSequenceUserGroupOpcodesRouteWithoutUnknownOpcodeReject`
- This document remains the parser implementation authority; checklist/risk docs now record closure evidence for completed parity lanes.

### Legend

- **[impl]** = clearly supported by v3 AST shape today

- **[doc]** = explicitly stated in `05-index-framework.md`

- **[spec]** = recommended to add to final spec (or explicitly reject) so migration doesn’t guess

---

## 0) Lexical Conventions

```ebnf
<identifier>         ::= <unquoted-ident> | <quoted-ident> ;
<unquoted-ident>     ::= letter { letter | digit | '_' | '$' } ;
<quoted-ident>       ::= '"' { <qchar> } '"' ;

<integer>            ::= digit { digit } ;
<decimal>            ::= digit { digit } '.' digit { digit } ;
<string>             ::= '\'' { <schar> } '\'' ;
<boolean>            ::= TRUE | FALSE ;

<path>               ::= <string> | <identifier> ;
<filespace-name>     ::= <identifier> ;  (* database-scoped name *)

<comment>            ::= '--' { <any-char-except-newline> } '\n'
                      | '/*' { <any-char> } '*/' ;

<ws>                 ::= { ' ' | '\t' | '\r' | '\n' | <comment> } ;

(* Tokens are case-insensitive at parse-time. *)
```

---

## 1) Top-Level

```ebnf
<command-stream>     ::= <ws> <command> { <ws> ';' <ws> <command> } <ws> [ ';' ] <ws> ;

<command>            ::= <session-statement>
                      | <utility-statement>
                      | <ddl-statement>
                      | <dml-statement>
                      | <dcl-statement>
                      | <txn-statement>
                      | <control-statement> ;
```

---

## 2) Transaction & Session

```ebnf
<txn-statement>      ::= BEGIN [ WORK ]
                      | START TRANSACTION
                      | COMMIT [ WORK ]
                      | ROLLBACK [ WORK ]
                      | SAVEPOINT <identifier>
                      | RELEASE SAVEPOINT <identifier>
                      | PREPARE TRANSACTION <identifier>
                      | COMMIT PREPARED <identifier>
                      | ROLLBACK PREPARED <identifier> ;

<session-statement>  ::= SET <set-target>
                      | RESET <reset-target>
                      | SHOW <show-target>
                      | DESCRIBE <describe-target> ;

<set-target>         ::= SESSION <set-kv-list>
                      | TRANSACTION ISOLATION LEVEL <isolation-level>
                      | CURRENT SCHEMA <schema-path>
                      | SCHEMA SEARCH PATH <schema-search-path>
                      | SEARCH PATH <schema-search-path>
                      | <identifier> '=' <expr> ;

<reset-target>       ::= ALL
                      | SESSION
                      | SEARCH PATH
                      | CURRENT SCHEMA
                      | <identifier> ;

(* Recursive schema tree and path forms:
   - no leading dot => absolute from root
   - leading dot => relative from current schema *)
<absolute-qualified-name> ::= <identifier> { '.' <identifier> } ;
<current-relative-qualified-name> ::= '.' <identifier> { '.' <identifier> } ;
<qualified-name> ::= <absolute-qualified-name> | <current-relative-qualified-name> ;
<global-domain-name> ::= <identifier> ;

(* schema-path accepts absolute paths, current-schema marker, and current-relative paths *)
<schema-path> ::= <absolute-qualified-name>
               | '.'
               | <current-relative-qualified-name> ;

(* Linux-like ordered search path; first match wins *)
<schema-search-path> ::= <schema-path> { ':' <schema-path> } ;

<describe-target>    ::= <qualified-name>
                      | TABLE <qualified-name>
                      | VIEW <qualified-name>
                      | FUNCTION <qualified-name>
                      | PROCEDURE <qualified-name>
                      | DOMAIN <global-domain-name>
                      | SCHEMA <qualified-name> ;

<isolation-level>    ::= READ COMMITTED | READ UNCOMMITTED | REPEATABLE READ | SERIALIZABLE ;

<set-kv-list>        ::= '(' <set-kv> { ',' <set-kv> } ')' ;
<set-kv>             ::= <identifier> '=' <expr> ;
```

Name isolation and resolution semantics:

1. Schemas are recursive; any schema may contain child schemas.
2. Schema-scoped objects use object-family namespaces and must be unique within parent schema per family (`TABLE`, `VIEW`, `SEQUENCE`, `FUNCTION`, `PROCEDURE`, `PACKAGE`, `TYPE`, etc.).
3. Cross-family name reuse is allowed within the same parent schema (for example, a procedure and function may share a base name).
4. `FILESPACE` names are unique within a database scope.
5. `DOMAIN` names are globally scoped type-system names and must be unique across the full database namespace.
6. Domain identity is UUID-based and shared across database/group/cluster; conflicts are resolved by group consensus.
7. `DOMAIN` resolution does not use schema search path; domain references bind in the global domain namespace.
8. Table-scoped objects must be unique within their parent table/view:
   - `INDEX` names are unique per parent table/view.
   - `TRIGGER` names are unique per parent table/view.
9. `INDEX` and `TRIGGER` references for `ALTER`/`DROP`/`VALIDATE` require explicit parent qualification (`<name> ON <parent>`).
10. `FILESPACE` references bind in database scope and are not schema-path-resolved.
11. Sessions must always have a current schema.
12. User/role/group defaults may provide initial current schema and default search path.
13. Unqualified names resolve by ordered session search path, first match wins.
14. If current schema is omitted from search path, unqualified names do not implicitly prefer current schema.
15. Equivalent resolution examples:
   - If current schema is `users.public`, `CREATE TABLE john (...)` and `CREATE TABLE users.public.john (...)` resolve to the same object path.
   - If current schema is `users.john`, `.frank.franks_table` resolves to `users.john.frank.franks_table`.
   - If search path starts with `users.public` and excludes current schema, `SELECT * FROM john` resolves to `users.public.john` even when current schema has its own `john`.

---

## 3) SHOW / Introspection (canonicalized)

Canonical rule:

- Prefer plural list forms (SHOW TABLES, SHOW DATABASES, …)
- Provide SHOW CREATE forms where relevant
- Dedicated SHOW forms for CLUSTER/CUBE

```ebnf
<show-target>        ::= ALL
                      | PARSER VERSION
                      | SQL DIALECT
                      | VERSION
                      | SYSTEM
                      | METRICS
                      | VARIABLE [ <identifier> ]
                      | TRANSACTION ISOLATION LEVEL
                      | CURRENT SCHEMA
                      | SCHEMA SEARCH PATH
                      | SEARCH PATH

                      | DATABASES
                      | DATABASE [ <qualified-name> ]
                      | SCHEMAS [ IN | FROM <path> ]
                      | SCHEMA <qualified-name>

                      | TABLES [ IN | FROM <path> ]
                      | TABLE <qualified-name>
                      | COLUMNS [ IN | FROM <path> ]
                      | INDEXES [ IN | FROM <path> ]
                      | INDEX HEALTH | INDEX USAGE | INDEX STORAGE | INDEX CONTENTION | INDEX OPTIONS
                      | CREATE TABLE <qualified-name>

                      | VIEWS
                      | VIEW <qualified-name>
                      | CREATE VIEW <qualified-name>

                      | TRIGGERS
                      | TRIGGER <qualified-name>

                      | FUNCTIONS
                      | FUNCTION <qualified-name>
                      | PROCEDURES
                      | PROCEDURE <qualified-name>

                      | DOMAINS
                      | DOMAIN <global-domain-name>

                      | ROLES
                      | ROLE <qualified-name>
                      | GRANTS

                      | PACKAGES
                      | PACKAGE <qualified-name>

                      | JOBS
                      | JOB RUNS [ FOR <qualified-name> ]

                      | CHECKS
                      | COLLATIONS
                      | COMMENTS
                      | DEPENDENCIES [ FOR <qualified-name> ]

                      | PUBLICATIONS
                      | PUBLICATION <qualified-name>
                      | CREATE PUBLICATION <qualified-name>

                      | SUBSCRIPTIONS
                      | SUBSCRIPTION <qualified-name>
                      | CREATE SUBSCRIPTION <qualified-name>

                      | EVENTS
                      | EVENT <qualified-name>
                      | CREATE EVENT <qualified-name>

                      | CLUSTER <cluster-show-spec>
                      | CUBE <cube-show-spec>
                      | STATS [ <stats-show-spec> ] ;

<cluster-show-spec>  ::= <identifier> [ <cluster-show-options> ] ;
<cluster-show-options>::= '(' <kv-list> ')' ;

<cube-show-spec>     ::= <identifier> [ <cube-show-options> ] ;
<cube-show-options>  ::= '(' <kv-list> ')' ;

<stats-show-spec>    ::= <identifier> | TABLE <qualified-name> | DATABASE <qualified-name> ;

<kv-list>            ::= <kv> { ',' <kv> } ;
<kv>                 ::= <identifier> '=' <expr> ;
```

---

## 4) Utility Statements

```ebnf
<utility-statement>  ::= EXPLAIN <explain-spec>
                      | ANALYZE <analyze-spec>
                      | SECURITY LABEL <security-label-spec>
                      | COMMENT ON <comment-target>
                      | DROP COMMENT ON <comment-target> ;

<explain-spec>       ::= [ '(' <kv-list> ')' ] <command> ;
<analyze-spec>       ::= [ '(' <kv-list> ')' ] ( <command> | TABLE <qualified-name> ) ;

<security-label-spec>::= ON <security-label-target> IS <string> ;

<comment-target>     ::= TABLE <qualified-name>
                      | VIEW <qualified-name>
                      | INDEX <index-ref>
                      | COLUMN <qualified-name>
                      | SEQUENCE <qualified-name>
                      | DOMAIN <global-domain-name>
                      | TYPE <qualified-name>
                      | FUNCTION <qualified-name>
                      | PROCEDURE <qualified-name>
                      | TRIGGER <trigger-ref>
                      | PACKAGE <qualified-name>
                      | EXCEPTION <qualified-name>
                      | UDR <qualified-name>
                      | ROLE <qualified-name>
                      | USER <qualified-name>
                      | GROUP <qualified-name>
                      | POLICY <qualified-name> ON <qualified-name>
                      | TOKEN <qualified-name>
                      | QUOTA PROFILE <qualified-name>
                      | CONNECTION RULE <qualified-name>
                      | SERVER <qualified-name>
                      | FOREIGN DATA WRAPPER <qualified-name>
                      | FOREIGN TABLE <qualified-name>
                      | USER MAPPING FOR <principal> SERVER <qualified-name>
                      | SYNONYM <qualified-name>
                      | PUBLIC SYNONYM <qualified-name>
                      | JOB <qualified-name>
                      | PUBLICATION <qualified-name>
                      | SUBSCRIPTION <qualified-name>
                      | REPLICATION CHANNEL <qualified-name>
                      | EVENT <qualified-name>
                      | TABLESPACE <qualified-name>
                      | FILESPACE <filespace-name>
                      | CLUSTER <qualified-name>
                      | CUBE <qualified-name>
                      | DATABASE <qualified-name>
                      | SCHEMA <qualified-name> ;
```

---

## 5) Control / Admin

```ebnf
<control-statement>  ::= SWEEP <sweep-spec>
                      | BACKUP <backup-spec>
                      | RESTORE <restore-spec>
                      | VALIDATE <validate-spec>
                      | CONNECT <connect-spec>
                      | DISCONNECT [ <identifier> ]
                      | EXECUTE JOB <qualified-name> [ '(' <arg-list> ')' ]
                      | CANCEL JOB RUN <identifier>
                      | CALL <call-spec>
                      | RESYNC REPLICATION CHANNEL <qualified-name>
                      | SERVICE CHANNEL <service-channel-spec>
                      | REFRESH CUBE <qualified-name> [ '(' <kv-list> ')' ]
                      | REFRESH VIEW <refresh-view-spec>
                      | CLUSTER <cluster-control-spec>
                      | CUBE <cube-control-spec> ;

<sweep-spec>         ::= DATABASE <qualified-name> [ '(' <kv-list> ')' ] ;
<backup-spec>        ::= DATABASE <qualified-name> TO <path> [ '(' <kv-list> ')' ] ;
<restore-spec>       ::= DATABASE <qualified-name> FROM <path> [ '(' <kv-list> ')' ] ;
<validate-spec>      ::= DATABASE <qualified-name> [ '(' <kv-list> ')' ] ;

<connect-spec>       ::= <identifier> [ AS <identifier> ] [ '(' <kv-list> ')' ] ;

<call-spec>          ::= <qualified-name> '(' [ <arg-list> ] ')' ;
<arg-list>           ::= <expr> { ',' <expr> } ;

<service-channel-spec>::= <identifier> ( START | STOP | STATUS ) [ '(' <kv-list> ')' ] ;
<refresh-view-spec>  ::= <qualified-name> [ '(' <kv-list> ')' ] ;

<cluster-control-spec>::= SET STATE <cluster-state> [ '(' <kv-list> ')' ]
                       | START [ '(' <kv-list> ')' ]
                       | STOP [ '(' <kv-list> ')' ] ;
<cluster-state>      ::= ONLINE | OFFLINE | DEGRADED | MAINTENANCE ;

<cube-control-spec>  ::= START [ '(' <kv-list> ')' ]
                      | STOP [ '(' <kv-list> ')' ]
                      | REBUILD [ '(' <kv-list> ')' ] ;
```

---

## 6) DCL (Security / Privileges)

```ebnf
<dcl-statement>      ::= GRANT <grant-spec>
                      | REVOKE <revoke-spec>
                      | REVOKE TOKEN <qualified-name> ;

<grant-spec>         ::= <grant-spec-core> TO <principal-list> [ WITH GRANT OPTION ] ;
<revoke-spec>        ::= [ GRANT OPTION FOR ] <grant-spec-core> FROM <principal-list> ;

<grant-spec-core>    ::= <relation-privilege-list> ON <relation-object>
                      | <index-privilege-list> ON <index-object>
                      | <routine-privilege-list> ON <routine-object>
                      | <type-privilege-list> ON <type-object>
                      | <schema-privilege-list> ON SCHEMA <qualified-name>
                      | <database-privilege-list> ON DATABASE <qualified-name>
                      | <security-privilege-list> ON <security-object>
                      | <federation-privilege-list> ON <federation-object>
                      | <replication-privilege-list> ON <replication-object>
                      | <operations-privilege-list> ON <operations-object>
                      | <platform-admin-privilege-list> ON <platform-admin-object>
                      | <storage-privilege-list> ON <storage-object> ;

<principal-list>     ::= <principal> { ',' <principal> } ;
<principal>          ::= ROLE <qualified-name>
                      | USER <qualified-name>
                      | GROUP <qualified-name>
                      | <qualified-name>
                      | PUBLIC ;

<relation-privilege-list> ::= <relation-privilege> { ',' <relation-privilege> } ;
<relation-privilege> ::= SELECT | INSERT | UPDATE | DELETE | REFERENCES | TRIGGER | ALTER | DROP | OWNERSHIP | ALL ;
<relation-object>    ::= TABLE <qualified-name>
                      | VIEW <qualified-name>
                      | CDC TABLE <qualified-name>
                      | FOREIGN TABLE <qualified-name> ;

<index-privilege-list> ::= <index-privilege> { ',' <index-privilege> } ;
<index-privilege>    ::= USAGE | ALTER | DROP | OWNERSHIP | ALL ;
<index-object>       ::= INDEX <index-ref> ;

<routine-privilege-list> ::= <routine-privilege> { ',' <routine-privilege> } ;
<routine-privilege>  ::= EXECUTE | ALTER | DROP | OWNERSHIP | ALL ;
<routine-object>     ::= FUNCTION <qualified-name>
                      | PROCEDURE <qualified-name>
                      | TRIGGER <trigger-ref>
                      | PACKAGE <qualified-name>
                      | EXCEPTION <qualified-name>
                      | UDR <qualified-name> ;

<type-privilege-list> ::= <type-privilege> { ',' <type-privilege> } ;
<type-privilege>     ::= USAGE | ALTER | DROP | OWNERSHIP | ALL ;
<type-object>        ::= DOMAIN <global-domain-name>
                      | TYPE <qualified-name> ;

<schema-privilege-list> ::= <schema-privilege> { ',' <schema-privilege> } ;
<schema-privilege>   ::= USAGE | CREATE | ALTER | DROP | OWNERSHIP | ALL ;

<database-privilege-list> ::= <database-privilege> { ',' <database-privilege> } ;
<database-privilege> ::= CONNECT | CREATE | ALTER | DROP | OWNERSHIP | ALL ;

<security-privilege-list> ::= <security-privilege> { ',' <security-privilege> } ;
<security-privilege> ::= MANAGE | ALTER | DROP | OWNERSHIP | ALL ;
<security-object>    ::= POLICY <qualified-name> ON <qualified-name>
                      | TOKEN <qualified-name>
                      | QUOTA PROFILE <qualified-name>
                      | CONNECTION RULE <qualified-name> ;

<federation-privilege-list> ::= <federation-privilege> { ',' <federation-privilege> } ;
<federation-privilege> ::= USAGE | MANAGE | ALTER | DROP | OWNERSHIP | ALL ;
<federation-object>  ::= SERVER <qualified-name>
                      | FOREIGN DATA WRAPPER <qualified-name>
                      | FOREIGN TABLE <qualified-name>
                      | USER MAPPING FOR <principal> SERVER <qualified-name>
                      | SYNONYM <qualified-name>
                      | PUBLIC SYNONYM <qualified-name> ;

<replication-privilege-list> ::= <replication-privilege> { ',' <replication-privilege> } ;
<replication-privilege> ::= PUBLISH | SUBSCRIBE | REFRESH | RESYNC | MANAGE | ALTER | DROP | OWNERSHIP | ALL ;
<replication-object> ::= PUBLICATION <qualified-name>
                      | SUBSCRIPTION <qualified-name>
                      | REPLICATION CHANNEL <qualified-name>
                      | EVENT <qualified-name> ;

<operations-privilege-list> ::= <operations-privilege> { ',' <operations-privilege> } ;
<operations-privilege> ::= EXECUTE | MANAGE | MONITOR | ALTER | DROP | OWNERSHIP | ALL ;
<operations-object>  ::= JOB <qualified-name>
                      | SERVICE CHANNEL <qualified-name> ;

<platform-admin-privilege-list> ::= <platform-admin-privilege> { ',' <platform-admin-privilege> } ;
<platform-admin-privilege> ::= MANAGE | MONITOR | START | STOP | CONTROL | ALTER | DROP | OWNERSHIP | ALL ;
<platform-admin-object> ::= CLUSTER <qualified-name>
                         | CUBE <qualified-name> ;

<storage-privilege-list> ::= <storage-privilege> { ',' <storage-privilege> } ;
<storage-privilege>  ::= USAGE | MANAGE | ALTER | DROP | OWNERSHIP | ALL ;
<storage-object>     ::= TABLESPACE <qualified-name>
                      | FILESPACE <filespace-name> ;
```

---

## 7) DML (Core)

```ebnf
<dml-statement>      ::= SELECT <select-spec>
                      | INSERT <insert-spec>
                      | UPDATE <update-spec>
                      | DELETE <delete-spec>
                      | MERGE <merge-spec>
                      | VALUES <values-spec>
                      | WITH <with-spec> <dml-statement> ;

(* Skeletons only; detailed query grammar lives in the SELECT subsystem. *)
<select-spec>        ::= <query-expression> ;
<insert-spec>        ::= INTO <qualified-name> [ '(' <ident-list> ')' ] ( <query-expression> | VALUES <values-rows> ) ;
<update-spec>        ::= <qualified-name> SET <set-clause-list> [ WHERE <expr> ] ;
<delete-spec>        ::= FROM <qualified-name> [ WHERE <expr> ] ;
<merge-spec>         ::= INTO <qualified-name> USING <query-expression> ON <expr> <merge-actions> ;

<values-spec>        ::= <values-rows> ;
<values-rows>        ::= <values-row> { ',' <values-row> } ;
<values-row>         ::= '(' [ <expr-list> ] ')' ;

<set-clause-list>    ::= <set-clause> { ',' <set-clause> } ;
<set-clause>         ::= <identifier> '=' <expr> ;

<with-spec>          ::= <with-item> { ',' <with-item> } ;
<with-item>          ::= <identifier> [ '(' <ident-list> ')' ] AS '(' <query-expression> ')' ;

<ident-list>         ::= <identifier> { ',' <identifier> } ;
<expr-list>          ::= <expr> { ',' <expr> } ;

<query-expression>   ::= <expr> ;  (* placeholder for full query subsystem *)

(* Index identity is scoped to a table unless stated otherwise. *)

<index-name> ::= <identifier> | <qualified-name> ;

<index-ref>
  ::= <index-name> ON <qualified-name> ;              (* required parent-qualified reference *)
```

---

## 8) DDL – Core CREATE

```ebnf
<ddl-statement>      ::= <create-statement>
                      | <alter-statement>
                      | <drop-statement> ;

<create-statement>   ::= CREATE <create-body> ;

<create-body>        ::= DATABASE <create-database-spec>
                      | DATABASE CONNECTION <create-database-connection-spec>
                      | SCHEMA <create-schema-spec>
                      | TABLE <create-table-spec>
                      | CDC TABLE <create-cdc-table-spec>
                      | VIEW <create-view-spec>
                      | INDEX <create-index-spec>
                      | SEQUENCE <create-sequence-spec>
                      | DOMAIN <create-domain-spec>
                      | TYPE <create-type-spec>
                      | FUNCTION <create-function-spec>
                      | PROCEDURE <create-procedure-spec>
                      | TRIGGER <create-trigger-spec>
                      | PACKAGE <create-package-spec>
                      | EXCEPTION <create-exception-spec>
                      | ROLE <create-role-spec>
                      | GROUP <create-group-spec>
                      | USER <create-user-spec>
                      | USER MAPPING <create-user-mapping-spec>
                      | POLICY <create-policy-spec>
                      | SERVER <create-server-spec>
                      | FOREIGN TABLE <create-foreign-table-spec>
                      | FOREIGN DATA WRAPPER <create-fdw-spec>
                      | SYNONYM <create-synonym-spec>
                      | PUBLIC SYNONYM <create-public-synonym-spec>
                      | UDR <create-udr-spec>
                      | JOB <create-job-spec>
                      | CONNECTION RULE <create-connection-rule-spec>
                      | TOKEN <create-token-spec>
                      | QUOTA PROFILE <create-quota-profile-spec>
                      | EXTENSION <create-extension-spec>
                      | REPLICATION CHANNEL <create-repl-channel-spec>
                      | PUBLICATION <create-publication-spec>
                      | SUBSCRIPTION <create-subscription-spec>
                      | EVENT <create-event-spec>
                      | CLUSTER <create-cluster-spec>
                      | CUBE <create-cube-spec>
                      | TABLESPACE <create-tablespace-spec> ;
```

---

## 9) DDL – ALTER

```ebnf
<alter-statement>    ::= ALTER <alter-body> ;

<alter-body>         ::= DATABASE <alter-database-spec>
                      | DATABASE CONNECTION <alter-database-connection-spec>
                      | SCHEMA <alter-schema-spec>
                      | TABLE <alter-table-spec>
                      | CDC TABLE <alter-cdc-table-spec>
                      | VIEW <alter-view-spec>
                      | INDEX <alter-index-spec>
                      | SEQUENCE <alter-sequence-spec>
                      | DOMAIN <alter-domain-spec>
                      | TYPE <alter-type-spec>
                      | FUNCTION <alter-function-spec>
                      | PROCEDURE <alter-procedure-spec>
                      | TRIGGER <alter-trigger-spec>
                      | PACKAGE <alter-package-spec>
                      | ROLE <alter-role-spec>
                      | GROUP <alter-group-spec>
                      | USER <alter-user-spec>
                      | USER MAPPING <alter-user-mapping-spec>
                      | POLICY <alter-policy-spec>
                      | SERVER <alter-server-spec>
                      | FOREIGN TABLE <alter-foreign-table-spec>
                      | SYNONYM <alter-synonym-spec>
                      | PUBLIC SYNONYM <alter-public-synonym-spec>
                      | JOB <alter-job-spec>
                      | CONNECTION RULE <alter-connection-rule-spec>
                      | TOKEN <alter-token-spec>
                      | QUOTA PROFILE <alter-quota-profile-spec>
                      | EXTENSION <alter-extension-spec>
                      | REPLICATION CHANNEL <alter-repl-channel-spec>
                      | PUBLICATION <alter-publication-spec>
                      | SUBSCRIPTION <alter-subscription-spec>
                      | EVENT <alter-event-spec>
                      | CLUSTER <alter-cluster-spec>
                      | CUBE <alter-cube-spec>
                      | TABLESPACE <alter-tablespace-spec>
                      | SYSTEM <alter-system-spec>
                      | COMMENT <alter-comment-spec> ;
```

---

## 10) DDL – DROP

```ebnf
<drop-statement>     ::= DROP <drop-body> ;

<drop-body>          ::= DATABASE <drop-database-spec>
                      | DATABASE CONNECTION <drop-database-connection-spec>
                      | SCHEMA <drop-schema-spec>
                      | TABLE <drop-table-spec>
                      | CDC TABLE <drop-cdc-table-spec>
                      | VIEW <drop-view-spec>
                      | INDEX <drop-index-spec>
                      | SEQUENCE <drop-sequence-spec>
                      | DOMAIN <drop-domain-spec>
                      | TYPE <drop-type-spec>
                      | FUNCTION <drop-function-spec>
                      | PROCEDURE <drop-procedure-spec>
                      | TRIGGER <drop-trigger-spec>
                      | PACKAGE <drop-package-spec>
                      | EXCEPTION <drop-exception-spec>
                      | ROLE <drop-role-spec>
                      | GROUP <drop-group-spec>
                      | USER <drop-user-spec>
                      | USER MAPPING <drop-user-mapping-spec>
                      | POLICY <drop-policy-spec>
                      | SERVER <drop-server-spec>
                      | FOREIGN TABLE <drop-foreign-table-spec>
                      | UDR <drop-udr-spec>
                      | SYNONYM <drop-synonym-spec>
                      | PUBLIC SYNONYM <drop-public-synonym-spec>
                      | JOB <drop-job-spec>
                      | CONNECTION RULE <drop-connection-rule-spec>
                      | TOKEN <drop-token-spec>
                      | QUOTA PROFILE <drop-quota-profile-spec>
                      | EXTENSION <drop-extension-spec>
                      | REPLICATION CHANNEL <drop-repl-channel-spec>
                      | PUBLICATION <drop-publication-spec>
                      | SUBSCRIPTION <drop-subscription-spec>
                      | EVENT <drop-event-spec>
                      | CLUSTER <drop-cluster-spec>
                      | CUBE <drop-cube-spec>
                      | TABLESPACE <drop-tablespace-spec>
                      | COMMENT <drop-comment-spec> ;
```

---

## 11) Canonicalized Sub-Specs (selected)

### 11.1 Qualified Names

```ebnf
(* Canonical name forms are defined in section 2 and reused throughout this draft. *)
```

### 11.2 Index ALTER action family (canonicalized RELOCATE)

```ebnf
<index-option-list>  ::= '(' <kv-list> ')' ;
<index-reset-list>   ::= '(' <ident-list> ')' ;
```

### 11.3 DOMAIN option keys (canonicalized spelling)

Canonicalization:

- Use **spaced** form `REQUIRE PRIVILEGE` (underscore variant is accepted as alias and normalizes).

```ebnf
<domain-options>     ::= WITH '(' <domain-option> { ',' <domain-option> } ')' ;
<domain-option>      ::= DIALECT '=' <dialect-value>
                      | COMPAT '=' <compat-value>
                      | REQUIRE PRIVILEGE '=' <privilege-expr>
                      | REQUIRE_PRIVILEGE '=' <privilege-expr> ;  (* alias *)

<domain-reset-key-list> ::= WITH '(' <identifier> { ',' <identifier> } ')' ;
```

### 11.4 TYPE option keys (spacing normalized)

```ebnf
<type-options>       ::= WITH DIALECT '(' <dialect-value> ')'
                      | WITH COMPAT '(' <compat-value> ')'
                      | WITH '(' <kv-list> ')' ;
```

### 11.5 PUBLICATION / SUBSCRIPTION / EVENT (filled lifecycle)

```ebnf
<create-publication-spec>
                     ::= <qualified-name> [ FOR <publication-scope> ] [ WITH '(' <kv-list> ')' ] ;
<publication-scope>  ::= ALL TABLES | TABLE <qualified-name> { ',' <qualified-name> } ;

<alter-publication-spec>
                     ::= <qualified-name>
                         ( ADD TABLE <qualified-name> { ',' <qualified-name> }
                         | DROP TABLE <qualified-name> { ',' <qualified-name> }
                         | SET '(' <kv-list> ')'
                         | RENAME TO <identifier> ) ;

<drop-publication-spec>
                     ::= <qualified-name> [ IF EXISTS ] [ CASCADE | RESTRICT ] ;

<create-subscription-spec>
                     ::= <qualified-name> CONNECTION <string>
                         PUBLICATION <qualified-name> { ',' <qualified-name> }
                         [ WITH '(' <kv-list> ')' ] ;

<alter-subscription-spec>
                     ::= <qualified-name>
                         ( SET '(' <kv-list> ')'
                         | CONNECTION <string>
                         | REFRESH PUBLICATION
                         | ENABLE
                         | DISABLE
                         | RENAME TO <identifier> ) ;

<drop-subscription-spec>
                     ::= <qualified-name> [ IF EXISTS ] [ CASCADE | RESTRICT ] ;

<create-event-spec>  ::= <qualified-name>
                         ( ON SCHEDULE <schedule-expr> | AT <timestamp-expr> )
                         DO <event-body>
                         [ WITH '(' <kv-list> ')' ] ;

<alter-event-spec>   ::= <qualified-name>
                         ( ENABLE | DISABLE
                         | SET '(' <kv-list> ')'
                         | ON SCHEDULE <schedule-expr>
                         | AT <timestamp-expr>
                         | DO <event-body>
                         | RENAME TO <identifier> ) ;

<drop-event-spec>    ::= <qualified-name> [ IF EXISTS ] [ CASCADE | RESTRICT ] ;

<schedule-expr>      ::= <string> | <expr> ;
<timestamp-expr>     ::= <string> | <expr> ;
<event-body>         ::= <command> | <string> ;
```

---

## 12) Expression Subsystem (practical surface)

```ebnf
<expr>               ::= <literal>
                      | <qualified-name>
                      | '(' <expr> ')'
                      | <function-call>
                      | <cast-expr>
                      | <extract-expr>
                      | <case-expr>
                      | <expr> <binary-op> <expr>
                      | <unary-op> <expr> ;

<literal>            ::= NULL | <boolean> | <integer> | <decimal> | <string> ;

<function-call>      ::= <qualified-name> '(' [ <expr-list> ] ')' ;

<cast-expr>          ::= CAST '(' <expr> AS <type-ref> ')' ;

(* Canonical extraction form (preferred over getX()/f_getX() legacy): *)
<extract-expr>       ::= EXTRACT '(' <extract-field> FROM <expr> ')' ;
<extract-field>      ::= YEAR | MONTH | DAY | HOUR | MINUTE | SECOND | EPOCH | TIMEZONE ;

<case-expr>          ::= CASE { WHEN <expr> THEN <expr> } [ ELSE <expr> ] END ;

<binary-op>          ::= '+' | '-' | '*' | '/' | '%' | '=' | '<>' | '!=' | '<' | '<=' | '>' | '>=' | AND | OR | LIKE | IN ;
<unary-op>           ::= '+' | '-' | NOT ;

<type-ref>           ::= <qualified-name> [ '(' <integer> [ ',' <integer> ] ')' ] ;
```

---

## 13) Legacy Alias Policy

Legacy alias syntax is excluded from canonical grammar. Unsupported legacy spellings are treated as syntax errors.

---

## 14) Fully Enumerated Sub-Specs (from uploaded dialect docs)

This section replaces prior `{ <any-token> }` placeholders with explicit, enumerated clause families drawn from the uploaded V3 dialect documents. Where the documents intentionally defer details (e.g., full SELECT grammar, full table element grammar), the EBNF uses *named* nonterminals (e.g., `<table-element>`) rather than opaque token-catchalls.

---

### 14.1 DATABASE

```ebnf
<create-database-spec> ::= [ IF NOT EXISTS ] <qualified-name> [ EMULATED ] [ <db-options> ] ;
<db-options>           ::= WITH '(' <kv-list> ')' ;

<alter-database-spec>  ::= <qualified-name> <alter-database-action> ;
<alter-database-action>::= RENAME TO <identifier>
                        | OWNER TO <qualified-name>
                        | ALIAS ADD <identifier>
                        | ALIAS DROP <identifier> ;

<drop-database-spec>   ::= [ IF EXISTS ] <qualified-name> [ FORCE | CASCADE | RESTRICT ] ;
```

---

### 14.2 SCHEMA

```ebnf
<create-schema-spec> ::= [ IF NOT EXISTS ] <qualified-name> [ AUTHORIZATION <qualified-name> ] ;

<alter-schema-spec>  ::= <qualified-name>
                         ( RENAME TO <identifier>
                         | OWNER TO <qualified-name>
                         | SET PATH <path> ) ;

<drop-schema-spec>   ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.3 TABLE (core surface)

From core DDL:

- `CREATE TABLE [GLOBAL|TEMP|TEMPORARY] [UNLOGGED] [IF NOT EXISTS] <table_name>`
- `CREATE TABLE <table_name> AS <query_expr>`
- `ALTER TABLE <table_name> ...` supports enumerated operations (ADD/DROP/ALTER COLUMN/RENAME/SET TABLESPACE/SET SCHEMA/INHERIT/NO INHERIT/ATTACH PARTITION/DETACH PARTITION/ENABLE|DISABLE TRIGGER/ENABLE|DISABLE RLS/FORCE|NO FORCE RLS/VALIDATE CONSTRAINT)
- `ALTER TABLE ... ALTER COLUMN ...` supports enumerated column operations.

```ebnf
(* ------------------------------------------------------------------------- *)
(* 14.3 TABLE (core surface, reconciled with v3 AST)                          *)
(* ------------------------------------------------------------------------- *)

<create-table-spec> ::= [ <create-table-prefix> ] [ IF NOT EXISTS ] <qualified-name>
                       ( <table-definition> | <table-as-spec> ) ;

<create-table-prefix>
                    ::= ( GLOBAL | TEMP | TEMPORARY ) [ <temp-on-commit-clause> ]
                     | UNLOGGED ;

<temp-on-commit-clause>
                    ::= ON COMMIT ( DELETE ROWS | PRESERVE ROWS | DROP ) ;

<table-as-spec>     ::= AS <query-expression> [ WITH ( NO )? DATA ] ;

<table-definition>  ::= '(' <table-element> { ',' <table-element> } ')'
                        [ <inherits-clause> ]
                        [ <partitioning-clause> ]
                        [ <create-table-like-clause> ]
                        [ <table-storage-clause> ]
                        [ <table-options-clause> ] ;

<table-element>     ::= <column-def>
                     | <table-constraint> ;

<column-def>        ::= <identifie> <type-ref>
                        [ <column-charset-collate> ]
                        { <column-constraint-item> } ;

<column-charset-collate>
                    ::= [ CHARACTER SET <identifier> ] [ COLLATE <identifier> ] ;

<column-constraint-item>
                    ::= NOT NULL
                     | NULL
                     | DEFAULT <expr>
                     | UNIQUE
                     | PRIMARY KEY
                     | CHECK '(' <expr> ')'
                     | REFERENCES <qualified-name> [ '(' <ident-list> ')' ] [ <fk-actions> ] [ <deferrability-clause> ]
                     | GENERATED ALWAYS AS '(' <expr> ')' ( STORED | VIRTUAL )
                     | GENERATED ALWAYS AS IDENTITY
                     | COLLATE <identifier> ;

<fk-actions>        ::= { ( ON DELETE <fk-action> ) | ( ON UPDATE <fk-action> ) } ;
<fk-action>         ::= NO ACTION | RESTRICT | CASCADE | SET NULL | SET DEFAULT ;

<deferrability-clause>
                    ::= [ DEFERRABLE | NOT DEFERRABLE ]
                        [ INITIALLY DEFERRED | INITIALLY IMMEDIATE ] ;

<table-constraint>  ::= [ CONSTRAINT <identifier> ]
                        ( PRIMARY KEY '(' <ident-list> ')' [ <deferrability-clause> ]
                        | UNIQUE '(' <ident-list> ')' [ <deferrability-clause> ]
                        | CHECK '(' <expr> ')' [ <deferrability-clause> ]
                        | FOREIGN KEY '(' <ident-list> ')' REFERENCES <qualified-name> [ '(' <ident-list> ')' ] [ <fk-actions> ] [ <deferrability-clause> ]
                        | EXCLUDE [ USING <identifier> ] '(' <exclude-element> { ',' <exclude-element> } ')' [ WHERE '(' <expr> ')' ] ) ;

<exclude-element>   ::= ( <identifier> | '(' <expr> ')' ) WITH <identifier> ;

<inherits-clause>   ::= INHERITS '(' <qualified-name> { ',' <qualified-name> } ')' ;

<partitioning-clause>
                    ::= PARTITION BY <partition-method> '(' <ident-list> ')' ;
<partition-method>  ::= RANGE | LIST | HASH ;

<create-table-like-clause>
                    ::= LIKE <qualified-name> ;

<table-storage-clause>
                    ::= TABLESPACE <path> ;

<table-options-clause>
                    ::= WITH '(' <kv-list> ')' ;

<alter-table-spec>  ::= [ IF EXISTS ] [ ONLY ] <qualified-name> <alter-table-action> ;

<alter-table-action> ::= <alter-table-add>
                      | <alter-table-drop>
                      | <alter-table-alter-column>
                      | <alter-table-rename>
                      | SET TABLESPACE <path>
                      | SET SCHEMA <qualified-name>
                      | ( ENABLE | DISABLE ) ROW LEVEL SECURITY
                      | ( FORCE | NO FORCE ) ROW LEVEL SECURITY
                      | ATTACH PARTITION <qualified-name> [ FOR VALUES <partition-bound-spec> ]
                      | DETACH PARTITION <qualified-name>
                      | ( ENABLE | DISABLE ) TRIGGER ( ALL | <identifier> )
                      | INHERIT <qualified-name>
                      | NO INHERIT <qualified-name>
                      | VALIDATE CONSTRAINT <identifier> ;

<alter-table-add>    ::= ADD ( COLUMN <column-def> | <table-constraint> ) ;



<alter-table-drop>   ::= DROP ( COLUMN <identifier> [ RESTRICT | CASCADE ]
                             | CONSTRAINT <identifier> [ RESTRICT | CASCADE ] ) ;

<alter-table-alter-column>
                    ::= ALTER COLUMN <identifier> <alter-column-action> ;

<alter-column-action>::= ( SET DATA TYPE | TYPE ) <type-ref>
                      | SET DEFAULT <expr>
                      | DROP DEFAULT
                      | SET NOT NULL
                      | DROP NOT NULL
                      | SET STATISTICS <integer>
                      | SET STORAGE ( PLAIN | EXTERNAL | EXTENDED | MAIN )
                      | SET POSITION <integer> ;

<alter-table-rename> ::= RENAME TO <identifier>
                      | RENAME COLUMN <identifier> TO <identifier>
                      | RENAME CONSTRAINT <identifier> TO <identifier> ;

<partition-bound-spec> ::= <string> | <expr> ;

<drop-table-spec>    ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.4 VIEW (materialization as a view option)

From core DDL:

- `CREATE VIEW [OR REPLACE|OR ALTER] [IF NOT EXISTS] <view_name>` with explicit view options.
- Materialization is a view option, not a first-class object family.

```ebnf
(* ------------------------------------------------------------------------- *)
(* 14.4 VIEW (materialization stays in view options)                          *)
(* ------------------------------------------------------------------------- *)

<create-view-spec> ::= [ <view-replace-mode> ] [ TEMPORARY ] [ IF NOT EXISTS ] <qualified-name>
                      [ '(' <ident-list> ')' ]
                      [ <view-option-clause> ]
                      AS <query-expression>
                      [ <view-check-option> ]
                      [ <view-storage-clause> ]
                      [ <view-with-data-clause> ] ;

<view-replace-mode> ::= OR REPLACE | OR ALTER ;

<view-option-clause>
                    ::= WITH '(' <view-option-assignment> { ',' <view-option-assignment> } ')' ;

<view-option-assignment>
                    ::= MATERIALIZED '=' <boolean>
                     | <identifier> '=' <expr> ;

<view-check-option> ::= WITH ( LOCAL | CASCADED )? CHECK OPTION ;

<view-storage-clause>
                    ::= TABLESPACE <path> ;

<view-with-data-clause>
                    ::= WITH DATA | WITH NO DATA ;

<alter-view-spec>  ::= [ IF EXISTS ] <qualified-name>
                       ( RENAME TO <identifier>
                       | SET SCHEMA <qualified-name>
                       | SET '(' <view-option-assignment> { ',' <view-option-assignment> } ')'
                       | RESET '(' <ident-list> ')' ) ;

<drop-view-spec>   ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.5 SEQUENCE

From core DDL + scalar types notes:

- `CREATE SEQUENCE [IF NOT EXISTS] <sequence_name>`
- DROP SEQUENCE exists
- ALTER SEQUENCE is intended to support option families (dedicated AST/emitter exists) even if older parser routing used rename/move.

```ebnf
<create-sequence-spec> ::= [ IF NOT EXISTS ] <qualified-name> [ <sequence-options> ] ;

<sequence-options>     ::= <sequence-option> { <sequence-option> } ;

<sequence-option>      ::= AS <type-ref>
                        | START WITH <expr>
                        | INCREMENT BY <expr>
                        | MINVALUE <expr>
                        | NO MINVALUE
                        | MAXVALUE <expr>
                        | NO MAXVALUE
                        | CACHE <expr>
                        | CYCLE
                        | NO CYCLE
                        | OWNED BY <qualified-name> ;

<alter-sequence-spec>  ::= [ IF EXISTS ] <qualified-name>
                           ( RENAME TO <identifier>
                           | SET SCHEMA <qualified-name>
                           | RESTART [ WITH <expr> ]
                           | SET <sequence-options>
                           | OWNED BY <qualified-name> ) ;

<drop-sequence-spec>   ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE ] ;
```

---

### 14.6 TABLESPACE

From core DDL:

- `CREATE TABLESPACE <name> LOCATION <string> [AUTOEXTEND ON|OFF] [AUTOEXTEND_SIZE <int>] [MAXSIZE <UNLIMITED|int>] [PREALLOC <int>]`

```ebnf
<create-tablespace-spec> ::= <qualified-name> LOCATION <string>
                             [ AUTOEXTEND ( ON | OFF ) ]
                             [ AUTOEXTEND_SIZE <integer> ]
                             [ MAXSIZE ( UNLIMITED | <integer> ) ]
                             [ PREALLOC <integer> ] ;

<alter-tablespace-spec>  ::= <qualified-name>
                             ( RENAME TO <identifier>
                             | SET '(' <kv-list> ')'
                             | RESET '(' <ident-list> ')' ) ;

<drop-tablespace-spec>   ::= [ IF EXISTS ] <qualified-name> [ CASCADE | RESTRICT ] ;
```

---

### 14.7 DOMAIN (from Scalar Types section)

```ebnf
<create-domain-spec> ::= <global-domain-name> [ IF NOT EXISTS ] [ AS ] <domain-body>
                         [ INHERITS '(' <global-domain-name> ')' ]
                         { <domain-constraint-or-modifier> }
                         { <domain-with-block> } ;

<domain-body>        ::= <type-ref>
                      | RECORD '(' <domain-record-field> { ',' <domain-record-field> } ')'
                      | ENUM '(' <domain-enum-value> { ',' <domain-enum-value> } ')'
                      | SET OF <type-ref>
                      | VARIANT '(' <type-ref> { ',' <type-ref> } ')'
                      | RANGE OF <type-ref> ;

<domain-record-field> ::= <identifier> <type-ref>
                          [ <column-nullability> ]
                          [ DEFAULT <expr> ] ;

<domain-enum-value>  ::= <string> [ '=' <integer> ] ;

<domain-constraint-or-modifier>
                     ::= [ CONSTRAINT <identifier> ]
                         ( NOT NULL
                         | NULL
                         | CHECK '(' <expr> ')'
                         | DEFAULT <expr> )
                      | NOT NULL
                      | CHECK '(' <expr> ')'
                      | DEFAULT <expr>
                      | COLLATE <identifier> ;

<domain-with-block>  ::= WITH DIALECT '(' <dialect-value> ')'
                      | WITH COMPAT '(' <compat-value> ')'
                      | WITH INTEGRITY '(' <domain-integrity-kv> { ',' <domain-integrity-kv> } ')'
                      | WITH SECURITY '(' <domain-security-kv> { ',' <domain-security-kv> } ')'
                      | WITH VALIDATION '(' <domain-validation-kv> { ',' <domain-validation-kv> } ')'
                      | WITH QUALITY '(' <domain-quality-kv> { ',' <domain-quality-kv> } ')'
                      | WITH OPTIONS '(' <domain-options-kv> { ',' <domain-options-kv> } ')' ;

<domain-integrity-kv>::= UNIQUENESS '=' <boolean>
                      | NORMALIZATION_FUNCTION '=' <identifier-or-string>
                      | NORMALIZATION '=' ( <identifier-or-string> | NONE ) ;

<domain-security-kv> ::= MASKING '=' <identifier-or-string>
                      | MASK_PATTERN '=' <identifier-or-string>
                      | ENCRYPTION '=' <identifier-or-string>
                      | AUDIT_ACCESS '=' <boolean>
                      | REQUIRE PRIVILEGE '=' <identifier-or-string>
                      | REQUIRE_PRIVILEGE '=' <identifier-or-string> ; (* alias accepted *)

<domain-validation-kv>::= FUNCTION '=' <identifier-or-string>
                       | ERROR_MESSAGE '=' <identifier-or-string> ;

<domain-quality-kv>  ::= PARSE_FUNCTION '=' <identifier-or-string>
                      | STANDARDIZE_FUNCTION '=' <identifier-or-string>
                      | ENRICH_FUNCTION '=' <identifier-or-string> ;

<domain-options-kv>  ::= WRAP '=' <boolean>
                      | <identifier> '=' <expr> ;

<identifier-or-string> ::= <identifier> | <string> ;

<alter-domain-spec>  ::= <global-domain-name>
                         ( SET DEFAULT <expr>
                         | DROP DEFAULT
                         | SET COMPAT <compat-value>
                         | DROP COMPAT
                         | ADD CHECK '(' <expr> ')'
                         | DROP CONSTRAINT <identifier>
                         | RENAME TO <identifier>
                         | SET NOT NULL
                         | DROP NOT NULL
                         | DROP CHECK <identifier>
                         | SET OPTIONS '(' <kv-list> ')'
                         | RESET OPTIONS '(' <ident-list> ')' ) ;

<drop-domain-spec>   ::= [ IF EXISTS ] <global-domain-name> { ',' <global-domain-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.8 TYPE (from Scalar Types section)

```ebnf
<create-type-spec>   ::= [ IF NOT EXISTS ] <qualified-name> [ AS ] <type-body> { <type-modifier> } ;

<type-body>          ::= ENUM '(' <domain-enum-value> { ',' <domain-enum-value> } ')'
                      | RECORD '(' <domain-record-field> { ',' <domain-record-field> } ')'
                      | '(' <domain-record-field> { ',' <domain-record-field> } ')'
                      | RANGE '(' <range-type-kv> { ',' <range-type-kv> } ')'
                      | BASE '(' <base-type-kv> { ',' <base-type-kv> } ')'
                      | SHELL ;

<type-modifier>      ::= WITH DIALECT '(' <dialect-value> ')'
                      | WITH COMPAT '(' <compat-value> ')'
                      | COMMENT <string> ;

<range-type-kv>      ::= SUBTYPE '=' <type-ref>
                      | SUBTYPE_COLLATION '=' <identifier>
                      | SUBTYPE_OPCLASS '=' <identifier>
                      | CANONICAL '=' <identifier>
                      | SUBTYPE_DIFF '=' <identifier>
                      | MULTIRANGE '=' ( <boolean> | <integer> ) ;

<base-type-kv>       ::= STORAGE '=' <type-ref>
                      | INPUT '=' <identifier>
                      | OUTPUT '=' <identifier>
                      | RECEIVE '=' <identifier>
                      | SEND '=' <identifier>
                      | TYPMOD_IN '=' <identifier>
                      | TYPMOD_OUT '=' <identifier>
                      | ANALYZE '=' <identifier>
                      | ALIGNMENT '=' ( CHAR | SHORT | INT | DOUBLE )
                      | STORAGE_MODE '=' ( PLAIN | EXTERNAL | EXTENDED | MAIN )
                      | CATEGORY '=' <string>
                      | PREFERRED '=' ( <boolean> | <integer> ) ;

<alter-type-spec>    ::= <qualified-name>
                         ( RENAME TO <identifier>
                         | SET SCHEMA <qualified-name>
                         | RENAME VALUE <string> TO <string>
                         | SET '(' <type-set-kv> { ',' <type-set-kv> } ')'
                         | ADD VALUE <string> [ ( BEFORE | AFTER ) <string> ]
                         | FINALIZE ) ;

<type-set-kv>        ::= <range-type-kv> | <base-type-kv> ;

<drop-type-spec>     ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.9 INDEX (fully expanded from Index Framework section)

```ebnf
 (* ===================================================================== *)
(* 14.9 INDEX (ScratchBird v3 canonical + audit edition)                  *)
(*                                                                        *)
(* Key design decision baked in:                                          *)
(* - Index names are NOT globally unique; they are scoped to a table.     *)
(* - Therefore ALTER/DROP must support a table-qualified index reference. *)
(* - Canonical disambiguator:  <index-name> ON <table-path>               *)
(* - Bare <index-name> is accepted only if resolver can prove uniqueness. *)
(*                                                                        *)
(* Legend:                                                                *)
(*   [impl] = present in v3 AST shape today                               *)
(*   [doc]  = present in provided dialect docs (index framework)          *)
(*   [spec] = required/planned for final spec completeness                *)
(* ===================================================================== *)

(* --------------------------------------------------------------------- *)
(* Shared name forms                                                     *)
(* --------------------------------------------------------------------- *)

(* Reuse canonical <qualified-name> and <index-name> definitions from earlier sections. *)

<index-ref>
  ::= <index-name> ON <qualified-name> ;         (* required parent-qualified reference *)

(* --------------------------------------------------------------------- *)
(* CREATE INDEX                                                          *)
(* --------------------------------------------------------------------- *)

<create-index-spec>
  ::= CREATE
      [ UNIQUE ]                                (* [doc][impl] *)
      [ CONCURRENTLY ]                          (* [doc][impl] *)
      INDEX
      [ IF NOT EXISTS ]                         (* [impl][spec] AST supports; keep in spec *)
      [ <identifier> ]                          (* [doc][impl] index name optional *)
      ON <qualified-name>                       (* [doc][impl] table path *)
      [ USING <index-method-name> ]             (* [doc][impl] *)
      '(' <index-key> { ',' <index-key> } ')'
      [ <index-include-clause> ]                (* [doc][impl] *)
      [ WHERE <expr> ]                          (* [doc][impl] partial index *)
      [ TABLESPACE <path> ]                     (* [doc][impl] placement *)
      [ WITH '(' <index-option-assignment> { ',' <index-option-assignment> } ')' ] ;

<index-method-name>
  ::= BTREE | HASH | HNSW | FULLTEXT | GIN | GIST | BRIN | RTREE | SPGIST
   | BITMAP | COLUMNSTORE | LSM | IVF | ZONEMAP | ART | BLOOM
   | VECTOR_FLAT | VECTOR_BIN_FLAT | IVF_FLAT | BIN_IVF_FLAT | IVF_PQ
   | IVF_SQ8 | IVF_SQ8_HYBRID | RHNSW_PQ | RHNSW_SQ
   | ANNOY | NSG | DISKANN | SCANN | GPU_CAGRA
   | MINHASH_LSH | SPARSE_INVERTED | SPARSE_WAND
   | TRIE | INVERTED | STL_SORT | NGRAM
   | MONGODB_2D | MONGODB_2DSPHERE | MONGODB_2DSPHERE_BUCKET
   | MONGODB_GEO_HAYSTACK | MONGODB_WILDCARD | MONGODB_ENCRYPTED_RANGE
   | NEO4J_LOOKUP | NEO4J_TEXT | NEO4J_RANGE | NEO4J_POINT | NEO4J_VECTOR
   | CASSANDRA_SASI | CASSANDRA_SAI
   | REDIS_STRING | REDIS_HASH | REDIS_LIST | REDIS_SET | REDIS_ZSET
   | REDIS_STREAM | REDIS_BITMAP | REDIS_HLL | REDIS_GEO
   | <identifier> ;                             (* forward/custom *)

<index-key>
  ::= <index-key-target>
      [ <index-key-ordering> ]
      [ <index-key-nulls-order> ]
      [ <index-key-opclass> ]                   (* [impl][spec] opclass exists in AST shape *)
      [ <index-key-collation> ] ;               (* [spec] Option A: implement *)

<index-key-target>
  ::= <identifier>
   | '(' <expr> ')' ;

<index-key-ordering>    ::= ASC | DESC ;
<index-key-nulls-order> ::= NULLS FIRST | NULLS LAST ;

<index-key-opclass>     ::= OPCLASS <identifier> ;

(* Option A chosen: keep in spec as required surface; implement in AST/parser/emitter. *)
<index-key-collation>   ::= COLLATE <identifier> ;

<index-include-clause>  ::= INCLUDE '(' <ident-list> ')' ;

<index-option-assignment>
  ::= <identifier> '=' <index-option-value> ;

<index-option-value>
  ::= <string> | <decimal> | <integer> | <boolean> | <identifier> ;

(* Recommended to document known keys even if parsing remains generic. *)
<known-index-option-key>
  ::= BLOOM_FILTER
   | BLOOM_FPR ;

(* --------------------------------------------------------------------- *)
(* ALTER INDEX                                                           *)
(* --------------------------------------------------------------------- *)

<alter-index-spec>
  ::= <alter-index-defaults-spec>               (* [doc][impl] *)
   | <alter-index-instance-spec>                (* [doc][impl] *)
   | <alter-index-rename-move-spec>             (* [spec] lifecycle completeness; may compile to generic stmts *)
   | <validate-index-spec> ;                    (* [spec] surface exists in v3 parser interface *)

(* A) ALTER INDEX DEFAULTS FOR <method> ... *)
<alter-index-defaults-spec>
  ::= ALTER INDEX DEFAULTS FOR <index-method-name> <alter-index-defaults-action> ;

<alter-index-defaults-action>
  ::= SET '(' <index-option-assignment> { ',' <index-option-assignment> } ')'
   | RESET '(' <identifier> { ',' <identifier> } ')' ;

(* B) ALTER INDEX <index-ref> ... *)
<alter-index-instance-spec>
  ::= ALTER INDEX [ IF EXISTS ] <index-ref> <alter-index-action> ;

<alter-index-action>
  ::= <alter-index-availability>
   | <alter-index-set-options>
   | <alter-index-reset-options>
   | <alter-index-rebuild>
   | <alter-index-rebalance>
   | <alter-index-relocate>
   | <alter-index-light-scan>
   | <alter-index-diagnostic-scan> ;

<alter-index-availability>
  ::= ACTIVE | INACTIVE
   | ENABLE                                    (* [spec] alias → ACTIVE *)
   | DISABLE ;                                 (* [spec] alias → INACTIVE *)

<alter-index-set-options>
  ::= SET '(' <index-option-assignment> { ',' <index-option-assignment> } ')' ;

<alter-index-reset-options>
  ::= RESET '(' <identifier> { ',' <identifier> } ')' ;

<index-maintenance-mode> ::= ONLINE | OFFLINE ;

<alter-index-rebuild>
  ::= REBUILD [ <index-maintenance-mode> ] [ WITH '(' <kv-list> ')' ] ;

<alter-index-rebalance>
  ::= REBALANCE [ <index-maintenance-mode> ] [ WITH '(' <kv-list> ')' ] ;

(* Canonical relocation spelling (normalized): *)
<alter-index-relocate>
  ::= RELOCATE TO FILESPACE <filespace-name>
      [ <index-maintenance-mode> ]
      [ WITH '(' <kv-list> ')' ]
   | RELOCATE TO TABLESPACE <qualified-name>
      [ <index-maintenance-mode> ]
      [ WITH '(' <kv-list> ')' ] ;

<alter-index-light-scan>
  ::= LIGHT SCAN [ WITH '(' <kv-list> ')' ] ;

<alter-index-diagnostic-scan>
  ::= DIAGNOSTIC SCAN [ WITH '(' <kv-list> ')' ] ;

(* C) Rename/move (table-scoped index identity preserved via <index-ref>) *)
<alter-index-rename-move-spec>
  ::= ALTER INDEX [ IF EXISTS ] <index-ref> RENAME TO <identifier>
   | ALTER INDEX [ IF EXISTS ] <index-ref> SET SCHEMA <qualified-name> ;

(* D) Validate index *)
<validate-index-spec>
  ::= VALIDATE INDEX <index-ref> [ WITH '(' <kv-list> ')' ] ;

(* --------------------------------------------------------------------- *)
(* DROP INDEX                                                            *)
(* --------------------------------------------------------------------- *)

<drop-index-spec>
  ::= DROP INDEX
      [ CONCURRENTLY ]
      [ IF EXISTS ]
      <index-ref> { ',' <index-ref> }
      [ <drop-index-dependency-mode> ] ;

<drop-index-dependency-mode>
  ::= CASCADE
   | RESTRICT ;
```

---

### 14.10 POLICY

From core DDL:

- `CREATE POLICY <name> ON <table_name>`
- `ALTER POLICY <name> ON <table_name>` supports `TO` roles, `USING`, and `WITH CHECK`.
- `DROP POLICY <name> ON <table_name>`

```ebnf
<create-policy-spec> ::= <qualified-name> ON <qualified-name>
                         [ AS ( PERMISSIVE | RESTRICTIVE ) ]
                         [ FOR ( ALL | SELECT | INSERT | UPDATE | DELETE ) ]
                         [ TO <principal-list> ]
                         [ USING '(' <expr> ')' ]
                         [ WITH CHECK '(' <expr> ')' ] ;

<alter-policy-spec>  ::= <qualified-name> ON <qualified-name>
                         ( TO <principal-list>
                         | USING '(' <expr> ')'
                         | WITH CHECK '(' <expr> ')' ) ;

<drop-policy-spec>   ::= [ IF EXISTS ] <qualified-name> ON <qualified-name> ;
```

---

### 14.11 ROLE / USER / GROUP (canonical + compat)

Core DDL includes CREATE/DROP ROLE, and ALTER USER is explicitly noted as supporting inline password/superuser changes.

```ebnf
<create-role-spec>   ::= <qualified-name> [ WITH <role-options> ] ;
<role-options>       ::= <role-option> { <role-option> } ;
<role-option>        ::= LOGIN | NOLOGIN
                      | SUPERUSER | NOSUPERUSER
                      | CREATEDB | NOCREATEDB
                      | CREATEROLE | NOCREATEROLE
                      | INHERIT | NOINHERIT
                      | REPLICATION | NOREPLICATION
                      | BYPASSRLS | NOBYPASSRLS
                      | PASSWORD <string>
                      | VALID UNTIL <string>
                      | IN ROLE <principal-list>
                      | ROLE <principal-list>
                      | ADMIN <principal-list> ;

<alter-role-spec>    ::= [ IF EXISTS ] <qualified-name>
                         ( RENAME TO <identifier>
                         | SET '(' <kv-list> ')'
                         | RESET '(' <ident-list> ')'
                         | WITH <role-options> ) ;

<drop-role-spec>     ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;

(* Canonical: USER/GROUP are treated as ROLE variants; parser may accept dedicated forms as aliases. *)
<create-user-spec>   ::= <qualified-name> [ WITH ( PASSWORD <string> | SUPERUSER | NOSUPERUSER | <role-options> ) ] ;
<create-group-spec>  ::= <qualified-name> [ WITH <role-options> ] ;

<alter-user-spec>    ::= [ IF EXISTS ] <qualified-name>
                         ( RENAME TO <identifier>
                         | SET SCHEMA <qualified-name>
                         | WITH ( PASSWORD <string> | SUPERUSER | NOSUPERUSER | <role-options> ) ) ;

<drop-user-spec>     ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
<drop-group-spec>    ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.12 FUNCTION / PROCEDURE / TRIGGER / PACKAGE (rename-or-move family)

Core DDL notes these are handled by generic rename/move helpers for ALTER (IF EXISTS / RENAME TO / SET SCHEMA), plus CREATE/DROP are present.

```ebnf
<create-function-spec>  ::= <qualified-name> '(' [ <routine-param-list> ] ')' <routine-body> ;
<create-procedure-spec> ::= <qualified-name> '(' [ <routine-param-list> ] ')' <routine-body> ;

<routine-param-list>    ::= <routine-param> { ',' <routine-param> } ;
<routine-param>         ::= [ IN | OUT | INOUT ] <identifier> <type-ref> [ DEFAULT <expr> ] ;

<routine-body>          ::= ( LANGUAGE <identifier> )
                            [ AS <string> ]
                            [ WITH '(' <kv-list> ')' ] ;

<alter-function-spec>   ::= [ IF EXISTS ] <qualified-name>
                            ( RENAME TO <identifier> | SET SCHEMA <qualified-name> ) ;

<alter-procedure-spec>  ::= [ IF EXISTS ] <qualified-name>
                            ( RENAME TO <identifier> | SET SCHEMA <qualified-name> ) ;

<drop-function-spec>    ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;
<drop-procedure-spec>   ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;



---------
(* ===================================================================== *)
(* TRIGGER (ScratchBird v3 canonical + audit edition)                     *)
(*                                                                        *)
(* Design intent for audit/spec:                                          *)
(* - Trigger names are scoped to parent relation/database, not global.    *)
(* - For table/view triggers, parent relation must not contain duplicate   *)
(*   trigger names.                                                        *)
(* - Therefore require a table-qualified trigger reference for ALTER/DROP.*)
(* - Canonical disambiguator: <trigger-name> ON <table-path>              *)
(* - Database triggers use ON DATABASE and a separate event set.          *)
(* - Keep WHEN(...) as a planned surface (easy to miss).                  *)
(* ===================================================================== *)

(* --------------------------------------------------------------------- *)
(* Shared: trigger identity + scope                                       *)
(* --------------------------------------------------------------------- *)

<trigger-name> ::= <identifier> | <qualified-name> ;

<trigger-target>
  ::= ON <qualified-name>            (* table/view target *)
   | ON DATABASE ;                   (* database trigger *)

(* Canonical trigger reference for ALTER/DROP when duplicates can exist. *)
<trigger-ref>
  ::= <trigger-name> ON <qualified-name>                  (* required table-scoped ref *)
   | <trigger-name> ON DATABASE ;                         (* required db-scoped ref *)

(* --------------------------------------------------------------------- *)
(* CREATE TRIGGER                                                        *)
(* --------------------------------------------------------------------- *)

<create-trigger-spec>
  ::= CREATE
      [ <trigger-replace-mode> ]                          (* OR REPLACE | OR ALTER *)
      TRIGGER
      <trigger-name>
      <trigger-activation>                                (* ACTIVE | INACTIVE *)
      <trigger-target>                                    (* ON <table> | ON DATABASE *)
      <trigger-timing>                                    (* BEFORE | AFTER | INSTEAD OF *)
      <trigger-event-list>
      [ <trigger-granularity> ]                           (* FOR EACH ROW|STATEMENT *)
      [ <trigger-position-clause> ]                       (* planned: execution order *)
      [ <trigger-sql-security-clause> ]                   (* SQL SECURITY DEFINER|INVOKER *)
      [ <trigger-when-clause> ]                           (* planned: row predicate *)
      AS <trigger-body> ;                                 (* body captured as text *)

<trigger-replace-mode>
  ::= OR REPLACE
   | OR ALTER ;

<trigger-activation>
  ::= ACTIVE
   | INACTIVE ;

<trigger-target>
  ::= ON <qualified-name>
   | ON DATABASE ;

<trigger-timing>
  ::= BEFORE
   | AFTER
   | INSTEAD OF ;                                         (* mainly for view triggers *)

(* Events are split by target: table triggers vs database triggers. *)
<trigger-event-list>
  ::= <table-trigger-event> { OR <table-trigger-event> }
   | <database-trigger-event> { OR <database-trigger-event> } ;

<table-trigger-event>
  ::= INSERT
   | UPDATE
   | DELETE ;

(* Database trigger events (audit/spec completeness) *)
<database-trigger-event>
  ::= CONNECT
   | DISCONNECT
   | TRANSACTION START
   | TRANSACTION COMMIT
   | TRANSACTION ROLLBACK ;

<trigger-granularity>
  ::= FOR EACH ROW
   | FOR EACH STATEMENT ;

(* Planned: ordering controls. If you decide to forbid it, explicitly reject it. *)
<trigger-position-clause>
  ::= POSITION <integer> ;

<trigger-sql-security-clause>
  ::= SQL SECURITY ( DEFINER | INVOKER ) ;

(* Planned: common SQL feature; include in spec so migration doesn’t guess. *)
<trigger-when-clause>
  ::= WHEN '(' <expr> ')' ;

<trigger-body>
  ::= <string>
   | <psql-body> ;                                        (* DECLARE/BEGIN/END etc. *)

(* --------------------------------------------------------------------- *)
(* ALTER TRIGGER (spec-required lifecycle completeness)                   *)
(*                                                                       *)
(* Note: even if engine compiles rename/move through generic statements,  *)
(* the dialect spec should still include these surfaces.                  *)
(* --------------------------------------------------------------------- *)

<alter-trigger-spec>
  ::= ALTER TRIGGER [ IF EXISTS ] <trigger-ref>
      ( <alter-trigger-activation>
      | <alter-trigger-rename>
      | <alter-trigger-move>
      | <alter-trigger-sql-security>                       (* planned *)
      | <alter-trigger-body>                               (* planned *)
      | <alter-trigger-position> ) ;                       (* planned *)

<alter-trigger-activation>
  ::= ACTIVE
   | INACTIVE
   | ENABLE                                                (* alias → ACTIVE *)
   | DISABLE ;                                              (* alias → INACTIVE *)

<alter-trigger-rename>
  ::= RENAME TO <identifier> ;

<alter-trigger-move>
  ::= SET SCHEMA <qualified-name> ;

(* Planned: if CREATE supports SQL SECURITY, ALTER should too. *)
<alter-trigger-sql-security>
  ::= SQL SECURITY ( DEFINER | INVOKER ) ;

(* Planned: allow replacing the body without drop/recreate. *)
<alter-trigger-body>
  ::= AS <trigger-body> ;

<alter-trigger-position>
  ::= POSITION <integer> ;

(* --------------------------------------------------------------------- *)
(* DROP TRIGGER                                                          *)
(* --------------------------------------------------------------------- *)

<drop-trigger-spec>
  ::= DROP TRIGGER
      [ IF EXISTS ]
      <trigger-ref> { ',' <trigger-ref> }
      [ CASCADE | RESTRICT ] ;


---------
<create-package-spec>   ::= <qualified-name> <package-body> ;
<package-body>          ::= ( AS | IS ) <string> ;

<alter-package-spec>    ::= [ IF EXISTS ] <qualified-name>
                            ( RENAME TO <identifier> | SET SCHEMA <qualified-name> ) ;

<drop-package-spec>     ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;
```

---

### 14.13 JOB (with nested schedule and measurement semantics)

Schedule and measurement controls are modeled as nested `JOB` semantics, not as top-level object families. The following EBNF uses explicit clause families and avoids catchall placeholders.

```ebnf
(* ------------------------------------------------------------------------- *)
(* 14.13 JOB (enumerated from v3 AST)                                        *)
(* ------------------------------------------------------------------------- *)

<create-job-spec> ::= CREATE JOB [ ( OR ALTER ) | RECREATE ] <identifier>
                      <job-body-clause>
                      <job-schedule-clause>
                      [ <job-measurement-clause> ]
                      { <job-option-clause> } ;

<job-body-clause>  ::= AS ( SQL <string>
                          | PROCEDURE <qualified-name>
                          | EXTERNAL <string> ) ;

<job-schedule-clause>
                  ::= SCHEDULE ( CRON <string>
                             | AT <string>
                             | EVERY <integer> ( SECOND | MINUTE | HOUR | DAY ) )
                      [ STARTS AT <string> ]
                      [ ENDS AT <string> ] ;

<job-measurement-clause>
                  ::= MEASUREMENT '(' <job-measurement-option> { ',' <job-measurement-option> } ')' ;

<job-measurement-option>
                  ::= ENABLED '=' <boolean>
                   | WINDOW '=' <string>
                   | RETENTION '=' <string>
                   | GRANULARITY '=' <string>
                   | <identifier> '=' <expr> ;

<job-option-clause>
                  ::= MAX RETRIES <integer>
                   | RETRY BACKOFF <integer> ( SECOND | MINUTE | HOUR )
                   | TIMEOUT <integer> ( SECOND | MINUTE | HOUR )
                   | ON COMPLETION ( PRESERVE | DROP )
                   | RUN AS ROLE <qualified-name>
                   | DESCRIPTION <string>
                   | STATE ( ENABLED | DISABLED | PAUSED )
                   | CLASS <identifier>
                   | PARTITION BY <identifier> '(' <expr> ')' [ SHARD <identifier> ]
                   | DEPENDS ON '(' <ident-list> ')' ;

<alter-job-spec>   ::= ALTER JOB <identifier>
                       ( <job-body-alter>
                       | <job-schedule-alter>
                       | <job-measurement-alter>
                       | <job-option-alter>
                       | RENAME TO <identifier> ) ;

<job-body-alter>   ::= SET <job-body-clause> ;

<job-schedule-alter>
                  ::= SET <job-schedule-clause>
                   | DROP SCHEDULE ;

<job-measurement-alter>
                  ::= SET <job-measurement-clause>
                   | DROP MEASUREMENT ;

<job-option-alter> ::= SET <job-option-clause>
                    | DROP ( DESCRIPTION | RUN AS ROLE | MAX RETRIES | RETRY BACKOFF | TIMEOUT | ON COMPLETION | CLASS | PARTITION | DEPENDS ON )
                    | SET SECRET <identifier> <string>
                    | DROP SECRET <identifier>
                    | ( ENABLE | DISABLE | PAUSE | RESUME ) ;

<drop-job-spec>    ::= DROP JOB <identifier> [ KEEP HISTORY ] ;

<execute-job-spec> ::= EXECUTE JOB <identifier> ;

<cancel-job-run-spec>
                  ::= CANCEL JOB RUN <uuid-literal> ;

<uuid-literal>     ::= <string> | <identifier> ;
```

---

### 14.14 Remaining object families declared in core DDL (explicit skeletons)

These objects are present as first-class statement families in the uploaded core DDL dispatch list, but their full clause grammars are not enumerated in the provided sections. They are therefore expressed as explicit structured skeletons without opaque token catchalls.

```ebnf
<create-synonym-spec>         ::= <qualified-name> FOR <qualified-name>
                                  [ WITH '(' <kv-list> ')' ] ;
<alter-synonym-spec>          ::= <qualified-name>
                                  ( RENAME TO <identifier>
                                  | SET SCHEMA <qualified-name>
                                  | SET TARGET <qualified-name>
                                  | SET '(' <kv-list> ')'
                                  | RESET '(' <ident-list> ')' ) ;
<drop-synonym-spec>           ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;

<create-public-synonym-spec>  ::= <qualified-name> FOR <qualified-name>
                                  [ WITH '(' <kv-list> ')' ] ;
<alter-public-synonym-spec>   ::= <qualified-name>
                                  ( RENAME TO <identifier>
                                  | SET TARGET <qualified-name>
                                  | SET '(' <kv-list> ')'
                                  | RESET '(' <ident-list> ')' ) ;
<drop-public-synonym-spec>    ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;

<create-udr-spec>             ::= <qualified-name> '(' [ <routine-param-list> ] ')' <routine-body> ;
<drop-udr-spec>               ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-extension-spec>       ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-extension-spec>        ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-extension-spec>         ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;

<create-token-spec>           ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-token-spec>            ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-token-spec>             ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-quota-profile-spec>   ::= <qualified-name> WITH '(' <kv-list> ')' ;
<alter-quota-profile-spec>    ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-quota-profile-spec>     ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-repl-channel-spec>    ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-repl-channel-spec>     ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-repl-channel-spec>      ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-cluster-spec>         ::= <qualified-name>
                                  [ WITH '(' <kv-list> ')' ]
                                  [ STATE <cluster-state> ] ;
<alter-cluster-spec>          ::= <qualified-name>
                                  ( SET '(' <kv-list> ')'
                                  | RESET '(' <ident-list> ')'
                                  | RENAME TO <identifier>
                                  | SET STATE <cluster-state>
                                  | START
                                  | STOP
                                  | REFRESH [ WITH '(' <kv-list> ')' ] ) ;
<drop-cluster-spec>           ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;

<create-cube-spec>            ::= <qualified-name>
                                  [ WITH '(' <kv-list> ')' ]
                                  [ AS <query-expression> ] ;
<alter-cube-spec>             ::= <qualified-name>
                                  ( SET '(' <kv-list> ')'
                                  | RESET '(' <ident-list> ')'
                                  | RENAME TO <identifier>
                                  | START
                                  | STOP
                                  | REBUILD [ WITH '(' <kv-list> ')' ]
                                  | REFRESH [ WITH '(' <kv-list> ')' ] ) ;
<drop-cube-spec>              ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;

<create-connection-rule-spec> ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-connection-rule-spec>  ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-connection-rule-spec>   ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-database-connection-spec> ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-database-connection-spec>  ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-database-connection-spec>   ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-user-mapping-spec>     ::= FOR <principal> SERVER <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-user-mapping-spec>      ::= FOR <principal> SERVER <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' ) ;
<drop-user-mapping-spec>       ::= FOR <principal> SERVER <qualified-name> ;

<create-server-spec>           ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-server-spec>            ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> ) ;
<drop-server-spec>             ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-fdw-spec>              ::= <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<drop-fdw-spec>                ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } ;

<create-foreign-table-spec>    ::= <qualified-name> '(' <table-element> { ',' <table-element> } ')' SERVER <qualified-name> [ WITH '(' <kv-list> ')' ] ;
<alter-foreign-table-spec>     ::= <qualified-name> ( SET '(' <kv-list> ')' | RESET '(' <ident-list> ')' | RENAME TO <identifier> | SET SCHEMA <qualified-name> ) ;
<drop-foreign-table-spec>      ::= [ IF EXISTS ] <qualified-name> { ',' <qualified-name> } [ CASCADE | RESTRICT ] ;
```

---

### 14.15 Dialect values (minimal closure)

```ebnf
<dialect-value> ::= <identifier> | <string> ;
<compat-value>  ::= <identifier> | <string> ;
```

---

## 15) What changed vs prior draft (placeholders removed)

- Replaced all `{ <any-token> }` placeholders in the statement sub-specs with explicit clause families.
- Pulled concrete option/key lists for DOMAIN and TYPE from the Scalar Types section.
- Pulled concrete CREATE/ALTER/DROP INDEX families from the Index Framework section.
- Pulled CREATE/ALTER/DROP DATABASE/SCHEMA/TABLE/VIEW/SEQUENCE/TABLESPACE surface families from Core DDL.
- Kept forward-compatible named nonterminals where the uploaded docs do not enumerate deeper grammar (e.g., partition bounds and full query-expression grammar). 
