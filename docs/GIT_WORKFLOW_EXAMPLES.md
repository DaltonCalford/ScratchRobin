# Git Workflow Examples

## Practical Examples of Dual-Repo Git Operations

### Example 1: New Feature Development

**Scenario**: Adding a customer loyalty program feature

```bash
# Step 1: Start from main in both repos
# ======================================

cd ~/projects/ecommerce-db

# Project repo
git checkout main
git pull origin main

# Database repo (via ScratchRobin)
scratchrobin db-git checkout main
scratchrobin db-git pull

# Create feature branch in both
scratchrobin git branch feature/loyalty-program --checkout
# This creates branch in both project and database repos

# Step 2: Design the feature
# ======================================

# Open ScratchRobin GUI
# 1. Create whiteboard: "Loyalty Program Design"
# 2. Create mindmap for loyalty domain
# 3. Create ERD diagram showing:
#    - loyalty_tiers table
#    - loyalty_points table  
#    - Modified: users table (add loyalty_tier_id)

# Step 3: Implement in project
# ======================================

# ScratchRobin auto-generates design files:
# designs/public.loyalty_tiers.table.json
cat > designs/public.loyalty_tiers.table.json << 'EOF'
{
  "name": "loyalty_tiers",
  "schema": "public",
  "kind": "table",
  "state": "new",
  "columns": [
    {"name": "tier_id", "type": "SERIAL", "primary_key": true},
    {"name": "name", "type": "VARCHAR(50)", "not_null": true},
    {"name": "min_points", "type": "INTEGER", "not_null": true},
    {"name": "discount_percent", "type": "DECIMAL(5,2)", "default": 0}
  ]
}
EOF

# designs/public.loyalty_points.table.json
cat > designs/public.loyalty_points.table.json << 'EOF'
{
  "name": "loyalty_points",
  "schema": "public", 
  "kind": "table",
  "state": "new",
  "columns": [
    {"name": "point_id", "type": "BIGSERIAL", "primary_key": true},
    {"name": "user_id", "type": "INTEGER", "not_null": true, "foreign_key": "users.user_id"},
    {"name": "points", "type": "INTEGER", "not_null": true},
    {"name": "reason", "type": "VARCHAR(255)"},
    {"name": "created_at", "type": "TIMESTAMP", "default": "now()"}
  ],
  "indexes": [
    {"name": "idx_points_user", "columns": ["user_id", "created_at"]}
  ]
}
EOF

# Modify existing: designs/public.users.table.json
# (Add loyalty_tier_id column via ScratchRobin GUI or edit JSON)

# Create test file
cat > tests/loyalty_program.yml << 'EOF'
test_suite: Loyalty Program

tests:
  - name: tier_creation
    type: unit
    sql: INSERT INTO loyalty_tiers (name, min_points) VALUES ('Gold', 1000)
    expect: success
    
  - name: points_accumulation
    type: integration
    steps:
      - setup: INSERT INTO users (user_id, email) VALUES (999, 'test@test.com')
      - test: |
          INSERT INTO loyalty_points (user_id, points, reason) 
          VALUES (999, 100, 'Purchase')
      - assert: SELECT points FROM loyalty_points WHERE user_id = 999 = 100
EOF

# Create documentation
cat > docs/loyalty-program-spec.md << 'EOF'
# Loyalty Program Specification

## Overview
Customer loyalty program with tier-based rewards.

## Database Changes
- New table: loyalty_tiers
- New table: loyalty_points
- Modified: users (add loyalty_tier_id)

## Business Rules
- Customers earn 1 point per $1 spent
- Tiers: Bronze (0), Silver (500), Gold (1000), Platinum (5000)
- Tier discounts apply automatically
EOF

# Step 4: Commit to project repo
# ======================================

git add designs/ tests/ docs/
git commit -m "feat: Add customer loyalty program

- Create loyalty_tiers table for tier management
- Create loyalty_points table for transaction tracking
- Add loyalty_tier_id to users table
- Include tests and documentation

Refs: SCR-456
"

# Step 5: Sync to database repo
# ======================================

# Generate DDL and sync
scratchrobin sync to-db --dry-run
# Review the generated SQL

# Apply to database branch
scratchrobin sync to-db

# Database repo now has:
# - schema/public/loyalty_tiers.table.sql
# - schema/public/loyalty_points.table.sql
# - migrations/0043_add_loyalty_program.sql

# Step 6: Test against database
# ======================================

scratchrobin test run --target=staging
# Or run specific tests
scratchrobin test run tests/loyalty_program.yml

# Step 7: Push both repos
# ======================================

# Push project
git push origin feature/loyalty-program

# Push database
scratchrobin db-git push origin feature/loyalty-program

# Step 8: Create pull requests
# ======================================

# Project PR (includes cross-repo links)
gh pr create --title "Add customer loyalty program" \
  --body "Cross-repo PR: Database changes at scratchbird://prod-db/feature/loyalty-program"

# Database PR (auto-created via integration)
# Or manually:
scratchrobin db-git pr create \
  --title "Add loyalty tables" \
  --from=feature/loyalty-program \
  --to=develop

# Step 9: Code review
# ======================================

# Reviewers see:
# - Design files with visual diff
# - Generated DDL
# - Migration script preview
# - Test results
# - Impact analysis

# Step 10: Merge and deploy
# ======================================

# After approval, merge both PRs
gh pr merge  # Project

# Database auto-merges or:
scratchrobin db-git merge feature/loyalty-program --into=develop

# Deploy to staging
scratchrobin deploy --plan=auto --target=staging

# Deploy to production (with approval)
scratchrobin deploy --plan=auto --target=production --approval=dba
```

