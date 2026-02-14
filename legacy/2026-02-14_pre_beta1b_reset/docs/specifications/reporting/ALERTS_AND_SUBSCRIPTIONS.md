# Alerts and Subscriptions

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how ScratchRobin delivers live reporting via alerts and scheduled subscriptions.

---

## Alerts

An **Alert** is a condition evaluated on a question result.

Rules:

- Alerts can be based on a single numeric value or threshold.
- Alerts may run on a schedule (default hourly).
- Alerts are disabled if the underlying question changes structure.
- Alerts can be configured to notify only on state changes (pass -> fail).
- Alerts should not fire if a query returns no rows unless explicitly configured.
- Alerts are only available for questions (not entire dashboards).

Alert example:

- Trigger when `total_revenue < 10000`.

---

## Subscriptions

A **Subscription** is a scheduled delivery of a question or dashboard.

Delivery channels:

- Email (default)
- Webhook (optional)
- Chat integrations (optional, project-configured)

Rules:

- Subscriptions include rendered charts and links.
- Subscriptions inherit collection permissions.
- Dashboards can be subscribed with a specific filter state.
 - Subscriptions can attach CSV exports when allowed by policy.

---

## Scheduling

- Default schedule: daily at 08:00 (project-configurable).
- Supported schedules: hourly, daily, weekly, cron-like (optional).

---

## Governance

- Alerts and subscriptions require `Docs` permission.
- High-sensitivity collections may restrict external delivery.

---

## Related Specs

- `docs/specifications/reporting/CACHING_AND_REFRESH.md`
- `docs/specifications/reporting/COLLECTIONS_AND_PERMISSIONS.md`
