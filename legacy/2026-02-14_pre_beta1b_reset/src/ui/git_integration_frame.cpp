/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */

#include "ui/git_integration_frame.h"
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/statbox.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/filename.h>
#include <wx/artprov.h>

#include "ui/window_manager.h"
#include "core/connection_manager.h"
#include "core/config.h"
#include "core/git_client.h"

namespace scratchrobin {

enum {
    ID_INIT_REPO = wxID_HIGHEST + 1,
    ID_CLONE_REPO,
    ID_OPEN_REPO,
    ID_COMMIT,
    ID_PUSH,
    ID_PULL,
    ID_FETCH,
    ID_REFRESH,
    ID_CREATE_BRANCH,
    ID_CHECKOUT_BRANCH,
    ID_MERGE_BRANCH,
    ID_DELETE_BRANCH,
    ID_ADD_REMOTE,
    ID_CHANGED_FILES,
    ID_COMMIT_LIST,
    ID_BRANCH_LIST,
    ID_TIMER_REFRESH,
    ID_SHOW_DOCUMENTATION
};

wxBEGIN_EVENT_TABLE(GitIntegrationFrame, wxFrame)
    EVT_CLOSE(GitIntegrationFrame::OnClose)
    EVT_BUTTON(ID_INIT_REPO, GitIntegrationFrame::OnInitRepository)
    EVT_BUTTON(ID_CLONE_REPO, GitIntegrationFrame::OnCloneRepository)
    EVT_BUTTON(ID_OPEN_REPO, GitIntegrationFrame::OnOpenRepository)
    EVT_BUTTON(ID_COMMIT, GitIntegrationFrame::OnCommit)
    EVT_BUTTON(ID_PUSH, GitIntegrationFrame::OnPush)
    EVT_BUTTON(ID_PULL, GitIntegrationFrame::OnPull)
    EVT_BUTTON(ID_FETCH, GitIntegrationFrame::OnFetch)
    EVT_BUTTON(ID_REFRESH, GitIntegrationFrame::OnRefresh)
    EVT_BUTTON(ID_CREATE_BRANCH, GitIntegrationFrame::OnCreateBranch)
    EVT_BUTTON(ID_CHECKOUT_BRANCH, GitIntegrationFrame::OnCheckoutBranch)
    EVT_BUTTON(ID_MERGE_BRANCH, GitIntegrationFrame::OnMergeBranch)
    EVT_BUTTON(ID_DELETE_BRANCH, GitIntegrationFrame::OnDeleteBranch)
    EVT_LIST_ITEM_SELECTED(ID_CHANGED_FILES, GitIntegrationFrame::OnFileSelected)
    EVT_LIST_ITEM_SELECTED(ID_COMMIT_LIST, GitIntegrationFrame::OnCommitSelected)
    EVT_LIST_ITEM_SELECTED(ID_BRANCH_LIST, GitIntegrationFrame::OnBranchSelected)
    EVT_TIMER(ID_TIMER_REFRESH, GitIntegrationFrame::OnTimer)
    EVT_MENU(ID_SHOW_DOCUMENTATION, GitIntegrationFrame::OnShowDocumentation)
wxEND_EVENT_TABLE()

GitIntegrationFrame::GitIntegrationFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, _("Git Integration"),
              wxDefaultPosition, wxSize(1200, 800),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , connection_manager_(connectionManager)
    , connections_(connections)
    , app_config_(appConfig)
    , git_(std::make_unique<GitClient>())
    , refresh_timer_(this, ID_TIMER_REFRESH) {
    
    BuildMenu();
    BuildToolbar();
    BuildLayout();
    CentreOnScreen();
    refresh_timer_.Start(5000);
}

GitIntegrationFrame::~GitIntegrationFrame() = default;

void GitIntegrationFrame::SetProjectPath(const std::string& path) {
    project_path_ = path;
    if (git_->IsRepository(path)) {
        has_repository_ = git_->OpenRepository(path);
    } else {
        has_repository_ = false;
    }
    RefreshStatus();
    RefreshHistory();
    RefreshBranches();
}

void GitIntegrationFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();
    
    auto* file_menu = new wxMenu();
    file_menu->Append(ID_INIT_REPO, _("&Initialize Repository..."));
    file_menu->Append(ID_CLONE_REPO, _("&Clone Repository..."));
    file_menu->Append(ID_OPEN_REPO, _("&Open Repository..."));
    file_menu->AppendSeparator();
    file_menu->Append(wxID_CLOSE, _("&Close\tCtrl+W"));
    menu_bar->Append(file_menu, _("&File"));
    
