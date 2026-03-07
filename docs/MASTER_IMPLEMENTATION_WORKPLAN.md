# ScratchRobin Master Implementation Workplan

## Executive Summary

This workplan consolidates all missing specifications from:
- `MENU_SPECIFICATION.md` (389 lines)
- `FORM_SPECIFICATION.md` (610 lines)
- `IMPLEMENTATION_WORKPLAN.md` (495 lines)
- `CONFORMANCE_MATRIX.md` (130 lines)

**Total Estimated Effort**: ~16-20 weeks (2 developers)

---

## Phase Overview

| Phase | Name | Duration | Focus | Deliverables |
|-------|------|----------|-------|--------------|
| 1 | Core UI Completion | 3 weeks | Menus, dialogs, preferences | All menus functional, 6 preferences tabs |
| 2 | Data Management | 4 weeks | Import/Export, Table Designer | Full data lifecycle support |
| 3 | SQL Editor Enhancement | 3 weeks | Completion, History, Find/Replace | Professional SQL IDE features |
| 4 | Advanced Features | 4 weeks | Transactions, Debugging, Monitoring | Enterprise features |
| 5 | Polish & Conformance | 2 weeks | Type rendering, performance, tests | Release ready |

---

## Phase 1: Core UI Completion (Weeks 1-3)

### Week 1: Menus & Actions

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Implement Query menu (execute, explain, format) | MENU_SPEC §Query | query_menu.h/cpp | 6 |
| 2 | Implement Transaction menu | MENU_SPEC §Transaction | transaction_menu.h/cpp | 8 |
| 3 | Implement Window menu | MENU_SPEC §Window | window_menu.h/cpp | 4 |
| 4 | Implement Tools menu (import/export submenus) | MENU_SPEC §Tools | tools_menu.h/cpp | 6 |
| 5 | Add Help menu items (tip of day, shortcuts) | MENU_SPEC §Help | help_menu_ext.cpp | 4 |
| 6 | Implement dynamic menu state (connection-aware) | MENU_SPEC §State Mgmt | menu_state_manager.h/cpp | 8 |
| 7 | Integration & testing | - | All menus functional | 4 |

**Week 1 Deliverables**:
- [ ] All 9 menus fully implemented per specification
- [ ] Dynamic enable/disable based on connection state
- [ ] All keyboard shortcuts working

---

### Week 2: Dialog Framework & Find/Replace

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Create Find/Replace dialog | FORM_SPEC §Find/Replace | find_replace_dialog.h/cpp | 8 |
| 2 | Integrate find/replace with SqlEditor | - | Editor search functional | 4 |
| 3 | Implement About dialog | FORM_SPEC §About | about_dialog.h/cpp | 3 |
| 4 | Create dialog base class (validation, buttons) | FORM_SPEC §Common | dialog_base.h/cpp | 6 |
| 5 | Implement keyboard shortcuts dialog | MENU_SPEC §Help | shortcuts_dialog.h/cpp | 6 |
| 6 | Add tip of the day system | MENU_SPEC §Help | tip_of_day.h/cpp | 5 |
| 7 | Testing & refinement | - | Dialogs stable | 4 |

**Week 2 Deliverables**:
- [ ] Find/Replace fully functional in SQL editor
- [ ] About dialog with version info
- [ ] Shortcuts reference dialog
- [ ] Tip of the day system

---

### Week 3: Preferences Dialog (6 Tabs)

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Refactor PreferencesDialog to 6-tab layout | FORM_SPEC §Preferences | preferences_dialog.h/cpp rewrite | 6 |
| 2 | Implement General tab (language, theme, confirmations) | FORM_SPEC §General | general_tab.cpp | 4 |
| 3 | Implement Editor tab (font, indentation, completion) | FORM_SPEC §Editor | editor_tab.cpp | 6 |
| 4 | Implement Results tab (grid settings, formats) | FORM_SPEC §Results | results_tab.cpp | 5 |
| 5 | Implement Connection tab (defaults, auto-connect) | FORM_SPEC §Connection | connection_tab.cpp | 4 |
| 6 | Implement Shortcuts tab | FORM_SPEC §Shortcuts | shortcuts_tab.cpp | 6 |
| 7 | Settings persistence & integration | - | All prefs save/load | 5 |

**Week 3 Deliverables**:
- [ ] 6-tab preferences dialog per specification
- [ ] All settings persist to JSON
- [ ] Real-time preview where applicable

---

## Phase 2: Data Management (Weeks 4-7)

