/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/bmpbndl.h>
#include <wx/artprov.h>
#include <map>
#include <vector>

#include "res/art_provider.h"

namespace scratchrobin::res {

// TreeImageList - manages scalable icons for the tree control using wxBitmapBundle
// Designed for use with wxGenericTreeCtrl which supports wxBitmapBundle natively
class TreeImageList {
public:
    TreeImageList();
    
    // Singleton accessor
    static TreeImageList& get();
    
    // Get the bitmap bundle for a given art ID
    wxBitmapBundle getBitmapBundle(const wxArtID& id) const;
    
    // Get bitmap at specific size
    wxBitmap getBitmap(const wxArtID& id, wxSize size = wxSize(16, 16)) const;
    
    // Get the image index for compatibility with wxImageList-based code
    // Returns -1 if not found
    int getImageIndex(const wxArtID& id) const;
    
    // Get all bundles as a vector (for wxGenericTreeCtrl::SetImages)
    const std::vector<wxBitmapBundle>& getAllBundles() const { return bundles_; }
    
    // Get all bundles at a specific size (for creating wxImageList)
    wxImageList* createImageList(wxSize size = wxSize(16, 16)) const;
    
    // Predefined icon indices for common objects (for compatibility)
    int getObjectIndex() const { return getImageIndex(ART_Object); }
    int getColumnIndex() const { return getImageIndex(ART_Column); }
    int getColumnPKIndex() const { return getImageIndex(ART_PrimaryKey); }
    int getColumnFKIndex() const { return getImageIndex(ART_ForeignKey); }
    int getColumnPKFKIndex() const { return getImageIndex(ART_PrimaryAndForeignKey); }
    int getComputedIndex() const { return getImageIndex(ART_Computed); }
    int getDatabaseConnectedIndex() const { return getImageIndex(ART_DatabaseConnected); }
    int getDatabaseDisconnectedIndex() const { return getImageIndex(ART_DatabaseDisconnected); }
    int getServerIndex() const { return getImageIndex(ART_Server); }
    int getTableIndex() const { return getImageIndex(ART_Table); }
    int getTablesIndex() const { return getImageIndex(ART_Tables); }
    int getViewIndex() const { return getImageIndex(ART_View); }
    int getViewsIndex() const { return getImageIndex(ART_Views); }
    int getProcedureIndex() const { return getImageIndex(ART_Procedure); }
    int getProceduresIndex() const { return getImageIndex(ART_Procedures); }
    int getFunctionIndex() const { return getImageIndex(ART_Function); }
    int getFunctionsIndex() const { return getImageIndex(ART_Functions); }
    int getGeneratorIndex() const { return getImageIndex(ART_Generator); }
    int getGeneratorsIndex() const { return getImageIndex(ART_Generators); }
    int getTriggerIndex() const { return getImageIndex(ART_Trigger); }
    int getTriggersIndex() const { return getImageIndex(ART_Triggers); }
    int getDMLTriggerIndex() const { return getImageIndex(ART_DMLTrigger); }
    int getDMLTriggersIndex() const { return getImageIndex(ART_DMLTriggers); }
    int getDBTriggerIndex() const { return getImageIndex(ART_DBTrigger); }
    int getDBTriggersIndex() const { return getImageIndex(ART_DBTriggers); }
    int getDDLTriggerIndex() const { return getImageIndex(ART_DDLTrigger); }
    int getDDLTriggersIndex() const { return getImageIndex(ART_DDLTriggers); }
    int getDomainIndex() const { return getImageIndex(ART_Domain); }
    int getDomainsIndex() const { return getImageIndex(ART_Domains); }
    int getIndexIndex() const { return getImageIndex(ART_Index); }
    int getIndicesIndex() const { return getImageIndex(ART_Indices); }
    int getPackageIndex() const { return getImageIndex(ART_Package); }
    int getPackagesIndex() const { return getImageIndex(ART_Packages); }
    int getRoleIndex() const { return getImageIndex(ART_Role); }
    int getRolesIndex() const { return getImageIndex(ART_Roles); }
    int getUserIndex() const { return getImageIndex(ART_User); }
    int getUsersIndex() const { return getImageIndex(ART_Users); }
    int getExceptionIndex() const { return getImageIndex(ART_Exception); }
    int getExceptionsIndex() const { return getImageIndex(ART_Exceptions); }
    int getUDFIndex() const { return getImageIndex(ART_UDF); }
    int getUDFsIndex() const { return getImageIndex(ART_UDFs); }
    int getFolderIndex() const { return getImageIndex(ART_Folder); }
    int getFolderOpenIndex() const { return getImageIndex(ART_FolderOpen); }
    int getInputIndex() const { return getImageIndex(ART_Input); }
    int getOutputIndex() const { return getImageIndex(ART_Output); }
    int getRootIndex() const { return getImageIndex(ART_Root); }
    int getSystemTableIndex() const { return getImageIndex(ART_SystemTable); }
    int getSystemTablesIndex() const { return getImageIndex(ART_SystemTables); }
    int getSystemDomainIndex() const { return getImageIndex(ART_SystemDomain); }
    int getSystemDomainsIndex() const { return getImageIndex(ART_SystemDomains); }
    int getSystemPackageIndex() const { return getImageIndex(ART_SystemPackage); }
    int getSystemPackagesIndex() const { return getImageIndex(ART_SystemPackages); }
    int getSystemRoleIndex() const { return getImageIndex(ART_SystemRole); }
    int getSystemRolesIndex() const { return getImageIndex(ART_SystemRoles); }
    int getSystemIndexIndex() const { return getImageIndex(ART_SystemIndex); }
    int getSystemIndicesIndex() const { return getImageIndex(ART_SystemIndices); }
    int getGlobalTemporaryIndex() const { return getImageIndex(ART_GlobalTemporary); }
    int getGlobalTemporariesIndex() const { return getImageIndex(ART_GlobalTemporaries); }
    int getCharacterSetIndex() const { return getImageIndex(ART_CharacterSet); }
    int getCharacterSetsIndex() const { return getImageIndex(ART_CharacterSets); }
    int getCollationIndex() const { return getImageIndex(ART_Collation); }
    int getCollationsIndex() const { return getImageIndex(ART_Collations); }

private:
    void addBundle(const wxArtID& art);
    
    std::map<wxArtID, int> artIdIndices_;
    std::vector<wxBitmapBundle> bundles_;
};

}  // namespace scratchrobin::res
