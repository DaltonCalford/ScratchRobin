/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DATA_MASKING_H
#define SCRATCHROBIN_DATA_MASKING_H

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// Masking Method Types
// ============================================================================
enum class MaskingMethod {
    NONE,           // No masking
    REDACTION,      // Replace with fixed string (e.g., "***")
    PARTIAL,        // Partial masking (e.g., "***@domain.com")
    REGEX,          // Regex pattern replacement
    HASH,           // Cryptographic hash (SHA-256, etc.)
    ENCRYPTION,     // Format-preserving encryption
    RANDOMIZATION,  // Random value substitution
    SHUFFLING,      // Shuffle values within column
    NULLIFICATION,  // Replace with NULL
    TRUNCATION,     // Truncate data
    FORMAT_PRESERVING,  // FPE (e.g., 1234-5678-9012-3456 -> 9876-5432-1098-7654)
    SUBSTITUTION,   // Substitute with fake/generated data
    DATE_SHIFTING,  // Shift dates by random offset
    NOISE_ADDITION  // Add random noise to numeric values
};

std::string MaskingMethodToString(MaskingMethod method);
std::string MaskingMethodDescription(MaskingMethod method);

// ============================================================================
// Data Classification Types
// ============================================================================
enum class DataClassification {
    UNCLASSIFIED,
    PUBLIC,
    INTERNAL,
    CONFIDENTIAL,
    RESTRICTED,
    PII,            // Personally Identifiable Information
    PHI,            // Protected Health Information
    PCI,            // Payment Card Industry
    GDPR_SENSITIVE  // GDPR special categories
};

std::string ClassificationToString(DataClassification classification);

// ============================================================================
// Masking Rule
// ============================================================================
struct MaskingRule {
    std::string id;
    std::string name;
    std::string description;
    
    // Target
    std::string schema;
    std::string table;
    std::string column;
    
    // Classification
    DataClassification classification = DataClassification::UNCLASSIFIED;
    std::vector<std::string> tags;  // "email", "ssn", "credit_card", etc.
    
    // Masking configuration
    MaskingMethod method = MaskingMethod::NONE;
    
    // Method-specific parameters
    struct Parameters {
        // For PARTIAL
        int visible_chars_start = 0;
        int visible_chars_end = 0;
        std::string mask_char = "*";
        
        // For REGEX
        std::string regex_pattern;
        std::string regex_replacement;
        
        // For HASH
        std::string hash_algorithm = "SHA256";  // SHA256, MD5, etc.
        std::string hash_salt;
        
        // For ENCRYPTION
        std::string encryption_key_id;
        std::string encryption_algorithm = "AES-256-FPE";
        
        // For RANDOMIZATION / SUBSTITUTION
        std::string fake_data_generator;  // "name", "email", "phone", "address", etc.
        int randomization_seed = 0;
        
        // For DATE_SHIFTING
        int min_shift_days = -365;
        int max_shift_days = 365;
        
        // For NOISE_ADDITION
        double noise_percentage = 0.05;  // 5% noise
        
        // For TRUNCATION
        int max_length = 0;
        
        // For REDACTION
        std::string replacement_string = "***";
    };
    Parameters parameters;
    
    // Conditional masking
    std::string condition;  // SQL WHERE clause (e.g., "user_role = 'admin'")
    
    // Scope
    bool apply_to_dev = true;
    bool apply_to_test = true;
    bool apply_to_staging = false;
    bool apply_to_prod = false;  // Never apply to prod by default
    
    // Audit
    std::string created_by;
    std::time_t created_at;
    std::string modified_by;
    std::time_t modified_at;
    
    // Status
    bool enabled = true;
    
    MaskingRule();
    
    // Helpers
    std::string GetFullColumnName() const;
    bool AppliesToEnvironment(const std::string& env) const;
};

// ============================================================================
// Masking Profile
// ============================================================================
class MaskingProfile {
public:
    std::string id;
    std::string name;
    std::string description;
    std::string target_environment;  // "development", "testing", etc.
    