### Week 4: Export Results Dialog

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Design ExportResultsDialog structure | FORM_SPEC §Export | export_results_dialog.h/cpp | 6 |
| 2 | Implement format selection (CSV, SQL, JSON, Excel, XML, HTML) | FORM_SPEC §Export | format_selector.cpp | 4 |
| 3 | Implement CSV options panel | FORM_SPEC §CSV | csv_options_panel.cpp | 5 |
| 4 | Implement SQL options panel | FORM_SPEC §SQL | sql_options_panel.cpp | 4 |
| 5 | Implement JSON options panel | FORM_SPEC §JSON | json_options_panel.cpp | 3 |
| 6 | Add file browser and scope selection | FORM_SPEC §Export | export_controller.cpp | 5 |
| 7 | Testing with sample data | - | Export functional | 5 |

**Week 4 Deliverables**:
- [ ] Export dialog with 6 format options
- [ ] Format-specific options panels
- [ ] Export to file with progress

---

### Week 5-6: Import Data Wizard (4-Step)

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Create ImportDataWizard framework | FORM_SPEC §Import | import_wizard.h/cpp | 6 |
| 2 | Step 1: Source selection (file, format, encoding) | FORM_SPEC §Step 1 | import_step1.cpp | 5 |
| 3 | Step 2: Preview & column mapping UI | FORM_SPEC §Step 2 | import_step2.cpp | 8 |
| 4 | Step 2: Auto-detect column types | FORM_SPEC §Step 2 | type_detector.cpp | 6 |
| 5 | Step 3: Options (delimiter, null handling, target) | FORM_SPEC §Step 3 | import_step3.cpp | 6 |
| 6 | Step 4: Execution with progress | FORM_SPEC §Step 4 | import_step4.cpp | 6 |
| 7 | CSV parser implementation | - | csv_parser.h/cpp | 8 |
| 8 | Error handling and validation | - | import_validator.cpp | 5 |
| 9 | Integration testing | - | Wizard end-to-end | 6 |
| 10 | Performance testing (large files) | - | Handles 100K+ rows | 4 |

**Weeks 5-6 Deliverables**:
- [ ] 4-step import wizard per specification
- [ ] CSV parsing with preview
- [ ] Column mapping and type detection
- [ ] Progress tracking and error handling

---

### Week 7: Table Designer

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Create TableDesignerDialog (4-tab) | FORM_SPEC §Table Designer | table_designer_dialog.h/cpp | 6 |
| 2 | Columns tab: grid and property panel | FORM_SPEC §Columns | columns_tab.cpp | 8 |
| 3 | Constraints tab: PK, FK, Unique | FORM_SPEC §Constraints | constraints_tab.cpp | 6 |
| 4 | Indexes tab: index designer | FORM_SPEC §Indexes | indexes_tab.cpp | 5 |
| 5 | Preview SQL tab | FORM_SPEC §Preview | sql_preview_tab.cpp | 4 |
| 6 | DDL generation engine | - | ddl_generator.cpp | 6 |
| 7 | Testing & integration | - | Designer functional | 7 |

**Week 7 Deliverables**:
- [ ] 4-tab table designer
- [ ] Visual column/constraints/indexes editing
- [ ] Live SQL preview

---

## Phase 3: SQL Editor Enhancement (Weeks 8-10)

### Week 8: Advanced Editor Features

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Implement code folding | IMPLEMENTATION Week 8 | code_folding.cpp | 6 |
| 2 | Implement auto-indentation | IMPLEMENTATION Week 8 | auto_indent.cpp | 5 |
| 3 | Add word wrap toggle | IMPLEMENTATION Week 8 | word_wrap.cpp | 3 |
| 4 | Implement zoom controls (Ctrl++, Ctrl+-, Ctrl+0) | IMPLEMENTATION Week 8 | zoom_controls.cpp | 3 |
| 5 | Add occurrences highlighting | IMPLEMENTATION Week 8 | occurrences_highlighter.cpp | 5 |
| 6 | SQL formatter integration | MENU_SPEC §Query | sql_formatter.cpp | 6 |
| 7 | Testing | - | All features working | 4 |

**Week 8 Deliverables**:
- [ ] Code folding in editor
- [ ] Auto-indentation
- [ ] Zoom and word wrap
- [ ] SQL formatter

---

