/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "git_client.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace scratchrobin {

namespace fs = std::filesystem;

// ============================================================================
// GitClient Implementation
// ============================================================================
GitClient::GitClient() = default;
GitClient::~GitClient() = default;

bool GitClient::InitRepository(const std::string& path) {
    if (!fs::exists(path)) {
        fs::create_directories(path);
    }

    auto result = ExecuteCommand("cd \"" + path + "\" && git init");
    if (result) {
        repoPath_ = path;
        return true;
    }
    return false;
}

bool GitClient::CloneRepository(const std::string& url, const std::string& localPath,
                                 const std::optional<std::string>& branch) {
    if (fs::exists(localPath)) {
        return false;  // Path already exists
    }

    std::string cmd = "git clone";
    if (branch) {
        cmd += " -b " + *branch;
    }
    cmd += " \"" + url + "\" \"" + localPath + "\"";

    auto result = ExecuteCommand(cmd);
    if (result) {
        repoPath_ = localPath;
        return true;
    }
    return false;
}

bool GitClient::OpenRepository(const std::string& path) {
    if (!IsRepository(path)) {
        return false;
    }
    repoPath_ = path;
    return true;
}

void GitClient::CloseRepository() {
    repoPath_.clear();
}

bool GitClient::IsRepository(const std::string& path) const {
    return fs::exists(path + "/.git");
}

bool GitClient::SetConfig(const std::string& key, const std::string& value) {
    if (!IsOpen()) return false;
    auto result = ExecuteCommand("cd \"" + repoPath_ + "\" && git config \"" + key + "\" \"" + value + "\"");
    return result.has_value();
}

std::optional<std::string> GitClient::GetConfig(const std::string& key) {
    if (!IsOpen()) return std::nullopt;
    return ExecuteCommand("cd \"" + repoPath_ + "\" && git config \"" + key + "\"");
}

GitOperationResult GitClient::Status() {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git status --porcelain -b");
    if (output) {
        result.success = true;
        result.output = output;
    } else {
        result.errorMessage = "Failed to get status";
    }
    return result;
}

std::vector<GitChangedFile> GitClient::GetChangedFiles() const {
    std::vector<GitChangedFile> files;
    if (!IsOpen()) return files;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git status --porcelain");
    if (!output) return files;

    auto lines = SplitLines(*output);
    for (const auto& line : lines) {
        if (line.length() < 3) continue;

        GitChangedFile file;
        std::string indexStatus = line.substr(0, 1);
        std::string worktreeStatus = line.substr(1, 1);
        file.path = Trim(line.substr(3));

        // Parse status codes
        if (indexStatus != " " && indexStatus != "?") {
            file.isStaged = true;
            file.status = ParseStatusCode(indexStatus);
        } else {
            file.isStaged = false;
            file.status = ParseStatusCode(worktreeStatus);
        }

        files.push_back(file);
    }

    return files;
}

GitOperationResult GitClient::Add(const std::string& path) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git add \"" + path + "\"");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to add file";
    }
    return result;
}

GitOperationResult GitClient::AddAll() {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git add -A");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to add files";
    }
    return result;
}

GitOperationResult GitClient::Remove(const std::string& path) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git rm \"" + path + "\"");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to remove file";
    }
    return result;
}

