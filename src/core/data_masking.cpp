/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "data_masking.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// Masking Method Helpers
// ============================================================================
std::string MaskingMethodToString(MaskingMethod method) {
    switch (method) {
        case MaskingMethod::NONE: return "NONE";
        case MaskingMethod::REDACTION: return "REDACTION";
        case MaskingMethod::PARTIAL: return "PARTIAL";
        case MaskingMethod::REGEX: return "REGEX";
        case MaskingMethod::HASH: return "HASH";
        case MaskingMethod::ENCRYPTION: return "ENCRYPTION";
        case MaskingMethod::RANDOMIZATION: return "RANDOMIZATION";
        case MaskingMethod::SHUFFLING: return "SHUFFLING";
        case MaskingMethod::NULLIFICATION: return "NULLIFICATION";
        case MaskingMethod::TRUNCATION: return "TRUNCATION";
        case MaskingMethod::FORMAT_PRESERVING: return "FORMAT_PRESERVING";
        case MaskingMethod::SUBSTITUTION: return "SUBSTITUTION";
        case MaskingMethod::DATE_SHIFTING: return "DATE_SHIFTING";
        case MaskingMethod::NOISE_ADDITION: return "NOISE_ADDITION";
        default: return "UNKNOWN";
    }
}

std::string MaskingMethodDescription(MaskingMethod method) {
    switch (method) {
        case MaskingMethod::REDACTION:
            return "Replace with fixed string (e.g., ***)";
        case MaskingMethod::PARTIAL:
            return "Partial masking (e.g., ***@domain.com)";
        case MaskingMethod::HASH:
            return "Cryptographic hash (irreversible)";
        case MaskingMethod::ENCRYPTION:
            return "Format-preserving encryption (reversible)";
        case MaskingMethod::SUBSTITUTION:
            return "Substitute with fake/generated data";
        case MaskingMethod::DATE_SHIFTING:
            return "Shift dates by random offset";
        default:
            return "";
    }
}

