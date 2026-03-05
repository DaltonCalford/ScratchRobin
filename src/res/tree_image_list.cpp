/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "res/tree_image_list.h"

#include <wx/imaglist.h>
#include <wx/icon.h>

namespace scratchrobin::res {

TreeImageList::TreeImageList() {
    // Add bundles in the same order as FlameRobin's DBHTreeImageList
    // This ensures consistent icon indices
    addBundle(ART_Object);
    addBundle(ART_Column);
    addBundle(ART_CharacterSet);
    addBundle(ART_CharacterSets);
    addBundle(ART_Collation);
    addBundle(ART_Collations);
    addBundle(ART_Computed);
    addBundle(ART_DatabaseConnected);
    addBundle(ART_DatabaseDisconnected);
    addBundle(ART_DatabaseServer);
    addBundle(ART_DBTrigger);
    addBundle(ART_DBTriggers);
    addBundle(ART_DDLTrigger);
    addBundle(ART_DDLTriggers);
    addBundle(ART_DMLTrigger);
    addBundle(ART_DMLTriggers);
    addBundle(ART_Domain);
    addBundle(ART_Domains);
    addBundle(ART_Exception);
    addBundle(ART_Exceptions);
    addBundle(ART_ForeignKey);
    addBundle(ART_Function);
    addBundle(ART_Functions);
    addBundle(ART_Generator);
    addBundle(ART_Generators);
    addBundle(ART_GlobalTemporary);
    addBundle(ART_GlobalTemporaries);
    addBundle(ART_Index);
    addBundle(ART_Indices);
    addBundle(ART_Input);
    addBundle(ART_Output);
    addBundle(ART_Package);
    addBundle(ART_Packages);
    addBundle(ART_ParameterInput);
    addBundle(ART_ParameterOutput);
    addBundle(ART_PrimaryAndForeignKey);
    addBundle(ART_PrimaryKey);
    addBundle(ART_Procedure);
    addBundle(ART_Procedures);
    addBundle(ART_Role);
    addBundle(ART_Roles);
    addBundle(ART_Root);
    addBundle(ART_Server);
    addBundle(ART_SystemIndex);
    addBundle(ART_SystemIndices);
    addBundle(ART_SystemDomain);
    addBundle(ART_SystemDomains);
    addBundle(ART_SystemPackage);
    addBundle(ART_SystemPackages);
    addBundle(ART_SystemRole);
    addBundle(ART_SystemRoles);
    addBundle(ART_SystemTable);
    addBundle(ART_SystemTables);
    addBundle(ART_Table);
    addBundle(ART_Tables);
    addBundle(ART_Trigger);
    addBundle(ART_Triggers);
    addBundle(ART_UDF);
    addBundle(ART_UDFs);
    addBundle(ART_User);
    addBundle(ART_Users);
    addBundle(ART_View);
    addBundle(ART_Views);
    addBundle(ART_Folder);
    addBundle(ART_FolderOpen);
}

TreeImageList& TreeImageList::get() {
    static TreeImageList til;
    return til;
}

void TreeImageList::addBundle(const wxArtID& art) {
    wxBitmapBundle bundle = wxArtProvider::GetBitmapBundle(art, wxART_OTHER, wxSize(16, 16));
    if (bundle.IsOk()) {
        artIdIndices_[art] = static_cast<int>(bundles_.size());
        bundles_.push_back(bundle);
    } else {
        // Art ID not found, use a default empty bundle
        artIdIndices_[art] = -1;
    }
}

wxBitmapBundle TreeImageList::getBitmapBundle(const wxArtID& id) const {
    auto it = artIdIndices_.find(id);
    if (it != artIdIndices_.end() && it->second >= 0 && it->second < static_cast<int>(bundles_.size())) {
        return bundles_[it->second];
    }
    return wxBitmapBundle();
}

wxBitmap TreeImageList::getBitmap(const wxArtID& id, wxSize size) const {
    wxBitmapBundle bundle = getBitmapBundle(id);
    if (bundle.IsOk()) {
        return bundle.GetBitmap(size);
    }
    return wxNullBitmap;
}

int TreeImageList::getImageIndex(const wxArtID& id) const {
    auto it = artIdIndices_.find(id);
    if (it != artIdIndices_.end()) {
        return it->second;
    }
    return -1;
}

wxImageList* TreeImageList::createImageList(wxSize size) const {
    wxImageList* il = new wxImageList(size.GetWidth(), size.GetHeight());
    for (const auto& bundle : bundles_) {
        if (bundle.IsOk()) {
            wxBitmap bmp = bundle.GetBitmap(size);
            wxIcon icon;
            icon.CopyFromBitmap(bmp);
            il->Add(icon);
        } else {
            // Add empty icon for missing bundles
            il->Add(wxIcon());
        }
    }
    return il;
}

}  // namespace scratchrobin::res
