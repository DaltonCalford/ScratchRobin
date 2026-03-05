/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "res/art_provider.h"

#include <wx/image.h>
#include <wx/mstream.h>

#include "res/default_icons.h"

namespace scratchrobin::res {

// Map of art IDs to SVG data
const char* GetSVGData(const wxArtID& id) {
    if (id == ART_Object) return DEFAULT_OBJECT_SVG;
    if (id == ART_Column) return COLUMN_SVG;
    if (id == ART_CharacterSet) return CHARSET_SVG;
    if (id == ART_CharacterSets) return CHARSET_SVG;
    if (id == ART_Collation) return COLLATION_SVG;
    if (id == ART_Collations) return COLLATION_SVG;
    if (id == ART_Computed) return COLUMN_SVG;  // TODO: Custom computed column SVG
    if (id == ART_DatabaseConnected) return DATABASE_SVG;
    if (id == ART_DatabaseDisconnected) return DEFAULT_OBJECT_SVG;  // TODO: Grayed database
    if (id == ART_DatabaseServer) return ROOT_SVG;
    if (id == ART_DBTrigger) return DB_TRIGGER_SVG;
    if (id == ART_DBTriggers) return DB_TRIGGER_SVG;
    if (id == ART_DDLTrigger) return DDL_TRIGGER_SVG;
    if (id == ART_DDLTriggers) return DDL_TRIGGER_SVG;
    if (id == ART_DMLTrigger) return DML_TRIGGER_SVG;
    if (id == ART_DMLTriggers) return DML_TRIGGER_SVG;
    if (id == ART_Domain) return DOMAIN_SVG;
    if (id == ART_Domains) return FOLDER_SVG;
    if (id == ART_Exception) return EXCEPTION_SVG;
    if (id == ART_Exceptions) return FOLDER_SVG;
    if (id == ART_ForeignKey) return COLUMN_FK_SVG;
    if (id == ART_Function) return FUNCTION_SVG;
    if (id == ART_Functions) return FOLDER_SVG;
    if (id == ART_Generator) return GENERATOR_SVG;
    if (id == ART_Generators) return FOLDER_SVG;
    if (id == ART_GlobalTemporary) return GTT_SVG;
    if (id == ART_GlobalTemporaries) return FOLDER_SVG;
    if (id == ART_Index) return INDEX_SVG;
    if (id == ART_Indices) return FOLDER_SVG;
    if (id == ART_Input) return INPUT_SVG;
    if (id == ART_Output) return OUTPUT_SVG;
    if (id == ART_Package) return PACKAGE_SVG;
    if (id == ART_Packages) return FOLDER_SVG;
    if (id == ART_ParameterInput) return INPUT_SVG;
    if (id == ART_ParameterOutput) return OUTPUT_SVG;
    if (id == ART_PrimaryAndForeignKey) return COLUMN_PK_SVG;  // TODO: Combined PK+FK icon
    if (id == ART_PrimaryKey) return COLUMN_PK_SVG;
    if (id == ART_Procedure) return PROCEDURE_SVG;
    if (id == ART_Procedures) return FOLDER_SVG;
    if (id == ART_Role) return ROLE_SVG;
    if (id == ART_Roles) return FOLDER_SVG;
    if (id == ART_Root) return ROOT_SVG;
    if (id == ART_Server) return ROOT_SVG;
    if (id == ART_SystemIndex) return INDEX_SVG;  // TODO: Grayed version
    if (id == ART_SystemIndices) return FOLDER_SVG;
    if (id == ART_SystemDomain) return SYSTEM_DOMAIN_SVG;
    if (id == ART_SystemDomains) return FOLDER_SVG;
    if (id == ART_SystemPackage) return PACKAGE_SVG;  // TODO: Grayed version
    if (id == ART_SystemPackages) return FOLDER_SVG;
    if (id == ART_SystemRole) return ROLE_SVG;  // TODO: Grayed version
    if (id == ART_SystemRoles) return FOLDER_SVG;
    if (id == ART_SystemTable) return SYSTEM_TABLE_SVG;
    if (id == ART_SystemTables) return FOLDER_SVG;
    if (id == ART_Table) return TABLE_SVG;
    if (id == ART_Tables) return FOLDER_SVG;
    if (id == ART_Trigger) return TRIGGER_SVG;
    if (id == ART_Triggers) return FOLDER_SVG;
    if (id == ART_UDF) return UDF_SVG;
    if (id == ART_UDFs) return FOLDER_SVG;
    if (id == ART_User) return USER_SVG;
    if (id == ART_Users) return FOLDER_SVG;
    if (id == ART_View) return VIEW_SVG;
    if (id == ART_Views) return FOLDER_SVG;
    if (id == ART_Folder) return FOLDER_SVG;
    if (id == ART_FolderOpen) return FOLDER_OPEN_SVG;
    
    return nullptr;
}

wxBitmapBundle ArtProvider::CreateBitmapBundle(const wxArtID& id,
                                                const wxArtClient& client,
                                                const wxSize& size) {
    const char* svg_data = GetSVGData(id);
    if (svg_data) {
        // Create bitmap bundle from SVG - it will scale to any size
        return wxBitmapBundle::FromSVG(svg_data, wxSize(16, 16));
    }
    
    return wxBitmapBundle();
}

wxBitmap ArtProvider::CreateBitmap(const wxArtID& id,
                                    const wxArtClient& client,
                                    const wxSize& size) {
    wxBitmapBundle bundle = CreateBitmapBundle(id, client, size);
    if (bundle.IsOk()) {
        wxSize use_size = size;
        if (use_size == wxDefaultSize) {
            use_size = wxSize(16, 16);
        }
        return bundle.GetBitmap(use_size);
    }
    return wxNullBitmap;
}

void InitArtProvider() {
    wxArtProvider::Push(new ArtProvider);
}

}  // namespace scratchrobin::res
