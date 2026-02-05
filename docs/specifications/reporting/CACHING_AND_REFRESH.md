# Caching and Refresh Policies

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines caching behavior for questions and dashboards and refresh policies for live reporting.

---

## Cache Scope

- **Question cache**: cached result set for a single question.
- **Dashboard cache**: cached rendering of all cards in a dashboard.

---

## Cache Modes

- **Off**: always query live data.
- **Time-based**: cache for N minutes.
- **Event-based**: invalidate on schema change or table update.

---

## Default Policy

- Questions: 15 minutes
- Dashboards: 30 minutes
- Alerts: bypass cache by default

---

## Invalidation Rules

- Schema changes invalidate dependent questions.
- Manual refresh always bypasses cache.
- Admins can clear cache per question or dashboard.

---

## Related Specs

- `docs/specifications/reporting/ALERTS_AND_SUBSCRIPTIONS.md`
- `docs/specifications/reporting/QUESTIONS_AND_QUERY_BUILDER.md`
