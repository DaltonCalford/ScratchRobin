/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MENU_IDS_H
#define SCRATCHROBIN_MENU_IDS_H

#include <wx/defs.h>

namespace scratchrobin {

constexpr int ID_MENU_NEW_SQL_EDITOR = wxID_HIGHEST + 500;
constexpr int ID_MENU_NEW_DIAGRAM = wxID_HIGHEST + 501;
constexpr int ID_MENU_MONITORING = wxID_HIGHEST + 502;
constexpr int ID_MENU_USERS_ROLES = wxID_HIGHEST + 503;
constexpr int ID_MENU_JOB_SCHEDULER = wxID_HIGHEST + 504;
constexpr int ID_MENU_DOMAIN_MANAGER = wxID_HIGHEST + 505;
constexpr int ID_MENU_SCHEMA_MANAGER = wxID_HIGHEST + 506;
constexpr int ID_MENU_TABLE_DESIGNER = wxID_HIGHEST + 507;
constexpr int ID_MENU_INDEX_DESIGNER = wxID_HIGHEST + 508;
constexpr int ID_MENU_SEQUENCE_MANAGER = wxID_HIGHEST + 509;
constexpr int ID_MENU_VIEW_MANAGER = wxID_HIGHEST + 510;
constexpr int ID_MENU_TRIGGER_MANAGER = wxID_HIGHEST + 511;
constexpr int ID_MENU_PROCEDURE_MANAGER = wxID_HIGHEST + 512;
constexpr int ID_MENU_PACKAGE_MANAGER = wxID_HIGHEST + 513;
constexpr int ID_MENU_STORAGE_MANAGER = wxID_HIGHEST + 514;
constexpr int ID_MENU_DATABASE_MANAGER = wxID_HIGHEST + 515;
constexpr int ID_MENU_BACKUP = wxID_HIGHEST + 516;
constexpr int ID_MENU_RESTORE = wxID_HIGHEST + 517;
constexpr int ID_MENU_BACKUP_HISTORY = wxID_HIGHEST + 518;
constexpr int ID_MENU_BACKUP_SCHEDULE = wxID_HIGHEST + 519;
constexpr int ID_MENU_PREFERENCES = wxID_HIGHEST + 520;
constexpr int ID_MENU_SHORTCUTS = wxID_HIGHEST + 521;
constexpr int ID_MENU_CHEAT_SHEET = wxID_HIGHEST + 522;
constexpr int ID_MENU_HELP_WINDOW = wxID_HIGHEST + 530;
constexpr int ID_MENU_HELP_COMMAND = wxID_HIGHEST + 531;
constexpr int ID_MENU_HELP_LANGUAGE = wxID_HIGHEST + 532;

constexpr int ID_SQL_RUN = wxID_HIGHEST + 520;
constexpr int ID_SQL_CANCEL = wxID_HIGHEST + 521;
constexpr int ID_SQL_EXPORT_CSV = wxID_HIGHEST + 522;
constexpr int ID_SQL_EXPORT_JSON = wxID_HIGHEST + 523;

constexpr int ID_CONN_SERVER_CREATE = wxID_HIGHEST + 540;
constexpr int ID_CONN_SERVER_CONNECT = wxID_HIGHEST + 541;
constexpr int ID_CONN_SERVER_DISCONNECT = wxID_HIGHEST + 542;
constexpr int ID_CONN_SERVER_DROP = wxID_HIGHEST + 543;
constexpr int ID_CONN_SERVER_REMOVE = wxID_HIGHEST + 544;

constexpr int ID_CONN_CLUSTER_CREATE = wxID_HIGHEST + 550;
constexpr int ID_CONN_CLUSTER_CONNECT = wxID_HIGHEST + 551;
constexpr int ID_CONN_CLUSTER_DISCONNECT = wxID_HIGHEST + 552;
constexpr int ID_CONN_CLUSTER_DROP = wxID_HIGHEST + 553;
constexpr int ID_CONN_CLUSTER_REMOVE = wxID_HIGHEST + 554;

constexpr int ID_CONN_DATABASE_CREATE = wxID_HIGHEST + 560;
constexpr int ID_CONN_DATABASE_CONNECT = wxID_HIGHEST + 561;
constexpr int ID_CONN_DATABASE_DISCONNECT = wxID_HIGHEST + 562;
constexpr int ID_CONN_DATABASE_DROP = wxID_HIGHEST + 563;

constexpr int ID_CONN_PROJECT_CREATE = wxID_HIGHEST + 570;
constexpr int ID_CONN_PROJECT_CONNECT = wxID_HIGHEST + 571;
constexpr int ID_CONN_PROJECT_DISCONNECT = wxID_HIGHEST + 572;
constexpr int ID_CONN_PROJECT_DROP = wxID_HIGHEST + 573;

constexpr int ID_CONN_DIAGRAM_CREATE_ERD = wxID_HIGHEST + 580;
constexpr int ID_CONN_DIAGRAM_CREATE_FLOW = wxID_HIGHEST + 581;
constexpr int ID_CONN_DIAGRAM_CREATE_UML = wxID_HIGHEST + 582;
constexpr int ID_CONN_DIAGRAM_OPEN = wxID_HIGHEST + 583;
constexpr int ID_CONN_DIAGRAM_DROP = wxID_HIGHEST + 584;

constexpr int ID_CONN_GIT_CONFIGURE = wxID_HIGHEST + 590;
constexpr int ID_CONN_GIT_CONNECT = wxID_HIGHEST + 591;
constexpr int ID_CONN_GIT_OPEN = wxID_HIGHEST + 592;
constexpr int ID_CONN_GIT_STATUS = wxID_HIGHEST + 593;
constexpr int ID_CONN_GIT_PULL = wxID_HIGHEST + 594;
constexpr int ID_CONN_GIT_PUSH = wxID_HIGHEST + 595;

constexpr int ID_CONN_MANAGE = wxID_HIGHEST + 600;

// Beta Placeholder Menus (Phase 7)
constexpr int ID_MENU_CLUSTER_MANAGER = wxID_HIGHEST + 700;
constexpr int ID_MENU_REPLICATION_MANAGER = wxID_HIGHEST + 701;
constexpr int ID_MENU_ETL_MANAGER = wxID_HIGHEST + 702;
constexpr int ID_MENU_GIT_INTEGRATION = wxID_HIGHEST + 703;

} // namespace scratchrobin

#endif // SCRATCHROBIN_MENU_IDS_H
