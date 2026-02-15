#include "gui/main_frame.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>

#include "core/reject.h"
#include "core/sha256.h"
#include "gui/diagram_canvas.h"

namespace scratchrobin::gui {

namespace {

std::string NowUtc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream out;
    out << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

wxString ToWx(const std::string& value) {
    return wxString::FromUTF8(value);
}

double OverlapRatio(const wxRect& a, const wxRect& b) {
    const wxRect overlap = a.Intersect(b);
    if (overlap.IsEmpty()) {
        return 0.0;
    }
    const double overlap_area = static_cast<double>(overlap.GetWidth()) * static_cast<double>(overlap.GetHeight());
    const double a_area = std::max(1.0, static_cast<double>(a.GetWidth()) * static_cast<double>(a.GetHeight()));
    return overlap_area / a_area;
}

bool SelectDiagramLinkByType(wxListCtrl* links, const std::string& type_name) {
    if (links == nullptr || type_name.empty()) {
        return false;
    }
    const long count = links->GetItemCount();
    for (long i = 0; i < count; ++i) {
        if (links->GetItemText(i, 0).ToStdString() == type_name) {
            links->SetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
            links->EnsureVisible(i);
            return true;
        }
    }
    return false;
}

std::vector<std::string> SilverstonObjectTypes() {
    return {"subject_area", "entity", "fact", "dimension", "lookup", "hub", "link", "satellite"};
}

std::vector<std::string> SilverstonDisplayModes() {
    return {"header_only", "summary", "full"};
}

std::vector<std::string> SilverstonAlignmentPolicies() {
    return {"free", "strict_grid", "column_flow"};
}

std::vector<std::string> SilverstonDropPolicies() {
    return {"containment", "containment_strict", "free"};
}

std::vector<std::string> SilverstonResizePolicies() {
    return {"free", "snap_step", "fixed_classes"};
}

std::vector<std::string> SilverstonDisplayProfiles() {
    return {"standard", "analysis", "catalog"};
}

std::vector<std::string> SilverstonPresetNames() {
    return {"standard_default", "analysis_focus", "catalog_review"};
}

const std::map<std::string, std::vector<std::string>>& SilverstonIconCatalogByType() {
    static const std::map<std::string, std::vector<std::string>> kCatalog = {
        {"subject_area", {"subject_generic", "subject_sales", "subject_finance", "subject_operations"}},
        {"entity", {"entity_generic", "entity_reference", "entity_transactional", "entity_master"}},
        {"fact", {"fact_measure", "fact_event", "fact_snapshot"}},
        {"dimension", {"dimension_time", "dimension_geo", "dimension_org"}},
        {"lookup", {"lookup_code", "lookup_status", "lookup_domain"}},
        {"hub", {"hub_business_key", "hub_master"}},
        {"link", {"link_association", "link_relationship"}},
        {"satellite", {"satellite_context", "satellite_history"}},
    };
    return kCatalog;
}

std::vector<std::string> SilverstonIconsForType(const std::string& object_type) {
    const auto& catalog = SilverstonIconCatalogByType();
    const auto it = catalog.find(ToLower(object_type));
    if (it != catalog.end()) {
        return it->second;
    }
    return {"entity_generic"};
}

std::string DefaultSilverstonIconForType(const std::string& object_type) {
    const auto options = SilverstonIconsForType(object_type);
    if (!options.empty()) {
        return options.front();
    }
    return "entity_generic";
}

bool IsSilverstonIconAllowed(const std::string& object_type, const std::string& icon_slot) {
    const std::string normalized_icon = ToLower(icon_slot);
    if (normalized_icon.empty()) {
        return false;
    }
    const auto options = SilverstonIconsForType(object_type);
    return std::find(options.begin(), options.end(), normalized_icon) != options.end();
}

std::string JoinValues(const std::vector<std::string>& values, const std::string& separator) {
    std::ostringstream out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << separator;
        }
        out << values[i];
    }
    return out.str();
}

void SelectChoiceValue(wxChoice* choice, const std::string& value) {
    if (choice == nullptr || value.empty()) {
        return;
    }
    const std::string normalized = ToLower(value);
    for (unsigned int i = 0; i < choice->GetCount(); ++i) {
        if (ToLower(choice->GetString(i).ToStdString()) == normalized) {
            choice->SetSelection(static_cast<int>(i));
            return;
        }
    }
}

void RefreshSilverstonIconPicker(wxChoice* icon_picker, const std::string& object_type, const std::string& current_icon_slot) {
    if (icon_picker == nullptr) {
        return;
    }
    icon_picker->Clear();
    const auto options = SilverstonIconsForType(object_type);
    int selected_index = wxNOT_FOUND;
    for (size_t i = 0; i < options.size(); ++i) {
        icon_picker->Append(options[i]);
        if (ToLower(options[i]) == ToLower(current_icon_slot)) {
            selected_index = static_cast<int>(i);
        }
    }
    if (selected_index == wxNOT_FOUND && !options.empty()) {
        selected_index = 0;
    }
    if (selected_index != wxNOT_FOUND) {
        icon_picker->SetSelection(selected_index);
    }
}

void UpdateSilverstonValidationHint(wxStaticText* hint, const std::string& object_type, const std::string& icon_slot) {
    if (hint == nullptr) {
        return;
    }
    const auto allowed_icons = SilverstonIconsForType(object_type);
    const bool valid = IsSilverstonIconAllowed(object_type, icon_slot);
    std::string text = "Silverston icon policy: ";
    if (valid) {
        text += "OK (" + ToLower(icon_slot) + ")";
        hint->SetForegroundColour(wxColour(32, 96, 48));
    } else {
        text += "invalid icon for type '" + ToLower(object_type) + "'; allowed: " + JoinValues(allowed_icons, ", ");
        hint->SetForegroundColour(wxColour(165, 42, 42));
    }
    hint->SetLabel(ToWx(text));
}

struct SilverstonPreset {
    std::string node_display_mode;
    int grid_size = 20;
    std::string alignment_policy;
    std::string drop_policy;
    std::string resize_policy;
    std::string display_profile;
};

bool ResolveSilverstonPreset(const std::string& preset_name, SilverstonPreset* preset) {
    if (preset == nullptr) {
        return false;
    }
    const std::string normalized = ToLower(preset_name);
    if (normalized == "analysis_focus") {
        preset->node_display_mode = "summary";
        preset->grid_size = 24;
        preset->alignment_policy = "strict_grid";
        preset->drop_policy = "containment_strict";
        preset->resize_policy = "snap_step";
        preset->display_profile = "analysis";
        return true;
    }
    if (normalized == "catalog_review") {
        preset->node_display_mode = "header_only";
        preset->grid_size = 16;
        preset->alignment_policy = "column_flow";
        preset->drop_policy = "containment";
        preset->resize_policy = "fixed_classes";
        preset->display_profile = "catalog";
        return true;
    }
    if (normalized == "standard_default") {
        preset->node_display_mode = "full";
        preset->grid_size = 20;
        preset->alignment_policy = "free";
        preset->drop_policy = "containment";
        preset->resize_policy = "free";
        preset->display_profile = "standard";
        return true;
    }
    return false;
}

std::string Trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, (last - first) + 1U);
}

std::string CanonicalDiagramType(const std::string& type_name) {
    const std::string normalized = ToLower(Trim(type_name));
    if (normalized == "erd" || normalized == "entity relationship diagram") {
        return "Erd";
    }
    if (normalized == "silverston") {
        return "Silverston";
    }
    if (normalized == "whiteboard") {
        return "Whiteboard";
    }
    if (normalized == "mind map" || normalized == "mindmap") {
        return "MindMap";
    }
    return "Erd";
}

std::string DiagramTypeDisplayName(const std::string& type_name) {
    const std::string canonical = CanonicalDiagramType(type_name);
    if (canonical == "Erd") {
        return "ERD";
    }
    if (canonical == "MindMap") {
        return "Mind Map";
    }
    return canonical;
}

std::string SlugifyDiagramName(const std::string& raw_name) {
    const std::string trimmed = Trim(raw_name);
    if (trimmed.empty()) {
        return "untitled";
    }
    std::string out;
    out.reserve(trimmed.size());
    bool last_dash = false;
    for (char ch : trimmed) {
        const unsigned char uc = static_cast<unsigned char>(ch);
        if (std::isalnum(uc)) {
            out.push_back(static_cast<char>(std::tolower(uc)));
            last_dash = false;
        } else {
            if (!last_dash) {
                out.push_back('-');
                last_dash = true;
            }
        }
    }
    while (!out.empty() && out.back() == '-') {
        out.pop_back();
    }
    return out.empty() ? "untitled" : out;
}

std::string BuildDiagramHeadingText(const std::string& type_name, const std::string& diagram_name) {
    return DiagramTypeDisplayName(type_name) + " : " + Trim(diagram_name);
}

std::vector<std::string> DefaultPaletteItemsForType(const std::string& type_name) {
    const std::string canonical = CanonicalDiagramType(type_name);
    if (canonical == "Silverston") {
        return {"subject_area", "entity", "fact", "dimension", "lookup", "hub", "link", "satellite"};
    }
    if (canonical == "Whiteboard") {
        return {"note", "task", "risk", "decision", "milestone"};
    }
    if (canonical == "MindMap") {
        return {"topic", "branch", "idea", "question", "action"};
    }
    return {"table", "view", "index", "domain", "note", "relation"};
}

int PaletteIconIndexForItem(const std::string& item_type) {
    const std::string t = ToLower(item_type);
    if (t == "note" || t == "idea" || t == "question") {
        return 1;
    }
    if (t == "relation" || t == "link" || t == "branch") {
        return 2;
    }
    if (t == "task" || t == "action" || t == "milestone") {
        return 3;
    }
    return 0;
}

wxString DefaultDiagramNameForType(const std::string& type_name) {
    const std::string canonical = CanonicalDiagramType(type_name);
    if (canonical == "Silverston") {
        return "New Silverston Subject Area";
    }
    if (canonical == "Whiteboard") {
        return "New Whiteboard";
    }
    if (canonical == "MindMap") {
        return "New Mind Map";
    }
    return "New ERD";
}

beta1b::DiagramDocument BuildSampleDiagram(const std::string& diagram_id, const std::string& type_name) {
    beta1b::DiagramDocument doc;
    doc.diagram_id = diagram_id;
    const std::string diagram_type = type_name.empty() ? "Erd" : type_name;
    doc.diagram_type = diagram_type;
    const std::string lowered = ToLower(diagram_type);

    if (lowered == "silverston") {
        doc.notation = "idef1x";
        doc.nodes.push_back({"n1", "subject_area", "", 40, 36, 260, 130, "N/A", "Party", {"PartyId", "PartyType"},
                             "Silverston subject area", {"silverston", "subject_area"}, {"catalog:party"},
                             "subject_generic", "full", false, true, false, 2});
        doc.nodes.push_back({"n2", "entity", "n1", 360, 24, 250, 120, "BIGINT", "Person", {"PersonId", "PartyId"},
                             "Entity under Party", {"silverston", "entity"}, {"catalog:person"},
                             "entity_generic", "full", false, false, false, 1});
        doc.nodes.push_back({"n3", "entity", "n1", 360, 194, 250, 120, "BIGINT", "Organization", {"OrgId", "PartyId"},
                             "Entity under Party", {"silverston", "entity"}, {"catalog:organization"},
                             "entity_generic", "full", false, false, true, 3});
        doc.edges.push_back({"e1", "n1", "n2", "is_a", "Party->Person", "is_a", true, true, "1", "N"});
        doc.edges.push_back({"e2", "n1", "n3", "is_a", "Party->Organization", "is_a", true, true, "1", "N"});
        return doc;
    }
    if (lowered == "whiteboard") {
        doc.notation = "uml";
        doc.nodes.push_back({"n1", "note", "", 60, 34, 300, 130, "N/A", "Migration Goal",
                             {"Zero downtime", "No data loss"}, "Roll out emulation with phased cutover",
                             {"whiteboard", "planning"}, {"workitem:goal"},
                             "", "full", false, true, false, 1});
        doc.nodes.push_back({"n2", "task", "", 410, 72, 280, 120, "N/A", "Cutover Checklist",
                             {"Shadow traffic", "Compare deltas"}, "Operational readiness checks",
                             {"whiteboard", "task"}, {"workitem:cutover"},
                             "", "full", false, false, false, 1});
        doc.nodes.push_back({"n3", "risk", "", 250, 240, 300, 110, "N/A", "Rollback Plan",
                             {"Trigger threshold", "Owner on-call"}, "Fallback steps and SLA impact",
                             {"whiteboard", "risk"}, {"workitem:rollback"},
                             "", "full", false, false, false, 1});
        doc.edges.push_back({"e1", "n1", "n2", "next", "next", "flow", true, false, "", ""});
        doc.edges.push_back({"e2", "n2", "n3", "depends", "depends", "flow", true, false, "", ""});
        return doc;
    }
    if (lowered == "mindmap") {
        doc.notation = "uml";
        doc.nodes.push_back({"n1", "topic", "", 340, 26, 280, 110, "N/A", "ScratchRobin Beta1b",
                             {"Spec lock", "QA closure"}, "Implementation focus map",
                             {"mindmap", "root"}, {"spec:beta1b"},
                             "", "full", false, true, false, 1});
        doc.nodes.push_back({"n2", "topic", "n1", 60, 180, 240, 100, "N/A", "Diagramming",
                             {"Canvas", "Export", "Reverse"}, "Visual modeling track",
                             {"mindmap", "branch"}, {"spec:diagramming"},
                             "", "full", true, false, false, 3});
        doc.nodes.push_back({"n3", "topic", "n1", 380, 196, 240, 100, "N/A", "Reporting",
                             {"Runtime", "Repository", "Cache"}, "Analytics track",
                             {"mindmap", "branch"}, {"spec:reporting"},
                             "", "full", false, false, false, 2});
        doc.nodes.push_back({"n4", "topic", "n1", 700, 180, 240, 100, "N/A", "Admin Surfaces",
                             {"CDC", "Masking", "Git"}, "Operations track",
                             {"mindmap", "branch"}, {"spec:ui"},
                             "", "full", false, false, false, 2});
        doc.nodes.push_back({"n5", "topic", "n3", 390, 332, 240, 95, "N/A", "Conformance",
                             {"RPT-001..010"}, "Case depth hardening",
                             {"mindmap", "leaf"}, {"spec:conformance"},
                             "", "full", false, false, false, 1});
        doc.edges.push_back({"e1", "n1", "n2", "branch", "branch", "branch", true, false, "", ""});
        doc.edges.push_back({"e2", "n1", "n3", "branch", "branch", "branch", true, false, "", ""});
        doc.edges.push_back({"e3", "n1", "n4", "branch", "branch", "branch", true, false, "", ""});
        doc.edges.push_back({"e4", "n3", "n5", "branch", "branch", "branch", true, false, "", ""});
        return doc;
    }

    doc.notation = "crowsfoot";
    doc.diagram_type = "Erd";
    doc.nodes.push_back({"n1", "table", "", 20, 20, 220, 120, "VARCHAR", "customer", {"id BIGINT", "name VARCHAR(120)"},
                         "Core customer table", {"core", "subject:customer"}, {"catalog:public.customer"},
                         "", "full", false, false, false, 1});
    doc.nodes.push_back({"n2", "table", "", 320, 20, 220, 120, "INT", "orders", {"id BIGINT", "customer_id BIGINT"},
                         "Orders fact table", {"core", "subject:order"}, {"catalog:public.orders"},
                         "", "full", false, false, false, 1});
    doc.nodes.push_back({"n3", "view", "", 180, 220, 240, 100, "TIMESTAMP", "customer_summary", {"customer_id BIGINT"},
                         "Aggregation view", {"analytics"}, {"catalog:public.customer_summary"},
                         "", "full", false, false, false, 1});
    doc.edges.push_back({"e1", "n1", "n2", "fk", "customer->orders", "fk", true, true, "1", "N"});
    doc.edges.push_back({"e2", "n2", "n3", "projection", "orders->summary", "projection", true, false, "N", "1"});
    return doc;
}

}  // namespace

MainFrame::MainFrame(const runtime::StartupReport& report, const std::filesystem::path& repo_root)
    : wxFrame(nullptr, wxID_ANY, "ScratchRobin Beta1b Workbench", wxDefaultPosition, wxSize(1600, 980)),
      startup_report_(report),
      repo_root_(repo_root),
      ui_service_(&adapter_, &specset_service_),
      reporting_service_(&adapter_) {
    SetMinSize(wxSize(1200, 760));
    LoadProfiles();
    BuildMenu();
    BuildToolbar();
    BuildLayout();
    CreateStatusBar(2);
    SetStatusText("Workbench ready", 0);
    SetStatusText("Disconnected", 1);
    SeedUiState();
    CentreOnScreen();
}

void MainFrame::LoadProfiles() {
    const auto app_path = (repo_root_ / "config/scratchrobin.toml").string();
    const auto app_example_path = (repo_root_ / "config/scratchrobin.toml.example").string();
    const auto connections_path = (repo_root_ / "config/connections.toml").string();
    const auto connections_example_path = (repo_root_ / "config/connections.toml.example").string();

    const auto config = config_store_.LoadWithFallback(app_path, app_example_path, connections_path, connections_example_path);
    profiles_ = config.connections;
    if (profiles_.empty()) {
        runtime::ConnectionProfile fallback;
        fallback.name = "offline_mock";
        fallback.backend = "mock";
        fallback.mode = runtime::ConnectionMode::Network;
        fallback.host = "127.0.0.1";
        fallback.port = 0;
        fallback.database = "scratchrobin";
        fallback.username = "tester";
        fallback.credential_id = "none";
        profiles_.push_back(std::move(fallback));
    }

    const auto reporting_root = (repo_root_ / "work/reporting").string();
    reporting_service_.SetPersistenceRoot(reporting_root);
    reporting_service_.LoadPersistentState();
}

void MainFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();

    std::map<std::string, wxMenu*> menus;
    for (const auto& title : ui_service_.MainMenuTopology()) {
        auto* menu = new wxMenu();
        menus[title] = menu;
        menu_bar->Append(menu, ToWx(title));
    }

    if (menus.count("Connections") > 0U) {
        menus["Connections"]->Append(kCmdConnect, "Connect Selected\tCtrl+L", "Connect active profile");
        menus["Connections"]->Append(kCmdDisconnect, "Disconnect\tCtrl+Shift+L", "Disconnect active profile");
    }

    if (menus.count("Objects") > 0U) {
        menus["Objects"]->Append(kCmdOpenSqlEditorFrame, "Open SQL Editor", "Open SQL editor surface");
        menus["Objects"]->Append(kCmdOpenObjectEditorFrame, "Open Object Editor", "Open object editor surface");
        menus["Objects"]->Append(kCmdOpenDiagramFrame, "Open Diagram Surface", "Open diagram surface");
        menus["Objects"]->Append(kCmdOpenMonitoringFrame, "Open Monitoring Surface", "Open monitoring surface");
    }

    if (menus.count("Tools") > 0U) {
        for (const auto& item : ui_service_.ToolsMenu()) {
            if (item.second == "open_spec_workspace") {
                menus["Tools"]->Append(kCmdOpenSpecWorkspaceFrame, ToWx(item.first), "Open spec workspace surface");
            } else if (item.second == "open_reporting") {
                menus["Tools"]->Append(kCmdOpenReportingFrame, ToWx(item.first), "Open reporting surface");
            } else if (item.second == "open_data_masking") {
                menus["Tools"]->Append(kCmdOpenDataMaskingFrame, ToWx(item.first), "Open data masking surface");
            }
        }
        menus["Tools"]->AppendSeparator();
        menus["Tools"]->Append(kCmdOpenCdcConfigFrame, "CDC Config", "Open CDC configuration surface");
        menus["Tools"]->Append(kCmdOpenGitIntegrationFrame, "Git Integration", "Open Git integration surface");
    }

    if (menus.count("Admin") > 0U) {
        menus["Admin"]->Append(kCmdOpenSchemaManagerFrame, "Schema Manager", "Open SchemaManagerFrame");
        menus["Admin"]->Append(kCmdOpenTableManagerFrame, "Table Manager", "Open TableDesignerFrame");
        menus["Admin"]->Append(kCmdOpenIndexManagerFrame, "Index Manager", "Open IndexDesignerFrame");
        menus["Admin"]->Append(kCmdOpenDomainManagerFrame, "Domain Manager", "Open DomainManagerFrame");
        menus["Admin"]->Append(kCmdOpenSequenceManagerFrame, "Sequence Manager", "Open SequenceManagerFrame");
        menus["Admin"]->Append(kCmdOpenViewManagerFrame, "View Manager", "Open ViewManagerFrame");
        menus["Admin"]->Append(kCmdOpenTriggerManagerFrame, "Trigger Manager", "Open TriggerManagerFrame");
        menus["Admin"]->Append(kCmdOpenProcedureManagerFrame, "Procedure Manager", "Open ProcedureManagerFrame");
        menus["Admin"]->Append(kCmdOpenPackageManagerFrame, "Package Manager", "Open PackageManagerFrame");
        menus["Admin"]->Append(kCmdOpenUsersManagerFrame, "Users/Roles Manager", "Open UsersRolesFrame");
        menus["Admin"]->Append(kCmdOpenJobsManagerFrame, "Jobs Manager", "Open JobSchedulerFrame");
        menus["Admin"]->Append(kCmdOpenStorageManagerFrame, "Storage Manager", "Open StorageManagerFrame");
        menus["Admin"]->Append(kCmdOpenBackupManagerFrame, "Backup Manager", "Open BackupManagerFrame");
    }

    if (menus.count("Window") > 0U) {
        menus["Window"]->Append(kCmdOpenSqlEditorFrame, "SQL Editor", "Open/focus SQL editor window");
        menus["Window"]->Append(kCmdOpenObjectEditorFrame, "Object Editor", "Open/focus object editor window");
        menus["Window"]->Append(kCmdOpenDiagramFrame, "Diagram", "Open/focus diagram window");
        menus["Window"]->Append(kCmdOpenSpecWorkspaceFrame, "Spec Workspace", "Open/focus spec workspace window");
        menus["Window"]->Append(kCmdOpenMonitoringFrame, "Monitoring", "Open/focus monitoring window");
        menus["Window"]->Append(kCmdOpenReportingFrame, "Reporting", "Open/focus reporting window");
        menus["Window"]->Append(kCmdOpenDataMaskingFrame, "Data Masking", "Open/focus data masking window");
        menus["Window"]->Append(kCmdOpenCdcConfigFrame, "CDC Config", "Open/focus CDC configuration window");
        menus["Window"]->Append(kCmdOpenGitIntegrationFrame, "Git Integration", "Open/focus Git integration window");
        menus["Window"]->AppendSeparator();
        menus["Window"]->Append(kCmdOpenSchemaManagerFrame, "Schema Manager", "Open/focus SchemaManagerFrame");
        menus["Window"]->Append(kCmdOpenTableManagerFrame, "Table Manager", "Open/focus TableDesignerFrame");
        menus["Window"]->Append(kCmdOpenIndexManagerFrame, "Index Manager", "Open/focus IndexDesignerFrame");
        menus["Window"]->Append(kCmdOpenDomainManagerFrame, "Domain Manager", "Open/focus DomainManagerFrame");
        menus["Window"]->Append(kCmdOpenSequenceManagerFrame, "Sequence Manager", "Open/focus SequenceManagerFrame");
        menus["Window"]->Append(kCmdOpenViewManagerFrame, "View Manager", "Open/focus ViewManagerFrame");
        menus["Window"]->Append(kCmdOpenTriggerManagerFrame, "Trigger Manager", "Open/focus TriggerManagerFrame");
        menus["Window"]->Append(kCmdOpenProcedureManagerFrame, "Procedure Manager", "Open/focus ProcedureManagerFrame");
        menus["Window"]->Append(kCmdOpenPackageManagerFrame, "Package Manager", "Open/focus PackageManagerFrame");
        menus["Window"]->Append(kCmdOpenUsersManagerFrame, "Users/Roles Manager", "Open/focus UsersRolesFrame");
        menus["Window"]->Append(kCmdOpenJobsManagerFrame, "Jobs Manager", "Open/focus JobSchedulerFrame");
        menus["Window"]->Append(kCmdOpenStorageManagerFrame, "Storage Manager", "Open/focus StorageManagerFrame");
        menus["Window"]->Append(kCmdOpenBackupManagerFrame, "Backup Manager", "Open/focus BackupManagerFrame");
        menus["Window"]->AppendSeparator();
        menus["Window"]->Append(wxID_EXIT, "Exit", "Close ScratchRobin");
    }

    if (menus.count("Help") > 0U) {
        menus["Help"]->Append(wxID_ABOUT, "About", "About ScratchRobin");
    }

    SetMenuBar(menu_bar);

    Bind(wxEVT_MENU, &MainFrame::OnConnect, this, kCmdConnect);
    Bind(wxEVT_MENU, &MainFrame::OnDisconnect, this, kCmdDisconnect);
    Bind(wxEVT_MENU, &MainFrame::OnRunSql, this, kCmdRunSql);
    Bind(wxEVT_MENU, &MainFrame::OnCancelSql, this, kCmdCancelSql);
    Bind(wxEVT_MENU, &MainFrame::OnExportHistoryCsv, this, kCmdExportHistoryCsv);
    Bind(wxEVT_MENU, &MainFrame::OnSaveObject, this, kCmdSaveObject);
    Bind(wxEVT_MENU, &MainFrame::OnGenerateMigration, this, kCmdGenerateMigration);
    Bind(wxEVT_MENU, &MainFrame::OnOpenDiagramLink, this, kCmdOpenDiagramLink);
    Bind(wxEVT_MENU, &MainFrame::OnExportDiagramSvg, this, kCmdExportDiagramSvg);
    Bind(wxEVT_MENU, &MainFrame::OnExportDiagramPng, this, kCmdExportDiagramPng);
    Bind(wxEVT_MENU, &MainFrame::OnRefreshSpecWorkspace, this, kCmdRefreshSpecWorkspace);
    Bind(wxEVT_MENU, &MainFrame::OnRefreshMonitoring, this, kCmdRefreshMonitoring);
    Bind(wxEVT_MENU, &MainFrame::OnOpenSqlEditorFrame, this, kCmdOpenSqlEditorFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenObjectEditorFrame, this, kCmdOpenObjectEditorFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenDiagramFrame, this, kCmdOpenDiagramFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenMonitoringFrame, this, kCmdOpenMonitoringFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenReportingFrame, this, kCmdOpenReportingFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenDataMaskingFrame, this, kCmdOpenDataMaskingFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenCdcConfigFrame, this, kCmdOpenCdcConfigFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenGitIntegrationFrame, this, kCmdOpenGitIntegrationFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenSpecWorkspaceFrame, this, kCmdOpenSpecWorkspaceFrame);
    Bind(wxEVT_MENU, &MainFrame::OnOpenAdminManager, this, kCmdOpenSchemaManagerFrame, kCmdOpenBackupManagerFrame);
    Bind(wxEVT_MENU, &MainFrame::OnTreeCopyObjectName, this, kCmdTreeCopyObjectName);
    Bind(wxEVT_MENU, &MainFrame::OnTreeCopyDdl, this, kCmdTreeCopyDdl);
    Bind(wxEVT_MENU, &MainFrame::OnTreeShowDependencies, this, kCmdTreeShowDependencies);
    Bind(wxEVT_MENU, &MainFrame::OnTreeRefreshNode, this, kCmdTreeRefreshNode);
    Bind(wxEVT_MENU, &MainFrame::OnExitMenu, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnAboutMenu, this, wxID_ABOUT);
}