std::string ClassificationToString(DataClassification classification) {
    switch (classification) {
        case DataClassification::UNCLASSIFIED: return "UNCLASSIFIED";
        case DataClassification::PUBLIC: return "PUBLIC";
        case DataClassification::INTERNAL: return "INTERNAL";
        case DataClassification::CONFIDENTIAL: return "CONFIDENTIAL";
        case DataClassification::RESTRICTED: return "RESTRICTED";
        case DataClassification::PII: return "PII";
        case DataClassification::PHI: return "PHI";
        case DataClassification::PCI: return "PCI";
        case DataClassification::GDPR_SENSITIVE: return "GDPR_SENSITIVE";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Masking Rule Implementation
// ============================================================================
MaskingRule::MaskingRule() : created_at(std::time(nullptr)), 
                              modified_at(std::time(nullptr)) {}

std::string MaskingRule::GetFullColumnName() const {
    if (!schema.empty()) {
        return schema + "." + table + "." + column;
    }
    return table + "." + column;
}

bool MaskingRule::AppliesToEnvironment(const std::string& env) const {
    std::string env_lower = env;
    std::transform(env_lower.begin(), env_lower.end(), env_lower.begin(), ::tolower);
    
    if (env_lower.find("dev") != std::string::npos) return apply_to_dev;
    if (env_lower.find("test") != std::string::npos) return apply_to_test;
    if (env_lower.find("staging") != std::string::npos) return apply_to_staging;
    if (env_lower.find("prod") != std::string::npos) return apply_to_prod;
    
    return false;
}

// ============================================================================
// Masking Profile Implementation
// ============================================================================
MaskingProfile::MaskingProfile() = default;

MaskingProfile::MaskingProfile(const std::string& name_) : name(name_) {}

void MaskingProfile::AddRule(std::unique_ptr<MaskingRule> rule) {
    if (rule) {
        rules.push_back(std::move(rule));
    }
}

void MaskingProfile::RemoveRule(const std::string& rule_id) {
    rules.erase(
        std::remove_if(rules.begin(), rules.end(),
            [&rule_id](const auto& rule) { return rule->id == rule_id; }),
        rules.end());
}

MaskingRule* MaskingProfile::FindRule(const std::string& rule_id) {
    for (auto& rule : rules) {
        if (rule->id == rule_id) {
            return rule.get();
        }
    }
    return nullptr;
}

MaskingRule* MaskingProfile::FindRuleForColumn(const std::string& schema,
                                                const std::string& table,
                                                const std::string& column) {
    for (auto& rule : rules) {
        if (rule->schema == schema && rule->table == table && rule->column == column) {
            return rule.get();
        }
    }
    return nullptr;
}

std::vector<MaskingRule*> MaskingProfile::GetRulesForTable(const std::string& schema,
                                                            const std::string& table) {
    std::vector<MaskingRule*> result;
    for (auto& rule : rules) {
        if (rule->schema == schema && rule->table == table && rule->enabled) {
            result.push_back(rule.get());
        }
    }
    return result;
}

void MaskingProfile::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    // Simple JSON export
    file << "{\n";
    file << "  \"id\": \"" << id << "\",\n";
    file << "  \"name\": \"" << name << "\",\n";
    file << "  \"target_environment\": \"" << target_environment << "\",\n";
    file << "  \"rules\": [\n";
    
    bool first = true;
    for (const auto& rule : rules) {
        if (!first) file << ",\n";
        first = false;
        file << "    {\"id\": \"" << rule->id << "\", ";
        file << "\"table\": \"" << rule->table << "\", ";
        file << "\"column\": \"" << rule->column << "\", ";
        file << "\"method\": \"" << MaskingMethodToString(rule->method) << "\"}";
    }
    
    file << "\n  ]\n";
    file << "}\n";
}

std::unique_ptr<MaskingProfile> MaskingProfile::LoadFromFile(const std::string& path) {
    auto profile = std::make_unique<MaskingProfile>();
    // Stub implementation
    return profile;
}

bool MaskingProfile::Validate(std::vector<std::string>& errors) const {
    bool valid = true;
    
    if (name.empty()) {
        errors.push_back("Profile name is required");
        valid = false;
    }
    
    for (const auto& rule : rules) {
        if (rule->table.empty() || rule->column.empty()) {
            errors.push_back("Rule " + rule->id + " is missing table or column");
            valid = false;
        }
    }
    
    return valid;
}

// ============================================================================
// Masking Engine Implementation
// ============================================================================
void MaskingEngine::RegisterMaskFunction(MaskingMethod method, MaskFunction func) {
    mask_functions_[method] = func;
}

std::string MaskingEngine::Mask(const std::string& value, const MaskingRule& rule) {
    if (rule.method == MaskingMethod::NONE) {
        return value;
    }
    
    // Check if custom function is registered
    auto it = mask_functions_.find(rule.method);
    if (it != mask_functions_.end()) {
        return it->second(value, rule.parameters);
    }
    
    // Use built-in implementation
    switch (rule.method) {
        case MaskingMethod::REDACTION:
            return Redact(value, rule.parameters);
        case MaskingMethod::PARTIAL:
            return PartialMask(value, rule.parameters);
        case MaskingMethod::REGEX:
            return RegexReplace(value, rule.parameters);
        case MaskingMethod::HASH:
            return Hash(value, rule.parameters);
        case MaskingMethod::ENCRYPTION:
            return Encrypt(value, rule.parameters);
        case MaskingMethod::RANDOMIZATION:
            return Randomize(value, rule.parameters);
        case MaskingMethod::SUBSTITUTION:
            return Substitute(value, rule.parameters);
        case MaskingMethod::DATE_SHIFTING:
            return ShiftDate(value, rule.parameters);
        case MaskingMethod::NOISE_ADDITION:
            return AddNoise(value, rule.parameters);
        case MaskingMethod::TRUNCATION:
            return Truncate(value, rule.parameters);
        case MaskingMethod::NULLIFICATION:
            return "";
        default:
            return value;
    }
}

std::vector<std::string> MaskingEngine::MaskBatch(const std::vector<std::string>& values,
                                                   const MaskingRule& rule) {
    std::vector<std::string> result;
    result.reserve(values.size());
    
    for (const auto& value : values) {
        result.push_back(Mask(value, rule));
    }
    
    return result;
}

std::string MaskingEngine::Redact(const std::string& value,
                                   const MaskingRule::Parameters& params) {
    return params.replacement_string;
}

std::string MaskingEngine::PartialMask(const std::string& value,
                                        const MaskingRule::Parameters& params) {
    if (value.length() <= (size_t)(params.visible_chars_start + params.visible_chars_end)) {
        return std::string(value.length(), params.mask_char[0]);
    }
    
    std::string result = value.substr(0, params.visible_chars_start);
    
    int mask_length = value.length() - params.visible_chars_start - params.visible_chars_end;
    result += std::string(mask_length, params.mask_char[0]);
    
    if (params.visible_chars_end > 0) {
        result += value.substr(value.length() - params.visible_chars_end);
    }
    
    return result;
}

std::string MaskingEngine::RegexReplace(const std::string& value,
                                         const MaskingRule::Parameters& params) {
    try {
        std::regex pattern(params.regex_pattern);
        return std::regex_replace(value, pattern, params.regex_replacement);
    } catch (...) {
        return value;
    }
}

std::string MaskingEngine::Hash(const std::string& value,
                                 const MaskingRule::Parameters& params) {
    // Simplified hash - in production would use proper crypto library
    std::string salted = value + params.hash_salt;
    
    // Simple string hash for demonstration
    std::hash<std::string> hasher;
    size_t hash = hasher(salted);
    
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    
    if (params.hash_algorithm == "SHA256") {
        return "sha256:" + ss.str();
    }
    return ss.str();
}

std::string MaskingEngine::Encrypt(const std::string& value,
                                    const MaskingRule::Parameters& params) {
    // Stub - would use proper FPE encryption
    return "ENC(" + value + ")";
}

std::string MaskingEngine::Randomize(const std::string& value,
                                      const MaskingRule::Parameters& params) {
    // Generate random string of same length
    std::string result;
    result.reserve(value.length());
    
    std::random_device rd;
    std::mt19937 gen(params.randomization_seed ? params.randomization_seed : rd());
    std::uniform_int_distribution<> dis(32, 126);  // Printable ASCII
    
    for (size_t i = 0; i < value.length(); ++i) {
        result.push_back(static_cast<char>(dis(gen)));
    }
    
    return result;
}

std::string MaskingEngine::Substitute(const std::string& value,
                                       const MaskingRule::Parameters& params) {
    // Return fake data based on generator type
    if (params.fake_data_generator == "email") {
        return "user" + std::to_string(std::hash<std::string>{}(value) % 10000) + "@example.com";
    } else if (params.fake_data_generator == "name") {
        return "Person " + std::to_string(std::hash<std::string>{}(value) % 10000);
    } else if (params.fake_data_generator == "phone") {
        return "555-" + std::to_string(1000 + (std::hash<std::string>{}(value) % 9000));
    } else if (params.fake_data_generator == "ssn") {
        int hash = std::hash<std::string>{}(value);
        return std::to_string(100 + (hash % 900)) + "-" +
               std::to_string(10 + (hash % 90)) + "-" +
               std::to_string(1000 + (hash % 9000));
    }
    
    return "[MASKED]";
}

std::string MaskingEngine::ShiftDate(const std::string& value,
                                      const MaskingRule::Parameters& params) {
    // Stub - would parse date and apply offset
    return value + "[SHIFTED]";
}

std::string MaskingEngine::AddNoise(const std::string& value,
                                     const MaskingRule::Parameters& params) {
    try {
        double num = std::stod(value);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-params.noise_percentage, params.noise_percentage);
        
        double noise = num * dis(gen);
        double result = num + noise;
        
        return std::to_string(result);
    } catch (...) {
        return value;
    }
}