### Week 9: Code Completion Engine

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Design completion engine architecture | IMPLEMENTATION Week 9 | completion_engine.h | 4 |
| 2 | Keyword completion (SQL dialect) | IMPLEMENTATION Week 9 | keyword_provider.cpp | 5 |
| 3 | Schema object completion | IMPLEMENTATION Week 9 | schema_provider.cpp | 6 |
| 4 | Table/column completion from metadata | IMPLEMENTATION Week 9 | metadata_provider.cpp | 6 |
| 5 | Fuzzy matching | IMPLEMENTATION Week 9 | fuzzy_matcher.cpp | 5 |
| 6 | UI: completion popup, icons, navigation | - | completion_popup.cpp | 6 |
| 7 | Performance optimization (<500ms) | - | Caching layer | 4 |

**Week 9 Deliverables**:
- [ ] Intelligent code completion
- [ ] Schema-aware suggestions
- [ ] <500ms response time

---

### Week 10: Query History & Execution

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Design history storage schema | IMPLEMENTATION Week 13 | history_schema.sql | 3 |
| 2 | Implement history logging | IMPLEMENTATION Week 13 | history_logger.cpp | 5 |
| 3 | Create history dialog with search/filter | IMPLEMENTATION Week 13 | history_dialog.h/cpp | 6 |
| 4 | Integrate history with editor | - | History insert/recall | 4 |
| 5 | Execute selection feature | MENU_SPEC §Query | execute_selection.cpp | 3 |
| 6 | Execute script (multi-statement) | MENU_SPEC §Query | script_executor.cpp | 6 |
| 7 | Testing | - | History and execution | 5 |

**Week 10 Deliverables**:
- [ ] Query history with search
- [ ] Execute selection vs execute all
- [ ] Script execution with progress

---

## Phase 4: Advanced Features (Weeks 11-14)

### Week 11: Transaction Support

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Implement transaction state tracking | CONFORMANCE SB-TXN | transaction_state.h/cpp | 5 |
| 2 | Transaction menu integration | MENU_SPEC §Transaction | tx_menu_integration.cpp | 4 |
| 3 | Auto-commit toggle | CONFORMANCE SB-TXN-008 | autocommit_toggle.cpp | 3 |
| 4 | Isolation level selection | MENU_SPEC §Transaction | isolation_selector.cpp | 4 |
| 5 | Visual transaction indicator | - | status_bar_tx_widget.cpp | 4 |
| 6 | Commit/rollback with confirmation | - | tx_confirmation.cpp | 4 |
| 7 | Testing | - | Transactions working | 4 |

**Week 11 Deliverables**:
- [ ] Full transaction menu support
- [ ] Auto-commit toggle
- [ ] Visual transaction state

---

### Week 12: Explain Plan & Results Enhancement

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Query plan capture | CONFORMANCE SB-ADMIN-004 | plan_capture.cpp | 5 |
| 2 | Create Explain Plan dialog | IMPLEMENTATION Week 13 | explain_dialog.h/cpp | 6 |
| 3 | Plan visualization (tree view) | IMPLEMENTATION Week 13 | plan_tree_view.cpp | 6 |
| 4 | Record view (single row display) | IMPLEMENTATION Week 12 | record_view.cpp | 5 |
| 5 | View toggle (grid/record/text) | FORM_SPEC §Results | view_toggle.cpp | 4 |
| 6 | Result pagination | IMPLEMENTATION Week 11 | pagination.cpp | 4 |
| 7 | Testing | - | Enhanced results | 4 |

**Week 12 Deliverables**:
- [ ] Query plan visualization
- [ ] Grid/Record/Text view toggle
- [ ] Pagination for large results

---

### Week 13-14: Monitoring & Tools

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Create Monitor submenu actions | MENU_SPEC §Monitor | monitor_actions.cpp | 4 |
| 2 | Connections monitor panel | MENU_SPEC §Monitor | connections_monitor.cpp | 6 |
| 3 | Transactions monitor panel | MENU_SPEC §Monitor | transactions_monitor.cpp | 6 |
| 4 | Locks monitor panel | MENU_SPEC §Monitor | locks_monitor.cpp | 5 |
| 5 | Performance monitor panel | MENU_SPEC §Monitor | performance_monitor.cpp | 6 |
| 6 | Generate DDL dialog | MENU_SPEC §Tools | generate_ddl_dialog.h/cpp | 6 |
| 7 | Schema compare (basic) | MENU_SPEC §Tools | schema_compare.cpp | 6 |
| 8 | Cancel running query | CONFORMANCE SB-EXEC-010 | query_canceller.cpp | 5 |
| 9 | Testing & integration | - | All tools working | 6 |

