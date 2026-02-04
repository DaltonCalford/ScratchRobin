/**
 * @file git_model.h
 * @brief Data models for Git integration (Beta placeholder)
 * 
 * This file defines the data structures for Git version control
 * integration with database objects and migration scripts.
 * 
 * Status: Beta Placeholder - UI structure only
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <map>

namespace scratchrobin {

/**
 * @brief Git repository information
 */
struct GitRepository {
    std::string repoId;
    std::string name;
    std::string localPath;
    std::optional<std::string> remoteUrl;
    
    // Current state
    std::string currentBranch;
    std::optional<std::string> currentCommit;
    bool hasUncommittedChanges = false;
    int aheadCount = 0;  // Commits ahead of remote
    int behindCount = 0; // Commits behind remote
    
    // Configuration
    std::string defaultRemote = "origin";
    std::string defaultBranch = "main";
    
    // Last sync
    std::optional<std::chrono::system_clock::time_point> lastFetch;
    std::optional<std::chrono::system_clock::time_point> lastPull;
    std::optional<std::chrono::system_clock::time_point> lastPush;
};

/**
 * @brief Git branch information
 */
struct GitBranch {
    std::string name;
    std::string commitHash;
    std::string commitMessage;
    std::chrono::system_clock::time_point commitDate;
    std::string author;
    
    // Branch type
    bool isLocal = true;
    bool isRemote = false;
    bool isCurrent = false;
    bool isDefault = false;
    
    // Tracking info
    std::optional<std::string> upstreamBranch;
    int aheadCount = 0;
    int behindCount = 0;
};

/**
 * @brief Git commit information
 */
struct GitCommit {
    std::string hash;
    std::string shortHash;
    std::string message;
    std::string authorName;
    std::string authorEmail;
    std::chrono::system_clock::time_point authorDate;
    std::string committerName;
    std::chrono::system_clock::time_point commitDate;
    std::vector<std::string> parentHashes;
    std::vector<std::string> changedFiles;
};

/**
 * @brief File change status
 */
enum class FileChangeStatus {
    Untracked,
    Modified,
    Staged,
    Deleted,
    Renamed,
    Conflicted,
    Ignored
};

/**
 * @brief Changed file entry
 */
struct GitChangedFile {
    std::string path;
    FileChangeStatus status = FileChangeStatus::Untracked;
    std::optional<std::string> oldPath;  // For renames
    int additions = 0;
    int deletions = 0;
    bool isStaged = false;
};

/**
 * @brief Diff hunk (change section)
 */
struct GitDiffHunk {
    std::string oldRange;  // e.g., "@@ -10,5 +10,7 @@"
    std::string newRange;
    std::vector<std::string> lines;  // With +/- prefixes
};

/**
 * @brief File diff
 */
struct GitFileDiff {
    std::string oldPath;
    std::string newPath;
    std::string oldMode;
    std::string newMode;
    bool isNewFile = false;
    bool isDeleted = false;
    bool isBinary = false;
    std::vector<GitDiffHunk> hunks;
};

/**
 * @brief Stash entry
 */
struct GitStash {
    std::string stashId;  // e.g., "stash@{0}"
    std::string message;
    std::string commitHash;
    std::chrono::system_clock::time_point timestamp;
    std::string author;
};

/**
 * @brief Tag information
 */
struct GitTag {
    std::string name;
    std::string commitHash;
    std::optional<std::string> message;
    bool isAnnotated = false;
    std::optional<std::string> tagger;
    std::optional<std::chrono::system_clock::time_point> tagDate;
};

/**
 * @brief Remote information
 */
struct GitRemote {
    std::string name;
    std::string fetchUrl;
    std::optional<std::string> pushUrl;
    std::vector<std::string> branches;  // Remote tracking branches
};

/**
 * @brief Merge/Rebase status
 */
enum class MergeStatus {
    None,
    Merging,
    Rebasing,
    CherryPicking,
    Reverting,
    Bisecting
};

/**
 * @brief Git operation result
 */
struct GitOperationResult {
    bool success = false;
    std::optional<std::string> errorMessage;
    std::optional<std::string> output;
    
    // For operations that return data
    std::optional<GitCommit> commit;
    std::optional<GitBranch> branch;
};

/**
 * @brief Database object to Git file mapping
 * 
 * Maps database objects to their representation in version control.
 */
struct DbObjectGitMapping {
    std::string objectType;  // "table", "view", "procedure", etc.
    std::string schemaName;
    std::string objectName;
    std::string gitPath;     // Relative path in repo
    std::string filePattern; // e.g., "tables/{schema}/{table}.sql"
    
    // Current state
    std::optional<std::string> committedVersion;  // Hash of committed version
    std::optional<std::string> currentDefinition; // Current hash
    bool isModified = false;
};

/**
 * @brief Migration script tracking
 */
struct MigrationScript {
    std::string scriptId;
    std::string version;        // Semantic version or timestamp
    std::string description;
    std::string author;
    std::chrono::system_clock::time_point createdAt;
    std::string filename;
    
    // Script type
    enum Type { Upgrade, Downgrade, Repeatable, Baseline } type = Upgrade;
    
    // Execution status
    bool isApplied = false;
    std::optional<std::chrono::system_clock::time_point> appliedAt;
    std::optional<std::string> appliedBy;
    std::optional<std::chrono::milliseconds> executionTime;
    std::optional<std::string> checksum;
};

/**
 * @brief Git configuration for database project
 */
struct GitDbConfig {
    // Repository structure
    std::string schemaDirectory = "schema/";
    std::string migrationsDirectory = "migrations/";
    std::string seedsDirectory = "seeds/";
    std::string proceduresDirectory = "procedures/";
    
    // File patterns
    std::string tableFilePattern = "tables/{schema}/{table}.sql";
    std::string viewFilePattern = "views/{schema}/{view}.sql";
    std::string indexFilePattern = "indexes/{schema}/{table}/{index}.sql";
    std::string triggerFilePattern = "triggers/{schema}/{table}/{trigger}.sql";
    std::string procedureFilePattern = "procedures/{schema}/{procedure}.sql";
    
    // DDL generation options
    bool includeDropStatements = false;
    bool includeIfNotExists = true;
    bool separateConstraints = true;
    bool orderByDependency = true;
    
    // Ignore patterns
    std::vector<std::string> ignorePatterns = {
        "*.log",
        ".env",
        "local_*"
    };
};

/**
 * @brief Git workflow settings
 */
struct GitWorkflowConfig {
    // Branch naming
    std::string featureBranchPrefix = "feature/";
    std::string bugfixBranchPrefix = "bugfix/";
    std::string releaseBranchPrefix = "release/";
    std::string hotfixBranchPrefix = "hotfix/";
    
    // Commit conventions
    bool requireConventionalCommits = false;
    std::vector<std::string> commitTypes = {
        "feat", "fix", "docs", "style", "refactor", 
        "perf", "test", "chore", "db", "migration"
    };
    
    // Hooks
    bool preCommitValidation = true;  // Validate SQL before commit
    bool prePushTests = false;        // Run tests before push
    
    // Integration
    bool autoGenerateMigrations = true;
    bool trackDatabaseState = true;
};

} // namespace scratchrobin
