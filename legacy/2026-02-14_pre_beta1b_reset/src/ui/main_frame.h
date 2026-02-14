/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MAIN_FRAME_H
#define SCRATCHROBIN_MAIN_FRAME_H

#include <memory>

#include <wx/frame.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/treectrl.h>

#include "connection_editor_dialog.h"
#include "core/config.h"
#include "core/metadata_model.h"
#include "diagram_model.h"
#include "form_container.h"
#include "preferences_dialog.h"
#include "layout/layout_manager.h"

class wxTextCtrl;
class wxButton;

namespace scratchrobin {

class WindowManager;
class LayoutManager;
class NavigatorPanel;
class InspectorPanel;

class MainFrame : public wxFrame, public MetadataObserver {
public:
    MainFrame(WindowManager* windowManager,
              MetadataModel* metadataModel,
              ConnectionManager* connectionManager,
              std::vector<ConnectionProfile>* connections,
              const AppConfig* appConfig);

    void OnMetadataUpdated(const MetadataSnapshot& snapshot) override;
    bool SelectMetadataPath(const std::string& path);

private:
    void BuildMenu();
    void BuildLayout();

    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnNewERDDiagram(wxCommandEvent& event);
    void OnNewDFDDiagram(wxCommandEvent& event);
    void OnNewUMLDiagram(wxCommandEvent& event);
    void OnNewMindMap(wxCommandEvent& event);
    void OnNewWhiteboard(wxCommandEvent& event);
    void OnNewDiagramOfType(DiagramType type);
    void OnQuit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTreeSelection(wxTreeEvent& event);
    void OnTreeItemMenu(wxTreeEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnFilterClear(wxCommandEvent& event);
    void OnTreeOpenEditor(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnOpenSequenceManager(wxCommandEvent& event);
    void OnOpenViewManager(wxCommandEvent& event);
    void OnOpenTriggerManager(wxCommandEvent& event);
    void OnOpenProcedureManager(wxCommandEvent& event);
    void OnOpenPackageManager(wxCommandEvent& event);
    void OnOpenStorageManager(wxCommandEvent& event);
    void OnOpenDatabaseManager(wxCommandEvent& event);
    void OnBackup(wxCommandEvent& event);
    void OnRestore(wxCommandEvent& event);
    void OnOpenClusterManager(wxCommandEvent& event);
    void OnOpenReplicationManager(wxCommandEvent& event);
    void OnOpenEtlManager(wxCommandEvent& event);
    void OnOpenGitIntegration(wxCommandEvent& event);
    void OnBackupHistory(wxCommandEvent& event);
    void OnBackupSchedule(wxCommandEvent& event);
    void OnPreferences(wxCommandEvent& event);
    void OnShortcuts(wxCommandEvent& event);
    void OnShortcutsCheatSheet(wxCommandEvent& event);
    void OnCustomizeToolbars(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);
    void OnHelpLanguage(wxCommandEvent& event);
    void OnTreeCopyName(wxCommandEvent& event);
    void OnTreeCopyDdl(wxCommandEvent& event);
    void OnTreeShowDependencies(wxCommandEvent& event);
    void OnTreeRefresh(wxCommandEvent& event);
    void OnManageConnections(wxCommandEvent& event);
    
    // Connection submenu handlers
    void OnServerCreate(wxCommandEvent& event);
    void OnServerConnect(wxCommandEvent& event);
    void OnServerDisconnect(wxCommandEvent& event);
    void OnServerDrop(wxCommandEvent& event);
    void OnServerRemove(wxCommandEvent& event);
    void OnClusterCreate(wxCommandEvent& event);
    void OnClusterConnect(wxCommandEvent& event);
    void OnClusterDisconnect(wxCommandEvent& event);
    void OnClusterDrop(wxCommandEvent& event);
    void OnClusterRemove(wxCommandEvent& event);
    void OnDatabaseCreate(wxCommandEvent& event);
    void OnDatabaseConnect(wxCommandEvent& event);
    void OnDatabaseDisconnect(wxCommandEvent& event);
    void OnDatabaseDrop(wxCommandEvent& event);
    void PopulateTree(const MetadataSnapshot& snapshot);
    void UpdateInspector(const MetadataNode* node);
    
    // Layout management integration
    void SetupLayoutManager();
    void CreateDockablePanels();
    void SwitchLayout(const std::string& preset_name);
    void SaveCurrentLayout(const std::string& name);
    
    // Layout menu handlers
    void OnLayoutDefault(wxCommandEvent& event);
    void OnLayoutSingleMonitor(wxCommandEvent& event);
    void OnLayoutDualMonitor(wxCommandEvent& event);
    void OnLayoutWideScreen(wxCommandEvent& event);
    void OnLayoutCompact(wxCommandEvent& event);
    void OnLayoutSaveCurrent(wxCommandEvent& event);
    void OnLayoutManage(wxCommandEvent& event);
    
    // =====================================================================
    // COMPREHENSIVE TREE ICON SYSTEM
    // =====================================================================
    // Maps metadata node types to visual icons in the catalog browser.
    // 
    // ICON RANGES (121 slots total, 0-120):
    //   000-009: Application & Navigation (root, connection, settings, error, 
    //             diagram, bookmark, tag)
    //   010-024: Database Objects (tables, views, columns, indexes, sequences,
    //             triggers, constraints, procedures, functions, packages,
    //             domains, collations, tablespaces)
    //   025-029: Schema Organization (database, catalog, schema, folder)
    //   030-034: Security & Admin (users, hosts, permissions, audit, history)
    //   035-044: Project Objects (projects, SQL, notes, timelines, jobs)
    //   045-049: Version Control (git repos, branches)
    //   050-059: Maintenance & Operations (backup, restore)
    //   060-069: Infrastructure (servers, clients, filespaces, networks,
    //             clusters, instances, replicas, shards)
    //   070-079: Design & Planning (whiteboards, mindmaps, designs, drafts,
    //             templates, blueprints, concepts, plans)
    //   080-089: Design States (implemented, pending, modified, deleted, new)
    //   090-099: Synchronization (sync, diff, compare, migrate, deploy)
    //   100-109: Collaboration (shared, collaboration, team)
    //   110-119: Security States (locked, unlocked)
    //   120:     Default/Unknown
    //
    // DESIGN WORKFLOW SUPPORT:
    //   This icon system supports a complete design-to-implementation workflow:
    //   1. Extract metadata from database -> icons 10-24
    //   2. Create design drafts/changes -> icons 70-79 (design, draft, concept)
    //   3. Track design state -> icons 80-89 (implemented, pending, modified)
    //   4. Collaborate on designs -> icons 100-109 (shared, team)
    //   5. Deploy changes -> icons 90-99 (sync, diff, deploy)
    //
    // ADDING NEW ICONS:
    //   1. Create SVG in resources/icons/<name>.svg
    //   2. Add to generate-pngs.sh ICONS array
    //   3. Run: cd resources/icons && ./generate-pngs.sh
    //   4. Add loadIcon() in InitializeTreeIcons() with next available index
    //   5. Add kind-to-index mapping in GetIconIndexForNode()
    //   6. Rebuild
    //
    // SUPPORTED KIND VALUES (100+ types):
    //   Database:    table, view, column, index, sequence, trigger, constraint,
    //                procedure, function, package, domain, collation, tablespace
    //   Schema:      database, catalog, schema, folder, group, category
    //   Security:    user, role, host, server, permission, grant, audit, history
    //   Project:     project, sql, script, query, note, timeline, workflow,
    //                job, task, schedule
    //   VCS:         git, repository, branch, tag, commit
    //   Infrastructure: server, client, filespace, network, cluster, instance,
    //                   replica, shard
    //   Design:      whiteboard, mindmap, design, draft, template, blueprint,
    //                concept, plan
    //   Design State: implemented, pending, modified, deleted, new
    //   Sync:        sync, diff, compare, migrate, deploy
    //   Collaboration: shared, collaboration, team
    //   Security:    lock, unlock
    //   Ops:         backup, restore
    //   System:      root, connection, settings, error, diagram, bookmark, tag
    // =====================================================================
    void InitializeTreeIcons();
    int GetIconIndexForNode(const MetadataNode& node) const;
    const MetadataNode* GetSelectedNode() const;
    bool HasMatch(const MetadataNode& node, const std::string& filter) const;
    std::string BuildSeedSql(const MetadataNode* node) const;
    bool CopyToClipboard(const std::string& text) const;

    WindowManager* window_manager_ = nullptr;
    MetadataModel* metadata_model_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    ApplicationPreferences preferences_;
    
    // Layout management
    std::unique_ptr<LayoutManager> layout_manager_;
    NavigatorPanel* navigator_panel_ = nullptr;
    InspectorPanel* inspector_panel_ = nullptr;
    class wxAuiNotebook* document_notebook_ = nullptr;
    
    // Legacy UI elements (to be migrated to dockable panels)
    wxTreeCtrl* tree_ = nullptr;
    wxImageList* tree_images_ = nullptr;
    wxTextCtrl* filter_ctrl_ = nullptr;
    wxButton* filter_clear_button_ = nullptr;
    
    // Inspector as FormContainer (replaces wxNotebook)
    ui::FormContainer* inspector_container_ = nullptr;
    
    // Legacy text controls (now as forms in container)
    wxTextCtrl* overview_text_ = nullptr;
    wxTextCtrl* ddl_text_ = nullptr;
    wxTextCtrl* deps_text_ = nullptr;
    
    const MetadataNode* context_node_ = nullptr;
    std::string filter_text_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MAIN_FRAME_H