**Weeks 13-14 Deliverables**:
- [ ] 5 monitor panels
- [ ] Generate DDL dialog
- [ ] Query cancellation

---

## Phase 5: Polish & Conformance (Weeks 15-16)

### Week 15: Type Rendering & Data Types

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Date/time type decoder | CONFORMANCE SB-TYPE-004 | datetime_decoder.cpp | 5 |
| 2 | Decimal/money decoder | CONFORMANCE SB-TYPE-005 | decimal_decoder.cpp | 5 |
| 3 | JSON/JSONB decoder | CONFORMANCE SB-TYPE-006 | json_decoder.cpp | 4 |
| 4 | Array/composite decoder | CONFORMANCE SB-TYPE-007 | array_decoder.cpp | 5 |
| 5 | Grid cell renderers per type | - | type_renderers.cpp | 5 |
| 6 | Format options (date/time/decimal) | FORM_SPEC §Results | format_preferences.cpp | 4 |
| 7 | Testing with type matrix | - | All types render | 4 |

**Week 15 Deliverables**:
- [ ] All data types render correctly
- [ ] Format preferences respected

---

### Week 16: Performance & Testing

| Day | Task | Spec Reference | Deliverable | Est. Hours |
|-----|------|----------------|-------------|------------|
| 1 | Editor performance (10K lines) | IMPLEMENTATION Phase 2 | perf_editor.cpp | 5 |
| 2 | Grid performance (10K rows) | IMPLEMENTATION Phase 2 | perf_grid.cpp | 5 |
| 3 | Export performance (100K rows) | IMPLEMENTATION Phase 2 | perf_export.cpp | 4 |
| 4 | Unit test coverage | - | >75% coverage | 6 |
| 5 | Integration tests | - | Core paths tested | 5 |
| 6 | UI automation tests | - | Critical paths | 4 |
| 7 | Final bug fixes | - | Release ready | 3 |

**Week 16 Deliverables**:
- [ ] Performance targets met
- [ ] Test coverage >75%
- [ ] No critical bugs

---

## Dependencies & Blockers

### External Dependencies
| ID | Dependency | Impact | Mitigation |
|----|-----------|--------|------------|
| DEP-1 | ScratchBird SBWP protocol stability | High | Coordinate with server team |
| DEP-2 | TLS/SSL transport | Medium | Defer to Phase 4 if needed |
| DEP-3 | Native parser compiler | High | Ensure backend availability |

### Internal Blockers
| ID | Blocker | Resolution |
|----|---------|------------|
| BLK-1 | wxWidgets to Qt6 migration | Already completed |
| BLK-2 | Backend contract stability | Monitor conformance matrix |

---

## Resource Requirements

### Personnel
- 2 C++/Qt developers (full-time)
- 1 QA engineer (half-time from Week 4)

### Development Environment
- Qt6 development libraries
- CMake 3.20+
- ScratchBird test server
- Test data sets (CSV, SQL)

---

## Risk Management

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| SBWP protocol changes | Medium | High | Versioned protocol, abstraction layer |
| Performance issues | Medium | Medium | Early profiling, optimization sprint |
| Scope creep | High | Medium | Strict change control, phased delivery |
| Resource availability | Low | High | Cross-training, documentation |

---

## Success Criteria

### Completion Metrics
| Metric | Target |
|--------|--------|
| All Phase 1-3 features | 100% complete |
| Phase 4 features | 80% complete (core monitoring) |
| Test coverage | >75% |
| Performance targets | 100% met |
| Critical bugs at release | 0 |

### Quality Gates
| Gate | Criteria |
|------|----------|
| Phase 1 Gate | All menus/preferences functional |
| Phase 2 Gate | Import/export working, table designer functional |
| Phase 3 Gate | Code completion <500ms, history working |
| Phase 4 Gate | Transaction support, monitoring panels |
| Release Gate | All metrics met, no critical bugs |

---

## Appendix: Specification Coverage Matrix

| Specification | Sections | Status |
|--------------|----------|--------|
| MENU_SPECIFICATION.md | 10 sections | Tracked in Phase 1, 4 |
| FORM_SPECIFICATION.md | 12 forms | Tracked in Phases 1-4 |
| IMPLEMENTATION_WORKPLAN.md | 30 weeks | Condensed to 16 weeks |
| CONFORMANCE_MATRIX.md | 122 features | Tracked throughout |

---

*Workplan Version: 1.0*  
*Last Updated: 2026-03-06*  
*Next Review: Weekly on Fridays*