GitOperationResult GitClient::Reset(const std::string& path) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git reset HEAD \"" + path + "\"");
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::ResetAll() {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git reset HEAD");
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::Commit(const std::string& message,
                                      const std::optional<std::string>& author) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git commit -m \"" + message + "\"";
    if (author) {
        cmd += " --author=\"" + *author + "\"";
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (result.success) {
        // Get the commit we just created
        result.commit = GetLatestCommit();
    } else {
        result.errorMessage = "Failed to commit";
    }
    return result;
}

std::optional<GitCommit> GitClient::GetCommit(const std::string& hash) {
    if (!IsOpen()) return std::nullopt;

    std::string cmd = "cd \"" + repoPath_ + "\" && git show -s --format=\"%H|%h|%s|%an|%ae|%at|%cn|%ct\" " + hash;
    auto output = ExecuteCommand(cmd);
    if (!output) return std::nullopt;

    // Parse output
    GitCommit commit;
    std::istringstream iss(*output);
    std::string token;
    std::getline(iss, commit.hash, '|');
    std::getline(iss, commit.shortHash, '|');
    std::getline(iss, commit.message, '|');
    std::getline(iss, commit.authorName, '|');
    std::getline(iss, commit.authorEmail, '|');
    std::string atime;
    std::getline(iss, atime, '|');
    commit.authorDate = std::chrono::system_clock::from_time_t(std::stoll(atime));
    std::getline(iss, commit.committerName, '|');
    std::string ctime;
    std::getline(iss, ctime, '|');
    commit.commitDate = std::chrono::system_clock::from_time_t(std::stoll(ctime));

    return commit;
}

std::vector<GitCommit> GitClient::GetCommitHistory(int count,
                                                    const std::optional<std::string>& branch) {
    std::vector<GitCommit> commits;
    if (!IsOpen()) return commits;

    std::string cmd = "cd \"" + repoPath_ + "\" && git log -" + std::to_string(count) +
                      " --format=\"%H|%h|%s|%an|%ae|%at|%cn|%ct\"";
    if (branch) {
        cmd += " " + *branch;
    }

    auto output = ExecuteCommand(cmd);
    if (!output) return commits;

    auto lines = SplitLines(*output);
    for (const auto& line : lines) {
        if (line.empty()) continue;

        GitCommit commit;
        std::istringstream iss(line);
        std::string token;
        std::getline(iss, commit.hash, '|');
        std::getline(iss, commit.shortHash, '|');
        std::getline(iss, commit.message, '|');
        std::getline(iss, commit.authorName, '|');
        std::getline(iss, commit.authorEmail, '|');
        std::string atime;
        std::getline(iss, atime, '|');
        if (!atime.empty()) {
            commit.authorDate = std::chrono::system_clock::from_time_t(std::stoll(atime));
        }
        std::getline(iss, commit.committerName, '|');
        std::string ctime;
        std::getline(iss, ctime, '|');
        if (!ctime.empty()) {
            commit.commitDate = std::chrono::system_clock::from_time_t(std::stoll(ctime));
        }

        commits.push_back(commit);
    }

    return commits;
}

std::optional<GitCommit> GitClient::GetLatestCommit(const std::optional<std::string>& branch) {
    auto commits = GetCommitHistory(1, branch);
    if (!commits.empty()) {
        return commits[0];
    }
    return std::nullopt;
}

std::vector<GitBranch> GitClient::GetBranches() {
    std::vector<GitBranch> branches;
    if (!IsOpen()) return branches;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git branch -a -v --format=\"%(refname:short)|%(objectname:short)|%(subject)|%(authorname)|%(authordate:unix)\"");
    if (!output) return branches;

    auto current = GetCurrentBranch();

    auto lines = SplitLines(*output);
    for (const auto& line : lines) {
        if (line.empty()) continue;

        GitBranch branch = ParseBranchLine(line);
        if (current && branch.name == current->name) {
            branch.isCurrent = true;
        }
        branches.push_back(branch);
    }

    return branches;
}

std::optional<GitBranch> GitClient::GetCurrentBranch() {
    if (!IsOpen()) return std::nullopt;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git branch --show-current");
    if (!output) return std::nullopt;

    GitBranch branch;
    branch.name = Trim(*output);
    branch.isCurrent = true;
    branch.isLocal = true;

    // Get additional info
    auto latest = GetLatestCommit();
    if (latest) {
        branch.commitHash = latest->shortHash;
        branch.commitMessage = latest->message;
        branch.commitDate = latest->authorDate;
        branch.author = latest->authorName;
    }

    return branch;
}

GitOperationResult GitClient::CreateBranch(const std::string& name,
                                            const std::optional<std::string>& base) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git branch \"" + name + "\"";
    if (base) {
        cmd += " \"" + *base + "\"";
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to create branch";
    }
    return result;
}

GitOperationResult GitClient::CheckoutBranch(const std::string& name) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git checkout \"" + name + "\"");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to checkout branch";
    }
    return result;
}