void MainFrame::BuildToolbar() {
    auto* toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT | wxTB_FLAT);
    toolbar->AddTool(kCmdConnect, "Connect", wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_TOOLBAR));
    toolbar->AddTool(kCmdDisconnect, "Disconnect", wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_TOOLBAR));
    toolbar->AddSeparator();
    toolbar->AddTool(kCmdRunSql, "Run SQL", wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR));
    toolbar->AddTool(kCmdCancelSql, "Cancel", wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR));
    toolbar->AddSeparator();
    toolbar->AddTool(kCmdOpenSqlEditorFrame, "SQL Editor", wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_TOOLBAR));
    toolbar->AddTool(kCmdOpenObjectEditorFrame,
                     "Object Editor",
                     wxArtProvider::GetBitmap(wxART_NEW_DIR, wxART_TOOLBAR));
    toolbar->AddTool(kCmdOpenDiagramFrame, "Diagram", wxArtProvider::GetBitmap(wxART_REPORT_VIEW, wxART_TOOLBAR));
    toolbar->AddTool(kCmdOpenSpecWorkspaceFrame, "Spec Workspace", wxArtProvider::GetBitmap(wxART_LIST_VIEW, wxART_TOOLBAR));
    toolbar->AddTool(kCmdOpenMonitoringFrame, "Monitoring", wxArtProvider::GetBitmap(wxART_TIP, wxART_TOOLBAR));
    toolbar->AddTool(kCmdOpenReportingFrame, "Reporting", wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR));
    toolbar->Realize();
}

