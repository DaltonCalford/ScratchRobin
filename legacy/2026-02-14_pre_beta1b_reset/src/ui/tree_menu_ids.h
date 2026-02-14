/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TREE_MENU_IDS_H
#define SCRATCHROBIN_TREE_MENU_IDS_H

#include <wx/defs.h>

namespace scratchrobin {

// ============================================================================
// Tree Context Menu IDs
// Range: wxID_HIGHEST + 1000 to wxID_HIGHEST + 1100
// ============================================================================

// Basic operations (1000-1009)
constexpr int ID_TREE_OPEN_EDITOR      = wxID_HIGHEST + 1000;
constexpr int ID_TREE_COPY_NAME        = wxID_HIGHEST + 1001;
constexpr int ID_TREE_COPY_DDL         = wxID_HIGHEST + 1002;
constexpr int ID_TREE_SHOW_DEPS        = wxID_HIGHEST + 1003;
constexpr int ID_TREE_REFRESH          = wxID_HIGHEST + 1004;
constexpr int ID_TREE_PROPERTIES       = wxID_HIGHEST + 1005;

// Create operations (1010-1029)
constexpr int ID_TREE_CREATE_TABLE     = wxID_HIGHEST + 1010;
constexpr int ID_TREE_CREATE_VIEW      = wxID_HIGHEST + 1011;
constexpr int ID_TREE_CREATE_PROCEDURE = wxID_HIGHEST + 1012;
constexpr int ID_TREE_CREATE_FUNCTION  = wxID_HIGHEST + 1013;
constexpr int ID_TREE_CREATE_TRIGGER   = wxID_HIGHEST + 1014;
constexpr int ID_TREE_CREATE_SEQUENCE  = wxID_HIGHEST + 1015;
constexpr int ID_TREE_CREATE_INDEX     = wxID_HIGHEST + 1016;
constexpr int ID_TREE_CREATE_DOMAIN    = wxID_HIGHEST + 1017;
constexpr int ID_TREE_CREATE_SCHEMA    = wxID_HIGHEST + 1018;
constexpr int ID_TREE_CREATE_COLUMN    = wxID_HIGHEST + 1019;

// Query operations (1030-1039)
constexpr int ID_TREE_QUERY_SELECT     = wxID_HIGHEST + 1030;
constexpr int ID_TREE_QUERY_INSERT     = wxID_HIGHEST + 1031;
constexpr int ID_TREE_QUERY_UPDATE     = wxID_HIGHEST + 1032;
constexpr int ID_TREE_QUERY_DELETE     = wxID_HIGHEST + 1033;
constexpr int ID_TREE_VIEW_DATA        = wxID_HIGHEST + 1034;

// Procedure/Function operations (1040-1049)
constexpr int ID_TREE_EXECUTE          = wxID_HIGHEST + 1040;
constexpr int ID_TREE_EXECUTE_WITH_PARAMS = wxID_HIGHEST + 1041;
constexpr int ID_TREE_DEBUG_PROCEDURE  = wxID_HIGHEST + 1042;

// Trigger operations (1050-1059)
constexpr int ID_TREE_ENABLE_TRIGGER   = wxID_HIGHEST + 1050;
constexpr int ID_TREE_DISABLE_TRIGGER  = wxID_HIGHEST + 1051;

// Sequence operations (1060-1069)
constexpr int ID_TREE_RESET_SEQUENCE   = wxID_HIGHEST + 1060;
constexpr int ID_TREE_ALTER_SEQUENCE   = wxID_HIGHEST + 1061;

// View operations (1070-1079)
constexpr int ID_TREE_REFRESH_VIEW     = wxID_HIGHEST + 1070;
constexpr int ID_TREE_ALTER_VIEW       = wxID_HIGHEST + 1071;

// Modification operations (1080-1089)
constexpr int ID_TREE_ALTER_OBJECT     = wxID_HIGHEST + 1080;
constexpr int ID_TREE_RENAME_OBJECT    = wxID_HIGHEST + 1081;
constexpr int ID_TREE_DROP_OBJECT      = wxID_HIGHEST + 1082;

// Delete modes (1090-1095)
constexpr int ID_TREE_DELETE_DIAGRAM   = wxID_HIGHEST + 1090;
constexpr int ID_TREE_DELETE_PROJECT   = wxID_HIGHEST + 1091;

// Diagram operations (1096-1099)
constexpr int ID_TREE_ADD_TO_DIAGRAM   = wxID_HIGHEST + 1096;
constexpr int ID_TREE_GENERATE_DDL     = wxID_HIGHEST + 1097;
constexpr int ID_TREE_COMPARE_DB       = wxID_HIGHEST + 1098;

} // namespace scratchrobin

#endif // SCRATCHROBIN_TREE_MENU_IDS_H