std::string MaskingEngine::Truncate(const std::string& value,
                                     const MaskingRule::Parameters& params) {
    if (params.max_length > 0 && (int)value.length() > params.max_length) {
        return value.substr(0, params.max_length);
    }
    return value;
}

// ============================================================================
// Classification Engine Implementation
// ============================================================================
ClassificationEngine::ClassificationResult ClassificationEngine::Classify(
    const std::string& column_name,
    const std::vector<std::string>& sample_values) {
    
    // Combine name-based and data-based classification
    auto name_result = ClassifyByName(column_name);
    auto data_result = ClassifyByData(sample_values);
    
    // Return the higher confidence result
    if (name_result.confidence > data_result.confidence) {
        return name_result;
    }
    return data_result;
}

ClassificationEngine::ClassificationResult ClassificationEngine::ClassifyByName(
    const std::string& column_name) {
    
    std::string name_lower = column_name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
    
    // Pattern matching on column name
    if (name_lower.find("email") != std::string::npos ||
        name_lower.find("e_mail") != std::string::npos) {
        return {DataClassification::PII, 0.9, "email", "Column name suggests email"};
    }
    
    if (name_lower.find("ssn") != std::string::npos ||
        name_lower.find("social") != std::string::npos) {
        return {DataClassification::PII, 0.95, "ssn", "Column name suggests SSN"};
    }
    
    if (name_lower.find("password") != std::string::npos ||
        name_lower.find("pwd") != std::string::npos) {
        return {DataClassification::RESTRICTED, 0.95, "password", "Column name suggests password"};
    }
    
    if (name_lower.find("credit") != std::string::npos ||
        name_lower.find("card") != std::string::npos ||
        name_lower.find("ccnum") != std::string::npos) {
        return {DataClassification::PCI, 0.9, "credit_card", "Column name suggests credit card"};
    }
    
    if (name_lower.find("phone") != std::string::npos ||
        name_lower.find("mobile") != std::string::npos ||
        name_lower.find("tel") != std::string::npos) {
        return {DataClassification::PII, 0.85, "phone", "Column name suggests phone number"};
    }
    
    if (name_lower.find("dob") != std::string::npos ||
        name_lower.find("birth") != std::string::npos) {
        return {DataClassification::PII, 0.85, "date_of_birth", "Column name suggests DOB"};
    }
    
    if (name_lower.find("address") != std::string::npos ||
        name_lower.find("street") != std::string::npos) {
        return {DataClassification::PII, 0.8, "address", "Column name suggests address"};
    }
    
    if (name_lower.find("name") != std::string::npos &&
        name_lower.find("username") == std::string::npos) {
        return {DataClassification::PII, 0.7, "name", "Column name suggests personal name"};
    }
    
    return {DataClassification::UNCLASSIFIED, 0.0, "", "No pattern match"};
}