void MainFrame::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* connection_panel = new wxPanel(this, wxID_ANY);
    auto* connection_sizer = new wxBoxSizer(wxHORIZONTAL);
    connection_sizer->Add(new wxStaticText(connection_panel, wxID_ANY, "Profile"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);

    profile_choice_ = new wxChoice(connection_panel, wxID_ANY);
    for (const auto& profile : profiles_) {
        profile_choice_->Append(ToWx(profile.name));
    }
    if (!profiles_.empty()) {
        profile_choice_->SetSelection(0);
    }
    connection_sizer->Add(profile_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    connection_sizer->Add(new wxButton(connection_panel, kCmdConnect, "Connect"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connection_sizer->Add(new wxButton(connection_panel, kCmdDisconnect, "Disconnect"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    connection_sizer->Add(new wxStaticText(connection_panel, wxID_ANY,
                                           "Workbench + detached windows: SQL editor, object editor, diagrams, spec workspace, monitoring."),
                         0,
                         wxALIGN_CENTER_VERTICAL | wxRIGHT,
                         8);
    connection_panel->SetSizer(connection_sizer);
    root->Add(connection_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetSashGravity(0.20);
    splitter->SetMinimumPaneSize(240);

    auto* tree_panel = new wxPanel(splitter, wxID_ANY);
    auto* tree_sizer = new wxBoxSizer(wxVERTICAL);
    tree_sizer->Add(new wxStaticText(tree_panel, wxID_ANY, "Catalog / Surfaces"), 0, wxALL, 6);
    tree_ = new wxTreeCtrl(tree_panel,
                           wxID_ANY,
                           wxDefaultPosition,
                           wxDefaultSize,
                           wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT);
    tree_sizer->Add(tree_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    tree_panel->SetSizer(tree_sizer);

    auto* workspace_panel = new wxPanel(splitter, wxID_ANY);
    auto* workspace_sizer = new wxBoxSizer(wxVERTICAL);
    workspace_notebook_ = new wxNotebook(workspace_panel, wxID_ANY);
    workspace_notebook_->AddPage(BuildSqlEditorTab(workspace_notebook_), "SQL Editor", true);
    workspace_notebook_->AddPage(BuildObjectEditorTab(workspace_notebook_), "Object Editor", false);
    workspace_notebook_->AddPage(BuildDiagramTab(workspace_notebook_), "Diagrams", false);
    workspace_notebook_->AddPage(BuildPlanTab(workspace_notebook_), "Plan", false);
    workspace_notebook_->AddPage(BuildSpecWorkspaceTab(workspace_notebook_), "Spec Workspace", false);
    workspace_notebook_->AddPage(BuildMonitoringTab(workspace_notebook_), "Monitoring", false);
    workspace_sizer->Add(workspace_notebook_, 1, wxEXPAND | wxALL, 6);
    workspace_panel->SetSizer(workspace_sizer);

    splitter->SplitVertically(tree_panel, workspace_panel, 310);
    root->Add(splitter, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    auto* log_panel = new wxPanel(this, wxID_ANY);
    auto* log_sizer = new wxBoxSizer(wxVERTICAL);
    log_sizer->Add(new wxStaticText(log_panel, wxID_ANY, "Activity Log"), 0, wxLEFT | wxTOP | wxRIGHT, 6);
    log_output_ = new wxTextCtrl(log_panel,
                                 wxID_ANY,
                                 "",
                                 wxDefaultPosition,
                                 wxSize(-1, 120),
                                 wxTE_MULTILINE | wxTE_READONLY);
    log_sizer->Add(log_output_, 1, wxEXPAND | wxALL, 6);
    log_panel->SetSizer(log_sizer);
    root->Add(log_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    SetSizer(root);

    tree_->Bind(wxEVT_TREE_ITEM_MENU, &MainFrame::OnTreeContext, this);
    tree_->Bind(wxEVT_TREE_ITEM_ACTIVATED, &MainFrame::OnTreeActivate, this);
    workspace_notebook_->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &MainFrame::OnWorkspaceNotebookPageChanged, this);
}

wxPanel* MainFrame::BuildSqlEditorTab(wxWindow* parent) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "SQL Editor"), 0, wxLEFT | wxTOP | wxRIGHT, 8);
    sql_editor_ = new wxTextCtrl(panel,
                                 wxID_ANY,
                                 "SELECT id, name FROM customer WHERE active = 1 ORDER BY name LIMIT 25;",
                                 wxDefaultPosition,
                                 wxSize(-1, 160),
                                 wxTE_MULTILINE);
    sizer->Add(sql_editor_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* button_row = new wxBoxSizer(wxHORIZONTAL);
    button_row->Add(new wxButton(panel, kCmdRunSql, "Run SQL"), 0, wxRIGHT, 6);
    button_row->Add(new wxButton(panel, kCmdCancelSql, "Cancel"), 0, wxRIGHT, 6);
    button_row->Add(new wxButton(panel, kCmdExportHistoryCsv, "Export History CSV"), 0, wxRIGHT, 6);
    sizer->Add(button_row, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    sql_results_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 140), wxLC_REPORT | wxLC_SINGLE_SEL);
    sql_results_->InsertColumn(0, "Command", wxLIST_FORMAT_LEFT, 180);
    sql_results_->InsertColumn(1, "Rows", wxLIST_FORMAT_LEFT, 90);
    sql_results_->InsertColumn(2, "Message", wxLIST_FORMAT_LEFT, 420);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Results"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(sql_results_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    sql_status_ = new wxTextCtrl(panel,
                                 wxID_ANY,
                                 "",
                                 wxDefaultPosition,
                                 wxSize(-1, 90),
                                 wxTE_MULTILINE | wxTE_READONLY);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Snapshot"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(sql_status_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    sql_history_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    sql_history_->InsertColumn(0, "query_id", wxLIST_FORMAT_LEFT, 110);
    sql_history_->InsertColumn(1, "profile", wxLIST_FORMAT_LEFT, 130);
    sql_history_->InsertColumn(2, "started_at_utc", wxLIST_FORMAT_LEFT, 170);
    sql_history_->InsertColumn(3, "status", wxLIST_FORMAT_LEFT, 100);
    sql_history_->InsertColumn(4, "sql_hash", wxLIST_FORMAT_LEFT, 320);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Query History"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(sql_history_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::BuildObjectEditorTab(wxWindow* parent) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(panel, wxID_ANY, "Class"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    object_class_ = new wxChoice(panel, wxID_ANY);
    object_class_->Append("TABLE");
    object_class_->Append("VIEW");
    object_class_->Append("INDEX");
    object_class_->Append("TRIGGER");
    object_class_->Append("PROCEDURE");
    object_class_->SetSelection(0);
    row->Add(object_class_, 0, wxRIGHT, 12);

    row->Add(new wxStaticText(panel, wxID_ANY, "Path"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    object_path_ = new wxTextCtrl(panel, wxID_ANY, "public.customer");
    row->Add(object_path_, 1, wxRIGHT, 8);

    row->Add(new wxButton(panel, kCmdSaveObject, "Save Object"), 0, wxRIGHT, 6);
    row->Add(new wxButton(panel, kCmdGenerateMigration, "Generate Migration"), 0, wxRIGHT, 6);
    sizer->Add(row, 0, wxEXPAND | wxALL, 8);

    object_ddl_ = new wxTextCtrl(panel,
                                 wxID_ANY,
                                 "CREATE TABLE public.customer (\n"
                                 "  id BIGINT PRIMARY KEY,\n"
                                 "  name VARCHAR(120) NOT NULL,\n"
                                 "  active BOOLEAN NOT NULL\n"
                                 ");",
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 wxTE_MULTILINE);
    sizer->Add(object_ddl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::BuildDiagramTab(wxWindow* parent) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    diagram_links_ = nullptr;
    diagram_heading_ = new wxStaticText(panel, wxID_ANY, "ERD : Core Domain ERD");
    auto title_font = diagram_heading_->GetFont();
    title_font.MakeBold();
    title_font.SetPointSize(title_font.GetPointSize() + 1);
    diagram_heading_->SetFont(title_font);
    sizer->Add(diagram_heading_, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    diagram_type_choice_ = new wxChoice(panel, wxID_ANY);
    diagram_type_choice_->Append("ERD");
    diagram_type_choice_->Append("Silverston");
    diagram_type_choice_->Append("Whiteboard");
    diagram_type_choice_->Append("Mind Map");
    diagram_type_choice_->SetSelection(0);
    diagram_name_input_ = new wxTextCtrl(panel, wxID_ANY, "Core Domain ERD");
    auto* new_diagram_btn = new wxButton(panel, wxID_ANY, "New Diagram");
    row->Add(new wxStaticText(panel, wxID_ANY, "Type"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row->Add(diagram_type_choice_, 0, wxRIGHT, 8);
    row->Add(new wxStaticText(panel, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row->Add(diagram_name_input_, 1, wxRIGHT, 8);
    row->Add(new_diagram_btn, 0, wxRIGHT, 8);
    row->Add(new wxButton(panel, kCmdExportDiagramSvg, "Export SVG"), 0, wxRIGHT, 6);
    row->Add(new wxButton(panel, kCmdExportDiagramPng, "Export PNG"), 0, wxRIGHT, 6);
    sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* tools = new wxBoxSizer(wxHORIZONTAL);
    auto* nudge_left_btn = new wxButton(panel, wxID_ANY, "<");
    auto* nudge_up_btn = new wxButton(panel, wxID_ANY, "^");
    auto* nudge_down_btn = new wxButton(panel, wxID_ANY, "v");
    auto* nudge_right_btn = new wxButton(panel, wxID_ANY, ">");
    auto* resize_btn = new wxButton(panel, wxID_ANY, "Resize +");
    auto* connect_btn = new wxButton(panel, wxID_ANY, "Connect Next");
    auto* reparent_btn = new wxButton(panel, wxID_ANY, "Reparent");
    auto* add_node_btn = new wxButton(panel, wxID_ANY, "Add Node");
    auto* delete_btn = new wxButton(panel, wxID_ANY, "Delete Node");
    auto* delete_project_btn = new wxButton(panel, wxID_ANY, "Delete Project");
    auto* undo_btn = new wxButton(panel, wxID_ANY, "Undo");
    auto* redo_btn = new wxButton(panel, wxID_ANY, "Redo");
    auto* zoom_in_btn = new wxButton(panel, wxID_ANY, "Zoom +");
    auto* zoom_out_btn = new wxButton(panel, wxID_ANY, "Zoom -");
    auto* zoom_reset_btn = new wxButton(panel, wxID_ANY, "Zoom 100%");
    diagram_grid_toggle_ = new wxCheckBox(panel, wxID_ANY, "Grid");
    diagram_grid_toggle_->SetValue(true);
    diagram_snap_toggle_ = new wxCheckBox(panel, wxID_ANY, "Snap");
    auto* silverston_type = new wxChoice(panel, wxID_ANY);
    for (const auto& type : SilverstonObjectTypes()) {
        silverston_type->Append(type);
    }
    silverston_type->SetSelection(1);
    auto* silverston_icon_catalog = new wxChoice(panel, wxID_ANY);
    auto* silverston_icon_slot = new wxTextCtrl(panel, wxID_ANY, DefaultSilverstonIconForType("entity"));
    RefreshSilverstonIconPicker(silverston_icon_catalog, "entity", silverston_icon_slot->GetValue().ToStdString());
    auto* silverston_display_mode = new wxChoice(panel, wxID_ANY);
    for (const auto& mode : SilverstonDisplayModes()) {
        silverston_display_mode->Append(mode);
    }
    silverston_display_mode->SetSelection(2);
    auto* silverston_chamfer = new wxCheckBox(panel, wxID_ANY, "Chamfer Notes");
    auto* silverston_apply_node = new wxButton(panel, wxID_ANY, "Apply Node Profile");
    auto* silverston_grid_size = new wxTextCtrl(panel, wxID_ANY, "20");
    auto* silverston_alignment = new wxChoice(panel, wxID_ANY);
    for (const auto& policy : SilverstonAlignmentPolicies()) {
        silverston_alignment->Append(policy);
    }
    silverston_alignment->SetSelection(0);
    auto* silverston_drop = new wxChoice(panel, wxID_ANY);
    for (const auto& policy : SilverstonDropPolicies()) {
        silverston_drop->Append(policy);
    }
    silverston_drop->SetSelection(0);
    auto* silverston_resize = new wxChoice(panel, wxID_ANY);
    for (const auto& policy : SilverstonResizePolicies()) {
        silverston_resize->Append(policy);
    }
    silverston_resize->SetSelection(0);
    auto* silverston_display_profile = new wxChoice(panel, wxID_ANY);
    for (const auto& profile : SilverstonDisplayProfiles()) {
        silverston_display_profile->Append(profile);
    }
    silverston_display_profile->SetSelection(0);
    auto* silverston_preset = new wxChoice(panel, wxID_ANY);
    for (const auto& preset : SilverstonPresetNames()) {
        silverston_preset->Append(preset);
    }
    silverston_preset->SetSelection(0);
    auto* silverston_apply_preset = new wxButton(panel, wxID_ANY, "Apply Preset");
    auto* silverston_validation_hint = new wxStaticText(panel, wxID_ANY, "");
    auto* silverston_apply_diagram = new wxButton(panel, wxID_ANY, "Apply Diagram Policy");

    tools->Add(new wxStaticText(panel, wxID_ANY, "Canvas"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    tools->Add(nudge_left_btn, 0, wxRIGHT, 4);
    tools->Add(nudge_up_btn, 0, wxRIGHT, 4);
    tools->Add(nudge_down_btn, 0, wxRIGHT, 4);
    tools->Add(nudge_right_btn, 0, wxRIGHT, 8);
    tools->Add(resize_btn, 0, wxRIGHT, 6);
    tools->Add(connect_btn, 0, wxRIGHT, 6);
    tools->Add(reparent_btn, 0, wxRIGHT, 6);
    tools->Add(add_node_btn, 0, wxRIGHT, 6);
    tools->Add(delete_btn, 0, wxRIGHT, 8);
    tools->Add(delete_project_btn, 0, wxRIGHT, 8);
    tools->Add(undo_btn, 0, wxRIGHT, 4);
    tools->Add(redo_btn, 0, wxRIGHT, 8);
    tools->Add(zoom_in_btn, 0, wxRIGHT, 4);
    tools->Add(zoom_out_btn, 0, wxRIGHT, 4);
    tools->Add(zoom_reset_btn, 0, wxRIGHT, 10);
    tools->Add(diagram_grid_toggle_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    tools->Add(diagram_snap_toggle_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(tools, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    auto* silverston_node_row = new wxBoxSizer(wxHORIZONTAL);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Silverston Node"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Type"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_type, 0, wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Catalog"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_icon_catalog, 0, wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Icon Slot"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_icon_slot, 0, wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Display"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_display_mode, 0, wxRIGHT, 8);
    silverston_node_row->Add(silverston_chamfer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_node_row->Add(silverston_apply_node, 0, wxRIGHT, 8);
    sizer->Add(silverston_node_row, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    auto* silverston_policy_row = new wxBoxSizer(wxHORIZONTAL);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Silverston Diagram"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Grid"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_grid_size, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Align"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_alignment, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Drop"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_drop, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Resize"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_resize, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Display"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_display_profile, 0, wxRIGHT, 8);
    silverston_policy_row->Add(silverston_apply_diagram, 0, wxRIGHT, 8);
    sizer->Add(silverston_policy_row, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    auto* silverston_preset_row = new wxBoxSizer(wxHORIZONTAL);
    silverston_preset_row->Add(new wxStaticText(panel, wxID_ANY, "Silverston Preset"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_preset_row->Add(silverston_preset, 0, wxRIGHT, 8);
    silverston_preset_row->Add(silverston_apply_preset, 0, wxRIGHT, 8);
    silverston_preset_row->Add(silverston_validation_hint, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(silverston_preset_row, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    diagram_splitter_ = new wxSplitterWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    diagram_splitter_->SetMinimumPaneSize(140);
    diagram_splitter_->SetSashGravity(0.0);

    diagram_palette_panel_docked_ = new wxPanel(diagram_splitter_, wxID_ANY);
    auto* palette_sizer = new wxBoxSizer(wxVERTICAL);
    auto* palette_top = new wxBoxSizer(wxHORIZONTAL);
    auto* palette_detach_btn = new wxButton(diagram_palette_panel_docked_, wxID_ANY, "Detach");
    palette_top->Add(new wxStaticText(diagram_palette_panel_docked_, wxID_ANY, "Palette"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    palette_top->Add(palette_detach_btn, 0, wxRIGHT, 2);
    palette_sizer->Add(palette_top, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
    diagram_palette_list_docked_ = new wxListCtrl(diagram_palette_panel_docked_,
                                                  wxID_ANY,
                                                  wxDefaultPosition,
                                                  wxDefaultSize,
                                                  wxLC_ICON | wxLC_SINGLE_SEL | wxLC_AUTOARRANGE | wxLC_EDIT_LABELS | wxBORDER_SIMPLE);
    palette_sizer->Add(diagram_palette_list_docked_, 1, wxEXPAND | wxALL, 6);
    diagram_palette_panel_docked_->SetSizer(palette_sizer);

    diagram_canvas_panel_ = new wxPanel(diagram_splitter_, wxID_ANY);
    auto* canvas_panel_sizer = new wxBoxSizer(wxVERTICAL);
    diagram_canvas_ = new DiagramCanvasPanel(diagram_canvas_panel_, &diagram_service_);
    canvas_panel_sizer->Add(new wxStaticText(diagram_canvas_panel_, wxID_ANY, "Diagram Canvas"), 0, wxLEFT | wxRIGHT | wxTOP, 2);
    canvas_panel_sizer->Add(diagram_canvas_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);

    diagram_output_ = new wxTextCtrl(diagram_canvas_panel_,
                                     wxID_ANY,
                                     "",
                                     wxDefaultPosition,
                                     wxSize(-1, 100),
                                     wxTE_MULTILINE | wxTE_READONLY);
    canvas_panel_sizer->Add(new wxStaticText(diagram_canvas_panel_, wxID_ANY, "Diagram Output"), 0, wxLEFT | wxRIGHT | wxTOP, 4);
    canvas_panel_sizer->Add(diagram_output_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 4);
    diagram_canvas_panel_->SetSizer(canvas_panel_sizer);

    diagram_splitter_->SplitVertically(diagram_palette_panel_docked_, diagram_canvas_panel_, 220);
    sizer->Add(diagram_splitter_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

    if (diagram_canvas_ != nullptr) {
        diagram_canvas_->SetStatusSink([this](const std::string& message) {
            if (diagram_output_ != nullptr) {
                diagram_output_->SetValue(ToWx(message));
            }
            AppendLogLine("Diagram canvas: " + message);
        });
        diagram_canvas_->SetMutationSink([this](const std::string& mutation) {
            AppendLogLine("Diagram mutation: " + mutation);
            RefreshCatalog();
            SetStatusText(ToWx("Diagram dirty: " + mutation), 0);
        });
    }

    auto refresh_silverston_hint =
        [silverston_type, silverston_icon_catalog, silverston_icon_slot, silverston_validation_hint]() {
            const std::string object_type = silverston_type->GetStringSelection().ToStdString();
            const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
            RefreshSilverstonIconPicker(silverston_icon_catalog, object_type, icon_slot);
            UpdateSilverstonValidationHint(silverston_validation_hint, object_type, icon_slot);
        };
    refresh_silverston_hint();
    silverston_type->Bind(wxEVT_CHOICE,
                          [silverston_type, silverston_icon_slot, refresh_silverston_hint](wxCommandEvent&) {
        const std::string selected_type = silverston_type->GetStringSelection().ToStdString();
        const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
        if (!IsSilverstonIconAllowed(selected_type, icon_slot)) {
            silverston_icon_slot->SetValue(DefaultSilverstonIconForType(selected_type));
        }
        refresh_silverston_hint();
    });
    silverston_icon_catalog->Bind(wxEVT_CHOICE,
                                  [silverston_icon_catalog, silverston_icon_slot, refresh_silverston_hint](wxCommandEvent&) {
        silverston_icon_slot->SetValue(silverston_icon_catalog->GetStringSelection());
        refresh_silverston_hint();
    });
    silverston_icon_slot->Bind(wxEVT_TEXT, [refresh_silverston_hint](wxCommandEvent&) { refresh_silverston_hint(); });
    if (diagram_canvas_ != nullptr) {
        diagram_canvas_->SetSelectionSink(
            [this, silverston_type, silverston_icon_slot, silverston_display_mode, silverston_chamfer,
             silverston_validation_hint, refresh_silverston_hint](const std::string& node_id,
                                                                  const std::string& object_type,
                                                                  const std::string& icon_slot,
                                                                  const std::string& display_mode,
                                                                  bool chamfer_notes) {
                if (ToLower(active_diagram_.diagram_type) != "silverston") {
                    silverston_validation_hint->SetForegroundColour(wxColour(96, 96, 96));
                    silverston_validation_hint->SetLabel("Open a Silverston diagram to use Silverston editor controls.");
                    return;
                }
                if (node_id.empty()) {
                    silverston_validation_hint->SetForegroundColour(wxColour(96, 96, 96));
                    silverston_validation_hint->SetLabel("Select a Silverston node to inspect/edit profile settings.");
                    return;
                }
                SelectChoiceValue(silverston_type, object_type.empty() ? "entity" : object_type);
                const std::string selected_type = silverston_type->GetStringSelection().ToStdString();
                std::string selected_icon = ToLower(icon_slot);
                if (!IsSilverstonIconAllowed(selected_type, selected_icon)) {
                    selected_icon = DefaultSilverstonIconForType(selected_type);
                }
                silverston_icon_slot->SetValue(selected_icon);
                SelectChoiceValue(silverston_display_mode, display_mode.empty() ? "full" : display_mode);
                silverston_chamfer->SetValue(chamfer_notes);
                refresh_silverston_hint();
            });
    }
    BindDiagramPaletteInteractions(diagram_palette_list_docked_);
    RefreshDiagramPaletteControls("Erd");
    if (palette_detach_btn != nullptr) {
        palette_detach_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ToggleDiagramPaletteDetached(true); });
    }
    if (diagram_type_choice_ != nullptr) {
        diagram_type_choice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            RefreshDiagramPaletteControls(diagram_type_choice_->GetStringSelection().ToStdString());
            if (diagram_name_input_ != nullptr && diagram_name_input_->GetValue().ToStdString().empty()) {
                diagram_name_input_->SetValue(DefaultDiagramNameForType(diagram_type_choice_->GetStringSelection().ToStdString()));
            }
        });
    }
    if (new_diagram_btn != nullptr) {
        new_diagram_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            std::string type_name = "ERD";
            if (diagram_type_choice_ != nullptr) {
                type_name = diagram_type_choice_->GetStringSelection().ToStdString();
            }
            std::string name;
            if (diagram_name_input_ != nullptr) {
                name = Trim(diagram_name_input_->GetValue().ToStdString());
            }
            if (name.empty()) {
                wxTextEntryDialog dialog(this,
                                         "Enter a diagram name",
                                         "Create Diagram",
                                         DefaultDiagramNameForType(type_name));
                if (dialog.ShowModal() != wxID_OK) {
                    return;
                }
                name = Trim(dialog.GetValue().ToStdString());
                if (diagram_name_input_ != nullptr) {
                    diagram_name_input_->SetValue(ToWx(name));
                }
            }
            if (name.empty()) {
                if (diagram_output_ != nullptr) {
                    diagram_output_->SetValue("diagram name is required");
                }
                return;
            }
            OpenDiagramByTypeAndName(type_name, name, diagram_output_);
        });
    }

    nudge_left_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->NudgeSelectedNode(-20, 0, error);
        });
    });
    nudge_up_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->NudgeSelectedNode(0, -20, error);
        });
    });
    nudge_down_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->NudgeSelectedNode(0, 20, error);
        });
    });
    nudge_right_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->NudgeSelectedNode(20, 0, error);
        });
    });
    resize_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->ResizeSelectedNode(20, 10, error);
        });
    });
    connect_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->ConnectSelectedToNext(error);
        });
    });
    reparent_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->ReparentSelectedToNext(error);
        });
    });
    add_node_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->AddNode(error);
        });
    });
    delete_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->DeleteSelectedNode(false, error);
        });
    });
    delete_project_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->DeleteSelectedNode(true, error);
        });
    });
    undo_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->Undo(error);
        });
    });
    redo_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this](std::string* error) {
            return diagram_canvas_ != nullptr && diagram_canvas_->Redo(error);
        });
    });
    zoom_in_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (diagram_canvas_ != nullptr) {
            diagram_canvas_->ZoomIn();
        }
    });
    zoom_out_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (diagram_canvas_ != nullptr) {
            diagram_canvas_->ZoomOut();
        }
    });
    zoom_reset_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (diagram_canvas_ != nullptr) {
            diagram_canvas_->ZoomReset();
        }
    });
    diagram_grid_toggle_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        if (diagram_canvas_ != nullptr) {
            diagram_canvas_->SetGridVisible(event.IsChecked());
        }
    });
    diagram_snap_toggle_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        if (diagram_canvas_ != nullptr) {
            diagram_canvas_->SetSnapToGrid(event.IsChecked());
        }
    });
    silverston_apply_node->Bind(wxEVT_BUTTON,
                                [this, silverston_type, silverston_icon_slot, silverston_display_mode, silverston_chamfer](wxCommandEvent&) {
        if (ToLower(active_diagram_.diagram_type) != "silverston") {
            if (diagram_output_ != nullptr) {
                diagram_output_->SetValue("Silverston node editor requires a Silverston diagram.");
            }
            return;
        }
        const std::string object_type = silverston_type->GetStringSelection().ToStdString();
        const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
        if (!IsSilverstonIconAllowed(object_type, icon_slot)) {
            if (diagram_output_ != nullptr) {
                diagram_output_->SetValue("Silverston icon slot is invalid for selected object type.");
            }
            return;
        }
        ExecuteCanvasAction(diagram_canvas_, diagram_output_, [this, silverston_type, icon_slot, silverston_display_mode,
                                                               silverston_chamfer](std::string* error) {
            return diagram_canvas_ != nullptr &&
                   diagram_canvas_->ApplySilverstonNodeProfile(silverston_type->GetStringSelection().ToStdString(),
                                                               icon_slot,
                                                               silverston_display_mode->GetStringSelection().ToStdString(),
                                                               silverston_chamfer->GetValue(),
                                                               error);
        });
    });
    silverston_apply_diagram->Bind(wxEVT_BUTTON,
                                   [this, silverston_grid_size, silverston_alignment, silverston_drop, silverston_resize,
                                    silverston_display_profile](wxCommandEvent&) {
        if (ToLower(active_diagram_.diagram_type) != "silverston") {
            if (diagram_output_ != nullptr) {
                diagram_output_->SetValue("Silverston diagram editor requires a Silverston diagram.");
            }
            return;
        }
        ExecuteCanvasAction(diagram_canvas_, diagram_output_,
                            [this, silverston_grid_size, silverston_alignment, silverston_drop, silverston_resize,
                             silverston_display_profile](std::string* error) {
            int parsed_grid = 20;
            try {
                parsed_grid = std::stoi(silverston_grid_size->GetValue().ToStdString());
            } catch (...) {
                if (error != nullptr) {
                    *error = "invalid grid size";
                }
                return false;
            }
            const bool ok = diagram_canvas_ != nullptr &&
                            diagram_canvas_->ApplySilverstonDiagramPolicy(parsed_grid,
                                                                          silverston_alignment->GetStringSelection().ToStdString(),
                                                                          silverston_drop->GetStringSelection().ToStdString(),
                                                                          silverston_resize->GetStringSelection().ToStdString(),
                                                                          silverston_display_profile->GetStringSelection().ToStdString(),
                                                                          error);
            if (ok) {
                diagram_grid_toggle_->SetValue(true);
                diagram_snap_toggle_->SetValue(ToLower(silverston_alignment->GetStringSelection().ToStdString()) != "free");
            }
            return ok;
        });
    });
    silverston_apply_preset->Bind(wxEVT_BUTTON,
                                  [this, silverston_preset, silverston_type, silverston_icon_slot, silverston_display_mode,
                                   silverston_chamfer, silverston_grid_size, silverston_alignment, silverston_drop,
                                   silverston_resize, silverston_display_profile, refresh_silverston_hint](wxCommandEvent&) {
        if (ToLower(active_diagram_.diagram_type) != "silverston") {
            if (diagram_output_ != nullptr) {
                diagram_output_->SetValue("Silverston presets require a Silverston diagram.");
            }
            return;
        }
        SilverstonPreset preset;
        if (!ResolveSilverstonPreset(silverston_preset->GetStringSelection().ToStdString(), &preset)) {
            if (diagram_output_ != nullptr) {
                diagram_output_->SetValue("Unknown Silverston preset.");
            }
            return;
        }
        SelectChoiceValue(silverston_display_mode, preset.node_display_mode);
        silverston_grid_size->SetValue(std::to_string(preset.grid_size));
        SelectChoiceValue(silverston_alignment, preset.alignment_policy);
        SelectChoiceValue(silverston_drop, preset.drop_policy);
        SelectChoiceValue(silverston_resize, preset.resize_policy);
        SelectChoiceValue(silverston_display_profile, preset.display_profile);

        const std::string selected_type = silverston_type->GetStringSelection().ToStdString();
        const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
        if (!IsSilverstonIconAllowed(selected_type, icon_slot)) {
            silverston_icon_slot->SetValue(DefaultSilverstonIconForType(selected_type));
        }
        refresh_silverston_hint();

        ExecuteCanvasAction(diagram_canvas_, diagram_output_,
                            [this, silverston_grid_size, silverston_alignment, silverston_drop, silverston_resize,
                             silverston_display_profile, silverston_type, silverston_icon_slot, silverston_display_mode,
                             silverston_chamfer](std::string* error) {
            int parsed_grid = 20;
            try {
                parsed_grid = std::stoi(silverston_grid_size->GetValue().ToStdString());
            } catch (...) {
                if (error != nullptr) {
                    *error = "invalid grid size";
                }
                return false;
            }
            const bool diagram_ok = diagram_canvas_ != nullptr &&
                                    diagram_canvas_->ApplySilverstonDiagramPolicy(
                                        parsed_grid,
                                        silverston_alignment->GetStringSelection().ToStdString(),
                                        silverston_drop->GetStringSelection().ToStdString(),
                                        silverston_resize->GetStringSelection().ToStdString(),
                                        silverston_display_profile->GetStringSelection().ToStdString(),
                                        error);
            if (!diagram_ok) {
                return false;
            }
            if (diagram_canvas_->SelectedNodeId().empty()) {
                return true;
            }
            return diagram_canvas_->ApplySilverstonNodeProfile(
                silverston_type->GetStringSelection().ToStdString(),
                ToLower(silverston_icon_slot->GetValue().ToStdString()),
                silverston_display_mode->GetStringSelection().ToStdString(),
                silverston_chamfer->GetValue(),
                error);
        });
        if (diagram_canvas_ != nullptr && diagram_canvas_->SelectedNodeId().empty() && diagram_output_ != nullptr) {
            diagram_output_->SetValue("Preset applied to diagram policy. Select a node and apply node profile to update node visuals.");
        }
    });

    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::BuildPlanTab(wxWindow* parent) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Plan Layout (deterministic order)"), 0, wxALL, 8);
    plan_rows_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    plan_rows_->InsertColumn(0, "ordinal", wxLIST_FORMAT_LEFT, 80);
    plan_rows_->InsertColumn(1, "node_id", wxLIST_FORMAT_LEFT, 80);
    plan_rows_->InsertColumn(2, "depth", wxLIST_FORMAT_LEFT, 70);
    plan_rows_->InsertColumn(3, "operator", wxLIST_FORMAT_LEFT, 180);
    plan_rows_->InsertColumn(4, "estimated_cost", wxLIST_FORMAT_LEFT, 120);
    plan_rows_->InsertColumn(5, "predicate", wxLIST_FORMAT_LEFT, 380);
    sizer->Add(plan_rows_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::BuildSpecWorkspaceTab(wxWindow* parent) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(panel, wxID_ANY, "Spec set"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    specset_choice_ = new wxChoice(panel, wxID_ANY);
    specset_choice_->Append("sb_v3");
    specset_choice_->Append("sb_vnext");
    specset_choice_->Append("sb_beta1");
    specset_choice_->SetSelection(1);
    row->Add(specset_choice_, 0, wxRIGHT, 8);
    row->Add(new wxButton(panel, kCmdRefreshSpecWorkspace, "Refresh Workspace"), 0, wxRIGHT, 8);
    sizer->Add(row, 0, wxEXPAND | wxALL, 8);

    spec_summary_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE | wxTE_READONLY);
    spec_dashboard_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 120), wxTE_MULTILINE | wxTE_READONLY);
    spec_work_package_ =
        new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Gap Summary"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(spec_summary_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Coverage Dashboard"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(spec_dashboard_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Implementation Work Package"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    sizer->Add(spec_work_package_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::BuildMonitoringTab(wxWindow* parent) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxButton(panel, kCmdRefreshMonitoring, "Refresh Metrics"), 0, wxRIGHT, 6);
    sizer->Add(row, 0, wxALL, 8);

    monitoring_rows_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    monitoring_rows_->InsertColumn(0, "metric_key", wxLIST_FORMAT_LEFT, 220);
    monitoring_rows_->InsertColumn(1, "sample_count", wxLIST_FORMAT_LEFT, 120);
    monitoring_rows_->InsertColumn(2, "total_value", wxLIST_FORMAT_LEFT, 180);
    sizer->Add(monitoring_rows_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);
    return panel;
}

void MainFrame::SeedUiState() {
    try {
        ui_service_.EnsureSpecWorkspaceEntrypoint();
    } catch (const RejectError& ex) {
        AppendLogLine(ex.what());
    }

    if (active_diagram_.diagram_id.empty()) {
        active_diagram_ = BuildSampleDiagram("active:startup", "Erd");
        active_diagram_name_ = "Core Domain ERD";
    }
    if (diagram_canvas_ != nullptr) {
        diagram_canvas_->SetDocument(&active_diagram_);
    }
    RefreshDiagramPresentation();
    RefreshCatalog();
    RefreshHistory();
    RefreshMonitoring();
    RefreshSpecWorkspace();

    for (const auto& warning : startup_report_.warnings) {
        AppendLogLine(warning);
    }
}

void MainFrame::SelectWorkspacePage(int page_index) {
    if (workspace_notebook_ == nullptr || page_index < 0) {
        return;
    }
    const int page_count = static_cast<int>(workspace_notebook_->GetPageCount());
    if (page_index >= page_count) {
        return;
    }
    workspace_notebook_->SetSelection(page_index);
}

void MainFrame::EnsureDetachedSurfaceNotEmbedded(int page_index) {
    if (workspace_notebook_ == nullptr) {
        return;
    }
    const int selected_page = workspace_notebook_->GetSelection();
    if (selected_page != page_index) {
        return;
    }
    int fallback_page = kWorkspacePagePlan;
    const int page_count = static_cast<int>(workspace_notebook_->GetPageCount());
    if (fallback_page < 0 || fallback_page >= page_count || fallback_page == page_index) {
        fallback_page = page_index == 0 ? 1 : 0;
    }
    if (fallback_page >= 0 && fallback_page < page_count && fallback_page != page_index) {
        workspace_notebook_->SetSelection(fallback_page);
    }
}

void MainFrame::CloseDetachedSurfaceForPage(int page_index) {
    if (page_index == kWorkspacePageSql && sql_editor_frame_ != nullptr) {
        sql_editor_frame_->Close();
        return;
    }
    if (page_index == kWorkspacePageObject && object_editor_frame_ != nullptr) {
        object_editor_frame_->Close();
        return;
    }
    if (page_index == kWorkspacePageDiagram && diagram_frame_ != nullptr) {
        diagram_frame_->Close();
        return;
    }
    if (page_index == kWorkspacePageSpec && spec_workspace_frame_ != nullptr) {
        spec_workspace_frame_->Close();
        return;
    }
    if (page_index == kWorkspacePageMonitoring && monitoring_frame_ != nullptr) {
        monitoring_frame_->Close();
    }
}

void MainFrame::OnWorkspaceNotebookPageChanged(wxBookCtrlEvent& event) {
    if (event.GetSelection() == kWorkspacePageDiagram) {
        ToggleDiagramPaletteDetached(false);
    }
    CloseDetachedSurfaceForPage(event.GetSelection());
    event.Skip();
}

void MainFrame::BindDetachedFrameDropDock(wxFrame* frame, int page_index) {
    if (frame == nullptr) {
        return;
    }
    const wxPoint initial_position = frame->GetScreenPosition();
    auto move_armed = std::make_shared<bool>(false);
    frame->Bind(wxEVT_MOVE, [this, frame, page_index, move_armed, initial_position](wxMoveEvent& event) {
        const wxPoint current_position = frame->GetScreenPosition();
        if (!*move_armed) {
            if (current_position == initial_position) {
                event.Skip();
                return;
            }
            *move_armed = true;
        }
        const double overlap_ratio = OverlapRatio(GetScreenRect(), wxRect(current_position, frame->GetSize()));
        if (overlap_ratio >= 0.70) {
            SelectWorkspacePage(page_index);
        }
        event.Skip();
    });
}

void MainFrame::PopulateHistoryList(wxListCtrl* target) {
    if (target == nullptr) {
        return;
    }
    const auto rows = ui_service_.QueryHistoryByProfile(ActiveProfileName());
    target->DeleteAllItems();
    for (const auto& row : rows) {
        const long item = target->InsertItem(target->GetItemCount(), ToWx(row.query_id));
        target->SetItem(item, 1, ToWx(row.profile_id));
        target->SetItem(item, 2, ToWx(row.started_at_utc));
        target->SetItem(item, 3, ToWx(row.status));
        target->SetItem(item, 4, ToWx(row.sql_hash));
    }
}

