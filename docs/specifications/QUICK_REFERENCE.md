# Quick Reference

## Specification Documents

| Document | Purpose | Size |
|----------|---------|------|
| [ROBIN_MIGRATE_MASTER_SPECIFICATION.md](ROBIN_MIGRATE_MASTER_SPECIFICATION.md) | Overall architecture and phases | 22KB |
| [MENU_SPECIFICATION.md](MENU_SPECIFICATION.md) | All menus, items, shortcuts | 14KB |
| [FORM_SPECIFICATION.md](FORM_SPECIFICATION.md) | All dialogs and forms | 33KB |
| [IMPLEMENTATION_WORKPLAN.md](IMPLEMENTATION_WORKPLAN.md) | Step-by-step development plan | 17KB |

## Phase Summary

| Phase | Duration | Key Deliverables | Status |
|-------|----------|------------------|--------|
| 1 | 6 weeks | Main window, navigator, connections | Planned |
| 2 | 8 weeks | SQL editor, execution, results | Planned |
| 3 | 6 weeks | Data editor, import/export, migration | Planned |
| 4 | 10 weeks | ERD, debugging, dashboard, AI | Planned |

## Key Metrics

- **Total Actions**: 380+ (defined in ActionCatalog)
- **Window Types**: 40+ (defined in WindowTypeCatalog)
- **Menu Items**: 100+
- **Forms/Dialogs**: 15+
- **Development Weeks**: 30

## Critical Path

```
Main Window → Settings → Navigator → Connection → 
SQL Editor → Execution → Results → 
Data Editor → Import/Export → Migration → 
ERD → Debug → Dashboard → AI
```

## Immediate Next Steps

1. Review specifications with team
2. Set up development environment
3. Create detailed task breakdown for Week 1
4. Begin Phase 1 implementation

## Reference to Existing Code

| Component | Location | Status |
|-----------|----------|--------|
| Window Framework | `src/ui/window_framework/` | Partial |
| Action Catalog | `src/ui/window_framework/action_catalog.h/cpp` | Complete |
| Window Type Catalog | `src/ui/window_framework/window_type_catalog.h/cpp` | Complete |
| Icon Manager | `src/res/icon_manager.h/cpp` | Complete |
| Settings | `src/core/settings.h/cpp` | Partial |