ClassificationEngine::ClassificationResult ClassificationEngine::ClassifyByData(
    const std::vector<std::string>& values) {
    
    if (values.empty()) {
        return {DataClassification::UNCLASSIFIED, 0.0, "", "No sample data"};
    }
    
    int email_count = 0;
    int ssn_count = 0;
    int credit_card_count = 0;
    int phone_count = 0;
    
    for (const auto& value : values) {
        if (IsEmail(value)) email_count++;
        if (IsSSN(value)) ssn_count++;
        if (IsCreditCard(value)) credit_card_count++;
        if (IsPhoneNumber(value)) phone_count++;
    }
    
    int total = values.size();
    
    if (ssn_count > total * 0.5) {
        return {DataClassification::PII, 0.9, "ssn", "Data pattern matches SSN"};
    }
    
    if (credit_card_count > total * 0.5) {
        return {DataClassification::PCI, 0.9, "credit_card", "Data pattern matches credit card"};
    }
    
    if (email_count > total * 0.5) {
        return {DataClassification::PII, 0.85, "email", "Data pattern matches email"};
    }
    
    if (phone_count > total * 0.5) {
        return {DataClassification::PII, 0.8, "phone", "Data pattern matches phone"};
    }
    
    return {DataClassification::UNCLASSIFIED, 0.0, "", "No data pattern match"};
}

bool ClassificationEngine::IsEmail(const std::string& value) {
    std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(value, email_regex);
}