void MainFrame::SeedDiagramLinks(wxListCtrl* target) {
    if (target == nullptr) {
        return;
    }
    target->DeleteAllItems();

    const long erd_row = target->InsertItem(target->GetItemCount(), "Erd");
    target->SetItem(erd_row, 1, "Core Domain ERD");
    target->SetItem(erd_row, 2, "diagram://erd/core_domain");

    const long silverston_row = target->InsertItem(target->GetItemCount(), "Silverston");
    target->SetItem(silverston_row, 1, "Silverston Subject Areas");
    target->SetItem(silverston_row, 2, "diagram://silverston/subject_areas");

    const long whiteboard_row = target->InsertItem(target->GetItemCount(), "Whiteboard");
    target->SetItem(whiteboard_row, 1, "Migration Planning Board");
    target->SetItem(whiteboard_row, 2, "diagram://whiteboard/migration_plan");

    const long mindmap_row = target->InsertItem(target->GetItemCount(), "MindMap");
    target->SetItem(mindmap_row, 1, "Implementation Mind Map");
    target->SetItem(mindmap_row, 2, "diagram://mindmap/implementation_map");

    if (target->GetItemCount() > 0) {
        target->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
}

void MainFrame::RefreshMonitoringList(wxListCtrl* target) {
    if (target == nullptr) {
        return;
    }
    auto feed = reporting_service_.ActivityFeed();
    if (feed.empty()) {
        const std::string now = NowUtc();
        reporting_service_.AppendActivity({now, "query_count", 12});
        reporting_service_.AppendActivity({now, "error_count", 1});
        reporting_service_.AppendActivity({now, "latency_ms", 34});
        feed = reporting_service_.ActivityFeed();
    }

    const auto active_rows =
        reporting_service_.RunActivityQuery(feed, "1h", std::set<std::string>{"query_count", "error_count", "latency_ms"});
    const auto summary = reporting_service_.SummarizeActivity(active_rows);

    target->DeleteAllItems();
    for (const auto& row : summary) {
        const long idx = target->InsertItem(target->GetItemCount(), ToWx(row.metric_key));
        target->SetItem(idx, 1, ToWx(std::to_string(row.sample_count)));
        std::ostringstream val;
        val << std::fixed << std::setprecision(2) << row.total_value;
        target->SetItem(idx, 2, ToWx(val.str()));
    }
}

void MainFrame::RefreshSpecWorkspaceControls(wxChoice* set_choice,
                                             wxTextCtrl* summary,
                                             wxTextCtrl* dashboard,
                                             wxTextCtrl* work_package) {
    if (set_choice == nullptr || summary == nullptr || dashboard == nullptr || work_package == nullptr) {
        return;
    }

    std::string set_id = set_choice->GetStringSelection().ToStdString();
    if (set_id.empty() && set_choice->GetCount() > 0) {
        set_choice->SetSelection(0);
        set_id = set_choice->GetStringSelection().ToStdString();
    }

    try {
        const auto manifest_path = ManifestPathForSet(set_id);
        if (!std::filesystem::exists(manifest_path)) {
            throw MakeReject("SRB1-R-5402",
                             "specset manifest missing",
                             "spec_workspace",
                             "main_frame_refresh",
                             false,
                             manifest_path.string());
        }

        const auto index = specset_service_.BuildIndex(manifest_path.string(), NowUtc());

        std::vector<std::tuple<std::string, std::string, std::string>> coverage_links;
        std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> gaps;

        for (std::size_t i = 0; i < index.files.size(); ++i) {
            const auto& file = index.files[i];
            const std::string file_ref = file.set_id + ":" + file.relative_path;
            const std::string cls = (i % 3 == 0) ? "design" : ((i % 3 == 1) ? "development" : "management");
            const std::string state = (i % 7 == 0) ? "missing" : ((i % 2 == 0) ? "partial" : "covered");
            coverage_links.emplace_back(file_ref, cls, state);
            if (state == "missing") {
                if (cls == "design") {
                    gaps.emplace_back(file_ref, cls, std::vector<std::string>{"SPC-IDX-001", "SPC-NRM-001"});
                } else if (cls == "development") {
                    gaps.emplace_back(file_ref, cls, std::vector<std::string>{"SPC-COV-001", "SPC-COV-002"});
                } else {
                    gaps.emplace_back(file_ref, cls, std::vector<std::string>{"SPC-RPT-001", "SPC-WPK-001"});
                }
            }
        }

        summary->SetValue(ToWx(ui_service_.BuildSpecWorkspaceGapSummary(coverage_links)));
        dashboard->SetValue(ToWx(ui_service_.BuildSpecWorkspaceDashboard(coverage_links)));
        work_package->SetValue(ToWx(ui_service_.ExportSpecWorkspaceWorkPackage(set_id, gaps, NowUtc())));

        std::ostringstream status;
        status << "Spec set " << set_id << " files=" << index.files.size();
        SetStatusText(ToWx(status.str()), 0);
    } catch (const RejectError& ex) {
        summary->SetValue(ToWx(ex.what()));
        dashboard->SetValue("");
        work_package->SetValue("");
        AppendLogLine(ex.what());
    }
}

void MainFrame::EnsureConnected() {
    if (adapter_.IsConnected()) {
        return;
    }
    const auto profile = ActiveProfile();
    const auto session = adapter_.Connect(profile);
    SetStatusText(ToWx("Connected " + session.backend_name + " " + session.server_version), 1);
    AppendLogLine("Connected profile " + profile.name + " backend=" + session.backend_name);
}

void MainFrame::RefreshCatalog() {
    tree_->DeleteAllItems();
    const auto root = tree_->AddRoot("root");

    const auto shell = tree_->AppendItem(root, "MainFrame");
    tree_->AppendItem(shell, "SQL Editor");
    tree_->AppendItem(shell, "Object Editor");
    tree_->AppendItem(shell, "Plan");
    tree_->AppendItem(shell, "Diagrams");
    tree_->AppendItem(shell, "Spec Workspace");
    tree_->AppendItem(shell, "Monitoring");

    const auto detached = tree_->AppendItem(root, "Detached Windows");
    tree_->AppendItem(detached, "SqlEditorFrame");
    tree_->AppendItem(detached, "ObjectEditorFrame");
    tree_->AppendItem(detached, "DiagramFrame");
    tree_->AppendItem(detached, "MonitoringFrame");
    tree_->AppendItem(detached, "ReportingFrame");
    tree_->AppendItem(detached, "DataMaskingFrame");
    tree_->AppendItem(detached, "CdcConfigFrame");
    tree_->AppendItem(detached, "GitIntegrationFrame");
    tree_->AppendItem(detached, "SpecWorkspaceFrame");
    tree_->AppendItem(detached, "SchemaManagerFrame");
    tree_->AppendItem(detached, "TableDesignerFrame");
    tree_->AppendItem(detached, "IndexDesignerFrame");
    tree_->AppendItem(detached, "DomainManagerFrame");
    tree_->AppendItem(detached, "SequenceManagerFrame");
    tree_->AppendItem(detached, "ViewManagerFrame");
    tree_->AppendItem(detached, "TriggerManagerFrame");
    tree_->AppendItem(detached, "ProcedureManagerFrame");
    tree_->AppendItem(detached, "PackageManagerFrame");
    tree_->AppendItem(detached, "UsersRolesFrame");
    tree_->AppendItem(detached, "JobSchedulerFrame");
    tree_->AppendItem(detached, "StorageManagerFrame");
    tree_->AppendItem(detached, "BackupManagerFrame");

    const auto profiles = tree_->AppendItem(root, "Connections");
    for (const auto& profile : profiles_) {
        std::ostringstream out;
        out << profile.name << " (" << profile.backend << ")";
        tree_->AppendItem(profiles, ToWx(out.str()));
    }

    const auto objects = tree_->AppendItem(root, "Objects");
    const auto schema = tree_->AppendItem(objects, "public");
    tree_->AppendItem(schema, "table: customer");
    tree_->AppendItem(schema, "table: orders");
    tree_->AppendItem(schema, "view: customer_summary");
    tree_->AppendItem(schema, "index: idx_customer_name");

    const auto diagrams = tree_->AppendItem(root, "Diagrams");
    tree_->AppendItem(diagrams, "Erd/Core Domain");
    tree_->AppendItem(diagrams, "Silverston/Subject Areas");
    tree_->AppendItem(diagrams, "Whiteboard/Migration Plan");
    tree_->AppendItem(diagrams, "MindMap/Implementation Map");

    tree_->ExpandAll();
}

void MainFrame::RefreshHistory() {
    PopulateHistoryList(sql_history_);
    PopulateHistoryList(sql_history_detached_);
    BindDetachedFrameDropDock(sql_editor_frame_, kWorkspacePageSql);
}

void MainFrame::RefreshMonitoring() {
    try {
        RefreshMonitoringList(monitoring_rows_);
        RefreshMonitoringList(monitoring_rows_detached_);
    } catch (const RejectError& ex) {
        AppendLogLine(ex.what());
    }
}

void MainFrame::RefreshSpecWorkspace() {
    RefreshSpecWorkspaceControls(specset_choice_, spec_summary_, spec_dashboard_, spec_work_package_);
    RefreshSpecWorkspaceControls(specset_choice_detached_,
                                 spec_summary_detached_,
                                 spec_dashboard_detached_,
                                 spec_work_package_detached_);
}

void MainFrame::RefreshPlan(const std::string& sql) {
    std::vector<beta1b::PlanNode> nodes;
    const std::string lowered = ToLower(sql);
    if (lowered.find("join") != std::string::npos) {
        nodes.push_back({1, -1, "HashJoin", 1200, 18.5, "join customer.id = orders.customer_id"});
        nodes.push_back({2, 1, "SeqScan", 1000, 7.5, "customer.active = 1"});
        nodes.push_back({3, 1, "IndexScan", 1200, 4.2, "orders.order_date >= current_date - 30"});
    } else {
        nodes.push_back({1, -1, "SeqScan", 250, 6.3, "active = 1"});
        nodes.push_back({2, 1, "Filter", 120, 8.0, "name LIKE 'A%'"});
        nodes.push_back({3, 2, "Sort", 120, 9.2, "ORDER BY name"});
    }

    const auto rows = ui_service_.RenderPlanLayout(nodes);
    plan_rows_->DeleteAllItems();
    for (const auto& row : rows) {
        const long idx = plan_rows_->InsertItem(plan_rows_->GetItemCount(), ToWx(std::to_string(row.ordinal)));
        plan_rows_->SetItem(idx, 1, ToWx(std::to_string(row.node_id)));
        plan_rows_->SetItem(idx, 2, ToWx(std::to_string(row.depth)));
        plan_rows_->SetItem(idx, 3, ToWx(row.operator_name));
        std::ostringstream cost;
        cost << std::fixed << std::setprecision(2) << row.estimated_cost;
        plan_rows_->SetItem(idx, 4, ToWx(cost.str()));
        plan_rows_->SetItem(idx, 5, ToWx((lowered.find("join") != std::string::npos) ? "multi-source" : "single-source"));
    }
}

void MainFrame::AppendLogLine(const std::string& line) {
    if (log_output_ == nullptr) {
        return;
    }
    std::ostringstream out;
    out << "[" << NowUtc() << "] " << line << "\n";
    log_output_->AppendText(ToWx(out.str()));
}

void MainFrame::RunSqlIntoSurface(const std::string& sql,
                                  wxListCtrl* results,
                                  wxTextCtrl* status,
                                  wxListCtrl* history,
                                  bool focus_plan_tab) {
    try {
        EnsureConnected();
        adapter_.MarkActiveQuery(true);
        const auto result = ui_service_.RunSqlEditorQuery(sql, true, 1, 0);
        adapter_.MarkActiveQuery(false);

        if (results != nullptr) {
            const long row = results->InsertItem(results->GetItemCount(), ToWx(result.command_tag));
            results->SetItem(row, 1, ToWx(std::to_string(result.rows_affected)));
            results->SetItem(row, 2, "ok");
        }
        if (status != nullptr) {
            status->SetValue(ToWx(result.status_payload));
        }

        beta1b::QueryHistoryRow history_row;
        history_row.query_id = "q" + std::to_string(next_query_id_++);
        history_row.profile_id = ActiveProfileName();
        history_row.started_at_utc = NowUtc();
        history_row.duration_ms = 4;
        history_row.status = "success";
        history_row.error_code = "";
        history_row.sql_hash = Sha256Hex(sql);
        ui_service_.AppendHistoryRow(history_row);

        PopulateHistoryList(history);
        if (history != sql_history_) {
            PopulateHistoryList(sql_history_);
        }
        if (history != sql_history_detached_) {
            PopulateHistoryList(sql_history_detached_);
        }

        RefreshPlan(sql);
        if (focus_plan_tab && workspace_notebook_ != nullptr) {
            workspace_notebook_->SetSelection(3);
        }
        AppendLogLine("SQL executed query_id=" + history_row.query_id + " rows=" + std::to_string(result.rows_affected));
    } catch (const RejectError& ex) {
        adapter_.MarkActiveQuery(false);

        if (results != nullptr) {
            const long row = results->InsertItem(results->GetItemCount(), "ERROR");
            results->SetItem(row, 1, "0");
            results->SetItem(row, 2, ToWx(ex.what()));
        }
        if (status != nullptr) {
            status->SetValue(ToWx(ex.what()));
        }

        beta1b::QueryHistoryRow history_row;
        history_row.query_id = "q" + std::to_string(next_query_id_++);
        history_row.profile_id = ActiveProfileName();
        history_row.started_at_utc = NowUtc();
        history_row.duration_ms = 0;
        history_row.status = "error";
        history_row.error_code = ex.payload().code;
        history_row.sql_hash = Sha256Hex(sql);
        ui_service_.AppendHistoryRow(history_row);

        PopulateHistoryList(history);
        if (history != sql_history_) {
            PopulateHistoryList(sql_history_);
        }
        if (history != sql_history_detached_) {
            PopulateHistoryList(sql_history_detached_);
        }
        AppendLogLine(ex.what());
    }
}

void MainFrame::CancelSqlIntoStatus(wxTextCtrl* status) {
    try {
        adapter_.CancelActiveQuery();
        if (status != nullptr) {
            status->SetValue("cancelled active query");
        }
        AppendLogLine("Cancelled active query");
    } catch (const RejectError& ex) {
        if (status != nullptr) {
            status->SetValue(ToWx(ex.what()));
        }
        AppendLogLine(ex.what());
    }
}

void MainFrame::ExportHistoryIntoStatus(wxTextCtrl* status, wxListCtrl* history) {
    try {
        const auto result = ui_service_.PruneAndExportStoredHistory(ActiveProfileName(), "1970-01-01T00:00:00Z", "csv");
        if (status != nullptr) {
            status->SetValue(ToWx(result.payload));
        }
        PopulateHistoryList(history);
        if (history != sql_history_) {
            PopulateHistoryList(sql_history_);
        }
        if (history != sql_history_detached_) {
            PopulateHistoryList(sql_history_detached_);
        }
        AppendLogLine("Exported query history rows=" + std::to_string(result.retained_rows));
    } catch (const RejectError& ex) {
        if (status != nullptr) {
            status->SetValue(ToWx(ex.what()));
        }
        AppendLogLine(ex.what());
    }
}

void MainFrame::SaveObjectFromControls(wxChoice* object_class,
                                       wxTextCtrl* object_path,
                                       wxTextCtrl* object_ddl) {
    if (object_class == nullptr || object_path == nullptr || object_ddl == nullptr) {
        return;
    }

    const std::string class_name = object_class->GetStringSelection().ToStdString();
    const std::string path = object_path->GetValue().ToStdString();
    const std::string ddl = object_ddl->GetValue().ToStdString();

    if (class_name.empty() || path.empty() || ddl.empty()) {
        throw MakeReject("SRB1-R-5105", "object editor fields are required", "ui", "save_object");
    }

    const std::vector<ui::SchemaObjectSnapshot> left = {{class_name, path, "CREATE " + class_name + " " + path + " (...)"}};
    const std::vector<ui::SchemaObjectSnapshot> right = {{class_name, path, ddl}};

    const auto ops = ui_service_.BuildSchemaCompareFromSnapshots(left, right);
    if (ops.empty()) {
        AppendLogLine("Object editor save: no schema changes detected");
    } else {
        AppendLogLine("Object editor save generated " + std::to_string(ops.size()) + " schema operation(s)");
    }
}

void MainFrame::GenerateMigrationIntoControls(wxChoice* object_class,
                                              wxTextCtrl* object_path,
                                              wxTextCtrl* object_ddl) {
    if (object_class == nullptr || object_path == nullptr || object_ddl == nullptr) {
        return;
    }

    const std::string class_name = object_class->GetStringSelection().ToStdString();
    const std::string path = object_path->GetValue().ToStdString();
    const std::string ddl = object_ddl->GetValue().ToStdString();

    const std::vector<ui::SchemaObjectSnapshot> left = {{class_name, path, "CREATE " + class_name + " " + path + " (id BIGINT)"}};
    const std::vector<ui::SchemaObjectSnapshot> right = {{class_name, path, ddl}};

    const auto ops = ui_service_.BuildSchemaCompareFromSnapshots(left, right);
    const std::string script = ui_service_.BuildMigrationScript(ops, NowUtc(), "left_snapshot", "right_snapshot");
    object_ddl->SetValue(ToWx(script));
    AppendLogLine("Generated migration script operation_count=" + std::to_string(ops.size()));
}

void MainFrame::OpenDiagramFromControls(wxListCtrl* links, wxTextCtrl* output) {
    if (output == nullptr) {
        return;
    }

    std::string type_name = "Erd";
    std::string diagram_name = "Core Domain ERD";
    if (links != nullptr) {
        long selected = links->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected == -1 && links->GetItemCount() > 0) {
            selected = 0;
            links->SetItemState(selected, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        }
        if (selected != -1) {
            type_name = links->GetItemText(selected, 0).ToStdString();
            diagram_name = links->GetItemText(selected, 1).ToStdString();
        }
    } else {
        if (diagram_type_choice_ != nullptr) {
            type_name = diagram_type_choice_->GetStringSelection().ToStdString();
        }
        if (diagram_name_input_ != nullptr) {
            diagram_name = Trim(diagram_name_input_->GetValue().ToStdString());
        }
    }
    OpenDiagramByTypeAndName(type_name, diagram_name, output);
}

void MainFrame::OpenDiagramByTypeAndName(const std::string& type_name, const std::string& diagram_name, wxTextCtrl* output) {
    if (output == nullptr) {
        return;
    }
    const std::string canonical_type = CanonicalDiagramType(type_name);
    std::string cleaned_name = Trim(diagram_name);
    if (cleaned_name.empty()) {
        cleaned_name = DefaultDiagramNameForType(canonical_type).ToStdString();
    }
    const auto type = diagram::ParseDiagramType(canonical_type);
    diagram_service_.ValidateDiagramType(type);
    auto source_model = BuildSampleDiagram("active:" + SlugifyDiagramName(cleaned_name), canonical_type);
    source_model.diagram_type = canonical_type;
    diagram::ReverseModelSource reverse_source;
    reverse_source.diagram_id = source_model.diagram_id;
    reverse_source.notation = source_model.notation;
    reverse_source.nodes = source_model.nodes;
    reverse_source.edges = source_model.edges;
    active_diagram_ = diagram_service_.ReverseEngineerModel(type, reverse_source, true);
    active_diagram_name_ = cleaned_name;
    const std::string payload = beta1b::SerializeDiagramModel(active_diagram_);
    output->SetValue(ToWx(payload));
    if (diagram_canvas_ != nullptr) {
        diagram_canvas_->SetDocument(&active_diagram_);
    }
    if (diagram_canvas_detached_ != nullptr) {
        diagram_canvas_detached_->SetDocument(&active_diagram_);
    }
    RefreshDiagramPresentation();
    AppendLogLine("Opened diagram " + cleaned_name + " type=" + canonical_type);
}

void MainFrame::RefreshDiagramPresentation() {
    const std::string canonical_type = CanonicalDiagramType(active_diagram_.diagram_type);
    if (active_diagram_name_.empty()) {
        active_diagram_name_ = DefaultDiagramNameForType(canonical_type).ToStdString();
    }
    const std::string heading = BuildDiagramHeadingText(canonical_type, active_diagram_name_);
    if (diagram_heading_ != nullptr) {
        diagram_heading_->SetLabel(ToWx(heading));
    }
    if (diagram_heading_detached_ != nullptr) {
        diagram_heading_detached_->SetLabel(ToWx(heading));
    }
    if (diagram_frame_ != nullptr) {
        diagram_frame_->SetTitle(ToWx(heading));
    }
    if (diagram_type_choice_ != nullptr) {
        std::string choice_label = canonical_type;
        if (canonical_type == "Erd") {
            choice_label = "ERD";
        } else if (canonical_type == "MindMap") {
            choice_label = "Mind Map";
        }
        SelectChoiceValue(diagram_type_choice_, choice_label);
    }
    if (diagram_type_choice_detached_ != nullptr) {
        std::string choice_label = canonical_type;
        if (canonical_type == "Erd") {
            choice_label = "ERD";
        } else if (canonical_type == "MindMap") {
            choice_label = "Mind Map";
        }
        SelectChoiceValue(diagram_type_choice_detached_, choice_label);
    }
    if (diagram_name_input_ != nullptr && !diagram_name_input_->HasFocus()) {
        diagram_name_input_->SetValue(ToWx(active_diagram_name_));
    }
    if (diagram_name_input_detached_ != nullptr && !diagram_name_input_detached_->HasFocus()) {
        diagram_name_input_detached_->SetValue(ToWx(active_diagram_name_));
    }
    RefreshDiagramPaletteControls(canonical_type);
}

void MainFrame::RefreshDiagramPaletteControls(const std::string& type_name) {
    const std::string canonical_type = CanonicalDiagramType(type_name);
    PopulateDiagramPaletteList(diagram_palette_list_docked_, canonical_type);
    PopulateDiagramPaletteList(diagram_palette_list_floating_, canonical_type);
    PopulateDiagramPaletteList(diagram_palette_list_detached_, canonical_type);
}

void MainFrame::PopulateDiagramPaletteList(wxListCtrl* list, const std::string& type_name) {
    if (list == nullptr) {
        return;
    }
    if (list->GetImageList(wxIMAGE_LIST_NORMAL) == nullptr) {
        auto* image_list = new wxImageList(16, 16, true);
        image_list->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_TOOLBAR, wxSize(16, 16)));  // default object
        image_list->Add(wxArtProvider::GetBitmap(wxART_TIP, wxART_TOOLBAR, wxSize(16, 16)));          // note/idea
        image_list->Add(wxArtProvider::GetBitmap(wxART_PLUS, wxART_TOOLBAR, wxSize(16, 16)));         // relation/link
        image_list->Add(wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR, wxSize(16, 16)));  // task/action
        list->AssignImageList(image_list, wxIMAGE_LIST_NORMAL);
    }

    const std::string canonical_type = CanonicalDiagramType(type_name);
    std::vector<std::string> items = DefaultPaletteItemsForType(canonical_type);
    const auto custom_it = diagram_palette_custom_items_.find(canonical_type);
    if (custom_it != diagram_palette_custom_items_.end()) {
        for (const auto& item : custom_it->second) {
            const std::string token = ToLower(Trim(item));
            if (!token.empty() && std::find(items.begin(), items.end(), token) == items.end()) {
                items.push_back(token);
            }
        }
    }

    list->Freeze();
    list->DeleteAllItems();
    for (const auto& item : items) {
        const long idx = list->InsertItem(list->GetItemCount(), ToWx(item), PaletteIconIndexForItem(item));
        list->SetItemState(idx, 0, wxLIST_STATE_SELECTED);
    }
    list->Arrange();
    list->Thaw();
}