    auto* git_menu = new wxMenu();
    git_menu->Append(ID_COMMIT, _("&Commit...\tCtrl+Enter"));
    git_menu->AppendSeparator();
    git_menu->Append(ID_PULL, _("&Pull\tCtrl+Shift+Down"));
    git_menu->Append(ID_PUSH, _("&Push\tCtrl+Shift+Up"));
    git_menu->Append(ID_FETCH, _("&Fetch\tCtrl+Shift+F"));
    git_menu->AppendSeparator();
    git_menu->Append(ID_REFRESH, _("&Refresh\tF5"));
    menu_bar->Append(git_menu, _("&Git"));
    
    auto* branch_menu = new wxMenu();
    branch_menu->Append(ID_CREATE_BRANCH, _("&New Branch...\tCtrl+B"));
    branch_menu->Append(ID_CHECKOUT_BRANCH, _("&Checkout..."));
    branch_menu->Append(ID_MERGE_BRANCH, _("&Merge..."));
    branch_menu->Append(ID_DELETE_BRANCH, _("&Delete..."));
    menu_bar->Append(branch_menu, _("&Branch"));
    
    auto* help_menu = new wxMenu();
    help_menu->Append(ID_SHOW_DOCUMENTATION, _("&Documentation..."));
    menu_bar->Append(help_menu, _("&Help"));
    
    SetMenuBar(menu_bar);
}

void GitIntegrationFrame::BuildToolbar() {
    auto* toolbar = CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);
    toolbar->AddTool(ID_INIT_REPO, _("Init"), wxArtProvider::GetBitmap(wxART_NEW_DIR));
    toolbar->AddTool(ID_CLONE_REPO, _("Clone"), wxArtProvider::GetBitmap(wxART_COPY));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_COMMIT, _("Commit"), wxArtProvider::GetBitmap(wxART_FILE_SAVE));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_PULL, _("Pull"), wxArtProvider::GetBitmap(wxART_GO_DOWN));
    toolbar->AddTool(ID_PUSH, _("Push"), wxArtProvider::GetBitmap(wxART_GO_UP));
    toolbar->AddTool(ID_FETCH, _("Fetch"), wxArtProvider::GetBitmap(wxART_REDO));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_REFRESH, _("Refresh"), wxArtProvider::GetBitmap(wxART_REFRESH));
    toolbar->Realize();
}

void GitIntegrationFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Info bar
    auto* info_panel = new wxPanel(this, wxID_ANY);
    info_panel->SetBackgroundColour(wxColour(60, 60, 60));
    auto* info_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    lbl_repo_name_ = new wxStaticText(info_panel, wxID_ANY, _("No repository"));
    lbl_repo_name_->SetForegroundColour(*wxWHITE);
    lbl_repo_name_->SetFont(wxFont(wxFontInfo(11).Bold()));
    info_sizer->Add(lbl_repo_name_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    info_sizer->AddSpacer(20);
    
    lbl_branch_ = new wxStaticText(info_panel, wxID_ANY, wxEmptyString);
    lbl_branch_->SetForegroundColour(wxColour(200, 200, 200));
    info_sizer->Add(lbl_branch_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    info_sizer->AddStretchSpacer(1);
    
    lbl_ahead_behind_ = new wxStaticText(info_panel, wxID_ANY, wxEmptyString);
    lbl_ahead_behind_->SetForegroundColour(wxColour(200, 200, 200));
    info_sizer->Add(lbl_ahead_behind_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    
    info_panel->SetSizer(info_sizer);
    main_sizer->Add(info_panel, 0, wxEXPAND);
    
    // Notebook
    auto* notebook = new wxNotebook(this, wxID_ANY);
    
    // Status Tab
    auto* status_panel = new wxPanel(notebook, wxID_ANY);
    auto* status_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* files_label = new wxStaticText(status_panel, wxID_ANY, _("Changed Files:"));
    files_label->SetFont(wxFont(wxFontInfo(10).Bold()));
    status_sizer->Add(files_label, 0, wxALL, 5);
    
    list_changed_files_ = new wxListCtrl(status_panel, ID_CHANGED_FILES,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxLC_REPORT | wxLC_SINGLE_SEL);
    list_changed_files_->AppendColumn(_("Status"), wxLIST_FORMAT_LEFT, 80);
    list_changed_files_->AppendColumn(_("File"), wxLIST_FORMAT_LEFT, 500);
    status_sizer->Add(list_changed_files_, 1, wxEXPAND | wxALL, 5);
    
    auto* msg_label = new wxStaticText(status_panel, wxID_ANY, _("Commit Message:"));
    status_sizer->Add(msg_label, 0, wxLEFT | wxRIGHT | wxTOP, 5);
    
    txt_commit_message_ = new wxTextCtrl(status_panel, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxSize(-1, 60),
                                          wxTE_MULTILINE);
    status_sizer->Add(txt_commit_message_, 0, wxEXPAND | wxALL, 5);
    
    btn_commit_ = new wxButton(status_panel, ID_COMMIT, _("Commit Changes"));
    status_sizer->Add(btn_commit_, 0, wxALIGN_RIGHT | wxALL, 5);
    
    status_panel->SetSizer(status_sizer);
    notebook->AddPage(status_panel, _("Status"));
    
    // History Tab
    auto* history_panel = new wxPanel(notebook, wxID_ANY);
    auto* history_sizer = new wxBoxSizer(wxVERTICAL);
    
    list_commits_ = new wxListCtrl(history_panel, ID_COMMIT_LIST,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT | wxLC_SINGLE_SEL);
    list_commits_->AppendColumn(_("Commit"), wxLIST_FORMAT_LEFT, 80);
    list_commits_->AppendColumn(_("Message"), wxLIST_FORMAT_LEFT, 350);
    list_commits_->AppendColumn(_("Author"), wxLIST_FORMAT_LEFT, 150);
    list_commits_->AppendColumn(_("Date"), wxLIST_FORMAT_LEFT, 150);
    history_sizer->Add(list_commits_, 1, wxEXPAND | wxALL, 5);
    
    history_panel->SetSizer(history_sizer);
    notebook->AddPage(history_panel, _("History"));
    
    // Diff Tab
    auto* diff_panel = new wxPanel(notebook, wxID_ANY);
    auto* diff_sizer = new wxBoxSizer(wxVERTICAL);
    
    list_diff_files_ = new wxListCtrl(diff_panel, wxID_ANY,
                                       wxDefaultPosition, wxSize(-1, 150),
                                       wxLC_REPORT | wxLC_SINGLE_SEL);
    list_diff_files_->AppendColumn(_("File"), wxLIST_FORMAT_LEFT, 400);
    list_diff_files_->AppendColumn(_("Changes"), wxLIST_FORMAT_LEFT, 100);
    diff_sizer->Add(list_diff_files_, 0, wxEXPAND | wxALL, 5);
    
    txt_diff_content_ = new wxTextCtrl(diff_panel, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY);
    txt_diff_content_->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    diff_sizer->Add(txt_diff_content_, 1, wxEXPAND | wxALL, 5);
    
    diff_panel->SetSizer(diff_sizer);
    notebook->AddPage(diff_panel, _("Diff"));
    
    // Branches Tab
    auto* branch_panel = new wxPanel(notebook, wxID_ANY);
    auto* branch_sizer = new wxBoxSizer(wxVERTICAL);
    
    list_branches_ = new wxListCtrl(branch_panel, ID_BRANCH_LIST,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxLC_SINGLE_SEL);
    list_branches_->AppendColumn(_("Name"), wxLIST_FORMAT_LEFT, 200);
    list_branches_->AppendColumn(_("Commit"), wxLIST_FORMAT_LEFT, 80);
    list_branches_->AppendColumn(_("Last Commit"), wxLIST_FORMAT_LEFT, 350);
    branch_sizer->Add(list_branches_, 1, wxEXPAND | wxALL, 5);
    
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_new_branch_ = new wxButton(branch_panel, ID_CREATE_BRANCH, _("New Branch"));
    btn_checkout_ = new wxButton(branch_panel, ID_CHECKOUT_BRANCH, _("Checkout"));
    btn_merge_ = new wxButton(branch_panel, ID_MERGE_BRANCH, _("Merge"));
    btn_delete_branch_ = new wxButton(branch_panel, ID_DELETE_BRANCH, _("Delete"));
    
    btn_sizer->Add(btn_new_branch_, 0, wxRIGHT, 5);
    btn_sizer->Add(btn_checkout_, 0, wxRIGHT, 5);
    btn_sizer->Add(btn_merge_, 0, wxRIGHT, 5);
    btn_sizer->Add(btn_delete_branch_, 0);
    branch_sizer->Add(btn_sizer, 0, wxALL, 5);
    
    branch_panel->SetSizer(branch_sizer);
    notebook->AddPage(branch_panel, _("Branches"));
    
    // Remotes Tab
    auto* remote_panel = new wxPanel(notebook, wxID_ANY);
    auto* remote_sizer = new wxBoxSizer(wxVERTICAL);
    
    list_remotes_ = new wxListCtrl(remote_panel, wxID_ANY,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT);
    list_remotes_->AppendColumn(_("Name"), wxLIST_FORMAT_LEFT, 100);
    list_remotes_->AppendColumn(_("URL"), wxLIST_FORMAT_LEFT, 500);
    remote_sizer->Add(list_remotes_, 1, wxEXPAND | wxALL, 5);
    
    btn_add_remote_ = new wxButton(remote_panel, ID_ADD_REMOTE, _("Add Remote"));
    remote_sizer->Add(btn_add_remote_, 0, wxALIGN_RIGHT | wxALL, 5);
    
    remote_panel->SetSizer(remote_sizer);
    notebook->AddPage(remote_panel, _("Remotes"));
    
    main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    SetSizer(main_sizer);
}

// Git Operations
void GitIntegrationFrame::OnInitRepository(wxCommandEvent& /*event*/) {
    wxDirDialog dialog(this, _("Select directory to initialize"),
                       project_path_.empty() ? wxGetHomeDir() : wxString(project_path_));
    if (dialog.ShowModal() == wxID_OK) {
        if (git_->InitRepository(dialog.GetPath().ToStdString())) {
            has_repository_ = git_->OpenRepository(dialog.GetPath().ToStdString());
            project_path_ = dialog.GetPath().ToStdString();
            wxMessageBox(_("Repository initialized!"), _("Success"));
            RefreshStatus();
            RefreshHistory();
            RefreshBranches();
        } else {
            wxMessageBox(_("Failed to initialize repository."), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}

void GitIntegrationFrame::OnCloneRepository(wxCommandEvent& /*event*/) {
    wxString url = wxGetTextFromUser(_("Repository URL:"), _("Clone Repository"));
    if (url.IsEmpty()) return;
    
    wxDirDialog dialog(this, _("Select destination"), wxGetHomeDir());
    if (dialog.ShowModal() != wxID_OK) return;
    
    wxString dest = dialog.GetPath() + wxFileName::GetPathSeparator() + wxFileName(url).GetName();
    if (git_->CloneRepository(url.ToStdString(), dest.ToStdString())) {
        has_repository_ = git_->OpenRepository(dest.ToStdString());
        project_path_ = dest.ToStdString();
        wxMessageBox(_("Repository cloned!"), _("Success"));
        RefreshStatus();
        RefreshHistory();
        RefreshBranches();
    } else {
        wxMessageBox(_("Clone failed."), _("Error"), wxOK | wxICON_ERROR);
    }
}

void GitIntegrationFrame::OnOpenRepository(wxCommandEvent& /*event*/) {
    wxDirDialog dialog(this, _("Select repository"),
                       project_path_.empty() ? wxGetHomeDir() : wxString(project_path_));
    if (dialog.ShowModal() == wxID_OK) {
        if (git_->IsRepository(dialog.GetPath().ToStdString())) {
            has_repository_ = git_->OpenRepository(dialog.GetPath().ToStdString());
            project_path_ = dialog.GetPath().ToStdString();
            RefreshStatus();
            RefreshHistory();
            RefreshBranches();
        } else {
            wxMessageBox(_("Not a valid Git repository."), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}

void GitIntegrationFrame::OnCommit(wxCommandEvent& /*event*/) {
    if (!has_repository_) {
        wxMessageBox(_("No repository open."), _("Error"), wxOK | wxICON_ERROR);
        return;
    }
    
    wxString msg = txt_commit_message_->GetValue();
    if (msg.IsEmpty()) {
        msg = wxGetTextFromUser(_("Commit message:"), _("Commit"));
    }
    if (msg.IsEmpty()) return;
    
    git_->AddAll();
    auto result = git_->Commit(msg.ToStdString());
    if (result.success) {
        txt_commit_message_->Clear();
        wxMessageBox(_("Committed successfully!"), _("Success"));
        RefreshStatus();
        RefreshHistory();
    } else {
        wxMessageBox(_("Commit failed."), _("Error"), wxOK | wxICON_ERROR);
    }
}

void GitIntegrationFrame::OnPush(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    auto result = git_->Push();
    wxMessageBox(result.success ? _("Push successful!") : _("Push failed."),
                 result.success ? _("Success") : _("Error"),
                 wxOK | (result.success ? wxICON_INFORMATION : wxICON_ERROR));
    if (result.success) RefreshStatus();
}

void GitIntegrationFrame::OnPull(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    auto result = git_->Pull();
    wxMessageBox(result.success ? _("Pull successful!") : _("Pull failed."),
                 result.success ? _("Success") : _("Error"),
                 wxOK | (result.success ? wxICON_INFORMATION : wxICON_ERROR));
    if (result.success) {
        RefreshStatus();
        RefreshHistory();
    }
}

void GitIntegrationFrame::OnFetch(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    auto result = git_->Fetch();
    wxMessageBox(result.success ? _("Fetch successful!") : _("Fetch failed."),
                 result.success ? _("Success") : _("Error"),
                 wxOK | (result.success ? wxICON_INFORMATION : wxICON_ERROR));
    if (result.success) RefreshStatus();
}

void GitIntegrationFrame::OnRefresh(wxCommandEvent& /*event*/) {
    RefreshStatus();
    RefreshHistory();
    RefreshBranches();
}

// Branch Operations
void GitIntegrationFrame::OnCreateBranch(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    wxString name = wxGetTextFromUser(_("Branch name:"), _("New Branch"));
    if (name.IsEmpty()) return;
    
    auto result = git_->CreateBranch(name.ToStdString());
    wxMessageBox(result.success ? _("Branch created!") : _("Failed to create branch."),
                 result.success ? _("Success") : _("Error"),
                 wxOK | (result.success ? wxICON_INFORMATION : wxICON_ERROR));
    if (result.success) RefreshBranches();
}

void GitIntegrationFrame::OnCheckoutBranch(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    long sel = list_branches_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel == -1) {
        wxMessageBox(_("Select a branch first."), _("Info"), wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxString name = list_branches_->GetItemText(sel, 0);
    name.Replace("* ", "");
    
    auto result = git_->CheckoutBranch(name.ToStdString());
    wxMessageBox(result.success ? _("Checked out!") : _("Checkout failed."),
                 result.success ? _("Success") : _("Error"),
                 wxOK | (result.success ? wxICON_INFORMATION : wxICON_ERROR));
    if (result.success) {
        RefreshStatus();
        RefreshHistory();
    }
}

void GitIntegrationFrame::OnMergeBranch(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    long sel = list_branches_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel == -1) {
        wxMessageBox(_("Select a branch to merge."), _("Info"), wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxString name = list_branches_->GetItemText(sel, 0);
    name.Replace("* ", "");
    
    if (wxMessageBox(_("Merge '") + name + _("' into current branch?"),
                     _("Confirm"), wxYES_NO | wxICON_QUESTION) == wxYES) {
        auto result = git_->MergeBranch(name.ToStdString());
        if (result.success) {
            wxMessageBox(_("Merge successful!"), _("Success"));
            RefreshStatus();
            RefreshHistory();
        } else if (git_->IsMergeInProgress()) {
            wxMessageBox(_("Merge has conflicts. Resolve manually."), _("Conflict"), wxOK | wxICON_WARNING);
        } else {
            wxMessageBox(_("Merge failed."), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}

void GitIntegrationFrame::OnDeleteBranch(wxCommandEvent& /*event*/) {
    if (!has_repository_) return;
    long sel = list_branches_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel == -1) return;
    
    wxString name = list_branches_->GetItemText(sel, 0);
    name.Replace("* ", "");
    
    if (wxMessageBox(_("Delete branch '") + name + _("'?"),
                     _("Confirm"), wxYES_NO | wxICON_QUESTION) == wxYES) {
        auto result = git_->DeleteBranch(name.ToStdString());
        wxMessageBox(result.success ? _("Branch deleted.") : _("Delete failed."),
                     result.success ? _("Success") : _("Error"),
                     wxOK | (result.success ? wxICON_INFORMATION : wxICON_ERROR));
        if (result.success) RefreshBranches();
    }
}

// UI Updates
void GitIntegrationFrame::RefreshStatus() {
    if (!has_repository_ || !git_->IsOpen()) {
        lbl_repo_name_->SetLabel(_("No repository"));
        lbl_branch_->SetLabel(wxEmptyString);
        lbl_ahead_behind_->SetLabel(wxEmptyString);
        list_changed_files_->DeleteAllItems();
        return;
    }
    
    lbl_repo_name_->SetLabel(wxFileName(project_path_).GetName());
    
    auto current = git_->GetCurrentBranch();
    if (current) {
        lbl_branch_->SetLabel("  " + wxString(current->name));
        int ahead = git_->GetAheadCount();
        int behind = git_->GetBehindCount();
        if (ahead > 0 || behind > 0) {
            lbl_ahead_behind_->SetLabel(wxString::Format("  %d ahead | %d behind", ahead, behind));
        } else {
            lbl_ahead_behind_->SetLabel(wxEmptyString);
        }
    }
    
    list_changed_files_->DeleteAllItems();
    auto files = git_->GetChangedFiles();
    for (size_t i = 0; i < files.size(); ++i) {
        const auto& f = files[i];
        wxString status;
        switch (f.status) {
            case FileChangeStatus::Modified: status = _("M"); break;
            case FileChangeStatus::Staged: status = _("A"); break;
            case FileChangeStatus::Deleted: status = _("D"); break;
            case FileChangeStatus::Untracked: status = _("?"); break;
            case FileChangeStatus::Conflicted: status = _("C"); break;
            default: status = _(" ");
        }
        long idx = list_changed_files_->InsertItem(i, status);
        list_changed_files_->SetItem(idx, 1, f.path);
    }
}

void GitIntegrationFrame::RefreshHistory() {
    if (!has_repository_ || !git_->IsOpen()) {
        list_commits_->DeleteAllItems();
        return;
    }
    
    list_commits_->DeleteAllItems();
    auto commits = git_->GetCommitHistory(50);
    for (size_t i = 0; i < commits.size(); ++i) {
        const auto& c = commits[i];
        long idx = list_commits_->InsertItem(i, c.shortHash);
        list_commits_->SetItem(idx, 1, c.message);
        list_commits_->SetItem(idx, 2, c.authorName);
        auto time = std::chrono::system_clock::to_time_t(c.authorDate);
        wxString date = wxString::Format("%s", std::ctime(&time));
        date.Trim();
        list_commits_->SetItem(idx, 3, date);
    }
}

void GitIntegrationFrame::RefreshBranches() {
    if (!has_repository_ || !git_->IsOpen()) {
        list_branches_->DeleteAllItems();
        list_remotes_->DeleteAllItems();
        return;
    }
    
    list_branches_->DeleteAllItems();
    auto branches = git_->GetBranches();
    for (size_t i = 0; i < branches.size(); ++i) {
        const auto& b = branches[i];
        wxString name = b.isCurrent ? "* " + b.name : b.name;
        long idx = list_branches_->InsertItem(i, name);
        list_branches_->SetItem(idx, 1, b.commitHash);
        list_branches_->SetItem(idx, 2, b.commitMessage);
    }
    
    list_remotes_->DeleteAllItems();
    auto remotes = git_->GetRemotes();
    for (size_t i = 0; i < remotes.size(); ++i) {
        long idx = list_remotes_->InsertItem(i, remotes[i].name);
        list_remotes_->SetItem(idx, 1, remotes[i].fetchUrl);
    }
}

void GitIntegrationFrame::OnTimer(wxTimerEvent& /*event*/) {
    if (has_repository_) RefreshStatus();
}

void GitIntegrationFrame::OnFileSelected(wxListEvent& /*event*/) {}
void GitIntegrationFrame::OnCommitSelected(wxListEvent& /*event*/) {}
void GitIntegrationFrame::OnBranchSelected(wxListEvent& /*event*/) {}

void GitIntegrationFrame::OnClose(wxCloseEvent& event) {
    refresh_timer_.Stop();
    if (window_manager_) window_manager_->UnregisterWindow(this);
    Destroy();
}

void GitIntegrationFrame::OnShowDocumentation(wxCommandEvent& /*event*/) {
    wxLaunchDefaultBrowser("https://scratchbird.dev/docs/git-integration");
}

} // namespace scratchrobin