bool ClassificationEngine::IsSSN(const std::string& value) {
    // US SSN pattern: XXX-XX-XXXX or XXXXXXXXX
    std::regex ssn_regex(R"(\d{3}-?\d{2}-?\d{4})");
    return std::regex_match(value, ssn_regex);
}

bool ClassificationEngine::IsCreditCard(const std::string& value) {
    // Basic credit card pattern (simplified)
    std::regex cc_regex(R"(\d{4}[\s-]?\d{4}[\s-]?\d{4}[\s-]?\d{4})");
    return std::regex_match(value, cc_regex);
}

bool ClassificationEngine::IsPhoneNumber(const std::string& value) {
    // US phone pattern
    std::regex phone_regex(R"(\(?\d{3}\)?[\s.-]?\d{3}[\s.-]?\d{4})");
    return std::regex_match(value, phone_regex);
}

bool ClassificationEngine::IsIPAddress(const std::string& value) {
    std::regex ip_regex(R"((\d{1,3}\.){3}\d{1,3})");
    return std::regex_match(value, ip_regex);
}

bool ClassificationEngine::IsURL(const std::string& value) {
    std::regex url_regex(R"(https?://[\w\-\.]+[\w\-\.]*)");
    return std::regex_match(value, url_regex);
}

bool ClassificationEngine::IsDateOfBirth(const std::string& value) {
    // Various date formats
    std::regex dob_regex(R"((\d{1,2}[/-]\d{1,2}[/-]\d{2,4})|(\d{4}[/-]\d{1,2}[/-]\d{1,2}))");
    return std::regex_match(value, dob_regex);
}

// ============================================================================
// Masking Job Implementation
// ============================================================================
double MaskingJob::GetProgressPercent() const {
    if (total_rows == 0) return 0.0;
    return (double)processed_rows / total_rows * 100.0;
}

std::string MaskingJob::GetStatusString() const {
    switch (status) {
        case Status::PENDING: return "PENDING";
        case Status::RUNNING: return "RUNNING";
        case Status::COMPLETED: return "COMPLETED";
        case Status::FAILED: return "FAILED";
        case Status::CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Masking Manager Implementation
// ============================================================================
MaskingManager& MaskingManager::Instance() {
    static MaskingManager instance;
    return instance;
}

void MaskingManager::AddProfile(std::unique_ptr<MaskingProfile> profile) {
    if (profile) {
        profiles_[profile->id] = std::move(profile);
    }
}

void MaskingManager::RemoveProfile(const std::string& profile_id) {
    profiles_.erase(profile_id);
}

MaskingProfile* MaskingManager::GetProfile(const std::string& profile_id) {
    auto it = profiles_.find(profile_id);
    if (it != profiles_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<MaskingProfile*> MaskingManager::GetAllProfiles() {
    std::vector<MaskingProfile*> result;
    for (auto& [id, profile] : profiles_) {
        result.push_back(profile.get());
    }
    return result;
}

std::string MaskingManager::MaskValue(const std::string& value, const MaskingRule& rule) {
    return engine_.Mask(value, rule);
}

std::unique_ptr<MaskingProfile> MaskingManager::CreateDevProfile() {
    auto profile = std::make_unique<MaskingProfile>("Development Masking");
    profile->target_environment = "development";
    profile->auto_detect_pii = true;
    return profile;
}

std::unique_ptr<MaskingProfile> MaskingManager::CreateTestProfile() {
    auto profile = std::make_unique<MaskingProfile>("Testing Masking");
    profile->target_environment = "testing";
    profile->auto_detect_pii = true;
    profile->auto_detect_pci = true;
    return profile;
}

std::unique_ptr<MaskingProfile> MaskingManager::CreateComplianceProfile() {
    auto profile = std::make_unique<MaskingProfile>("Compliance Masking");
    profile->target_environment = "compliance_testing";
    profile->auto_detect_pii = true;
    profile->auto_detect_pci = true;
    profile->auto_detect_phi = true;
    return profile;
}

} // namespace scratchrobin
