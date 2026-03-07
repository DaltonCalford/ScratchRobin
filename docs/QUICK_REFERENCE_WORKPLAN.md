# Quick Reference: Master Implementation Workplan

## Current Status at a Glance

| Phase | Name | Progress | Key Deliverables |
|-------|------|----------|------------------|
| 1 | Core UI Completion | 0% | Menus, Find/Replace, Preferences (6 tabs) |
| 2 | Data Management | 0% | Export, Import Wizard, Table Designer |
| 3 | SQL Editor Enhancement | 0% | Completion, History, Advanced features |
| 4 | Advanced Features | 0% | Transactions, Explain Plan, Monitoring |
| 5 | Polish & Conformance | 0% | Type rendering, Performance, Tests |

---

## This Week's Focus (Week 1)

### Day-by-Day Breakdown

| Day | Primary Task | Deliverable | Hours |
|-----|--------------|-------------|-------|
| Monday | Query Menu | `query_menu.h/cpp` | 6 |
| Tuesday | Transaction Menu | `transaction_menu.h/cpp` | 8 |
| Wednesday | Window & Tools Menus | `window_menu.cpp` `tools_menu.cpp` | 10 |
| Thursday | Help Menu & Menu State | `help_menu.cpp` `menu_state_manager.cpp` | 12 |
| Friday | Integration Testing | All menus functional | 4 |

### Success Criteria for Week 1
- [ ] All 9 menus visible and clickable
- [ ] Menu items enable/disable based on connection state
- [ ] All keyboard shortcuts registered

---

## Critical Path

The following features are on the **critical path** and block other work:

1. **Menu State Management** (PH1-W1-006) - Blocks: Transaction features
2. **Import Wizard Framework** (PH2-W5-001) - Blocks: All import features
3. **Table Designer Framework** (PH2-W7-001) - Blocks: All designer features
4. **Completion Engine** (PH3-W9-001) - Blocks: All completion features
5. **Transaction State Tracking** (PH4-W11-001) - Blocks: All transaction features

---

## Quick Navigation

### By Specification

| Spec Document | Implementation Phase | Tracker Rows |
|--------------|----------------------|--------------|
| MENU_SPECIFICATION.md | Phase 1, 4 | PH1-W1-* PH4-W11-* PH4-W13-* |
| FORM_SPECIFICATION.md | Phase 1, 2, 4 | PH1-W2-* PH1-W3-* PH2-W4-* PH2-W5-* PH2-W7-* PH4-W12-* |
| IMPLEMENTATION_WORKPLAN.md | Phase 3, 5 | PH3-W8-* PH3-W9-* PH3-W10-* PH5-W16-* |
| CONFORMANCE_MATRIX.md | Phase 4, 5 | PH4-W11-* PH5-W15-* |

### By Component

| Component | Files to Create/Modify | Phase |
|-----------|------------------------|-------|
| Menus | `*_menu.h/cpp` | 1 |
| Dialogs | `*_dialog.h/cpp` | 1, 2, 4 |
| Preferences | `preferences_*.cpp` | 1 |
| Import/Export | `import_*.cpp` `export_*.cpp` | 2 |
| Table Designer | `table_designer_*.cpp` | 2 |
| Editor Features | `sql_editor_*.cpp` | 3 |
| Code Completion | `completion_*.cpp` | 3 |
| Query History | `history_*.cpp` | 3 |
| Transactions | `transaction_*.cpp` | 4 |
| Monitoring | `monitor_*.cpp` | 4 |

---

## Definition of Done

### For Menus (Phase 1)
- All items specified in MENU_SPECIFICATION.md are present
- Keyboard shortcuts work as specified
- Items enable/disable correctly based on state
- Icons displayed where specified

### For Dialogs (Phase 1-2)
- Layout matches FORM_SPECIFICATION.md mockups
- All fields and controls functional
- Validation working per field specs
- Modal result handling correct
- Keyboard shortcuts (Enter/Escape) work

### For Import/Export (Phase 2)
- All format options from specification implemented
- Progress shown during operation
- Error handling with user-friendly messages
- Performance: 100K rows in < 30 seconds

### For Code Completion (Phase 3)
- < 500ms response time
- Schema-aware suggestions
- Fuzzy matching working
- Icons for different object types

### For Transactions (Phase 4)
- State machine correctly tracks transaction status
- Menu items enable/disable appropriately
- Visual indicator in status bar

---

## Risk Indicators

Watch for these warning signs:

| Risk | Indicator | Mitigation |
|------|-----------|------------|
| Scope creep | Adding features not in spec | Refer to WORK_BREAKDOWN.md |
| Performance issues | Operations > 1 second | Profile early, optimize |
| Integration issues | Features work in isolation but not together | Daily integration testing |
| Spec ambiguity | Unclear requirements | Document decision, move forward |

---

## Weekly Checklist Template

Copy this for each week:

```markdown
## Week X: [Name]

### Planned
- [ ] Task 1
- [ ] Task 2
- [ ] Task 3

### Completed
- [x] Task 1 (Actual: X hours)
- [x] Task 2 (Actual: X hours)

### Blocked
- Task 3: Waiting for [dependency]

### Notes
- [Any observations, decisions, or issues]

### Next Week Preview
- [What to expect next]
```

---

## File Naming Conventions

| Component Type | Naming Pattern | Example |
|----------------|----------------|---------|
| Menu | `[name]_menu.h/cpp` | `query_menu.cpp` |
| Dialog | `[name]_dialog.h/cpp` | `export_results_dialog.cpp` |
| Wizard | `[name]_wizard.h/cpp` | `import_data_wizard.cpp` |
| Panel/Tab | `[name]_panel.cpp` | `csv_options_panel.cpp` |
| Feature | `[name]_[feature].cpp` | `sql_editor_folding.cpp` |

---

## Tracker File Location

```
docs/
├── MASTER_IMPLEMENTATION_WORKPLAN.md    (This plan)
├── IMPLEMENTATION_TRACKER.csv           (Detailed tracker - 97 rows)
├── QUICK_REFERENCE_WORKPLAN.md          (This file)
└── specifications/
    ├── MENU_SPECIFICATION.md            (Source of truth for menus)
    ├── FORM_SPECIFICATION.md            (Source of truth for dialogs)
    ├── IMPLEMENTATION_WORKPLAN.md       (Original 30-week plan)
    └── CONFORMANCE_MATRIX.md            (Backend feature matrix)
```

---

## Estimation Guidelines

| Complexity | Hours | Examples |
|------------|-------|----------|
| Simple | 3-4 | About dialog, toggle button |
| Medium | 5-6 | Options panel, basic integration |
| Complex | 7-8 | Wizard step, parser implementation |
| Very Complex | 10+ | Completion engine, import wizard |

**Buffer**: Add 20% to estimates for testing and bug fixes.

---

## Communication

| Audience | Channel | Frequency |
|----------|---------|-----------|
| Team | Daily standup | Daily |
| Stakeholders | Status update | Weekly |
| External | Milestone demo | Per phase |

---

*This document should be updated weekly to reflect current status.*

*Last Updated: 2026-03-06*