void MainFrame::BindDiagramPaletteInteractions(wxListCtrl* list) {
    if (list == nullptr) {
        return;
    }
    list->Bind(wxEVT_LIST_BEGIN_DRAG, [this, list](wxListEvent& event) {
        const long row = event.GetIndex();
        if (row < 0) {
            return;
        }
        const std::string token = ToLower(Trim(list->GetItemText(row).ToStdString()));
        if (token.empty()) {
            return;
        }
        wxTextDataObject data(ToWx("diagram_item:" + token));
        wxDropSource source(list);
        source.SetData(data);
        source.DoDragDrop(wxDrag_CopyOnly);
    });

    list->Bind(wxEVT_CONTEXT_MENU, [this, list](wxContextMenuEvent& event) {
        wxMenu menu;
        const int add_id = wxWindow::NewControlId();
        const int remove_id = wxWindow::NewControlId();
        const int reset_id = wxWindow::NewControlId();
        menu.Append(add_id, "Add Palette Item");
        menu.Append(remove_id, "Remove Selected Item");
        menu.Append(reset_id, "Reset Palette Type");
        menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            const std::string active_type = CanonicalDiagramType(active_diagram_.diagram_type);
            wxTextEntryDialog dialog(this, "Enter palette item token", "Add Palette Item", "custom_item");
            if (dialog.ShowModal() != wxID_OK) {
                return;
            }
            const std::string token = ToLower(Trim(dialog.GetValue().ToStdString()));
            if (token.empty()) {
                return;
            }
            auto& custom = diagram_palette_custom_items_[active_type];
            if (std::find(custom.begin(), custom.end(), token) == custom.end()) {
                custom.push_back(token);
                RefreshDiagramPaletteControls(active_type);
                AppendLogLine("Palette item added: " + token + " type=" + active_type);
            }
        },
                  add_id);
        menu.Bind(wxEVT_MENU, [this, list](wxCommandEvent&) {
            const long selected = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (selected < 0) {
                return;
            }
            const std::string token = ToLower(Trim(list->GetItemText(selected).ToStdString()));
            const std::string active_type = CanonicalDiagramType(active_diagram_.diagram_type);
            auto custom_it = diagram_palette_custom_items_.find(active_type);
            if (custom_it == diagram_palette_custom_items_.end()) {
                AppendLogLine("Palette remove ignored (no custom entries) for type=" + active_type);
                return;
            }
            auto& custom = custom_it->second;
            auto erase_it = std::find(custom.begin(), custom.end(), token);
            if (erase_it == custom.end()) {
                AppendLogLine("Palette remove ignored for default item: " + token);
                return;
            }
            custom.erase(erase_it);
            RefreshDiagramPaletteControls(active_type);
            AppendLogLine("Palette item removed: " + token + " type=" + active_type);
        },
                  remove_id);
        menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            const std::string active_type = CanonicalDiagramType(active_diagram_.diagram_type);
            diagram_palette_custom_items_.erase(active_type);
            RefreshDiagramPaletteControls(active_type);
            AppendLogLine("Palette reset for type=" + active_type);
        },
                  reset_id);

        wxPoint point = event.GetPosition();
        if (point == wxDefaultPosition) {
            point = wxPoint(8, 8);
        } else {
            point = list->ScreenToClient(point);
        }
        list->PopupMenu(&menu, point);
    });
}

void MainFrame::ToggleDiagramPaletteDetached(bool detach) {
    if (detach) {
        if (diagram_palette_frame_ != nullptr) {
            diagram_palette_frame_->Show();
            diagram_palette_frame_->Raise();
            return;
        }
        if (diagram_splitter_ != nullptr && diagram_palette_panel_docked_ != nullptr && diagram_splitter_->IsSplit()) {
            diagram_splitter_->Unsplit(diagram_palette_panel_docked_);
        }

        diagram_palette_frame_ = new wxFrame(this,
                                             wxID_ANY,
                                             "",
                                             wxDefaultPosition,
                                             wxSize(240, 460),
                                             wxFRAME_TOOL_WINDOW | wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
        auto* panel = new wxPanel(diagram_palette_frame_, wxID_ANY);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* bar = new wxBoxSizer(wxHORIZONTAL);
        auto* attach_btn = new wxButton(panel, wxID_ANY, "Attach");
        bar->AddSpacer(1);
        bar->Add(attach_btn, 0, wxRIGHT, 2);
        sizer->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);

        diagram_palette_list_floating_ = new wxListCtrl(panel,
                                                        wxID_ANY,
                                                        wxDefaultPosition,
                                                        wxDefaultSize,
                                                        wxLC_ICON | wxLC_SINGLE_SEL | wxLC_AUTOARRANGE | wxLC_EDIT_LABELS |
                                                            wxBORDER_SIMPLE);
        sizer->Add(diagram_palette_list_floating_, 1, wxEXPAND | wxALL, 6);
        panel->SetSizer(sizer);

        BindDiagramPaletteInteractions(diagram_palette_list_floating_);
        RefreshDiagramPaletteControls(active_diagram_.diagram_type);
        attach_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ToggleDiagramPaletteDetached(false); });
        diagram_palette_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
            if (diagram_palette_frame_ == nullptr) {
                event.Skip();
                return;
            }
            event.Veto();
            ToggleDiagramPaletteDetached(false);
        });

        const wxPoint anchor = ClientToScreen(wxPoint(120, 160));
        diagram_palette_frame_->SetPosition(anchor);
        diagram_palette_frame_->Show();
        return;
    }

    if (diagram_palette_frame_ != nullptr) {
        wxFrame* frame = diagram_palette_frame_;
        diagram_palette_frame_ = nullptr;
        diagram_palette_list_floating_ = nullptr;
        frame->Destroy();
    }
    if (diagram_splitter_ != nullptr && diagram_palette_panel_docked_ != nullptr && diagram_canvas_panel_ != nullptr &&
        !diagram_splitter_->IsSplit()) {
        diagram_splitter_->SplitVertically(diagram_palette_panel_docked_, diagram_canvas_panel_, 220);
    }
    RefreshDiagramPaletteControls(active_diagram_.diagram_type);
}

void MainFrame::ExportDiagramToOutput(const std::string& format, wxTextCtrl* output) {
    if (output == nullptr) {
        return;
    }
    if (active_diagram_.diagram_id.empty()) {
        active_diagram_ = BuildSampleDiagram("active:auto", "Erd");
    }

    const std::string payload = diagram_service_.ExportDiagram(active_diagram_, format, "scratchbird");
    output->SetValue(ToWx(payload));
    AppendLogLine("Exported diagram as " + format);
}

void MainFrame::ExecuteCanvasAction(DiagramCanvasPanel* canvas,
                                    wxTextCtrl* output,
                                    const std::function<bool(std::string*)>& action) {
    if (canvas == nullptr || output == nullptr) {
        return;
    }
    std::string error;
    if (!action(&error) && !error.empty()) {
        output->SetValue(ToWx(error));
        AppendLogLine(error);
    }
}

void MainFrame::OpenOrFocusSqlEditorFrame() {
    EnsureDetachedSurfaceNotEmbedded(kWorkspacePageSql);
    if (sql_editor_frame_ != nullptr) {
        sql_editor_frame_->Show();
        sql_editor_frame_->Raise();
        return;
    }

    sql_editor_frame_ = new wxFrame(this, wxID_ANY, "SqlEditorFrame", wxDefaultPosition, wxSize(1300, 860));
    auto* panel = new wxPanel(sql_editor_frame_, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* dock_row = new wxBoxSizer(wxHORIZONTAL);
    dock_row->Add(new wxStaticText(panel, wxID_ANY, "Detached SQL Editor"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auto* dock_btn = new wxButton(panel, wxID_ANY, "Dock In Main");
    dock_row->Add(dock_btn, 0, wxRIGHT, 4);
    sizer->Add(dock_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 8);

    sql_editor_detached_ = new wxTextCtrl(panel,
                                          wxID_ANY,
                                          sql_editor_ != nullptr ? sql_editor_->GetValue() : "SELECT 1;",
                                          wxDefaultPosition,
                                          wxSize(-1, 170),
                                          wxTE_MULTILINE);
    sizer->Add(sql_editor_detached_, 0, wxEXPAND | wxALL, 8);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    auto* run_btn = new wxButton(panel, wxID_ANY, "Run SQL");
    auto* cancel_btn = new wxButton(panel, wxID_ANY, "Cancel");
    auto* export_btn = new wxButton(panel, wxID_ANY, "Export History CSV");
    buttons->Add(run_btn, 0, wxRIGHT, 6);
    buttons->Add(cancel_btn, 0, wxRIGHT, 6);
    buttons->Add(export_btn, 0, wxRIGHT, 6);
    sizer->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    sql_results_detached_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 160), wxLC_REPORT | wxLC_SINGLE_SEL);
    sql_results_detached_->InsertColumn(0, "Command", wxLIST_FORMAT_LEFT, 180);
    sql_results_detached_->InsertColumn(1, "Rows", wxLIST_FORMAT_LEFT, 90);
    sql_results_detached_->InsertColumn(2, "Message", wxLIST_FORMAT_LEFT, 450);
    sizer->Add(sql_results_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    sql_status_detached_ = new wxTextCtrl(panel,
                                          wxID_ANY,
                                          "",
                                          wxDefaultPosition,
                                          wxSize(-1, 90),
                                          wxTE_MULTILINE | wxTE_READONLY);
    sizer->Add(sql_status_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    sql_history_detached_ = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    sql_history_detached_->InsertColumn(0, "query_id", wxLIST_FORMAT_LEFT, 110);
    sql_history_detached_->InsertColumn(1, "profile", wxLIST_FORMAT_LEFT, 130);
    sql_history_detached_->InsertColumn(2, "started_at_utc", wxLIST_FORMAT_LEFT, 170);
    sql_history_detached_->InsertColumn(3, "status", wxLIST_FORMAT_LEFT, 100);
    sql_history_detached_->InsertColumn(4, "sql_hash", wxLIST_FORMAT_LEFT, 320);
    sizer->Add(sql_history_detached_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);

    run_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        RunSqlIntoSurface(sql_editor_detached_->GetValue().ToStdString(),
                          sql_results_detached_,
                          sql_status_detached_,
                          sql_history_detached_,
                          false);
    });
    cancel_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CancelSqlIntoStatus(sql_status_detached_); });
    export_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExportHistoryIntoStatus(sql_status_detached_, sql_history_detached_);
    });
    dock_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SelectWorkspacePage(kWorkspacePageSql); });

    PopulateHistoryList(sql_history_detached_);

    sql_editor_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (sql_editor_ != nullptr && sql_editor_detached_ != nullptr) {
            sql_editor_->SetValue(sql_editor_detached_->GetValue());
        }
        sql_editor_frame_ = nullptr;
        sql_editor_detached_ = nullptr;
        sql_results_detached_ = nullptr;
        sql_status_detached_ = nullptr;
        sql_history_detached_ = nullptr;
        event.Skip();
    });

    sql_editor_frame_->Show();
}

void MainFrame::OpenOrFocusObjectEditorFrame() {
    EnsureDetachedSurfaceNotEmbedded(kWorkspacePageObject);
    if (object_editor_frame_ != nullptr) {
        object_editor_frame_->Show();
        object_editor_frame_->Raise();
        return;
    }

    object_editor_frame_ = new wxFrame(this, wxID_ANY, "ObjectEditorFrame", wxDefaultPosition, wxSize(1100, 760));
    auto* panel = new wxPanel(object_editor_frame_, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* dock_row = new wxBoxSizer(wxHORIZONTAL);
    dock_row->Add(new wxStaticText(panel, wxID_ANY, "Detached Object Editor"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auto* dock_btn = new wxButton(panel, wxID_ANY, "Dock In Main");
    dock_row->Add(dock_btn, 0, wxRIGHT, 4);
    sizer->Add(dock_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(panel, wxID_ANY, "Class"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    object_class_detached_ = new wxChoice(panel, wxID_ANY);
    object_class_detached_->Append("TABLE");
    object_class_detached_->Append("VIEW");
    object_class_detached_->Append("INDEX");
    object_class_detached_->Append("TRIGGER");
    object_class_detached_->Append("PROCEDURE");
    object_class_detached_->SetSelection(object_class_ != nullptr ? object_class_->GetSelection() : 0);
    row->Add(object_class_detached_, 0, wxRIGHT, 10);

    row->Add(new wxStaticText(panel, wxID_ANY, "Path"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    object_path_detached_ = new wxTextCtrl(panel,
                                           wxID_ANY,
                                           object_path_ != nullptr ? object_path_->GetValue() : "public.customer");
    row->Add(object_path_detached_, 1, wxRIGHT, 8);

    auto* save_btn = new wxButton(panel, wxID_ANY, "Save Object");
    auto* migration_btn = new wxButton(panel, wxID_ANY, "Generate Migration");
    row->Add(save_btn, 0, wxRIGHT, 6);
    row->Add(migration_btn, 0, wxRIGHT, 6);
    sizer->Add(row, 0, wxEXPAND | wxALL, 8);

    object_ddl_detached_ = new wxTextCtrl(panel,
                                          wxID_ANY,
                                          object_ddl_ != nullptr ? object_ddl_->GetValue() : "CREATE TABLE ...",
                                          wxDefaultPosition,
                                          wxDefaultSize,
                                          wxTE_MULTILINE);
    sizer->Add(object_ddl_detached_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);

    save_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        try {
            SaveObjectFromControls(object_class_detached_, object_path_detached_, object_ddl_detached_);
        } catch (const RejectError& ex) {
            AppendLogLine(ex.what());
            wxMessageBox(ToWx(ex.what()), "Object save failed", wxOK | wxICON_ERROR, this);
        }
    });
    migration_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        try {
            GenerateMigrationIntoControls(object_class_detached_, object_path_detached_, object_ddl_detached_);
        } catch (const RejectError& ex) {
            AppendLogLine(ex.what());
            wxMessageBox(ToWx(ex.what()), "Migration generation failed", wxOK | wxICON_ERROR, this);
        }
    });
    dock_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SelectWorkspacePage(kWorkspacePageObject); });

    object_editor_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (object_class_ != nullptr && object_class_detached_ != nullptr) {
            object_class_->SetSelection(object_class_detached_->GetSelection());
        }
        if (object_path_ != nullptr && object_path_detached_ != nullptr) {
            object_path_->SetValue(object_path_detached_->GetValue());
        }
        if (object_ddl_ != nullptr && object_ddl_detached_ != nullptr) {
            object_ddl_->SetValue(object_ddl_detached_->GetValue());
        }
        object_editor_frame_ = nullptr;
        object_class_detached_ = nullptr;
        object_path_detached_ = nullptr;
        object_ddl_detached_ = nullptr;
        event.Skip();
    });

    BindDetachedFrameDropDock(object_editor_frame_, kWorkspacePageObject);
    object_editor_frame_->Show();
}

