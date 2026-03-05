# Conformance Matrix

## Scope

This matrix tracks feature-by-feature conformance for `robin-migrate` against
ScratchBird-native behavior, with strict adapter and execution boundary rules.

Coverage status values:
- `Wired`: implemented in current code path
- `Partial`: partly implemented, missing validations or edge cases
- `Planned`: specified but not implemented yet
- `Blocked`: requires upstream/runtime dependency not yet available

| Feature ID | Feature | ScratchBird Surface | Expected Path | Coverage Status | Validation Artifact | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| SB-CONN-001 | Native listener TCP connect | `sb_listener_native` port | GUI/CLI -> `NativeAdapterGateway` -> `ScratchbirdSbwpClient` | Wired | `tests/backend_contract_tests.cpp` | Fails closed on connect error |
| SB-CONN-002 | SBWP startup frame | `sbwp::MessageType::Startup` | client startup -> SBWP startup payload | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses database/application params |
| SB-CONN-003 | SBWP header decode | `sbwp::decodeHeader` | socket read -> header parse -> dispatch | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Rejects malformed/truncated headers |
| SB-CONN-004 | Attachment/session tracking | `AuthOk` session id | startup/auth -> attachment id in outgoing headers | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses server-issued attachment id |
| SB-CONN-005 | Ready-state transition | `sbwp::MessageType::Ready` | startup/query/txn loops complete on ready | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Updates `txn_id` from ready payload |
| SB-CONN-006 | Graceful terminate | `sbwp::MessageType::Terminate` | client disconnect -> terminate frame | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Best-effort send on shutdown |
| SB-CONN-007 | Host/port runtime override | runtime config | UI connect panel -> client runtime config | Wired | `src/ui/main_frame.cpp` | Port is validated at UI boundary |
| SB-CONN-008 | Per-port parser enforcement | parser registry | submit -> `ParserPortRegistry::Validate` | Wired | `tests/backend_contract_tests.cpp` | No parser auto-detect fallback |
| SB-CONN-009 | Native parser only dialect | dialect contract | submit -> dialect check before SBWP | Wired | `tests/backend_contract_tests.cpp` | `scratchbird-native` only |
| SB-CONN-010 | Reconnect on config change | client transport | set runtime config -> disconnect/reconnect | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Configuration changes invalidate socket |
| SB-AUTH-001 | Auth request parse | `sbwp::parseAuthRequest` | startup loop -> auth dispatch | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Handles Password/SCRAM method ids |
| SB-AUTH-002 | Password auth response | `AuthResponse` payload | auth request -> password payload send | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Plain payload path retained |
| SB-AUTH-003 | SCRAM client-first | `scratchbird::udr::SCRAMClient` | auth request -> client-first generation | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses bootstrap user if empty username |
| SB-AUTH-004 | SCRAM continue exchange | `sbwp::parseAuthContinue` | auth continue -> client-final response | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Server-first parsed via SCRAM client |
| SB-AUTH-005 | SCRAM server-final verify | SCRAM verify call | auth ok info -> verify server-final | Partial | `src/backend/scratchbird_sbwp_client.cpp` | Verification result not yet surfaced as hard failure |
| SB-AUTH-006 | AUTH_OK parsing | `sbwp::parseAuthOk` | auth ok -> session + info capture | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Session copied to attachment id |
| SB-AUTH-007 | Auth error propagation | `sbwp::MessageType::Error` | startup/auth -> structured error -> response | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Message/detail/hint composed to client status |
| SB-AUTH-008 | Multi-step auth sequencing | SCRAM stage handling | auth request/continue loops until ready | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Stage byte parsed and consumed |
| SB-AUTH-009 | MD5 auth handling | `sbwp::AuthMethod::Md5` | auth request -> response payload | Partial | `src/backend/scratchbird_sbwp_client.cpp` | Currently treated as password payload passthrough |
| SB-AUTH-010 | TLS/cert auth support | auth methods | startup/auth negotiation | Planned | `docs/plans/WORK_BREAKDOWN.md` | Requires TLS/certificate UI and transport support |
| SB-EXEC-001 | SQL query submission over SBWP | `sbwp::MessageType::Query` | submit SQL -> query payload -> native listener | Wired | `src/backend/native_adapter_gateway.cpp` | Real transport path replaces simulated execution |
| SB-EXEC-002 | Native adapter execution boundary | ScratchBird native adapter | SQL -> native parser compile -> engine bytecode execution | Wired | `docs/execution-contracts/NATIVE_ADAPTER_CONTRACT.md` | Client does not execute SQL in engine layer |
| SB-EXEC-003 | ServerSession bytecode-only rule | `ServerSession` contract | parser compiles to SBLR before engine execute | Wired | `docs/execution-contracts/SERVERSESSION_EXECUTION_CONTRACT.md` | Normative architecture constraint |
| SB-EXEC-004 | SQL text engine rejection guard | contract guard class | direct `ServerSessionGateway` SQL payload rejection | Wired | `tests/backend_contract_tests.cpp` | Local invariant safety test |
| SB-EXEC-005 | Bytecode flag required guard | contract guard class | local gateway rejects missing bytecode flag | Wired | `tests/backend_contract_tests.cpp` | Mirrors ScratchBird boundary semantics |
| SB-EXEC-006 | Command completion capture | `CommandComplete` parse | query loop -> rows/tag extraction | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Non-row commands produce command/rows output row |
| SB-EXEC-007 | Error frame parse | `sbwp::parseErrorMessage` | query loop -> failure mapping | Wired | `src/backend/scratchbird_sbwp_client.cpp` | SQLSTATE/detail/hint surfaced |
| SB-EXEC-008 | Ready frame completion | `Ready` parse | query loop exits on ready | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Ensures protocol synchronization |
| SB-EXEC-009 | Query timeout handling | timeout payload + socket timeout | query send/read with timeout controls | Partial | `src/backend/scratchbird_sbwp_client.cpp` | Socket timeouts set; per-query timeout field fixed at 0 |
| SB-EXEC-010 | Cancel query | `MessageType::Cancel` | UI cancel -> SBWP cancel frame | Planned | `docs/plans/WORK_BREAKDOWN.md` | UI/command wiring not added yet |
| SB-ROW-001 | Row description parse | `sbwp::parseRowDescription` | response -> column metadata capture | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Columns map to grid headers |
| SB-ROW-002 | Data row parse | `sbwp::parseDataRow` | response -> value vector extraction | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Handles NULL bitmap/column count |
| SB-ROW-003 | Null value handling | data row null map | parse -> `NULL` string rendering | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Deterministic null rendering |
| SB-ROW-004 | Numeric conversion | OID decode (int/float) | row decode -> string formatting | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses host-endian decode to mirror protocol codec |
| SB-ROW-005 | UUID conversion | OID UUID decode | row decode -> canonical UUID format | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Hex+hyphen formatting |
| SB-ROW-006 | Binary data rendering | OID bytea/default binary | row decode -> `0x...` | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Printable-ascii fallback for textlike payloads |
| SB-ROW-007 | Date/time decode | DATE/TIME/TIMESTAMP OIDs | row decode -> typed conversion | Planned | `docs/plans/WORK_BREAKDOWN.md` | Currently string/hex fallback |
| SB-ROW-008 | Decimal precision decode | NUMERIC/MONEY OIDs | row decode -> exact decimal formatting | Planned | `docs/plans/WORK_BREAKDOWN.md` | Precision-safe formatting pending |
| SB-ROW-009 | JSON/JSONB decode | JSON OIDs | row decode -> UTF-8 string rendering | Partial | `src/backend/scratchbird_sbwp_client.cpp` | Currently generic printable-text fallback |
| SB-ROW-010 | Column format awareness | text/binary format flag | parse -> format-aware decoding | Planned | `docs/plans/WORK_BREAKDOWN.md` | Current decode keyed only by OID |
| SB-TXN-001 | Begin transaction frame | `MessageType::TxnBegin` | client txn begin API -> SBWP | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses `buildTxnBeginPayload` defaults |
| SB-TXN-002 | Commit transaction frame | `MessageType::TxnCommit` | client txn commit API -> SBWP | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses `buildTxnCommitPayload` |
| SB-TXN-003 | Rollback transaction frame | `MessageType::TxnRollback` | client txn rollback API -> SBWP | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Uses `buildTxnRollbackPayload` |
| SB-TXN-004 | Savepoint support | savepoint messages | client savepoint API -> SBWP | Planned | `docs/plans/WORK_BREAKDOWN.md` | No savepoint API exposed in UI/client yet |
| SB-TXN-005 | Txn status tracking | ready payload txn id | ready parse updates `txn_id` | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Maintains transactional header context |
| SB-TXN-006 | Isolation/access mode controls | txn begin payload fields | UI config -> transaction options | Planned | `docs/plans/WORK_BREAKDOWN.md` | Payload currently defaulted to zeros |
| SB-TXN-007 | Transaction error propagation | error frame parse | txn commands -> structured failure | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Same error path as queries |
| SB-TXN-008 | Auto-commit mode toggle | client behavior | UI/session state -> txn framing policy | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet modeled in session layer |
| SB-STMT-001 | Parse statement | `MessageType::Parse` | prepare flow -> parse frame | Planned | `docs/plans/WORK_BREAKDOWN.md` | Needed for prepared-statement parity |
| SB-STMT-002 | Bind statement | `MessageType::Bind` | execute prepared -> bind frame | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet wired |
| SB-STMT-003 | Execute portal | `MessageType::Execute` | portal execution -> fetch loop | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet wired |
| SB-STMT-004 | Describe statement/portal | `MessageType::Describe` | metadata introspection | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet wired |
| SB-STMT-005 | Close statement/portal | `MessageType::Close` | statement cleanup | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet wired |
| SB-STMT-006 | Sync boundary | `MessageType::Sync` | statement lifecycle resync | Planned | `docs/plans/WORK_BREAKDOWN.md` | Pending prepared protocol support |
| SB-STMT-007 | Prepared cache policy | client-side statement cache | repeated exec path | Planned | `docs/plans/WORK_BREAKDOWN.md` | Cache and invalidation policy pending |
| SB-COPY-001 | Copy-in request handling | `CopyInResponse`/`CopyData` | COPY FROM STDIN flow | Planned | `docs/testing/VALIDATION_STRATEGY.md` | UI and streaming buffers pending |
| SB-COPY-002 | Copy-out response handling | `CopyOutResponse`/`CopyData` | COPY TO STDOUT flow | Planned | `docs/testing/VALIDATION_STRATEGY.md` | UI/file export integration pending |
| SB-COPY-003 | Copy fail handling | `CopyFail` parse | copy errors -> surfaced to user | Planned | `docs/testing/VALIDATION_STRATEGY.md` | No copy flow wired yet |
| SB-COPY-004 | Stream flow control | `StreamControl`/windowing | large transfer backpressure | Planned | `docs/testing/VALIDATION_STRATEGY.md` | Not yet implemented |
| SB-COPY-005 | Large object copy path | binary copy framing | BLOB/CLOB transfer | Planned | `docs/plans/WORK_BREAKDOWN.md` | Pending feature stage |
| SB-NOTIFY-001 | Subscribe channel | `MessageType::Subscribe` | subscription API -> notify stream | Planned | `docs/plans/WORK_BREAKDOWN.md` | No subscription UI yet |
| SB-NOTIFY-002 | Unsubscribe channel | `MessageType::Unsubscribe` | channel removal | Planned | `docs/plans/WORK_BREAKDOWN.md` | No subscription UI yet |
| SB-NOTIFY-003 | Notification parse | `parseNotification` | async message -> event feed | Planned | `docs/testing/VALIDATION_STRATEGY.md` | Async message pump pending |
| SB-NOTIFY-004 | UI notification pane | output/event view | async notices/notifications -> UI | Planned | `docs/plans/WORK_BREAKDOWN.md` | Current output tab is query-log only |
| SB-META-001 | Object tree bootstrap | schema/table/object nodes | source parse -> recursive schema tree model | Wired | `src/ui/main_frame.cpp` | Source-derived preview tree mirrors schema placement hierarchy |
| SB-META-002 | Schema enumeration | system metadata queries | catalog source -> schema path extraction -> tree population | Wired | `src/backend/scratchbird_catalog_preview.cpp` | Loads bootstrap and hinted schema paths from ScratchBird sources |
| SB-META-003 | Table metadata | table columns/indexes/triggers | catalog source + overlay -> table detail panel | Wired | `src/backend/scratchbird_catalog_preview.cpp` | Source-derived table metadata with overlay file support |
| SB-META-004 | UUID-backed identity mapping | ScratchBird UUID model | metadata/object ops -> UUID mapping | Planned | `docs/architecture/SYSTEM_ARCHITECTURE.md` | Not yet implemented in client model |
| SB-META-005 | Object properties dialogs | table/view/procedure/function/sequence/trigger/domain/package/job/schedule/user/role/group + all first-class ScratchBird ObjectType kinds | tree selection -> per-kind properties dialog | Wired | `src/ui/main_frame.cpp` | Table editor + specialized SQL object metadata editor profiles across first-class typed objects |
| SB-META-006 | Refresh invalidation | metadata reload | explicit refresh -> delta update | Planned | `docs/plans/DELIVERY_STAGES.md` | Not yet implemented |
| SB-TYPE-001 | Boolean/int/float rendering | core scalar OIDs | row decode -> grid render | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Baseline scalar decode implemented |
| SB-TYPE-002 | UUID rendering | UUID OID | decode -> canonical uuid | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Implemented |
| SB-TYPE-003 | Binary/blob rendering | bytea + opaque payloads | decode -> hex | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Implemented |
| SB-TYPE-004 | Date/time rendering | temporal OIDs | decode -> ISO text | Planned | `docs/plans/WORK_BREAKDOWN.md` | Pending typed decoder |
| SB-TYPE-005 | Decimal/money rendering | numeric/money OIDs | decode -> exact decimal text | Planned | `docs/plans/WORK_BREAKDOWN.md` | Pending decimal-safe parser |
| SB-TYPE-006 | JSON/JSONB rendering | json oids | decode -> formatted text | Partial | `src/backend/scratchbird_sbwp_client.cpp` | Generic text fallback today |
| SB-TYPE-007 | Array/composite rendering | array/composite OIDs | decode -> structured representation | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet implemented |
| SB-TYPE-008 | Vector type rendering | `kOidSbVector` | decode -> vector text/preview | Planned | `docs/plans/WORK_BREAKDOWN.md` | Pending analytics feature stage |
| SB-TYPE-009 | Network/geospatial types | inet/cidr/geo OIDs | decode -> canonical text | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet implemented |
| SB-TYPE-010 | Fulltext types | tsvector/tsquery OIDs | decode -> text semantics | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet implemented |
| SB-ADMIN-001 | Ping capability | `MessageType::Ping/Pong` | health check action -> pong status | Planned | `docs/plans/WORK_BREAKDOWN.md` | No ping UI action yet |
| SB-ADMIN-002 | Status request | status frames | diagnostics panel | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet wired |
| SB-ADMIN-003 | Parameter status capture | `ParameterStatus` frames | session diagnostics cache | Partial | `src/backend/scratchbird_sbwp_client.cpp` | Frames currently ignored in runtime state |
| SB-ADMIN-004 | Query plan capture | `QueryPlan` frame | plan tab/reporting | Partial | `src/backend/scratchbird_sbwp_client.cpp` | QueryPlan frames ignored currently |
| SB-ADMIN-005 | Capability reporting | startup/status UI | connect -> capability snapshot | Planned | `docs/plans/WORK_BREAKDOWN.md` | Capability table not yet exposed |
| SB-ERR-001 | Structured SQLSTATE propagation | Error frame parsing | response.status.message content | Wired | `src/backend/scratchbird_sbwp_client.cpp` | severity/message/detail/hint composition |
| SB-ERR-002 | Connection failure diagnostics | socket/open failures | connect -> explicit failure reason | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Includes errno/gai detail |
| SB-ERR-003 | Protocol violation handling | malformed frame checks | decode/parse -> fail closed + disconnect | Wired | `src/backend/scratchbird_sbwp_client.cpp` | Connection reset on protocol corruption |
| SB-ERR-004 | Parser-port mismatch errors | registry validation | wrong port/dialect -> reject before network | Wired | `src/backend/parser_port_registry.cpp` | Deterministic error message |
| SB-ERR-005 | UI surfaced error state | result/output panel | failed query -> status bar + output tab | Wired | `src/ui/main_frame.cpp` | Query path and message shown |
| SB-ERR-006 | Retry/backoff policy | connect/query retries | transient errors -> controlled retry | Planned | `docs/plans/WORK_BREAKDOWN.md` | No retry policy beyond reconnect-on-config |
| SB-UI-001 | FlameRobin-style split layout | tree + editor + result panes | wx frame shell structure | Wired | `src/ui/main_frame.cpp` | Left tree, tabbed editor, bottom grid |
| SB-UI-002 | SQL editor tab | multiline SQL workspace | wxStyledTextCtrl editor | Wired | `src/ui/main_frame.cpp` | F5 shortcut bound to execute |
| SB-UI-003 | Result grid rendering | table output | parsed rows -> wxGrid | Wired | `src/ui/main_frame.cpp` | Dynamic cols/rows |
| SB-UI-004 | Connection profile bar | host/port/db/user/pass | profile input -> runtime config | Wired | `src/ui/main_frame.cpp` | Uses `ConfigureRuntime` |
| SB-UI-005 | Object tree parity depth | metadata hierarchy | source-derived metadata -> tree update | Wired | `src/ui/main_frame.cpp` | Recursive schema tree with kind icons and table sub-branches |
| SB-UI-006 | Multi-tab SQL sessions | notebook SQL tabs | open/save/close query tabs | Planned | `docs/migration/UI_PARITY_MATRIX.md` | Single editor tab today |
| SB-UI-007 | Keyboard workflow parity | FlameRobin shortcuts | execute/history/navigation | Partial | `src/ui/main_frame.cpp` | F5 added; rest pending |
| SB-UI-008 | Query history panel | statement history | executed sql -> history model | Planned | `docs/migration/UI_PARITY_MATRIX.md` | Output log only for now |
| SB-UI-009 | Cancel running query action | cancel command | UI cancel -> SBWP cancel frame | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet wired |
| SB-UI-010 | Auth/SSL settings dialog | security configuration | persisted profile -> runtime config | Planned | `docs/plans/WORK_BREAKDOWN.md` | Not yet implemented |
| SB-REL-001 | CLI runtime smoke path | `robin-migrate` executable | CLI -> backend query path | Wired | `src/app/main.cpp` | Demonstrates live backend call path |
| SB-REL-002 | GUI runtime shell build | `robin-migrate-gui` executable | wx app -> backend query path | Wired | `src/CMakeLists.txt` | Built when wxWidgets present |
| SB-REL-003 | SBWP build guard | optional compile integration | CMake option + source detection | Wired | `src/CMakeLists.txt` | Defines `ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP` |
| SB-REL-004 | Cross-platform socket support | transport portability | POSIX + Windows socket abstraction | Planned | `docs/plans/WORK_BREAKDOWN.md` | Current transport is POSIX-oriented |
| SB-REL-005 | End-to-end CI integration test | live listener harness | spawn listener + run query | Planned | `docs/testing/VALIDATION_STRATEGY.md` | Requires stable non-root runtime harness |
| SB-REL-006 | Non-connected review mode | preview pipeline | SQL -> compile-to-SBLR -> local `ServerSessionGateway` simulation | Wired | `src/backend/native_adapter_gateway.cpp` | Default mode for UI/flow review without running listener |

## Gate Mapping

1. Stage A gate: SB-CONN, SB-AUTH, SB-EXEC baseline rows are `Wired`.
2. Stage B gate: SB-UI baseline rows are `Wired`/`Partial` with no `Blocked` rows.
3. Stage C gate: SB-TXN and SB-STMT baseline reaches `Wired` for required beta scope.
4. Stage D gate: SB-TYPE, SB-META, SB-NOTIFY rows have explicit test artifacts.
5. Stage E gate: SB-REL rows include CI-backed end-to-end listener evidence.