GitOperationResult GitClient::CheckoutCommit(const std::string& hash) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git checkout \"" + hash + "\"");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to checkout commit";
    }
    return result;
}

GitOperationResult GitClient::DeleteBranch(const std::string& name, bool force) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git branch ";
    cmd += (force ? "-D " : "-d ");
    cmd += "\"" + name + "\"";

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to delete branch";
    }
    return result;
}

GitOperationResult GitClient::RenameBranch(const std::string& oldName,
                                            const std::string& newName) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git branch -m \"" + oldName + "\" \"" + newName + "\"");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to rename branch";
    }
    return result;
}

GitOperationResult GitClient::MergeBranch(const std::string& branchName,
                                           const std::optional<std::string>& message) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git merge";
    if (message) {
        cmd += " -m \"" + *message + "\"";
    }
    cmd += " \"" + branchName + "\"";

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to merge branch";
    }
    return result;
}

GitOperationResult GitClient::AbortMerge() {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git merge --abort");
    result.success = output.has_value();
    return result;
}

bool GitClient::IsMergeInProgress() const {
    if (!IsOpen()) return false;
    return fs::exists(repoPath_ + "/.git/MERGE_HEAD");
}

std::vector<GitRemote> GitClient::GetRemotes() {
    std::vector<GitRemote> remotes;
    if (!IsOpen()) return remotes;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git remote -v");
    if (!output) return remotes;

    // Parse remote output
    auto lines = SplitLines(*output);
    std::map<std::string, GitRemote> remoteMap;

    for (const auto& line : lines) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string name, url, type;
        iss >> name >> url >> type;

        if (remoteMap.find(name) == remoteMap.end()) {
            remoteMap[name] = GitRemote{name, url, std::nullopt, {}};
        }

        if (type == "(fetch)") {
            remoteMap[name].fetchUrl = url;
        } else if (type == "(push)") {
            remoteMap[name].pushUrl = url;
        }
    }

    for (const auto& [name, remote] : remoteMap) {
        remotes.push_back(remote);
    }

    return remotes;
}

GitOperationResult GitClient::AddRemote(const std::string& name, const std::string& url) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git remote add \"" + name + "\" \"" + url + "\"");
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to add remote";
    }
    return result;
}

GitOperationResult GitClient::RemoveRemote(const std::string& name) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git remote remove \"" + name + "\"");
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::SetRemoteUrl(const std::string& name, const std::string& url) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git remote set-url \"" + name + "\" \"" + url + "\"");
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::Fetch(const std::optional<std::string>& remote,
                                     const std::optional<std::string>& branch) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git fetch";
    if (remote) {
        cmd += " \"" + *remote + "\"";
        if (branch) {
            cmd += " \"" + *branch + "\"";
        }
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to fetch";
    }
    return result;
}

GitOperationResult GitClient::Pull(const std::optional<std::string>& remote,
                                    const std::optional<std::string>& branch) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git pull";
    if (remote) {
        cmd += " \"" + *remote + "\"";
        if (branch) {
            cmd += " \"" + *branch + "\"";
        }
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to pull";
    }
    return result;
}

GitOperationResult GitClient::Push(const std::optional<std::string>& remote,
                                    const std::optional<std::string>& branch) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git push";
    if (remote) {
        cmd += " \"" + *remote + "\"";
        if (branch) {
            cmd += " \"" + *branch + "\"";
        }
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    if (!result.success) {
        result.errorMessage = "Failed to push";
    }
    return result;
}

std::vector<GitFileDiff> GitClient::GetDiff(const std::optional<std::string>& commit1,
                                             const std::optional<std::string>& commit2) {
    std::vector<GitFileDiff> diffs;
    // Simplified implementation - would parse git diff output
    return diffs;
}

std::optional<GitFileDiff> GitClient::GetFileDiff(const std::string& path,
                                                   const std::optional<std::string>& commit1,
                                                   const std::optional<std::string>& commit2) {
    // Simplified implementation
    return std::nullopt;
}

