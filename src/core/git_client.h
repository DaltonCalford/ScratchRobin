/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_GIT_CLIENT_H
#define SCRATCHROBIN_GIT_CLIENT_H

#include "git_model.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// Git Client - Wrapper for Git command-line operations
// ============================================================================
class GitClient {
public:
    GitClient();
    ~GitClient();

    // Repository operations
    bool InitRepository(const std::string& path);
    bool CloneRepository(const std::string& url, const std::string& localPath,
                         const std::optional<std::string>& branch = std::nullopt);
    bool OpenRepository(const std::string& path);
    bool IsRepository(const std::string& path) const;
    void CloseRepository();
    bool IsOpen() const { return !repoPath_.empty(); }
    std::string GetRepositoryPath() const { return repoPath_; }

    // Configuration
    bool SetConfig(const std::string& key, const std::string& value);
    std::optional<std::string> GetConfig(const std::string& key);

    // Basic operations
    GitOperationResult Status();
    std::vector<GitChangedFile> GetChangedFiles() const;
    GitOperationResult Add(const std::string& path);
    GitOperationResult AddAll();
    GitOperationResult Remove(const std::string& path);
    GitOperationResult Reset(const std::string& path);
    GitOperationResult ResetAll();

    // Commit operations
    GitOperationResult Commit(const std::string& message,
                               const std::optional<std::string>& author = std::nullopt);
    std::optional<GitCommit> GetCommit(const std::string& hash);
    std::vector<GitCommit> GetCommitHistory(int count = 20,
                                            const std::optional<std::string>& branch = std::nullopt);
    std::optional<GitCommit> GetLatestCommit(const std::optional<std::string>& branch = std::nullopt);

    // Branch operations
    std::vector<GitBranch> GetBranches();
    std::optional<GitBranch> GetCurrentBranch();
    GitOperationResult CreateBranch(const std::string& name,
                                     const std::optional<std::string>& base = std::nullopt);
    GitOperationResult CheckoutBranch(const std::string& name);
    GitOperationResult CheckoutCommit(const std::string& hash);
    GitOperationResult DeleteBranch(const std::string& name, bool force = false);
    GitOperationResult RenameBranch(const std::string& oldName, const std::string& newName);

    // Merge operations
    GitOperationResult MergeBranch(const std::string& branchName,
                                    const std::optional<std::string>& message = std::nullopt);
    GitOperationResult AbortMerge();
    bool IsMergeInProgress() const;

    // Remote operations
    std::vector<GitRemote> GetRemotes();
    GitOperationResult AddRemote(const std::string& name, const std::string& url);
    GitOperationResult RemoveRemote(const std::string& name);
    GitOperationResult SetRemoteUrl(const std::string& name, const std::string& url);
    GitOperationResult Fetch(const std::optional<std::string>& remote = std::nullopt,
                              const std::optional<std::string>& branch = std::nullopt);
    GitOperationResult Pull(const std::optional<std::string>& remote = std::nullopt,
                             const std::optional<std::string>& branch = std::nullopt);
    GitOperationResult Push(const std::optional<std::string>& remote = std::nullopt,
                             const std::optional<std::string>& branch = std::nullopt);

    // Diff operations
    std::vector<GitFileDiff> GetDiff(const std::optional<std::string>& commit1 = std::nullopt,
                                      const std::optional<std::string>& commit2 = std::nullopt);
    std::optional<GitFileDiff> GetFileDiff(const std::string& path,
                                            const std::optional<std::string>& commit1 = std::nullopt,
                                            const std::optional<std::string>& commit2 = std::nullopt);
    std::string GetFileContent(const std::string& path, const std::optional<std::string>& commit = std::nullopt);

    // Stash operations
    GitOperationResult Stash(const std::optional<std::string>& message = std::nullopt);
    GitOperationResult StashPop(const std::optional<std::string>& stashId = std::nullopt);
    GitOperationResult StashApply(const std::optional<std::string>& stashId = std::nullopt);
    GitOperationResult StashDrop(const std::string& stashId);
    std::vector<GitStash> GetStashList();

    // Tag operations
    GitOperationResult CreateTag(const std::string& name,
                                  const std::optional<std::string>& message = std::nullopt,
                                  const std::optional<std::string>& commit = std::nullopt);
    GitOperationResult DeleteTag(const std::string& name);
    std::vector<GitTag> GetTags();

    // Conflict resolution
    std::vector<std::string> GetConflictedFiles();
    GitOperationResult MarkResolved(const std::string& path);

    // Utility
    bool HasUncommittedChanges() const;
    bool HasUntrackedFiles() const;
    int GetAheadCount() const;
    int GetBehindCount() const;
    std::optional<std::string> ExecuteCommand(const std::string& command) const;

    // Progress callback
    using ProgressCallback = std::function<void(const std::string& stage, int percent)>;
    void SetProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }

private:
    std::string repoPath_;
    ProgressCallback progressCallback_;

    // Helper methods
    std::string EscapePath(const std::string& path) const;
    std::vector<std::string> SplitLines(const std::string& text) const;
    std::string Trim(const std::string& str) const;
    FileChangeStatus ParseStatusCode(const std::string& code) const;
    GitCommit ParseCommitLine(const std::string& line) const;
    GitBranch ParseBranchLine(const std::string& line) const;
};

// ============================================================================
// Project Git Manager - Integration between Project and Git
// ============================================================================
class ProjectGitManager {
public:
    static ProjectGitManager& Instance();

    // Project-level operations
    bool InitializeProjectRepository(const std::string& projectPath,
                                      const std::optional<std::string>& remoteUrl = std::nullopt);
    bool OpenProjectRepository(const std::string& projectPath);
    bool IsProjectRepositoryOpen() const;
    void CloseProjectRepository();

    // Sync operations
    bool SyncDesignToRepository(const std::string& designsPath);
    bool SyncRepositoryToDesign(const std::string& designsPath);
    bool ExtractFromDatabaseToRepository(const std::string& connectionString,
                                          const std::vector<std::string>& objectPatterns);

    // Conflict handling
    std::vector<std::string> GetConflictedObjects();
    bool ResolveObjectConflict(const std::string& objectPath,
                                const std::string& resolution);  // "ours", "theirs", "merge"

    // Status
    GitRepository GetRepositoryStatus();
    std::vector<GitChangedFile> GetChangedDesignFiles();
    bool HasUncommittedDesignChanges() const;

    // DDL Generation
    std::string GenerateObjectDDL(const std::string& objectType,
                                   const std::string& schema,
                                   const std::string& name);
    bool WriteObjectToRepository(const std::string& objectType,
                                  const std::string& schema,
                                  const std::string& name,
                                  const std::string& ddl);

    // Migration scripts
    std::string GenerateMigrationScript(const std::vector<std::string>& changedObjects);
    bool WriteMigrationScript(const std::string& script, const std::string& version);

private:
    ProjectGitManager() = default;
    ~ProjectGitManager() = default;

    GitClient git_;
    GitDbConfig config_;
    bool isOpen_ = false;

    std::string GetObjectFilePath(const std::string& objectType,
                                   const std::string& schema,
                                   const std::string& name);
    std::string ApplyFilePattern(const std::string& pattern,
                                  const std::string& schema,
                                  const std::string& name);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_GIT_CLIENT_H