    std::vector<std::unique_ptr<MaskingRule>> rules;
    
    // Default rules for common classifications
    bool auto_detect_pii = true;
    bool auto_detect_pci = true;
    bool auto_detect_phi = true;
    
    MaskingProfile();
    explicit MaskingProfile(const std::string& name);
    
    // Rule management
    void AddRule(std::unique_ptr<MaskingRule> rule);
    void RemoveRule(const std::string& rule_id);
    MaskingRule* FindRule(const std::string& rule_id);
    MaskingRule* FindRuleForColumn(const std::string& schema, 
                                    const std::string& table,
                                    const std::string& column);
    
    // Get applicable rules for a table
    std::vector<MaskingRule*> GetRulesForTable(const std::string& schema,
                                                const std::string& table);
    
    // Load/Save
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<MaskingProfile> LoadFromFile(const std::string& path);
    
    // Validation
    bool Validate(std::vector<std::string>& errors) const;
};

// ============================================================================
// Masking Engine
// ============================================================================
class MaskingEngine {
public:
    using MaskFunction = std::function<std::string(const std::string& value,
                                                    const MaskingRule::Parameters& params)>;
    
    // Register custom masking functions
    void RegisterMaskFunction(MaskingMethod method, MaskFunction func);
    
    // Apply masking to a value
    std::string Mask(const std::string& value, const MaskingRule& rule);
    
    // Batch masking
    std::vector<std::string> MaskBatch(const std::vector<std::string>& values,
                                        const MaskingRule& rule);
    
    // Built-in masking implementations
    static std::string Redact(const std::string& value,
                               const MaskingRule::Parameters& params);
    static std::string PartialMask(const std::string& value,
                                    const MaskingRule::Parameters& params);
    static std::string RegexReplace(const std::string& value,
                                     const MaskingRule::Parameters& params);
    static std::string Hash(const std::string& value,
                            const MaskingRule::Parameters& params);
    static std::string Encrypt(const std::string& value,
                                const MaskingRule::Parameters& params);
    static std::string Randomize(const std::string& value,
                                  const MaskingRule::Parameters& params);
    static std::string Substitute(const std::string& value,
                                   const MaskingRule::Parameters& params);
    static std::string ShiftDate(const std::string& value,
                                  const MaskingRule::Parameters& params);
    static std::string AddNoise(const std::string& value,
                                 const MaskingRule::Parameters& params);
    static std::string Truncate(const std::string& value,
                                 const MaskingRule::Parameters& params);
    static std::vector<std::string> Shuffle(std::vector<std::string> values,
                                             const MaskingRule::Parameters& params);
    
private:
    std::map<MaskingMethod, MaskFunction> mask_functions_;
};

// ============================================================================
// Data Classification Engine
// ============================================================================
class ClassificationEngine {
public:
    struct ClassificationResult {
        DataClassification classification;
        double confidence;  // 0.0 - 1.0
        std::string detected_type;  // "email", "ssn", "phone", etc.
        std::string explanation;
    };
    
    // Auto-classify a column based on name and sample data
    ClassificationResult Classify(const std::string& column_name,
                                   const std::vector<std::string>& sample_values);
    
    // Classify based on column name only (heuristic)
    ClassificationResult ClassifyByName(const std::string& column_name);
    
    // Classify based on data patterns
    ClassificationResult ClassifyByData(const std::vector<std::string>& values);
    
    // Pattern matching
    static bool IsEmail(const std::string& value);
    static bool IsSSN(const std::string& value);
    static bool IsCreditCard(const std::string& value);
    static bool IsPhoneNumber(const std::string& value);
    static bool IsIPAddress(const std::string& value);
    static bool IsURL(const std::string& value);
    static bool IsDateOfBirth(const std::string& value);
    
private:
    // Pattern definitions
    struct Pattern {
        std::string name;
        std::string regex;
        DataClassification classification;
        double confidence;
    };
    
    std::vector<Pattern> patterns_;
    