std::string GitClient::GetFileContent(const std::string& path,
                                       const std::optional<std::string>& commit) {
    if (!IsOpen()) return "";

    std::string cmd = "cd \"" + repoPath_ + "\" && git show ";
    if (commit) {
        cmd += *commit + ":";
    } else {
        cmd += "HEAD:";
    }
    cmd += path;

    auto output = ExecuteCommand(cmd);
    return output.value_or("");
}

GitOperationResult GitClient::Stash(const std::optional<std::string>& message) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git stash";
    if (message) {
        cmd += " push -m \"" + *message + "\"";
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::StashPop(const std::optional<std::string>& stashId) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git stash pop";
    if (stashId) {
        cmd += " \"" + *stashId + "\"";
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::StashApply(const std::optional<std::string>& stashId) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git stash apply";
    if (stashId) {
        cmd += " \"" + *stashId + "\"";
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::StashDrop(const std::string& stashId) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git stash drop \"" + stashId + "\"");
    result.success = output.has_value();
    return result;
}

std::vector<GitStash> GitClient::GetStashList() {
    std::vector<GitStash> stashes;
    if (!IsOpen()) return stashes;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git stash list --format=\"%gd|%s|%H|%ai|%an\"");
    if (!output) return stashes;

    auto lines = SplitLines(*output);
    for (const auto& line : lines) {
        if (line.empty()) continue;

        GitStash stash;
        std::istringstream iss(line);
        std::getline(iss, stash.stashId, '|');
        std::getline(iss, stash.message, '|');
        std::getline(iss, stash.commitHash, '|');
        std::string date;
        std::getline(iss, date, '|');
        std::getline(iss, stash.author, '|');

        stashes.push_back(stash);
    }

    return stashes;
}

GitOperationResult GitClient::CreateTag(const std::string& name,
                                         const std::optional<std::string>& message,
                                         const std::optional<std::string>& commit) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    std::string cmd = "cd \"" + repoPath_ + "\" && git tag";
    if (message) {
        cmd += " -a -m \"" + *message + "\"";
    }
    cmd += " \"" + name + "\"";
    if (commit) {
        cmd += " \"" + *commit + "\"";
    }

    auto output = ExecuteCommand(cmd);
    result.success = output.has_value();
    return result;
}

GitOperationResult GitClient::DeleteTag(const std::string& name) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git tag -d \"" + name + "\"");
    result.success = output.has_value();
    return result;
}

std::vector<GitTag> GitClient::GetTags() {
    std::vector<GitTag> tags;
    if (!IsOpen()) return tags;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git tag -l -n1");
    if (!output) return tags;

    auto lines = SplitLines(*output);
    for (const auto& line : lines) {
        if (line.empty()) continue;

        GitTag tag;
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            tag.name = line.substr(0, spacePos);
            tag.message = Trim(line.substr(spacePos + 1));
        } else {
            tag.name = line;
        }

        tags.push_back(tag);
    }

    return tags;
}

std::vector<std::string> GitClient::GetConflictedFiles() {
    std::vector<std::string> files;
    if (!IsOpen()) return files;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git diff --name-only --diff-filter=U");
    if (!output) return files;

    auto lines = SplitLines(*output);
    for (const auto& line : lines) {
        if (!line.empty()) {
            files.push_back(line);
        }
    }

    return files;
}

GitOperationResult GitClient::MarkResolved(const std::string& path) {
    GitOperationResult result;
    if (!IsOpen()) {
        result.errorMessage = "No repository open";
        return result;
    }

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git add \"" + path + "\"");
    result.success = output.has_value();
    return result;
}

bool GitClient::HasUncommittedChanges() const {
    if (!IsOpen()) return false;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git status --porcelain");
    if (!output) return false;

    return !output->empty();
}

bool GitClient::HasUntrackedFiles() const {
    if (!IsOpen()) return false;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git status --porcelain");
    if (!output) return false;

    return output->find("??") != std::string::npos;
}

int GitClient::GetAheadCount() const {
    if (!IsOpen()) return 0;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git rev-list --count HEAD@{u}..HEAD 2>/dev/null || echo 0");
    if (!output) return 0;

    try {
        return std::stoi(Trim(*output));
    } catch (...) {
        return 0;
    }
}