void MainFrame::OpenOrFocusDiagramFrame() {
    EnsureDetachedSurfaceNotEmbedded(kWorkspacePageDiagram);
    ToggleDiagramPaletteDetached(false);
    if (diagram_frame_ != nullptr) {
        diagram_frame_->Show();
        diagram_frame_->Raise();
        return;
    }

    const std::string frame_title =
        BuildDiagramHeadingText(active_diagram_.diagram_type.empty() ? "Erd" : active_diagram_.diagram_type, active_diagram_name_);
    diagram_frame_ = new wxFrame(this, wxID_ANY, ToWx(frame_title), wxDefaultPosition, wxSize(1200, 760));
    auto* panel = new wxPanel(diagram_frame_, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    diagram_links_detached_ = nullptr;
    diagram_heading_detached_ = new wxStaticText(panel, wxID_ANY, ToWx(frame_title));
    auto heading_font = diagram_heading_detached_->GetFont();
    heading_font.MakeBold();
    heading_font.SetPointSize(heading_font.GetPointSize() + 1);
    diagram_heading_detached_->SetFont(heading_font);
    sizer->Add(diagram_heading_detached_, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    diagram_type_choice_detached_ = new wxChoice(panel, wxID_ANY);
    diagram_type_choice_detached_->Append("ERD");
    diagram_type_choice_detached_->Append("Silverston");
    diagram_type_choice_detached_->Append("Whiteboard");
    diagram_type_choice_detached_->Append("Mind Map");
    SelectChoiceValue(diagram_type_choice_detached_, DiagramTypeDisplayName(active_diagram_.diagram_type));
    if (diagram_type_choice_detached_->GetSelection() == wxNOT_FOUND) {
        diagram_type_choice_detached_->SetSelection(0);
    }
    diagram_name_input_detached_ =
        new wxTextCtrl(panel, wxID_ANY, ToWx(active_diagram_name_.empty() ? "Core Domain ERD" : active_diagram_name_));
    auto* new_diagram_btn = new wxButton(panel, wxID_ANY, "New Diagram");
    auto* dock_btn = new wxButton(panel, wxID_ANY, "Dock In Main");
    auto* svg_btn = new wxButton(panel, wxID_ANY, "Export SVG");
    auto* png_btn = new wxButton(panel, wxID_ANY, "Export PNG");
    row->Add(new wxStaticText(panel, wxID_ANY, "Type"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row->Add(diagram_type_choice_detached_, 0, wxRIGHT, 8);
    row->Add(new wxStaticText(panel, wxID_ANY, "Name"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row->Add(diagram_name_input_detached_, 1, wxRIGHT, 8);
    row->Add(new_diagram_btn, 0, wxRIGHT, 8);
    row->Add(dock_btn, 0, wxRIGHT, 8);
    row->Add(svg_btn, 0, wxRIGHT, 6);
    row->Add(png_btn, 0, wxRIGHT, 6);
    sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* tools = new wxBoxSizer(wxHORIZONTAL);
    auto* nudge_left_btn = new wxButton(panel, wxID_ANY, "<");
    auto* nudge_up_btn = new wxButton(panel, wxID_ANY, "^");
    auto* nudge_down_btn = new wxButton(panel, wxID_ANY, "v");
    auto* nudge_right_btn = new wxButton(panel, wxID_ANY, ">");
    auto* resize_btn = new wxButton(panel, wxID_ANY, "Resize +");
    auto* connect_btn = new wxButton(panel, wxID_ANY, "Connect Next");
    auto* reparent_btn = new wxButton(panel, wxID_ANY, "Reparent");
    auto* add_node_btn = new wxButton(panel, wxID_ANY, "Add Node");
    auto* delete_btn = new wxButton(panel, wxID_ANY, "Delete Node");
    auto* delete_project_btn = new wxButton(panel, wxID_ANY, "Delete Project");
    auto* undo_btn = new wxButton(panel, wxID_ANY, "Undo");
    auto* redo_btn = new wxButton(panel, wxID_ANY, "Redo");
    auto* zoom_in_btn = new wxButton(panel, wxID_ANY, "Zoom +");
    auto* zoom_out_btn = new wxButton(panel, wxID_ANY, "Zoom -");
    auto* zoom_reset_btn = new wxButton(panel, wxID_ANY, "Zoom 100%");
    diagram_grid_toggle_detached_ = new wxCheckBox(panel, wxID_ANY, "Grid");
    diagram_grid_toggle_detached_->SetValue(true);
    diagram_snap_toggle_detached_ = new wxCheckBox(panel, wxID_ANY, "Snap");
    auto* silverston_type = new wxChoice(panel, wxID_ANY);
    for (const auto& type : SilverstonObjectTypes()) {
        silverston_type->Append(type);
    }
    silverston_type->SetSelection(1);
    auto* silverston_icon_catalog = new wxChoice(panel, wxID_ANY);
    auto* silverston_icon_slot = new wxTextCtrl(panel, wxID_ANY, DefaultSilverstonIconForType("entity"));
    RefreshSilverstonIconPicker(silverston_icon_catalog, "entity", silverston_icon_slot->GetValue().ToStdString());
    auto* silverston_display_mode = new wxChoice(panel, wxID_ANY);
    for (const auto& mode : SilverstonDisplayModes()) {
        silverston_display_mode->Append(mode);
    }
    silverston_display_mode->SetSelection(2);
    auto* silverston_chamfer = new wxCheckBox(panel, wxID_ANY, "Chamfer Notes");
    auto* silverston_apply_node = new wxButton(panel, wxID_ANY, "Apply Node Profile");
    auto* silverston_grid_size = new wxTextCtrl(panel, wxID_ANY, "20");
    auto* silverston_alignment = new wxChoice(panel, wxID_ANY);
    for (const auto& policy : SilverstonAlignmentPolicies()) {
        silverston_alignment->Append(policy);
    }
    silverston_alignment->SetSelection(0);
    auto* silverston_drop = new wxChoice(panel, wxID_ANY);
    for (const auto& policy : SilverstonDropPolicies()) {
        silverston_drop->Append(policy);
    }
    silverston_drop->SetSelection(0);
    auto* silverston_resize = new wxChoice(panel, wxID_ANY);
    for (const auto& policy : SilverstonResizePolicies()) {
        silverston_resize->Append(policy);
    }
    silverston_resize->SetSelection(0);
    auto* silverston_display_profile = new wxChoice(panel, wxID_ANY);
    for (const auto& profile : SilverstonDisplayProfiles()) {
        silverston_display_profile->Append(profile);
    }
    silverston_display_profile->SetSelection(0);
    auto* silverston_preset = new wxChoice(panel, wxID_ANY);
    for (const auto& preset : SilverstonPresetNames()) {
        silverston_preset->Append(preset);
    }
    silverston_preset->SetSelection(0);
    auto* silverston_apply_preset = new wxButton(panel, wxID_ANY, "Apply Preset");
    auto* silverston_validation_hint = new wxStaticText(panel, wxID_ANY, "");
    auto* silverston_apply_diagram = new wxButton(panel, wxID_ANY, "Apply Diagram Policy");

    tools->Add(new wxStaticText(panel, wxID_ANY, "Canvas"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    tools->Add(nudge_left_btn, 0, wxRIGHT, 4);
    tools->Add(nudge_up_btn, 0, wxRIGHT, 4);
    tools->Add(nudge_down_btn, 0, wxRIGHT, 4);
    tools->Add(nudge_right_btn, 0, wxRIGHT, 8);
    tools->Add(resize_btn, 0, wxRIGHT, 6);
    tools->Add(connect_btn, 0, wxRIGHT, 6);
    tools->Add(reparent_btn, 0, wxRIGHT, 6);
    tools->Add(add_node_btn, 0, wxRIGHT, 6);
    tools->Add(delete_btn, 0, wxRIGHT, 8);
    tools->Add(delete_project_btn, 0, wxRIGHT, 8);
    tools->Add(undo_btn, 0, wxRIGHT, 4);
    tools->Add(redo_btn, 0, wxRIGHT, 8);
    tools->Add(zoom_in_btn, 0, wxRIGHT, 4);
    tools->Add(zoom_out_btn, 0, wxRIGHT, 4);
    tools->Add(zoom_reset_btn, 0, wxRIGHT, 10);
    tools->Add(diagram_grid_toggle_detached_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    tools->Add(diagram_snap_toggle_detached_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(tools, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* silverston_node_row = new wxBoxSizer(wxHORIZONTAL);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Silverston Node"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Type"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_type, 0, wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Catalog"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_icon_catalog, 0, wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Icon Slot"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_icon_slot, 0, wxRIGHT, 8);
    silverston_node_row->Add(new wxStaticText(panel, wxID_ANY, "Display"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_node_row->Add(silverston_display_mode, 0, wxRIGHT, 8);
    silverston_node_row->Add(silverston_chamfer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_node_row->Add(silverston_apply_node, 0, wxRIGHT, 8);
    sizer->Add(silverston_node_row, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* silverston_policy_row = new wxBoxSizer(wxHORIZONTAL);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Silverston Diagram"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Grid"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_grid_size, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Align"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_alignment, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Drop"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_drop, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Resize"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_resize, 0, wxRIGHT, 8);
    silverston_policy_row->Add(new wxStaticText(panel, wxID_ANY, "Display"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    silverston_policy_row->Add(silverston_display_profile, 0, wxRIGHT, 8);
    silverston_policy_row->Add(silverston_apply_diagram, 0, wxRIGHT, 8);
    sizer->Add(silverston_policy_row, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* silverston_preset_row = new wxBoxSizer(wxHORIZONTAL);
    silverston_preset_row->Add(new wxStaticText(panel, wxID_ANY, "Silverston Preset"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    silverston_preset_row->Add(silverston_preset, 0, wxRIGHT, 8);
    silverston_preset_row->Add(silverston_apply_preset, 0, wxRIGHT, 8);
    silverston_preset_row->Add(silverston_validation_hint, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(silverston_preset_row, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(140);
    splitter->SetSashGravity(0.0);
    auto* palette_panel = new wxPanel(splitter, wxID_ANY);
    auto* palette_sizer = new wxBoxSizer(wxVERTICAL);
    auto* palette_bar = new wxBoxSizer(wxHORIZONTAL);
    auto* attach_palette_btn = new wxButton(palette_panel, wxID_ANY, "Attach");
    palette_bar->Add(new wxStaticText(palette_panel, wxID_ANY, ""), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    palette_bar->Add(attach_palette_btn, 0, wxRIGHT, 2);
    palette_sizer->Add(palette_bar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
    diagram_palette_list_detached_ =
        new wxListCtrl(palette_panel,
                       wxID_ANY,
                       wxDefaultPosition,
                       wxDefaultSize,
                       wxLC_ICON | wxLC_SINGLE_SEL | wxLC_AUTOARRANGE | wxLC_EDIT_LABELS | wxBORDER_SIMPLE);
    palette_sizer->Add(diagram_palette_list_detached_, 1, wxEXPAND | wxALL, 6);
    palette_panel->SetSizer(palette_sizer);

    auto* canvas_panel = new wxPanel(splitter, wxID_ANY);
    auto* canvas_panel_sizer = new wxBoxSizer(wxVERTICAL);
    diagram_canvas_detached_ = new DiagramCanvasPanel(canvas_panel, &diagram_service_);
    canvas_panel_sizer->Add(new wxStaticText(canvas_panel, wxID_ANY, "Diagram Canvas"), 0, wxLEFT | wxRIGHT | wxTOP, 2);
    canvas_panel_sizer->Add(diagram_canvas_detached_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);
    diagram_output_detached_ = new wxTextCtrl(canvas_panel,
                                              wxID_ANY,
                                              "",
                                              wxDefaultPosition,
                                              wxSize(-1, 120),
                                              wxTE_MULTILINE | wxTE_READONLY);
    canvas_panel_sizer->Add(new wxStaticText(canvas_panel, wxID_ANY, "Diagram Output"), 0, wxLEFT | wxRIGHT | wxTOP, 4);
    canvas_panel_sizer->Add(diagram_output_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 4);
    canvas_panel->SetSizer(canvas_panel_sizer);

    splitter->SplitVertically(palette_panel, canvas_panel, 220);
    sizer->Add(splitter, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    panel->SetSizer(sizer);
    if (active_diagram_.diagram_id.empty()) {
        active_diagram_ = BuildSampleDiagram("active:detached", "Erd");
    }
    if (diagram_canvas_detached_ != nullptr) {
        diagram_canvas_detached_->SetDocument(&active_diagram_);
        diagram_canvas_detached_->SetStatusSink([this](const std::string& message) {
            if (diagram_output_detached_ != nullptr) {
                diagram_output_detached_->SetValue(ToWx(message));
            }
            AppendLogLine("Diagram canvas(detached): " + message);
        });
        diagram_canvas_detached_->SetMutationSink([this](const std::string& mutation) {
            AppendLogLine("Diagram mutation(detached): " + mutation);
            RefreshCatalog();
            SetStatusText(ToWx("Diagram dirty: " + mutation), 0);
        });
    }
    RefreshDiagramPresentation();

    auto refresh_silverston_hint =
        [silverston_type, silverston_icon_catalog, silverston_icon_slot, silverston_validation_hint]() {
            const std::string object_type = silverston_type->GetStringSelection().ToStdString();
            const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
            RefreshSilverstonIconPicker(silverston_icon_catalog, object_type, icon_slot);
            UpdateSilverstonValidationHint(silverston_validation_hint, object_type, icon_slot);
        };
    refresh_silverston_hint();
    silverston_type->Bind(wxEVT_CHOICE,
                          [silverston_type, silverston_icon_slot, refresh_silverston_hint](wxCommandEvent&) {
        const std::string selected_type = silverston_type->GetStringSelection().ToStdString();
        const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
        if (!IsSilverstonIconAllowed(selected_type, icon_slot)) {
            silverston_icon_slot->SetValue(DefaultSilverstonIconForType(selected_type));
        }
        refresh_silverston_hint();
    });
    silverston_icon_catalog->Bind(wxEVT_CHOICE,
                                  [silverston_icon_catalog, silverston_icon_slot, refresh_silverston_hint](wxCommandEvent&) {
        silverston_icon_slot->SetValue(silverston_icon_catalog->GetStringSelection());
        refresh_silverston_hint();
    });
    silverston_icon_slot->Bind(wxEVT_TEXT, [refresh_silverston_hint](wxCommandEvent&) { refresh_silverston_hint(); });
    if (diagram_canvas_detached_ != nullptr) {
        diagram_canvas_detached_->SetSelectionSink(
            [this, silverston_type, silverston_icon_slot, silverston_display_mode, silverston_chamfer,
             silverston_validation_hint, refresh_silverston_hint](const std::string& node_id,
                                                                  const std::string& object_type,
                                                                  const std::string& icon_slot,
                                                                  const std::string& display_mode,
                                                                  bool chamfer_notes) {
                if (ToLower(active_diagram_.diagram_type) != "silverston") {
                    silverston_validation_hint->SetForegroundColour(wxColour(96, 96, 96));
                    silverston_validation_hint->SetLabel("Open a Silverston diagram to use Silverston editor controls.");
                    return;
                }
                if (node_id.empty()) {
                    silverston_validation_hint->SetForegroundColour(wxColour(96, 96, 96));
                    silverston_validation_hint->SetLabel("Select a Silverston node to inspect/edit profile settings.");
                    return;
                }
                SelectChoiceValue(silverston_type, object_type.empty() ? "entity" : object_type);
                const std::string selected_type = silverston_type->GetStringSelection().ToStdString();
                std::string selected_icon = ToLower(icon_slot);
                if (!IsSilverstonIconAllowed(selected_type, selected_icon)) {
                    selected_icon = DefaultSilverstonIconForType(selected_type);
                }
                silverston_icon_slot->SetValue(selected_icon);
                SelectChoiceValue(silverston_display_mode, display_mode.empty() ? "full" : display_mode);
                silverston_chamfer->SetValue(chamfer_notes);
                refresh_silverston_hint();
            });
    }

    BindDiagramPaletteInteractions(diagram_palette_list_detached_);
    RefreshDiagramPaletteControls(active_diagram_.diagram_type);

    if (diagram_type_choice_detached_ != nullptr) {
        diagram_type_choice_detached_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            if (diagram_type_choice_detached_ == nullptr) {
                return;
            }
            RefreshDiagramPaletteControls(diagram_type_choice_detached_->GetStringSelection().ToStdString());
            if (diagram_name_input_detached_ != nullptr && diagram_name_input_detached_->GetValue().ToStdString().empty()) {
                diagram_name_input_detached_->SetValue(
                    DefaultDiagramNameForType(diagram_type_choice_detached_->GetStringSelection().ToStdString()));
            }
        });
    }
    if (new_diagram_btn != nullptr) {
        new_diagram_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            std::string type_name = "ERD";
            if (diagram_type_choice_detached_ != nullptr) {
                type_name = diagram_type_choice_detached_->GetStringSelection().ToStdString();
            }
            std::string name;
            if (diagram_name_input_detached_ != nullptr) {
                name = Trim(diagram_name_input_detached_->GetValue().ToStdString());
            }
            if (name.empty()) {
                wxTextEntryDialog dialog(this,
                                         "Enter a diagram name",
                                         "Create Diagram",
                                         DefaultDiagramNameForType(type_name));
                if (dialog.ShowModal() != wxID_OK) {
                    return;
                }
                name = Trim(dialog.GetValue().ToStdString());
                if (diagram_name_input_detached_ != nullptr) {
                    diagram_name_input_detached_->SetValue(ToWx(name));
                }
            }
            if (name.empty()) {
                if (diagram_output_detached_ != nullptr) {
                    diagram_output_detached_->SetValue("diagram name is required");
                }
                return;
            }
            OpenDiagramByTypeAndName(type_name, name, diagram_output_detached_);
        });
    }

    auto dock_to_main = [this](wxCommandEvent&) {
        SelectWorkspacePage(kWorkspacePageDiagram);
    };
    if (dock_btn != nullptr) {
        dock_btn->Bind(wxEVT_BUTTON, dock_to_main);
    }
    if (attach_palette_btn != nullptr) {
        attach_palette_btn->Bind(wxEVT_BUTTON, dock_to_main);
    }

    svg_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        try {
            ExportDiagramToOutput("svg", diagram_output_detached_);
        } catch (const RejectError& ex) {
            diagram_output_detached_->SetValue(ToWx(ex.what()));
            AppendLogLine(ex.what());
        }
    });
    png_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        try {
            ExportDiagramToOutput("png", diagram_output_detached_);
        } catch (const RejectError& ex) {
            diagram_output_detached_->SetValue(ToWx(ex.what()));
            AppendLogLine(ex.what());
        }
    });

    nudge_left_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->NudgeSelectedNode(-20, 0, error);
        });
    });
    nudge_up_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->NudgeSelectedNode(0, -20, error);
        });
    });
    nudge_down_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->NudgeSelectedNode(0, 20, error);
        });
    });
    nudge_right_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->NudgeSelectedNode(20, 0, error);
        });
    });
    resize_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->ResizeSelectedNode(20, 10, error);
        });
    });
    connect_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->ConnectSelectedToNext(error);
        });
    });
    reparent_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->ReparentSelectedToNext(error);
        });
    });
    add_node_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->AddNode(error);
        });
    });
    delete_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->DeleteSelectedNode(false, error);
        });
    });
    delete_project_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->DeleteSelectedNode(true, error);
        });
    });
    undo_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->Undo(error);
        });
    });
    redo_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_, [this](std::string* error) {
            return diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->Redo(error);
        });
    });
    zoom_in_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (diagram_canvas_detached_ != nullptr) {
            diagram_canvas_detached_->ZoomIn();
        }
    });
    zoom_out_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (diagram_canvas_detached_ != nullptr) {
            diagram_canvas_detached_->ZoomOut();
        }
    });
    zoom_reset_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (diagram_canvas_detached_ != nullptr) {
            diagram_canvas_detached_->ZoomReset();
        }
    });
    diagram_grid_toggle_detached_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        if (diagram_canvas_detached_ != nullptr) {
            diagram_canvas_detached_->SetGridVisible(event.IsChecked());
        }
    });
    diagram_snap_toggle_detached_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        if (diagram_canvas_detached_ != nullptr) {
            diagram_canvas_detached_->SetSnapToGrid(event.IsChecked());
        }
    });
    silverston_apply_node->Bind(wxEVT_BUTTON,
                                [this, silverston_type, silverston_icon_slot, silverston_display_mode, silverston_chamfer](wxCommandEvent&) {
        if (ToLower(active_diagram_.diagram_type) != "silverston") {
            if (diagram_output_detached_ != nullptr) {
                diagram_output_detached_->SetValue("Silverston node editor requires a Silverston diagram.");
            }
            return;
        }
        const std::string object_type = silverston_type->GetStringSelection().ToStdString();
        const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
        if (!IsSilverstonIconAllowed(object_type, icon_slot)) {
            if (diagram_output_detached_ != nullptr) {
                diagram_output_detached_->SetValue("Silverston icon slot is invalid for selected object type.");
            }
            return;
        }
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_,
                            [this, silverston_type, icon_slot, silverston_display_mode,
                             silverston_chamfer](std::string* error) {
            return diagram_canvas_detached_ != nullptr &&
                   diagram_canvas_detached_->ApplySilverstonNodeProfile(silverston_type->GetStringSelection().ToStdString(),
                                                                        icon_slot,
                                                                        silverston_display_mode->GetStringSelection().ToStdString(),
                                                                        silverston_chamfer->GetValue(),
                                                                        error);
        });
    });
    silverston_apply_diagram->Bind(wxEVT_BUTTON,
                                   [this, silverston_grid_size, silverston_alignment, silverston_drop, silverston_resize,
                                    silverston_display_profile](wxCommandEvent&) {
        if (ToLower(active_diagram_.diagram_type) != "silverston") {
            if (diagram_output_detached_ != nullptr) {
                diagram_output_detached_->SetValue("Silverston diagram editor requires a Silverston diagram.");
            }
            return;
        }
        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_,
                            [this, silverston_grid_size, silverston_alignment, silverston_drop, silverston_resize,
                             silverston_display_profile](std::string* error) {
            int parsed_grid = 20;
            try {
                parsed_grid = std::stoi(silverston_grid_size->GetValue().ToStdString());
            } catch (...) {
                if (error != nullptr) {
                    *error = "invalid grid size";
                }
                return false;
            }
            const bool ok = diagram_canvas_detached_ != nullptr &&
                            diagram_canvas_detached_->ApplySilverstonDiagramPolicy(
                                parsed_grid,
                                silverston_alignment->GetStringSelection().ToStdString(),
                                silverston_drop->GetStringSelection().ToStdString(),
                                silverston_resize->GetStringSelection().ToStdString(),
                                silverston_display_profile->GetStringSelection().ToStdString(),
                                error);
            if (ok) {
                diagram_grid_toggle_detached_->SetValue(true);
                diagram_snap_toggle_detached_->SetValue(ToLower(silverston_alignment->GetStringSelection().ToStdString()) != "free");
            }
            return ok;
        });
    });
    silverston_apply_preset->Bind(wxEVT_BUTTON,
                                  [this, silverston_preset, silverston_type, silverston_icon_slot, silverston_display_mode,
                                   silverston_chamfer, silverston_grid_size, silverston_alignment, silverston_drop,
                                   silverston_resize, silverston_display_profile, refresh_silverston_hint](wxCommandEvent&) {
        if (ToLower(active_diagram_.diagram_type) != "silverston") {
            if (diagram_output_detached_ != nullptr) {
                diagram_output_detached_->SetValue("Silverston presets require a Silverston diagram.");
            }
            return;
        }
        SilverstonPreset preset;
        if (!ResolveSilverstonPreset(silverston_preset->GetStringSelection().ToStdString(), &preset)) {
            if (diagram_output_detached_ != nullptr) {
                diagram_output_detached_->SetValue("Unknown Silverston preset.");
            }
            return;
        }
        SelectChoiceValue(silverston_display_mode, preset.node_display_mode);
        silverston_grid_size->SetValue(std::to_string(preset.grid_size));
        SelectChoiceValue(silverston_alignment, preset.alignment_policy);
        SelectChoiceValue(silverston_drop, preset.drop_policy);
        SelectChoiceValue(silverston_resize, preset.resize_policy);
        SelectChoiceValue(silverston_display_profile, preset.display_profile);

        const std::string selected_type = silverston_type->GetStringSelection().ToStdString();
        const std::string icon_slot = ToLower(silverston_icon_slot->GetValue().ToStdString());
        if (!IsSilverstonIconAllowed(selected_type, icon_slot)) {
            silverston_icon_slot->SetValue(DefaultSilverstonIconForType(selected_type));
        }
        refresh_silverston_hint();

        ExecuteCanvasAction(diagram_canvas_detached_, diagram_output_detached_,
                            [this, silverston_grid_size, silverston_alignment, silverston_drop, silverston_resize,
                             silverston_display_profile, silverston_type, silverston_icon_slot, silverston_display_mode,
                             silverston_chamfer](std::string* error) {
            int parsed_grid = 20;
            try {
                parsed_grid = std::stoi(silverston_grid_size->GetValue().ToStdString());
            } catch (...) {
                if (error != nullptr) {
                    *error = "invalid grid size";
                }
                return false;
            }
            const bool diagram_ok = diagram_canvas_detached_ != nullptr &&
                                    diagram_canvas_detached_->ApplySilverstonDiagramPolicy(
                                        parsed_grid,
                                        silverston_alignment->GetStringSelection().ToStdString(),
                                        silverston_drop->GetStringSelection().ToStdString(),
                                        silverston_resize->GetStringSelection().ToStdString(),
                                        silverston_display_profile->GetStringSelection().ToStdString(),
                                        error);
            if (!diagram_ok) {
                return false;
            }
            if (diagram_canvas_detached_->SelectedNodeId().empty()) {
                return true;
            }
            return diagram_canvas_detached_->ApplySilverstonNodeProfile(
                silverston_type->GetStringSelection().ToStdString(),
                ToLower(silverston_icon_slot->GetValue().ToStdString()),
                silverston_display_mode->GetStringSelection().ToStdString(),
                silverston_chamfer->GetValue(),
                error);
        });
        if (diagram_canvas_detached_ != nullptr && diagram_canvas_detached_->SelectedNodeId().empty() &&
            diagram_output_detached_ != nullptr) {
            diagram_output_detached_->SetValue(
                "Preset applied to diagram policy. Select a node and apply node profile to update node visuals.");
        }
    });

    diagram_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (diagram_type_choice_detached_ != nullptr) {
            active_diagram_.diagram_type = CanonicalDiagramType(diagram_type_choice_detached_->GetStringSelection().ToStdString());
        }
        if (diagram_name_input_detached_ != nullptr) {
            const std::string detached_name = Trim(diagram_name_input_detached_->GetValue().ToStdString());
            if (!detached_name.empty()) {
                active_diagram_name_ = detached_name;
            }
        }
        RefreshDiagramPresentation();
        diagram_frame_ = nullptr;
        diagram_links_detached_ = nullptr;
        diagram_heading_detached_ = nullptr;
        diagram_type_choice_detached_ = nullptr;
        diagram_name_input_detached_ = nullptr;
        diagram_palette_list_detached_ = nullptr;
        diagram_output_detached_ = nullptr;
        diagram_canvas_detached_ = nullptr;
        diagram_grid_toggle_detached_ = nullptr;
        diagram_snap_toggle_detached_ = nullptr;
        event.Skip();
    });

    BindDetachedFrameDropDock(diagram_frame_, kWorkspacePageDiagram);
    diagram_frame_->Show();
}

void MainFrame::OpenOrFocusMonitoringFrame() {
    EnsureDetachedSurfaceNotEmbedded(kWorkspacePageMonitoring);
    if (monitoring_frame_ != nullptr) {
        monitoring_frame_->Show();
        monitoring_frame_->Raise();
        return;
    }

    monitoring_frame_ = new wxFrame(this, wxID_ANY, "MonitoringFrame", wxDefaultPosition, wxSize(980, 640));
    auto* panel = new wxPanel(monitoring_frame_, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* top_row = new wxBoxSizer(wxHORIZONTAL);
    auto* refresh_btn = new wxButton(panel, wxID_ANY, "Refresh Metrics");
    auto* dock_btn = new wxButton(panel, wxID_ANY, "Dock In Main");
    top_row->Add(refresh_btn, 0, wxRIGHT, 6);
    top_row->Add(dock_btn, 0, wxRIGHT, 6);
    sizer->Add(top_row, 0, wxALL, 8);

    monitoring_rows_detached_ =
        new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    monitoring_rows_detached_->InsertColumn(0, "metric_key", wxLIST_FORMAT_LEFT, 220);
    monitoring_rows_detached_->InsertColumn(1, "sample_count", wxLIST_FORMAT_LEFT, 120);
    monitoring_rows_detached_->InsertColumn(2, "total_value", wxLIST_FORMAT_LEFT, 180);
    sizer->Add(monitoring_rows_detached_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);

    refresh_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        try {
            RefreshMonitoringList(monitoring_rows_detached_);
        } catch (const RejectError& ex) {
            AppendLogLine(ex.what());
        }
    });
    dock_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SelectWorkspacePage(kWorkspacePageMonitoring); });

    RefreshMonitoringList(monitoring_rows_detached_);

    monitoring_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        monitoring_frame_ = nullptr;
        monitoring_rows_detached_ = nullptr;
        event.Skip();
    });

    BindDetachedFrameDropDock(monitoring_frame_, kWorkspacePageMonitoring);
    monitoring_frame_->Show();
}