### Example 2: Hotfix Production Issue

**Scenario**: Critical bug in production - need immediate fix

```bash
# Step 1: Identify the issue
# ======================================

# Production alert: Users unable to complete orders
# Investigation shows missing index on orders.status

# Step 2: Create hotfix branches
# ======================================

# In project repo - branch from production tag
git checkout -b hotfix/orders-index v1.2.0

# In database repo
scratchrobin db-git checkout -b hotfix/orders-index tags/v1.2.0

# Step 3: Implement fix
# ======================================

# Quick design change
cat > designs/emergency-index.json << 'EOF'
{
  "name": "idx_orders_status_created",
  "table": "orders",
  "schema": "public",
  "kind": "index",
  "state": "new",
  "columns": ["status", "created_at"],
  "include": ["order_id", "user_id"]
}
EOF

# Step 4: Commit with expedited review flag
# ======================================

git add .
git commit -m "hotfix: Add missing index on orders.status

Emergency fix for production performance issue.
Missing index causing table scans on order queries.

Impact: High (blocking orders)
Risk: Low (index addition only)
Testing: Verified on staging

Database-Commit: TBD
Refs: INCIDENT-789
HOTFIX: true
"

# Sync to database
scratchrobin sync to-db --force

# Step 5: Fast-track testing
# ======================================

# Quick validation
scratchrobin test run --quick

# Performance test
scratchrobin test performance --query="SELECT * FROM orders WHERE status = 'pending'"

# Step 6: Deploy with bypass
# ======================================

# Since it's an emergency, bypass normal approval
# But still log and notify
scratchrobin deploy --target=production \
  --emergency \
  --notify-team \
  --auto-rollback-on-failure

# Step 7: Verify fix
# ======================================

# Monitor query performance
scratchrobin monitor queries --table=orders --duration=5m

# Step 8: Merge back to main branches
# ======================================

# Merge hotfix to main
git checkout main
git merge hotfix/orders-index
git tag v1.2.1
git push origin main --tags

# Merge to develop
git checkout develop
git merge main
git push origin develop

# Close incident
scratchrobin incident resolve INCIDENT-789 \
  --resolution="Added index on orders.status" \
  --commit=v1.2.1
```