int GitClient::GetBehindCount() const {
    if (!IsOpen()) return 0;

    auto output = ExecuteCommand("cd \"" + repoPath_ + "\" && git rev-list --count HEAD..HEAD@{u} 2>/dev/null || echo 0");
    if (!output) return 0;

    try {
        return std::stoi(Trim(*output));
    } catch (...) {
        return 0;
    }
}

std::optional<std::string> GitClient::ExecuteCommand(const std::string& command) const {
    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status != 0) {
        return std::nullopt;
    }

    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

// Helper methods
std::string GitClient::EscapePath(const std::string& path) const {
    std::string escaped;
    for (char c : path) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}

std::vector<std::string> GitClient::SplitLines(const std::string& text) const {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string GitClient::Trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

FileChangeStatus GitClient::ParseStatusCode(const std::string& code) const {
    if (code == "M") return FileChangeStatus::Modified;
    if (code == "A") return FileChangeStatus::Staged;
    if (code == "D") return FileChangeStatus::Deleted;
    if (code == "R") return FileChangeStatus::Renamed;
    if (code == "C") return FileChangeStatus::Modified;
    if (code == "U") return FileChangeStatus::Conflicted;
    if (code == "?") return FileChangeStatus::Untracked;
    if (code == "!") return FileChangeStatus::Ignored;
    return FileChangeStatus::Untracked;
}

GitCommit GitClient::ParseCommitLine(const std::string& line) const {
    GitCommit commit;
    // Implementation would parse formatted commit line
    return commit;
}

GitBranch GitClient::ParseBranchLine(const std::string& line) const {
    GitBranch branch;
    std::istringstream iss(line);
    std::string token;
    std::getline(iss, branch.name, '|');
    std::getline(iss, branch.commitHash, '|');
    std::getline(iss, branch.commitMessage, '|');
    std::getline(iss, branch.author, '|');
    std::string date;
    std::getline(iss, date, '|');
    if (!date.empty()) {
        branch.commitDate = std::chrono::system_clock::from_time_t(std::stoll(date));
    }
    branch.isLocal = (branch.name.find("remotes/") != 0);
    branch.isRemote = (branch.name.find("remotes/") == 0);
    return branch;
}

// ============================================================================
// ProjectGitManager Implementation
// ============================================================================
ProjectGitManager& ProjectGitManager::Instance() {
    static ProjectGitManager instance;
    return instance;
}

bool ProjectGitManager::InitializeProjectRepository(const std::string& projectPath,
                                                     const std::optional<std::string>& remoteUrl) {
    // Create repository
    if (!git_.InitRepository(projectPath)) {
        return false;
    }

    // Add remote if provided
    if (remoteUrl) {
        git_.AddRemote("origin", *remoteUrl);
    }

    // Create initial structure
    fs::create_directories(projectPath + "/" + config_.schemaDirectory);
    fs::create_directories(projectPath + "/" + config_.migrationsDirectory);
    fs::create_directories(projectPath + "/" + config_.seedsDirectory);
    fs::create_directories(projectPath + "/" + config_.proceduresDirectory);

    // Create .gitignore
    std::ofstream gitignore(projectPath + "/.gitignore");
    for (const auto& pattern : config_.ignorePatterns) {
        gitignore << pattern << "\n";
    }
    gitignore.close();

    // Initial commit
    git_.AddAll();
    git_.Commit("Initial commit: Database project structure");

    isOpen_ = true;
    return true;
}

bool ProjectGitManager::OpenProjectRepository(const std::string& projectPath) {
    if (!git_.OpenRepository(projectPath)) {
        return false;
    }
    isOpen_ = true;
    return true;
}

bool ProjectGitManager::IsProjectRepositoryOpen() const {
    return isOpen_ && git_.IsOpen();
}

void ProjectGitManager::CloseProjectRepository() {
    git_.CloseRepository();
    isOpen_ = false;
}

bool ProjectGitManager::SyncDesignToRepository(const std::string& designsPath) {
    if (!IsProjectRepositoryOpen()) return false;

    // Add all changes
    git_.AddAll();

    // Check if there are changes to commit
    if (!git_.HasUncommittedChanges()) {
        return true;  // Nothing to sync
    }

    // Commit with message
    auto result = git_.Commit("Sync design changes to repository");
    return result.success;
}

bool ProjectGitManager::SyncRepositoryToDesign(const std::string& designsPath) {
    if (!IsProjectRepositoryOpen()) return false;

    // Pull latest changes
    auto result = git_.Pull();
    return result.success;
}

bool ProjectGitManager::ExtractFromDatabaseToRepository(const std::string& connectionString,
                                                         const std::vector<std::string>& objectPatterns) {
    if (!IsProjectRepositoryOpen()) return false;

    // This would connect to database and extract objects
    // For now, just create a marker file
    std::ofstream marker(git_.GetRepositoryPath() + "/.extracted");
    marker << "Extracted from: " << connectionString << "\n";
    marker << "Patterns: ";
    for (const auto& pattern : objectPatterns) {
        marker << pattern << " ";
    }
    marker << "\n";
    marker.close();

    git_.Add(".extracted");
    git_.Commit("Extract objects from database");

    return true;
}

std::vector<std::string> ProjectGitManager::GetConflictedObjects() {
    std::vector<std::string> conflicts;
    if (!IsProjectRepositoryOpen()) return conflicts;

    auto files = git_.GetConflictedFiles();
    for (const auto& file : files) {
        if (file.find(config_.schemaDirectory) == 0) {
            conflicts.push_back(file);
        }
    }

    return conflicts;
}

std::vector<std::string> ProjectGitManager::GetConflictedFilesInPath(const std::string& pathPrefix) {
    std::vector<std::string> conflicts;
    if (!IsProjectRepositoryOpen()) return conflicts;

    auto files = git_.GetConflictedFiles();
    for (const auto& file : files) {
        if (file.find(pathPrefix) == 0) {
            conflicts.push_back(file);
        }
    }

    return conflicts;
}

bool ProjectGitManager::ResolveObjectConflict(const std::string& objectPath,
                                               const std::string& resolution) {
    if (!IsProjectRepositoryOpen()) return false;

    // For now, just mark as resolved
    git_.MarkResolved(objectPath);

    // Commit the resolution
    git_.Commit("Resolve conflict in " + objectPath);

    return true;
}

GitRepository ProjectGitManager::GetRepositoryStatus() {
    GitRepository status;
    if (!IsProjectRepositoryOpen()) return status;

    status.localPath = git_.GetRepositoryPath();

    auto currentBranch = git_.GetCurrentBranch();
    if (currentBranch) {
        status.currentBranch = currentBranch->name;
    }

    auto latestCommit = git_.GetLatestCommit();
    if (latestCommit) {
        status.currentCommit = latestCommit->hash;
    }

    status.hasUncommittedChanges = git_.HasUncommittedChanges();
    status.aheadCount = git_.GetAheadCount();
    status.behindCount = git_.GetBehindCount();

    auto remotes = git_.GetRemotes();
    if (!remotes.empty()) {
        status.remoteUrl = remotes[0].fetchUrl;
    }

    return status;
}

std::vector<GitChangedFile> ProjectGitManager::GetChangedDesignFiles() {
    if (!IsProjectRepositoryOpen()) return {};

    auto files = git_.GetChangedFiles();
    std::vector<GitChangedFile> designFiles;

    for (const auto& file : files) {
        if (file.path.find(config_.schemaDirectory) == 0 ||
            file.path.find(config_.migrationsDirectory) == 0) {
            designFiles.push_back(file);
        }
    }

    return designFiles;
}

std::vector<GitChangedFile> ProjectGitManager::GetChangedFilesInPath(const std::string& pathPrefix) {
    if (!IsProjectRepositoryOpen()) return {};

    auto files = git_.GetChangedFiles();
    std::vector<GitChangedFile> result;
    for (const auto& file : files) {
        if (file.path.find(pathPrefix) == 0) {
            result.push_back(file);
        }
    }

    return result;
}

bool ProjectGitManager::HasUncommittedDesignChanges() const {
    if (!IsProjectRepositoryOpen()) return false;

    auto files = git_.GetChangedFiles();
    for (const auto& file : files) {
        if (file.path.find(config_.schemaDirectory) == 0 ||
            file.path.find(config_.migrationsDirectory) == 0) {
            return true;
        }
    }

    return false;
}

std::string ProjectGitManager::GenerateObjectDDL(const std::string& objectType,
                                                  const std::string& schema,
                                                  const std::string& name) {
    // This would generate actual DDL
    std::ostringstream oss;
    oss << "-- " << objectType << ": " << schema << "." << name << "\n";
    oss << "-- Generated by ScratchRobin\n";
    oss << "-- TODO: Implement actual DDL generation\n";
    return oss.str();
}

bool ProjectGitManager::WriteObjectToRepository(const std::string& objectType,
                                                 const std::string& schema,
                                                 const std::string& name,
                                                 const std::string& ddl) {
    if (!IsProjectRepositoryOpen()) return false;

    std::string filePath = GetObjectFilePath(objectType, schema, name);
    std::string fullPath = git_.GetRepositoryPath() + "/" + filePath;

    // Ensure directory exists
    fs::create_directories(fs::path(fullPath).parent_path());

    // Write DDL to file
    std::ofstream file(fullPath);
    file << ddl;
    file.close();

    // Stage the file
    git_.Add(filePath);

    return true;
}

std::string ProjectGitManager::GenerateMigrationScript(const std::vector<std::string>& changedObjects) {
    std::ostringstream oss;
    oss << "-- Migration script\n";
    oss << "-- Generated by ScratchRobin\n";
    oss << "-- Changed objects:\n";
    for (const auto& obj : changedObjects) {
        oss << "--   - " << obj << "\n";
    }
    oss << "\n-- TODO: Generate actual migration DDL\n";
    return oss.str();
}

bool ProjectGitManager::WriteMigrationScript(const std::string& script, const std::string& version) {
    if (!IsProjectRepositoryOpen()) return false;

    std::string filename = config_.migrationsDirectory + "V" + version + "__migration.sql";
    std::string fullPath = git_.GetRepositoryPath() + "/" + filename;

    fs::create_directories(fs::path(fullPath).parent_path());

    std::ofstream file(fullPath);
    file << script;
    file.close();

    git_.Add(filename);

    return true;
}

std::string ProjectGitManager::GetObjectFilePath(const std::string& objectType,
                                                  const std::string& schema,
                                                  const std::string& name) {
    std::string pattern;

    if (objectType == "table") {
        pattern = config_.tableFilePattern;
    } else if (objectType == "view") {
        pattern = config_.viewFilePattern;
    } else if (objectType == "index") {
        pattern = config_.indexFilePattern;
    } else if (objectType == "trigger") {
        pattern = config_.triggerFilePattern;
    } else if (objectType == "procedure") {
        pattern = config_.procedureFilePattern;
    } else {
        pattern = config_.schemaDirectory + "{type}/{schema}/{name}.sql";
    }

    return ApplyFilePattern(pattern, schema, name);
}

std::string ProjectGitManager::ApplyFilePattern(const std::string& pattern,
                                                 const std::string& schema,
                                                 const std::string& name) {
    std::string result = pattern;

    // Replace placeholders
    size_t pos = result.find("{schema}");
    if (pos != std::string::npos) {
        result.replace(pos, 8, schema);
    }

    pos = result.find("{table}");
    if (pos != std::string::npos) {
        result.replace(pos, 7, name);
    }

    pos = result.find("{view}");
    if (pos != std::string::npos) {
        result.replace(pos, 6, name);
    }

    pos = result.find("{index}");
    if (pos != std::string::npos) {
        result.replace(pos, 7, name);
    }

    pos = result.find("{trigger}");
    if (pos != std::string::npos) {
        result.replace(pos, 9, name);
    }

    pos = result.find("{procedure}");
    if (pos != std::string::npos) {
        result.replace(pos, 11, name);
    }

    pos = result.find("{name}");
    if (pos != std::string::npos) {
        result.replace(pos, 6, name);
    }

    return result;
}

} // namespace scratchrobin