void MainFrame::OpenOrFocusReportingFrame() {
    if (reporting_frame_ != nullptr) {
        reporting_frame_->Show();
        reporting_frame_->Raise();
        return;
    }

    reporting_frame_ = new wxFrame(this, wxID_ANY, "ReportingFrame", wxDefaultPosition, wxSize(1300, 860));
    auto* panel = new wxPanel(reporting_frame_, wxID_ANY);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* context_row = new wxBoxSizer(wxHORIZONTAL);
    auto* question_id = new wxTextCtrl(panel, wxID_ANY, "question:q1");
    auto* connection_id = new wxTextCtrl(panel, wxID_ANY, ActiveProfileName());
    auto* role_id = new wxTextCtrl(panel, wxID_ANY, "owner");
    auto* env_id = new wxTextCtrl(panel, wxID_ANY, "dev");
    auto* timeout_ms = new wxTextCtrl(panel, wxID_ANY, "30000");
    auto* validate_only = new wxCheckBox(panel, wxID_ANY, "Validate Only");
    auto* dry_run = new wxCheckBox(panel, wxID_ANY, "Dry Run");
    auto* bypass_cache = new wxCheckBox(panel, wxID_ANY, "Bypass Cache");
    context_row->Add(new wxStaticText(panel, wxID_ANY, "Question"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    context_row->Add(question_id, 0, wxRIGHT, 8);
    context_row->Add(new wxStaticText(panel, wxID_ANY, "Connection"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    context_row->Add(connection_id, 0, wxRIGHT, 8);
    context_row->Add(new wxStaticText(panel, wxID_ANY, "Role"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    context_row->Add(role_id, 0, wxRIGHT, 8);
    context_row->Add(new wxStaticText(panel, wxID_ANY, "Env"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    context_row->Add(env_id, 0, wxRIGHT, 8);
    context_row->Add(new wxStaticText(panel, wxID_ANY, "Timeout"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    context_row->Add(timeout_ms, 0, wxRIGHT, 8);
    context_row->Add(validate_only, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    context_row->Add(dry_run, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    context_row->Add(bypass_cache, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    root->Add(context_row, 0, wxEXPAND | wxALL, 8);

    reporting_sql_detached_ = new wxTextCtrl(panel,
                                             wxID_ANY,
                                             "SELECT id, name FROM customer WHERE active = 1 ORDER BY name LIMIT 25;",
                                             wxDefaultPosition,
                                             wxSize(-1, 170),
                                             wxTE_MULTILINE);
    root->Add(reporting_sql_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    auto* run_question_btn = new wxButton(panel, wxID_ANY, "Run Question");
    auto* run_dashboard_btn = new wxButton(panel, wxID_ANY, "Run Dashboard");
    auto* invalidate_conn_btn = new wxButton(panel, wxID_ANY, "Invalidate Conn Cache");
    auto* invalidate_all_btn = new wxButton(panel, wxID_ANY, "Invalidate All Cache");
    buttons->Add(run_question_btn, 0, wxRIGHT, 6);
    buttons->Add(run_dashboard_btn, 0, wxRIGHT, 6);
    buttons->Add(invalidate_conn_btn, 0, wxRIGHT, 6);
    buttons->Add(invalidate_all_btn, 0, wxRIGHT, 6);
    root->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    reporting_status_detached_ = new wxTextCtrl(panel,
                                                wxID_ANY,
                                                "",
                                                wxDefaultPosition,
                                                wxSize(-1, 180),
                                                wxTE_MULTILINE | wxTE_READONLY);
    root->Add(new wxStaticText(panel, wxID_ANY, "Question Runtime Output"), 0, wxLEFT | wxRIGHT, 8);
    root->Add(reporting_status_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    reporting_dashboard_output_detached_ = new wxTextCtrl(panel,
                                                          wxID_ANY,
                                                          "",
                                                          wxDefaultPosition,
                                                          wxSize(-1, 140),
                                                          wxTE_MULTILINE | wxTE_READONLY);
    root->Add(new wxStaticText(panel, wxID_ANY, "Dashboard Runtime Output"), 0, wxLEFT | wxRIGHT, 8);
    root->Add(reporting_dashboard_output_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* asset_row = new wxBoxSizer(wxHORIZONTAL);
    auto* asset_id = new wxTextCtrl(panel, wxID_ANY, "asset:q1");
    auto* asset_type = new wxChoice(panel, wxID_ANY);
    for (const auto& label : {"Question", "Dashboard", "Model", "Metric", "Segment", "Alert", "Subscription",
                              "Collection", "Timeline"}) {
        asset_type->Append(label);
    }
    asset_type->SetSelection(0);
    auto* asset_name = new wxTextCtrl(panel, wxID_ANY, "Adhoc Question");
    auto* asset_collection = new wxTextCtrl(panel, wxID_ANY, "default");
    auto* save_asset_btn = new wxButton(panel, wxID_ANY, "Save Asset");
    auto* load_collection_btn = new wxButton(panel, wxID_ANY, "Load Collection");
    asset_row->Add(new wxStaticText(panel, wxID_ANY, "Asset"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    asset_row->Add(asset_id, 0, wxRIGHT, 6);
    asset_row->Add(asset_type, 0, wxRIGHT, 6);
    asset_row->Add(asset_name, 1, wxRIGHT, 6);
    asset_row->Add(asset_collection, 0, wxRIGHT, 6);
    asset_row->Add(save_asset_btn, 0, wxRIGHT, 6);
    asset_row->Add(load_collection_btn, 0, wxRIGHT, 6);
    root->Add(asset_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    reporting_repository_rows_detached_ =
        new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    reporting_repository_rows_detached_->InsertColumn(0, "id", wxLIST_FORMAT_LEFT, 220);
    reporting_repository_rows_detached_->InsertColumn(1, "asset_type", wxLIST_FORMAT_LEFT, 120);
    reporting_repository_rows_detached_->InsertColumn(2, "name", wxLIST_FORMAT_LEFT, 260);
    reporting_repository_rows_detached_->InsertColumn(3, "collection", wxLIST_FORMAT_LEFT, 140);
    reporting_repository_rows_detached_->InsertColumn(4, "updated_at_utc", wxLIST_FORMAT_LEFT, 180);
    root->Add(reporting_repository_rows_detached_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(root);

    const auto refresh_repository = [this](const std::vector<beta1b::ReportingAsset>& assets) {
        if (reporting_repository_rows_detached_ == nullptr) {
            return;
        }
        reporting_repository_rows_detached_->DeleteAllItems();
        for (const auto& asset : assets) {
            const long row = reporting_repository_rows_detached_->InsertItem(reporting_repository_rows_detached_->GetItemCount(),
                                                                             ToWx(asset.id));
            reporting_repository_rows_detached_->SetItem(row, 1, ToWx(asset.asset_type));
            reporting_repository_rows_detached_->SetItem(row, 2, ToWx(asset.name));
            reporting_repository_rows_detached_->SetItem(row, 3, ToWx(asset.collection_id));
            reporting_repository_rows_detached_->SetItem(row, 4, ToWx(asset.updated_at_utc));
        }
    };

    run_question_btn->Bind(wxEVT_BUTTON, [this, question_id, connection_id, role_id, env_id, timeout_ms,
                                          validate_only, dry_run, bypass_cache](wxCommandEvent&) {
        try {
            reporting::QueryExecutionContext ctx;
            ctx.connection_id = connection_id->GetValue().ToStdString();
            ctx.role_id = role_id->GetValue().ToStdString();
            ctx.environment_id = env_id->GetValue().ToStdString();
            ctx.params_json = "{}";

            reporting::QueryExecutionOptions options;
            options.validate_only = validate_only->GetValue();
            options.dry_run = dry_run->GetValue();
            options.bypass_cache = bypass_cache->GetValue();
            options.timeout_ms = std::max(1, std::stoi(timeout_ms->GetValue().ToStdString()));

            const std::string payload = reporting_service_.RunQuestionWithContext(
                question_id->GetValue().ToStdString(),
                true,
                reporting_sql_detached_ != nullptr ? reporting_sql_detached_->GetValue().ToStdString() : "select 1",
                ctx,
                options);
            if (reporting_status_detached_ != nullptr) {
                reporting_status_detached_->SetValue(ToWx(payload));
            }
            AppendLogLine("Reporting question executed id=" + question_id->GetValue().ToStdString());
        } catch (const std::exception& ex) {
            if (reporting_status_detached_ != nullptr) {
                reporting_status_detached_->SetValue(ToWx(ex.what()));
            }
            AppendLogLine(ex.what());
        }
    });

    run_dashboard_btn->Bind(wxEVT_BUTTON, [this, question_id, connection_id, role_id, env_id, timeout_ms,
                                           validate_only, dry_run, bypass_cache](wxCommandEvent&) {
        try {
            reporting::QueryExecutionContext ctx;
            ctx.connection_id = connection_id->GetValue().ToStdString();
            ctx.role_id = role_id->GetValue().ToStdString();
            ctx.environment_id = env_id->GetValue().ToStdString();
            ctx.params_json = "{}";

            reporting::QueryExecutionOptions options;
            options.validate_only = validate_only->GetValue();
            options.dry_run = dry_run->GetValue();
            options.bypass_cache = bypass_cache->GetValue();
            options.timeout_ms = std::max(1, std::stoi(timeout_ms->GetValue().ToStdString()));

            std::vector<reporting::DashboardWidgetRequest> widgets;
            widgets.push_back({"w_primary", "dataset:primary",
                               reporting_sql_detached_ != nullptr ? reporting_sql_detached_->GetValue().ToStdString() : "select 1"});
            widgets.push_back({"w_health", "dataset:health", "select 1"});
            const std::string payload = reporting_service_.RunDashboardWithQueries(
                "dashboard:" + question_id->GetValue().ToStdString(), widgets, ctx, options);
            if (reporting_dashboard_output_detached_ != nullptr) {
                reporting_dashboard_output_detached_->SetValue(ToWx(payload));
            }
            AppendLogLine("Reporting dashboard executed for " + question_id->GetValue().ToStdString());
        } catch (const std::exception& ex) {
            if (reporting_dashboard_output_detached_ != nullptr) {
                reporting_dashboard_output_detached_->SetValue(ToWx(ex.what()));
            }
            AppendLogLine(ex.what());
        }
    });

    invalidate_conn_btn->Bind(wxEVT_BUTTON, [this, connection_id](wxCommandEvent&) {
        const auto removed = reporting_service_.InvalidateCacheByConnection(connection_id->GetValue().ToStdString());
        AppendLogLine("Reporting cache invalidated by connection removed=" + std::to_string(removed));
    });
    invalidate_all_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        const auto removed = reporting_service_.InvalidateAllCache();
        AppendLogLine("Reporting cache invalidated globally removed=" + std::to_string(removed));
    });

    save_asset_btn->Bind(wxEVT_BUTTON, [this, asset_id, asset_type, asset_name, asset_collection, refresh_repository](wxCommandEvent&) {
        try {
            beta1b::ReportingAsset asset;
            asset.id = asset_id->GetValue().ToStdString();
            asset.asset_type = asset_type->GetStringSelection().ToStdString();
            asset.name = asset_name->GetValue().ToStdString();
            asset.collection_id = asset_collection->GetValue().ToStdString();
            asset.payload_json = reporting_status_detached_ != nullptr ? reporting_status_detached_->GetValue().ToStdString()
                                                                       : "{}";
            asset.created_by = "ui";
            asset.updated_by = "ui";
            reporting_service_.UpsertAsset(asset);
            refresh_repository(reporting_service_.LoadRepositoryAssets());
            AppendLogLine("Reporting asset upserted id=" + asset.id);
        } catch (const std::exception& ex) {
            AppendLogLine(ex.what());
        }
    });

    load_collection_btn->Bind(wxEVT_BUTTON, [this, asset_collection, refresh_repository](wxCommandEvent&) {
        try {
            refresh_repository(reporting_service_.ListAssetsByCollection(asset_collection->GetValue().ToStdString()));
        } catch (const std::exception& ex) {
            AppendLogLine(ex.what());
        }
    });

    refresh_repository(reporting_service_.LoadRepositoryAssets());

    reporting_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        reporting_frame_ = nullptr;
        reporting_sql_detached_ = nullptr;
        reporting_status_detached_ = nullptr;
        reporting_dashboard_output_detached_ = nullptr;
        reporting_repository_rows_detached_ = nullptr;
        event.Skip();
    });

    reporting_frame_->Show();
}

void MainFrame::OpenOrFocusDataMaskingFrame() {
    if (data_masking_frame_ != nullptr) {
        data_masking_frame_->Show();
        data_masking_frame_->Raise();
        return;
    }

    data_masking_frame_ = new wxFrame(this, wxID_ANY, "DataMaskingFrame", wxDefaultPosition, wxSize(980, 640));
    auto* panel = new wxPanel(data_masking_frame_, wxID_ANY);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* profile_row = new wxBoxSizer(wxHORIZONTAL);
    auto* profile_id = new wxTextCtrl(panel, wxID_ANY, "default");
    profile_row->Add(new wxStaticText(panel, wxID_ANY, "Profile"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    profile_row->Add(profile_id, 1, wxRIGHT, 6);
    root->Add(profile_row, 0, wxEXPAND | wxALL, 8);

    auto* rules = new wxTextCtrl(panel,
                                 wxID_ANY,
                                 "email=redact\nssn=hash\nname=prefix_mask",
                                 wxDefaultPosition,
                                 wxSize(-1, 120),
                                 wxTE_MULTILINE);
    auto* sample = new wxTextCtrl(panel,
                                  wxID_ANY,
                                  "name=alice,email=alice@example.com,ssn=111-22-3333",
                                  wxDefaultPosition,
                                  wxSize(-1, 80),
                                  wxTE_MULTILINE);
    auto* output = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    root->Add(new wxStaticText(panel, wxID_ANY, "Rules (field=method)"), 0, wxLEFT | wxRIGHT, 8);
    root->Add(rules, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(new wxStaticText(panel, wxID_ANY, "Sample Row (key=value comma-separated)"), 0, wxLEFT | wxRIGHT, 8);
    root->Add(sample, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    auto* preview_btn = new wxButton(panel, wxID_ANY, "Preview");
    auto* execute_btn = new wxButton(panel, wxID_ANY, "Execute");
    buttons->Add(preview_btn, 0, wxRIGHT, 6);
    buttons->Add(execute_btn, 0, wxRIGHT, 6);
    root->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(output, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    panel->SetSizer(root);

    const auto run_preview = [this, profile_id, rules, sample, output]() {
        std::map<std::string, std::string> parsed_rules;
        std::stringstream rule_lines(rules->GetValue().ToStdString());
        std::string line;
        while (std::getline(rule_lines, line)) {
            const auto split = line.find('=');
            if (split == std::string::npos) {
                continue;
            }
            const std::string key = line.substr(0, split);
            const std::string value = line.substr(split + 1);
            if (!key.empty() && !value.empty()) {
                parsed_rules[key] = value;
            }
        }
        std::map<std::string, std::string> row;
        std::stringstream fields(sample->GetValue().ToStdString());
        while (std::getline(fields, line, ',')) {
            const auto split = line.find('=');
            if (split == std::string::npos) {
                continue;
            }
            row[line.substr(0, split)] = line.substr(split + 1);
        }
        advanced_service_.UpsertMaskingProfile(profile_id->GetValue().ToStdString(), parsed_rules);
        const auto masked = advanced_service_.PreviewMaskWithProfile(profile_id->GetValue().ToStdString(), {row});
        std::ostringstream out;
        for (const auto& [k, v] : masked.front()) {
            out << k << "=" << v << "\n";
        }
        output->SetValue(ToWx(out.str()));
    };

    preview_btn->Bind(wxEVT_BUTTON, [run_preview](wxCommandEvent&) {
        try {
            run_preview();
        } catch (const std::exception& ex) {
            wxMessageBox(ToWx(ex.what()), "Masking preview failed", wxOK | wxICON_ERROR);
        }
    });
    execute_btn->Bind(wxEVT_BUTTON, [this, run_preview](wxCommandEvent&) {
        try {
            run_preview();
            AppendLogLine("Data masking execution completed");
        } catch (const std::exception& ex) {
            AppendLogLine(ex.what());
        }
    });

    data_masking_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        data_masking_frame_ = nullptr;
        event.Skip();
    });
    data_masking_frame_->Show();
}

void MainFrame::OpenOrFocusCdcConfigFrame() {
    if (cdc_config_frame_ != nullptr) {
        cdc_config_frame_->Show();
        cdc_config_frame_->Raise();
        return;
    }

    cdc_config_frame_ = new wxFrame(this, wxID_ANY, "CdcConfigFrame", wxDefaultPosition, wxSize(980, 640));
    auto* panel = new wxPanel(cdc_config_frame_, wxID_ANY);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* max_attempts = new wxTextCtrl(panel, wxID_ANY, "3");
    auto* backoff_ms = new wxTextCtrl(panel, wxID_ANY, "10");
    auto* event_payload = new wxTextCtrl(panel, wxID_ANY, "event:customer_updated", wxDefaultPosition, wxSize(-1, 150), wxTE_MULTILINE);
    auto* output = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(panel, wxID_ANY, "Max Attempts"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row->Add(max_attempts, 0, wxRIGHT, 8);
    row->Add(new wxStaticText(panel, wxID_ANY, "Backoff(ms)"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row->Add(backoff_ms, 0, wxRIGHT, 8);
    root->Add(row, 0, wxEXPAND | wxALL, 8);

    root->Add(event_payload, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    auto* publish_btn = new wxButton(panel, wxID_ANY, "Publish Test Event");
    auto* batch_btn = new wxButton(panel, wxID_ANY, "Run Batch");
    buttons->Add(publish_btn, 0, wxRIGHT, 6);
    buttons->Add(batch_btn, 0, wxRIGHT, 6);
    root->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(output, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(root);

    publish_btn->Bind(wxEVT_BUTTON, [this, event_payload, max_attempts, backoff_ms, output](wxCommandEvent&) {
        try {
            const std::string payload = advanced_service_.RunCdcEvent(
                event_payload->GetValue().ToStdString(),
                std::max(1, std::stoi(max_attempts->GetValue().ToStdString())),
                std::max(0, std::stoi(backoff_ms->GetValue().ToStdString())),
                [](const std::string&) { return true; },
                [](const std::string&) {});
            output->SetValue(ToWx(payload));
            AppendLogLine("CDC test event published");
        } catch (const std::exception& ex) {
            output->SetValue(ToWx(ex.what()));
            AppendLogLine(ex.what());
        }
    });

    batch_btn->Bind(wxEVT_BUTTON, [this, event_payload, max_attempts, backoff_ms, output](wxCommandEvent&) {
        try {
            std::vector<std::string> events;
            std::stringstream ss(event_payload->GetValue().ToStdString());
            std::string line;
            while (std::getline(ss, line)) {
                if (!line.empty()) {
                    events.push_back(line);
                }
            }
            const auto result = advanced_service_.RunCdcBatch(events,
                                                              std::max(1, std::stoi(max_attempts->GetValue().ToStdString())),
                                                              std::max(0, std::stoi(backoff_ms->GetValue().ToStdString())),
                                                              [](const std::string&) { return true; });
            std::ostringstream out;
            out << "published=" << result.published << "\n"
                << "dead_lettered=" << result.dead_lettered;
            output->SetValue(ToWx(out.str()));
            AppendLogLine("CDC batch executed count=" + std::to_string(events.size()));
        } catch (const std::exception& ex) {
            output->SetValue(ToWx(ex.what()));
            AppendLogLine(ex.what());
        }
    });

    cdc_config_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        cdc_config_frame_ = nullptr;
        event.Skip();
    });
    cdc_config_frame_->Show();
}

void MainFrame::OpenOrFocusGitIntegrationFrame() {
    if (git_integration_frame_ != nullptr) {
        git_integration_frame_->Show();
        git_integration_frame_->Raise();
        return;
    }

    git_integration_frame_ = new wxFrame(this, wxID_ANY, "GitIntegrationFrame", wxDefaultPosition, wxSize(840, 480));
    auto* panel = new wxPanel(git_integration_frame_, wxID_ANY);
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* branch_selected = new wxCheckBox(panel, wxID_ANY, "Branch Selected");
    auto* remote_reachable = new wxCheckBox(panel, wxID_ANY, "Remote Reachable");
    auto* conflicts_resolved = new wxCheckBox(panel, wxID_ANY, "Conflicts Resolved");
    branch_selected->SetValue(true);
    remote_reachable->SetValue(true);
    conflicts_resolved->SetValue(true);
    auto* validate_btn = new wxButton(panel, wxID_ANY, "Validate Git Sync");
    auto* output = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    root->Add(branch_selected, 0, wxALL, 8);
    root->Add(remote_reachable, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(conflicts_resolved, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(validate_btn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(output, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    panel->SetSizer(root);

    validate_btn->Bind(wxEVT_BUTTON, [this, branch_selected, remote_reachable, conflicts_resolved, output](wxCommandEvent&) {
        try {
            advanced_service_.ValidateGitSyncState(branch_selected->GetValue(),
                                                   remote_reachable->GetValue(),
                                                   conflicts_resolved->GetValue());
            output->SetValue("Git integration status: OK");
            AppendLogLine("Git integration validated");
        } catch (const std::exception& ex) {
            output->SetValue(ToWx(ex.what()));
            AppendLogLine(ex.what());
        }
    });

    git_integration_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        git_integration_frame_ = nullptr;
        event.Skip();
    });
    git_integration_frame_->Show();
}

void MainFrame::OpenOrFocusSpecWorkspaceFrame() {
    EnsureDetachedSurfaceNotEmbedded(kWorkspacePageSpec);
    if (spec_workspace_frame_ != nullptr) {
        spec_workspace_frame_->Show();
        spec_workspace_frame_->Raise();
        return;
    }

    spec_workspace_frame_ = new wxFrame(this, wxID_ANY, "SpecWorkspaceFrame", wxDefaultPosition, wxSize(1200, 760));
    auto* panel = new wxPanel(spec_workspace_frame_, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(panel, wxID_ANY, "Spec set"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    specset_choice_detached_ = new wxChoice(panel, wxID_ANY);
    specset_choice_detached_->Append("sb_v3");
    specset_choice_detached_->Append("sb_vnext");
    specset_choice_detached_->Append("sb_beta1");
    specset_choice_detached_->SetSelection(specset_choice_ != nullptr ? specset_choice_->GetSelection() : 1);
    row->Add(specset_choice_detached_, 0, wxRIGHT, 8);

    auto* refresh_btn = new wxButton(panel, wxID_ANY, "Refresh Workspace");
    row->Add(refresh_btn, 0, wxRIGHT, 6);
    auto* dock_btn = new wxButton(panel, wxID_ANY, "Dock In Main");
    row->Add(dock_btn, 0, wxRIGHT, 6);
    sizer->Add(row, 0, wxEXPAND | wxALL, 8);

    spec_summary_detached_ =
        new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE | wxTE_READONLY);
    spec_dashboard_detached_ =
        new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 120), wxTE_MULTILINE | wxTE_READONLY);
    spec_work_package_detached_ =
        new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    sizer->Add(spec_summary_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    sizer->Add(spec_dashboard_detached_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    sizer->Add(spec_work_package_detached_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(sizer);

    refresh_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        RefreshSpecWorkspaceControls(specset_choice_detached_,
                                     spec_summary_detached_,
                                     spec_dashboard_detached_,
                                     spec_work_package_detached_);
    });
    dock_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SelectWorkspacePage(kWorkspacePageSpec); });

    RefreshSpecWorkspaceControls(specset_choice_detached_,
                                 spec_summary_detached_,
                                 spec_dashboard_detached_,
                                 spec_work_package_detached_);

    spec_workspace_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (specset_choice_ != nullptr && specset_choice_detached_ != nullptr) {
            specset_choice_->SetSelection(specset_choice_detached_->GetSelection());
            RefreshSpecWorkspace();
        }
        spec_workspace_frame_ = nullptr;
        specset_choice_detached_ = nullptr;
        spec_summary_detached_ = nullptr;
        spec_dashboard_detached_ = nullptr;
        spec_work_package_detached_ = nullptr;
        event.Skip();
    });

    BindDetachedFrameDropDock(spec_workspace_frame_, kWorkspacePageSpec);
    spec_workspace_frame_->Show();
}

std::string MainFrame::AdminManagerKeyForCommand(int command_id) const {
    switch (command_id) {
        case kCmdOpenSchemaManagerFrame: return "schema";
        case kCmdOpenTableManagerFrame: return "table";
        case kCmdOpenIndexManagerFrame: return "index";
        case kCmdOpenDomainManagerFrame: return "domain";
        case kCmdOpenSequenceManagerFrame: return "sequence";
        case kCmdOpenViewManagerFrame: return "view";
        case kCmdOpenTriggerManagerFrame: return "trigger";
        case kCmdOpenProcedureManagerFrame: return "procedure";
        case kCmdOpenPackageManagerFrame: return "package";
        case kCmdOpenUsersManagerFrame: return "users";
        case kCmdOpenJobsManagerFrame: return "jobs";
        case kCmdOpenStorageManagerFrame: return "storage";
        case kCmdOpenBackupManagerFrame: return "backup";
        default: return "";
    }
}

std::string MainFrame::AdminManagerTitle(const std::string& manager_key) const {
    if (manager_key == "schema") {
        return "SchemaManagerFrame";
    }
    if (manager_key == "table") {
        return "TableDesignerFrame";
    }
    if (manager_key == "index") {
        return "IndexDesignerFrame";
    }
    if (manager_key == "domain") {
        return "DomainManagerFrame";
    }
    if (manager_key == "sequence") {
        return "SequenceManagerFrame";
    }
    if (manager_key == "view") {
        return "ViewManagerFrame";
    }
    if (manager_key == "trigger") {
        return "TriggerManagerFrame";
    }
    if (manager_key == "procedure") {
        return "ProcedureManagerFrame";
    }
    if (manager_key == "package") {
        return "PackageManagerFrame";
    }
    if (manager_key == "users") {
        return "UsersRolesFrame";
    }
    if (manager_key == "jobs") {
        return "JobSchedulerFrame";
    }
    if (manager_key == "storage") {
        return "StorageManagerFrame";
    }
    if (manager_key == "backup") {
        return "BackupManagerFrame";
    }
    return "AdminManagerFrame";
}

std::string MainFrame::AdminManagerDescription(const std::string& manager_key) const {
    if (manager_key == "schema") {
        return "Create/alter/drop schemas and schema-scoped defaults.";
    }
    if (manager_key == "table") {
        return "Define tables, columns, keys, and physical options.";
    }
    if (manager_key == "index") {
        return "Define and rebuild indexes with deterministic ordering.";
    }
    if (manager_key == "domain") {
        return "Manage reusable domain datatypes and constraints.";
    }
    if (manager_key == "sequence") {
        return "Manage sequence generators and allocation policies.";
    }
    if (manager_key == "view") {
        return "Manage logical views and dependency-safe updates.";
    }
    if (manager_key == "trigger") {
        return "Manage trigger bodies, timing, and conditions.";
    }
    if (manager_key == "procedure") {
        return "Manage executable routines and signatures.";
    }
    if (manager_key == "package") {
        return "Manage packaged routines and shared state contracts.";
    }
    if (manager_key == "users") {
        return "Manage users, roles, memberships, and grants.";
    }
    if (manager_key == "jobs") {
        return "Manage scheduled jobs, status, and retry policies.";
    }
    if (manager_key == "storage") {
        return "Manage storage profiles, pages, and placement policies.";
    }
    if (manager_key == "backup") {
        return "Manage backup policies, execution, and restore points.";
    }
    return "Generic administrative manager.";
}

std::string MainFrame::AdminManagerDefaultPath(const std::string& manager_key) const {
    if (manager_key == "schema") {
        return "public";
    }
    if (manager_key == "table") {
        return "public.customer";
    }
    if (manager_key == "index") {
        return "public.idx_customer_name";
    }
    if (manager_key == "domain") {
        return "public.domain_customer_code";
    }
    if (manager_key == "sequence") {
        return "public.seq_customer_id";
    }
    if (manager_key == "view") {
        return "public.customer_summary";
    }
    if (manager_key == "trigger") {
        return "public.trg_customer_audit";
    }
    if (manager_key == "procedure") {
        return "public.proc_rebuild_customer_cache";
    }
    if (manager_key == "package") {
        return "public.pkg_customer_admin";
    }
    if (manager_key == "users") {
        return "security.roles";
    }
    if (manager_key == "jobs") {
        return "scheduler.nightly_refresh";
    }
    if (manager_key == "storage") {
        return "db.primary";
    }
    if (manager_key == "backup") {
        return "backup.policy.default";
    }
    return "admin.object";
}

std::string MainFrame::AdminManagerDefaultTemplate(const std::string& manager_key) const {
    if (manager_key == "schema") {
        return "CREATE SCHEMA public;";
    }
    if (manager_key == "table") {
        return "CREATE TABLE public.customer (id BIGINT PRIMARY KEY, name VARCHAR(120));";
    }
    if (manager_key == "index") {
        return "CREATE INDEX idx_customer_name ON public.customer(name);";
    }
    if (manager_key == "domain") {
        return "CREATE DOMAIN public.domain_customer_code AS VARCHAR(32);";
    }
    if (manager_key == "sequence") {
        return "CREATE SEQUENCE public.seq_customer_id START WITH 1;";
    }
    if (manager_key == "view") {
        return "CREATE VIEW public.customer_summary AS SELECT id, name FROM public.customer;";
    }
    if (manager_key == "trigger") {
        return "CREATE TRIGGER trg_customer_audit BEFORE UPDATE ON public.customer AS BEGIN END;";
    }
    if (manager_key == "procedure") {
        return "CREATE PROCEDURE public.proc_rebuild_customer_cache AS BEGIN END;";
    }
    if (manager_key == "package") {
        return "CREATE PACKAGE public.pkg_customer_admin AS BEGIN END;";
    }
    if (manager_key == "users") {
        return "CREATE ROLE analyst;";
    }
    if (manager_key == "jobs") {
        return "CREATE JOB nightly_refresh AS EXECUTE PROCEDURE public.proc_rebuild_customer_cache;";
    }
    if (manager_key == "storage") {
        return "ALTER DATABASE SET STORAGE_PROFILE='balanced';";
    }
    if (manager_key == "backup") {
        return "BACKUP DATABASE TO '/var/backups/scratchbird/customer.bkp';";
    }
    return "SELECT 1;";
}

std::string MainFrame::AdminManagerNodeLabelForKey(const std::string& manager_key) const {
    return AdminManagerTitle(manager_key);
}

std::string MainFrame::AdminManagerKeyForNodeLabel(const std::string& node_label) const {
    if (node_label == "SchemaManagerFrame") {
        return "schema";
    }
    if (node_label == "TableDesignerFrame") {
        return "table";
    }
    if (node_label == "IndexDesignerFrame") {
        return "index";
    }
    if (node_label == "DomainManagerFrame") {
        return "domain";
    }
    if (node_label == "SequenceManagerFrame") {
        return "sequence";
    }
    if (node_label == "ViewManagerFrame") {
        return "view";
    }
    if (node_label == "TriggerManagerFrame") {
        return "trigger";
    }
    if (node_label == "ProcedureManagerFrame") {
        return "procedure";
    }
    if (node_label == "PackageManagerFrame") {
        return "package";
    }
    if (node_label == "UsersRolesFrame") {
        return "users";
    }
    if (node_label == "JobSchedulerFrame") {
        return "jobs";
    }
    if (node_label == "StorageManagerFrame") {
        return "storage";
    }
    if (node_label == "BackupManagerFrame") {
        return "backup";
    }
    return "";
}

void MainFrame::OpenAdminManagerByCommand(int command_id) {
    const std::string manager_key = AdminManagerKeyForCommand(command_id);
    if (!manager_key.empty()) {
        OpenOrFocusAdminManager(manager_key);
    }
}

void MainFrame::OpenOrFocusAdminManager(const std::string& manager_key) {
    if (manager_key.empty()) {
        return;
    }

    auto found = admin_manager_frames_.find(manager_key);
    if (found != admin_manager_frames_.end() && found->second != nullptr) {
        found->second->Show();
        found->second->Raise();
        return;
    }

    auto* frame = new wxFrame(this, wxID_ANY, ToWx(AdminManagerTitle(manager_key)), wxDefaultPosition, wxSize(1100, 760));
    admin_manager_frames_[manager_key] = frame;

    auto* panel = new wxPanel(frame, wxID_ANY);
    auto* root = new wxBoxSizer(wxVERTICAL);

    root->Add(new wxStaticText(panel, wxID_ANY, ToWx(AdminManagerDescription(manager_key))), 0, wxALL, 8);

    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(new wxStaticText(panel, wxID_ANY, "Target"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto* target_path = new wxTextCtrl(panel, wxID_ANY, ToWx(AdminManagerDefaultPath(manager_key)));
    row->Add(target_path, 1, wxRIGHT, 8);
    root->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* command_sql = new wxTextCtrl(panel,
                                       wxID_ANY,
                                       ToWx(AdminManagerDefaultTemplate(manager_key)),
                                       wxDefaultPosition,
                                       wxSize(-1, 170),
                                       wxTE_MULTILINE);
    root->Add(command_sql, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    auto* preview_btn = new wxButton(panel, wxID_ANY, "Preview SQL");
    auto* apply_btn = new wxButton(panel, wxID_ANY, "Apply Change");
    auto* refresh_btn = new wxButton(panel, wxID_ANY, "Refresh Metadata");
    auto* open_sql_btn = new wxButton(panel, wxID_ANY, "Open SQL Editor");
    buttons->Add(preview_btn, 0, wxRIGHT, 6);
    buttons->Add(apply_btn, 0, wxRIGHT, 6);
    buttons->Add(refresh_btn, 0, wxRIGHT, 6);
    buttons->Add(open_sql_btn, 0, wxRIGHT, 6);
    root->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* output = new wxTextCtrl(panel,
                                  wxID_ANY,
                                  "",
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY);
    root->Add(output, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(root);

    preview_btn->Bind(wxEVT_BUTTON, [this, manager_key, target_path, command_sql, output](wxCommandEvent&) {
        std::ostringstream out;
        out << "manager=" << manager_key << "\n";
        out << "target=" << target_path->GetValue().ToStdString() << "\n";
        out << "command=" << command_sql->GetValue().ToStdString() << "\n";
        out << "mode=preview\n";
        output->SetValue(ToWx(out.str()));
        AppendLogLine("Previewed admin command for " + manager_key);
    });

    apply_btn->Bind(wxEVT_BUTTON, [this, manager_key, command_sql, output](wxCommandEvent&) {
        try {
            const std::string sql = command_sql->GetValue().ToStdString();
            if (sql.empty()) {
                throw MakeReject("SRB1-R-5101",
                                 "admin manager command cannot be empty",
                                 "ui",
                                 "apply_admin_manager",
                                 false,
                                 manager_key);
            }
            EnsureConnected();
            ui_service_.ValidateSurfaceOpen("admin_" + manager_key, adapter_.IsConnected(), true);
            const auto result = adapter_.ExecuteQuery(sql);
            std::ostringstream out;
            out << "command_tag=" << result.command_tag << "\n";
            out << "rows_affected=" << result.rows_affected << "\n";
            for (const auto& msg : result.messages) {
                out << "message=" << msg << "\n";
            }
            output->SetValue(ToWx(out.str()));
            AppendLogLine("Applied admin command for " + manager_key + " command_tag=" + result.command_tag);
            try {
                reporting_service_.AppendActivity({NowUtc(), "admin_" + manager_key + "_apply", 1.0});
                RefreshMonitoring();
            } catch (const RejectError& ex) {
                AppendLogLine(ex.what());
            }
        } catch (const RejectError& ex) {
            output->SetValue(ToWx(ex.what()));
            AppendLogLine(ex.what());
        }
    });

    refresh_btn->Bind(wxEVT_BUTTON, [this, manager_key, output](wxCommandEvent&) {
        RefreshCatalog();
        output->SetValue("metadata refreshed");
        AppendLogLine("Refreshed metadata from " + manager_key + " manager");
    });

    open_sql_btn->Bind(wxEVT_BUTTON, [this, command_sql](wxCommandEvent&) {
        OpenOrFocusSqlEditorFrame();
        if (sql_editor_ != nullptr) {
            sql_editor_->SetValue(command_sql->GetValue());
        }
        if (sql_editor_detached_ != nullptr) {
            sql_editor_detached_->SetValue(command_sql->GetValue());
        }
    });

    frame->Bind(wxEVT_CLOSE_WINDOW, [this, manager_key](wxCloseEvent& event) {
        auto it = admin_manager_frames_.find(manager_key);
        if (it != admin_manager_frames_.end()) {
            it->second = nullptr;
        }
        event.Skip();
    });

    frame->Show();
}

std::string MainFrame::ActiveProfileName() const {
    if (profile_choice_ == nullptr || profile_choice_->GetSelection() == wxNOT_FOUND) {
        return profiles_.empty() ? std::string("offline_mock") : profiles_.front().name;
    }
    return profile_choice_->GetStringSelection().ToStdString();
}

runtime::ConnectionProfile MainFrame::ActiveProfile() const {
    const std::string selected = ActiveProfileName();
    for (const auto& profile : profiles_) {
        if (profile.name == selected) {
            return profile;
        }
    }
    return profiles_.front();
}

std::filesystem::path MainFrame::SpecRootPath() const {
    return repo_root_.parent_path() / "local_work/docs/specifications_beta1b";
}

std::filesystem::path MainFrame::ManifestPathForSet(const std::string& set_id) const {
    return SpecRootPath() / "resources/specset_packages" / (set_id + "_specset_manifest.example.json");
}

wxMenu* MainFrame::BuildTreeContextMenu() const {
    auto* menu = new wxMenu();
    menu->Append(kCmdTreeCopyObjectName, "Copy object name");
    menu->Append(kCmdTreeCopyDdl, "Copy DDL");
    menu->Append(kCmdTreeShowDependencies, "Show dependencies");
    menu->AppendSeparator();
    menu->Append(kCmdTreeRefreshNode, "Refresh node metadata");
    return menu;
}

void MainFrame::OnAboutMenu(wxCommandEvent& event) {
    (void)event;
    wxMessageBox("ScratchRobin Beta1b Workbench\n"
                 "Includes main workbench and independent windows for SQL editor, object editor, diagrams,\n"
                 "monitoring, and spec workspace.",
                 "About ScratchRobin",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MainFrame::OnConnect(wxCommandEvent& event) {
    (void)event;
    try {
        const auto profile = ActiveProfile();
        const auto session = adapter_.Connect(profile);
        SetStatusText(ToWx("Connected " + session.backend_name + " " + session.server_version), 1);
        AppendLogLine("Connected to " + profile.name + " on port " + std::to_string(session.port));
        RefreshCatalog();
    } catch (const RejectError& ex) {
        AppendLogLine(ex.what());
        wxMessageBox(ToWx(ex.what()), "Connection failed", wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnDisconnect(wxCommandEvent& event) {
    (void)event;
    adapter_.Disconnect();
    SetStatusText("Disconnected", 1);
    AppendLogLine("Disconnected active profile");
}

void MainFrame::OnRunSql(wxCommandEvent& event) {
    (void)event;
    RunSqlIntoSurface(sql_editor_->GetValue().ToStdString(), sql_results_, sql_status_, sql_history_, true);
}

void MainFrame::OnCancelSql(wxCommandEvent& event) {
    (void)event;
    CancelSqlIntoStatus(sql_status_);
}

void MainFrame::OnExportHistoryCsv(wxCommandEvent& event) {
    (void)event;
    ExportHistoryIntoStatus(sql_status_, sql_history_);
}

void MainFrame::OnSaveObject(wxCommandEvent& event) {
    (void)event;
    try {
        SaveObjectFromControls(object_class_, object_path_, object_ddl_);
    } catch (const RejectError& ex) {
        AppendLogLine(ex.what());
        wxMessageBox(ToWx(ex.what()), "Object save failed", wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnGenerateMigration(wxCommandEvent& event) {
    (void)event;
    try {
        GenerateMigrationIntoControls(object_class_, object_path_, object_ddl_);
    } catch (const RejectError& ex) {
        AppendLogLine(ex.what());
        wxMessageBox(ToWx(ex.what()), "Migration generation failed", wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnOpenDiagramLink(wxCommandEvent& event) {
    (void)event;
    try {
        OpenDiagramFromControls(diagram_links_, diagram_output_);
    } catch (const RejectError& ex) {
        diagram_output_->SetValue(ToWx(ex.what()));
        AppendLogLine(ex.what());
    }
}

void MainFrame::OnExportDiagramSvg(wxCommandEvent& event) {
    (void)event;
    try {
        ExportDiagramToOutput("svg", diagram_output_);
    } catch (const RejectError& ex) {
        diagram_output_->SetValue(ToWx(ex.what()));
        AppendLogLine(ex.what());
    }
}

void MainFrame::OnExportDiagramPng(wxCommandEvent& event) {
    (void)event;
    try {
        ExportDiagramToOutput("png", diagram_output_);
    } catch (const RejectError& ex) {
        diagram_output_->SetValue(ToWx(ex.what()));
        AppendLogLine(ex.what());
    }
}

void MainFrame::OnRefreshSpecWorkspace(wxCommandEvent& event) {
    (void)event;
    RefreshSpecWorkspace();
}

void MainFrame::OnRefreshMonitoring(wxCommandEvent& event) {
    (void)event;
    RefreshMonitoring();
}

void MainFrame::OnOpenSqlEditorFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusSqlEditorFrame();
}

void MainFrame::OnOpenObjectEditorFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusObjectEditorFrame();
}

void MainFrame::OnOpenDiagramFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusDiagramFrame();
}

void MainFrame::OnOpenMonitoringFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusMonitoringFrame();
}

void MainFrame::OnOpenReportingFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusReportingFrame();
}

void MainFrame::OnOpenDataMaskingFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusDataMaskingFrame();
}

void MainFrame::OnOpenCdcConfigFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusCdcConfigFrame();
}

void MainFrame::OnOpenGitIntegrationFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusGitIntegrationFrame();
}

void MainFrame::OnOpenSpecWorkspaceFrame(wxCommandEvent& event) {
    (void)event;
    OpenOrFocusSpecWorkspaceFrame();
}

void MainFrame::OnOpenAdminManager(wxCommandEvent& event) {
    OpenAdminManagerByCommand(event.GetId());
}

void MainFrame::OnTreeContext(wxTreeEvent& event) {
    (void)event;
    std::unique_ptr<wxMenu> menu(BuildTreeContextMenu());
    PopupMenu(menu.get());
}

void MainFrame::OnTreeActivate(wxTreeEvent& event) {
    const auto item = event.GetItem();
    if (!item.IsOk()) {
        return;
    }

    const std::string label = tree_->GetItemText(item).ToStdString();
    if (label.find("SQL") != std::string::npos) {
        OpenOrFocusSqlEditorFrame();
    } else if (!AdminManagerKeyForNodeLabel(label).empty()) {
        OpenOrFocusAdminManager(AdminManagerKeyForNodeLabel(label));
    } else if (label.find("Object") != std::string::npos || label.find("table:") != std::string::npos ||
               label.find("view:") != std::string::npos || label.find("index:") != std::string::npos) {
        OpenOrFocusObjectEditorFrame();
        if (object_path_ != nullptr && (label.find("table:") != std::string::npos || label.find("view:") != std::string::npos ||
                                        label.find("index:") != std::string::npos)) {
            object_path_->SetValue(ToWx(label));
        }
        if (object_path_detached_ != nullptr &&
            (label.find("table:") != std::string::npos || label.find("view:") != std::string::npos ||
             label.find("index:") != std::string::npos)) {
            object_path_detached_->SetValue(ToWx(label));
        }
    } else if (label.find("Diagram") != std::string::npos || label.find("Erd") != std::string::npos ||
               label.find("Silverston") != std::string::npos || label.find("Whiteboard") != std::string::npos ||
               label.find("MindMap") != std::string::npos) {
        auto open_typed_diagram = [this](const std::string& type_name, const std::string& diagram_name) {
            OpenOrFocusDiagramFrame();
            if (diagram_output_detached_ != nullptr) {
                OpenDiagramByTypeAndName(type_name, diagram_name, diagram_output_detached_);
            } else if (diagram_output_ != nullptr) {
                OpenDiagramByTypeAndName(type_name, diagram_name, diagram_output_);
            }
        };
        if (label.find("MindMap") != std::string::npos) {
            open_typed_diagram("MindMap", "Implementation Map");
        } else if (label.find("Whiteboard") != std::string::npos) {
            open_typed_diagram("Whiteboard", "Migration Plan");
        } else if (label.find("Silverston") != std::string::npos) {
            open_typed_diagram("Silverston", "Subject Areas");
        } else {
            open_typed_diagram("Erd", "Core Domain");
        }
    } else if (label.find("Reporting") != std::string::npos) {
        OpenOrFocusReportingFrame();
    } else if (label.find("DataMasking") != std::string::npos) {
        OpenOrFocusDataMaskingFrame();
    } else if (label.find("CdcConfig") != std::string::npos) {
        OpenOrFocusCdcConfigFrame();
    } else if (label.find("GitIntegration") != std::string::npos) {
        OpenOrFocusGitIntegrationFrame();
    } else if (label.find("Spec Workspace") != std::string::npos) {
        OpenOrFocusSpecWorkspaceFrame();
    } else if (label.find("Monitoring") != std::string::npos) {
        OpenOrFocusMonitoringFrame();
    }
}

void MainFrame::OnTreeCopyObjectName(wxCommandEvent& event) {
    (void)event;
    const auto item = tree_->GetSelection();
    if (!item.IsOk()) {
        return;
    }

    const wxString value = tree_->GetItemText(item);
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(value));
        wxTheClipboard->Close();
    }
    AppendLogLine("Copied object name from catalog selection");
}

void MainFrame::OnTreeCopyDdl(wxCommandEvent& event) {
    (void)event;
    const wxString value = object_ddl_ != nullptr ? object_ddl_->GetValue() : "";
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(value));
        wxTheClipboard->Close();
    }
    AppendLogLine("Copied object DDL from object editor");
}

void MainFrame::OnTreeShowDependencies(wxCommandEvent& event) {
    (void)event;
    wxMessageBox("Dependencies\n- customer -> orders (FK)\n- customer_summary -> customer",
                 "Dependencies",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MainFrame::OnTreeRefreshNode(wxCommandEvent& event) {
    (void)event;
    RefreshCatalog();
    AppendLogLine("Catalog node metadata refreshed");
}

void MainFrame::OnExitMenu(wxCommandEvent& event) {
    (void)event;
    reporting_service_.FlushPersistentState();
    Close(true);
}

}  // namespace scratchrobin::gui