### Example 3: Database Refactoring

**Scenario**: Splitting monolithic users table into normalized structure

```bash
# Step 1: Create long-running refactor branch
# ======================================

scratchrobin git branch refactor/normalize-users --checkout

# Step 2: Design new schema
# ======================================

# Using ScratchRobin ERD tool:
# 1. Extract current users table structure
# 2. Design new tables:
#    - user_profiles (core identity)
#    - user_preferences (settings)
#    - user_auth (credentials)
# 3. Map data migration strategy

# Step 3: Create migration plan
# ======================================

# Migration phases for zero-downtime
cat > migrations/refactor-users.yml << 'EOF'
migration_plan: Normalize Users Table
version: "2.0.0"
phases:
  phase1_create_new:
    description: Create new tables alongside existing
    downtime: none
    steps:
      - create user_profiles
      - create user_preferences
      - create user_auth
      
  phase2_dual_write:
    description: Write to both old and new tables
    downtime: none
    steps:
      - create sync triggers
      - enable dual write
      - backfill existing data
      
  phase3_read_new:
    description: Switch reads to new tables
    downtime: brief (connection reset)
    steps:
      - update application config
      - verify reads
      
  phase4_cleanup:
    description: Remove old table and triggers
    downtime: none
    steps:
      - drop triggers
      - drop old users table (backup first)
      - rename new tables if needed
EOF

# Step 4: Generate DDL for each phase
# ======================================

# Phase 1: Create new tables
scratchrobin generate ddl --phase=1 --output=migrations/phase1.sql

# Phase 2: Sync triggers
scratchrobin generate triggers --dual-write --output=migrations/phase2_triggers.sql

# Step 5: Implement gradually
# ======================================

# Commit each phase separately

git add migrations/phase1.sql
git commit -m "refactor(users): Phase 1 - Create normalized tables

New tables:
- user_profiles (identity data)
- user_preferences (settings)
- user_auth (credentials)

Existing users table untouched.
Refs: REFACTOR-100
"

scratchrobin sync to-db
git push

# Deploy phase 1
scratchrobin deploy --phase=1 --target=staging

# Phase 2
git add migrations/phase2_triggers.sql
git commit -m "refactor(users): Phase 2 - Dual write triggers

Triggers to sync data between old and new tables.
Safe to deploy - no schema changes to existing.
"

# Continue through phases...

# Step 6: Final cleanup
# ======================================

# After all phases complete
git add migrations/phase4_cleanup.sql
git commit -m "refactor(users): Phase 4 - Cleanup

Remove old users table and sync triggers.
Migration complete.

BREAKING CHANGE: Old users table removed
Refs: REFACTOR-100
Closes: REFACTOR-100
"

# Tag release
git tag v2.0.0
git push origin v2.0.0
```

### Example 4: Multi-Developer Collaboration

**Scenario**: Two developers working on related features

```bash
# Developer A: Working on user profiles
# Developer B: Working on user authentication

# Developer A workflow:
# ======================================

git checkout -b feature/user-profiles
cat > designs/user_profiles.table.json << 'EOF'
{
  "name": "user_profiles",
  "columns": [
    {"name": "profile_id", "type": "SERIAL", "primary_key": true},
    {"name": "user_id", "type": "INTEGER", "foreign_key": "users.user_id"},
    {"name": "bio", "type": "TEXT"},
    {"name": "avatar_url", "type": "VARCHAR(500)"}
  ]
}
EOF

git commit -m "Add user_profiles table"
scratchrobin sync to-db
git push origin feature/user-profiles

# Developer B workflow:
# ======================================

git checkout -b feature/user-auth
cat > designs/user_auth.table.json << 'EOF'
{
  "name": "user_auth",
  "columns": [
    {"name": "auth_id", "type": "SERIAL", "primary_key": true},
    {"name": "user_id", "type": "INTEGER", "foreign_key": "users.user_id"},
    {"name": "password_hash", "type": "VARCHAR(255)"},
    {"name": "last_login", "type": "TIMESTAMP"}
  ]
}
EOF

git commit -m "Add user_auth table"
scratchrobin sync to-db
git push origin feature/user-auth

# Potential conflict:
# Both features add foreign key to users.user_id
# ScratchRobin detects this is compatible

# Integration testing:
# ======================================

# Create integration branch
git checkout -b integration/user-system
git merge origin/feature/user-profiles
git merge origin/feature/user-auth

scratchrobin sync to-db

# Run integration tests
scratchrobin test run --integration

# If tests pass, both features ready for develop
gh pr create --base develop --head feature/user-profiles
gh pr create --base develop --head feature/user-auth
```