    void InitializePatterns();
};

// ============================================================================
// Masking Job
// ============================================================================
struct MaskingJob {
    std::string id;
    std::string name;
    std::string description;
    
    // Source and target
    std::string source_connection_string;
    std::string target_connection_string;
    
    // Profile to apply
    std::string profile_id;
    
    // Scope
    std::vector<std::string> schemas;
    std::vector<std::string> tables;  // Empty = all tables
    std::vector<std::string> exclude_tables;
    
    // Options
    bool truncate_target = false;
    bool dry_run = false;
    int batch_size = 1000;
    int parallel_workers = 1;
    
    // Status
    enum class Status { PENDING, RUNNING, COMPLETED, FAILED, CANCELLED };
    Status status = Status::PENDING;
    
    // Progress
    int64_t total_rows = 0;
    int64_t processed_rows = 0;
    int64_t masked_values = 0;
    
    // Timing
    std::time_t started_at;
    std::time_t completed_at;
    
    // Results
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    double GetProgressPercent() const;
    std::string GetStatusString() const;
};

// ============================================================================
// Masking Executor
// ============================================================================
class MaskingExecutor {
public:
    using ProgressCallback = std::function<void(const MaskingJob& job)>;
    using LogCallback = std::function<void(const std::string& message)>;
    
    // Execute a masking job
    bool Execute(MaskingJob& job, 
                 MaskingProfile& profile,
                 MaskingEngine& engine,
                 ProgressCallback progress = nullptr,
                 LogCallback log = nullptr);
    
    // Cancel running job
    void Cancel(const std::string& job_id);
    
    // Validate job before execution
    bool Validate(const MaskingJob& job, std::vector<std::string>& errors);
    
private:
    std::map<std::string, bool> cancel_flags_;
    
    bool ShouldCancel(const std::string& job_id);
    bool CopyTable(const std::string& source_table,
                   const std::string& target_table,
                   const std::vector<MaskingRule*>& rules,
                   MaskingEngine& engine,
                   MaskingJob& job,
                   LogCallback log);
};

// ============================================================================
// Masking Manager (Main API)
// ============================================================================
class MaskingManager {
public:
    static MaskingManager& Instance();
    
    // Profile management
    void AddProfile(std::unique_ptr<MaskingProfile> profile);
    void RemoveProfile(const std::string& profile_id);
    MaskingProfile* GetProfile(const std::string& profile_id);
    std::vector<MaskingProfile*> GetAllProfiles();
    
    // Rule management
    MaskingRule* GetRule(const std::string& rule_id);
    std::vector<MaskingRule*> GetRulesForColumn(const std::string& schema,
                                                 const std::string& table,
                                                 const std::string& column);
    
    // Classification
    ClassificationEngine::ClassificationResult ClassifyColumn(
        const std::string& column_name,
        const std::vector<std::string>& sample_values);
    
    // Masking
    std::string MaskValue(const std::string& value, const MaskingRule& rule);
    
    // Job management
    std::string SubmitJob(const MaskingJob& job);
    MaskingJob* GetJob(const std::string& job_id);
    std::vector<MaskingJob*> GetJobs();
    void CancelJob(const std::string& job_id);
    
    // Auto-discovery
    std::vector<MaskingRule> DiscoverSensitiveColumns(
        const std::string& connection_string,
        const std::vector<std::string>& schemas);
    
    // Templates
    std::unique_ptr<MaskingProfile> CreateDevProfile();
    std::unique_ptr<MaskingProfile> CreateTestProfile();
    std::unique_ptr<MaskingProfile> CreateComplianceProfile();
    
private:
    MaskingManager();
    ~MaskingManager();
    
    std::map<std::string, std::unique_ptr<MaskingProfile>> profiles_;
    std::map<std::string, std::unique_ptr<MaskingJob>> jobs_;
    
    MaskingEngine engine_;
    ClassificationEngine classifier_;
    MaskingExecutor executor_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATA_MASKING_H