### Example 5: Extract Legacy Database

**Scenario**: Importing existing production database into ScratchRobin

```bash
# Step 1: Initialize project with extraction
# ======================================

scratchrobin init \
  --name="Legacy E-commerce DB" \
  --extract-from=postgresql://prod-db:5432/ecommerce \
  --git-init \
  --sync-mode=unidirectional-db-to-project

# Step 2: Review extracted schema
# ======================================

# ScratchRobin creates:
# designs/
#   public.users.table.json (state: extracted)
#   public.orders.table.json (state: extracted)
#   public.products.table.json (state: extracted)
#   ...

# Review each table
cat designs/public.users.table.json

# Step 3: Document the baseline
# ======================================

git add .
git commit -m "feat: Initial extraction from production

Extracted from: prod-db:5432/ecommerce
Extraction timestamp: 2024-02-04T10:00:00Z
DB Version: PostgreSQL 14.5
Objects: 45 tables, 128 indexes, 23 functions

State: All objects marked 'extracted' (read-only baseline)
"

git tag v1.0.0-baseline

# Step 4: Enable bidirectional sync
# ======================================

# Edit scratchrobin.yml
sed -i 's/sync_mode: unidirectional-db-to-project/sync_mode: bidirectional/' scratchrobin.yml

git add scratchrobin.yml
git commit -m "chore: Enable bidirectional sync"

# Step 5: Start making changes
# ======================================

# Change users table state from 'extracted' to 'modified'
# Edit designs/public.users.table.json
# Add new column or modify existing

# Follow normal workflow...
```

### Example 6: Disaster Recovery

**Scenario**: Corrupted database, need to restore from Git

```bash
# Step 1: Assess the damage
# ======================================

scratchrobin db-git status
# Output: Database in inconsistent state
#         Last good commit: v1.2.3

# Step 2: Identify restore point
# ======================================

scratchrobin db-git log --oneline --last=20

# Output:
# a1b2c3d (HEAD, main) Bad migration - corrupted data
# b2c3d4e Broken intermediate state
# c3d4e5f (tag: v1.2.3) Working state - before migration
# d4e5f6a Previous commit

# Step 3: Create restore plan
# ======================================

scratchrobin db-git restore --plan --to=c3d4e5f

# Output:
# Restore plan from a1b2c3d to c3d4e5f:
# 1. Drop tables added after c3d4e5f (3 tables)
# 2. Recreate tables modified after c3d4e5f (2 tables)
# 3. Restore data from snapshots (if available)
# 4. Rollback applied migrations

# Step 4: Backup current state (even if corrupted)
# ======================================

scratchrobin db-git snapshot --name="pre-restore-corrupted-$(date +%s)"

# Step 5: Execute restore
# ======================================

# Option 1: Hard reset (destructive)
scratchrobin db-git reset --hard c3d4e5f

# Option 2: Soft restore (creates rollback migration)
scratchrobin db-git restore --to=c3d4e5f --create-rollback

# Step 6: Verify
# ======================================

scratchrobin test run --all
scratchrobin validate schema

# Step 7: Document incident
# ======================================

cat > docs/incidents/2024-02-04-data-corruption.md << 'EOF'
# Incident Report: Database Corruption

## Timeline
- 10:00 Migration 0045 applied
- 10:05 Data corruption detected
- 10:15 Restore initiated
- 10:30 Restore complete
- 10:45 Verification complete

## Root Cause
Migration 0045 contained incorrect data transformation logic.

## Resolution
Restored database to commit c3d4e5f (tag v1.2.3).
Migration 0045 rolled back and fixed.

## Prevention
- Enhanced migration testing
- Add data validation checks to migrations
EOF

git add docs/incidents/
git commit -m "docs: Add incident report for data corruption"
```

## Common Commands Reference

### Daily Development

```bash
# Start work
scratchrobin git status
scratchrobin git branch feature/my-feature --checkout

# Make changes (in ScratchRobin GUI or edit files)
# ... design, modify, test ...

# Commit
scratchrobin git commit -m "feat: Description"
scratchrobin sync to-db

# Push
scratchrobin git push

# Create PR
scratchrobin git pr create
```

### Sync Operations

```bash
# Check sync status
scratchrobin sync status

# Pull database changes to project
scratchrobin sync from-db

# Push project changes to database
scratchrobin sync to-db

# Resolve conflicts
scratchrobin sync resolve --interactive

# Force sync (use with caution)
scratchrobin sync to-db --force
```

### Branch Operations

```bash
# List branches (both repos)
scratchrobin git branch --list

# Create and checkout
scratchrobin git branch feature/x --checkout

# Merge
scratchrobin git merge feature/x --into=develop

# Delete (after merge)
scratchrobin git branch -d feature/x
```

### Testing & Deployment

```bash
# Run tests
scratchrobin test run

# Generate deployment plan
scratchrobin deploy plan --from=v1.1.0 --to=v1.2.0

# Deploy
scratchrobin deploy --target=staging
scratchrobin deploy --target=production --approval

# Rollback
scratchrobin deploy rollback --target=production
```

## Best Practices

### 1. Commit Granularity

```bash
# Good: One logical change per commit
git commit -m "feat: Add user authentication table

- Create user_auth table with hashed passwords
- Add foreign key to users table
- Include index on email for login lookups
"

# Bad: Multiple unrelated changes
git commit -m "Various updates"
# (includes auth table, product changes, bug fix)
```

### 2. Branch Naming

```bash
# Feature branches
feature/add-user-profiles
feature/loyalty-program-phase-2

# Bug fixes
fix/orders-index-performance
fix/user-login-null-pointer

# Refactoring
refactor/normalize-order-schema
refactor/extract-payment-module

# Hotfixes
hotfix/security-patch-2024-02
hotfix/critical-order-rollback
```

### 3. Commit Messages

```
Format: <type>(<scope>): <subject>

Types:
  feat:     New feature
  fix:      Bug fix
  refactor: Code refactoring
  perf:     Performance improvement
  test:     Adding tests
  docs:     Documentation
  chore:    Maintenance

Scopes:
  users, orders, products, auth, etc.

Examples:
  feat(users): Add loyalty tier tracking
  fix(orders): Correct tax calculation for EU
  refactor(products): Split inventory to separate table
```

### 4. Sync Frequency

```bash
# Recommended: Sync at key points

# 1. Before switching branches
scratchrobin sync to-db
git checkout other-branch

# 2. Before committing
git add .
scratchrobin sync check  # Verify no conflicts
git commit

# 3. Before pushing
scratchrobin sync to-db
git push

# 4. After pulling
scratchrobin sync from-db
```

### 5. Testing Strategy

```bash
# Local development
scratchrobin test run --quick

# Before commit
scratchrobin test run --unit

# Before push  
scratchrobin test run --integration

# Before PR
scratchrobin test run --all
scratchrobin test performance

# Pre-deployment
scratchrobin deploy --target=staging --dry-run
scratchrobin test run --e2e
```
