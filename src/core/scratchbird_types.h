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

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace scratchrobin {
namespace core {

// ============================================================================
// Type Definitions
// ============================================================================

using Oid = uint32_t;

// Standard Oid constants
extern const Oid kOidInvalid;
extern const Oid kOidBoolean;
extern const Oid kOidBytea;
extern const Oid kOidChar;
extern const Oid kOidName;
extern const Oid kOidInt8;
extern const Oid kOidInt2;
extern const Oid kOidInt2Vector;
extern const Oid kOidInt4;
extern const Oid kOidRegproc;
extern const Oid kOidText;
extern const Oid kOidOid;
extern const Oid kOidTid;
extern const Oid kOidXid;
extern const Oid kOidCid;
extern const Oid kOidOidVector;
extern const Oid kOidJson;
extern const Oid kOidXml;
extern const Oid kOidPgNodeTree;
extern const Oid kOidFloat4;
extern const Oid kOidFloat8;
extern const Oid kOidAbstime;
extern const Oid kOidReltime;
extern const Oid kOidTinterval;
extern const Oid kOidUnknown;
extern const Oid kOidPolygon;
extern const Oid kOidOidArray;
extern const Oid kOidPoint;
extern const Oid kOidLseg;
extern const Oid kOidPath;
extern const Oid kOidBox;
extern const Oid kOidLine;
extern const Oid kOidLineArray;
extern const Oid kOidFloat4Array;
extern const Oid kOidFloat8Array;
extern const Oid kOidAbstimeArray;
extern const Oid kOidReltimeArray;
extern const Oid kOidTintervalArray;
extern const Oid kOidUnknownArray;
extern const Oid kOidPolygonArray;
extern const Oid kOidAclItem;
extern const Oid kOidAclItemArray;
extern const Oid kOidMacAddr;
extern const Oid kOidInet;
extern const Oid kOidCidr;
extern const Oid kOidMacAddrArray;
extern const Oid kOidInetArray;
extern const Oid kOidBpchar;
extern const Oid kOidVarchar;
extern const Oid kOidDate;
extern const Oid kOidTime;
extern const Oid kOidTimestamp;
extern const Oid kOidTimestamptz;
extern const Oid kOidInterval;
extern const Oid kOidTimetz;
extern const Oid kOidBit;
extern const Oid kOidVarbit;
extern const Oid kOidNumeric;
extern const Oid kOidRefcursor;
extern const Oid kOidRegprocedure;
extern const Oid kOidRegoper;
extern const Oid kOidRegoperator;
extern const Oid kOidRegclass;
extern const Oid kOidRegtype;
extern const Oid kOidRecord;
extern const Oid kOidCstring;
extern const Oid kOidAny;
extern const Oid kOidAnyArray;
extern const Oid kOidVoid;
extern const Oid kOidTrigger;
extern const Oid kOidLanguageHandler;
extern const Oid kOidInternal;
extern const Oid kOidOpaque;
extern const Oid kOidAnyElement;
extern const Oid kOidRecordArray;
extern const Oid kOidAnynonarray;
extern const Oid kOidUuid;
extern const Oid kOidTxidSnapshot;
extern const Oid kOidUuidArray;
extern const Oid kOidTxidSnapshotArray;
extern const Oid kOidFdwHandler;
extern const Oid kOidJsonb;
extern const Oid kOidAnyrange;
extern const Oid kOidInt4Range;
extern const Oid kOidNumRange;
extern const Oid kOidTsRange;
extern const Oid kOidTstzRange;
extern const Oid kOidDateRange;
extern const Oid kOidInt8Range;
extern const Oid kOidInt4RangeArray;
extern const Oid kOidNumRangeArray;
extern const Oid kOidTsRangeArray;
extern const Oid kOidTstzRangeArray;
extern const Oid kOidDateRangeArray;
extern const Oid kOidInt8RangeArray;

// ============================================================================
// Type Categories
// ============================================================================

enum class TypeCategory {
  kInvalid,
  kBoolean,
  kInteger,
  kFloatingPoint,
  kNumeric,
  kString,
  kBinary,
  kDate,
  kTime,
  kTimestamp,
  kInterval,
  kOid,
  kUuid,
  kComplex,
  kNetwork,
  kBitString,
  kGeometric,
  kArray,
  kRange,
  kComposite,
  kEnum,
  kPseudo,
  kUserDefined
};

// Get category for an Oid
TypeCategory GetTypeCategory(Oid oid);

// Convert category to string
std::string TypeCategoryToString(TypeCategory category);

// ============================================================================
// TypeInfo
// ============================================================================

struct TypeInfo {
  Oid oid = kOidInvalid;
  std::string name;
  int32_t size = 0;        // Positive = fixed length, negative = variable
  bool by_val = false;     // Pass by value
  char align = 'i';        // Alignment requirement
  char storage = 'p';      // Storage type
  
  TypeInfo();
  TypeInfo(Oid oid_val, const std::string& name_val, int32_t size_val,
           bool by_val_val, char align_val, char storage_val);
  
  bool IsFixedLength() const;
  bool IsVariableLength() const;
  TypeCategory GetCategory() const;
  bool IsArray() const;
  bool IsNumeric() const;
  bool IsTextual() const;
  bool IsTemporal() const;
};

// ============================================================================
// TypeRegistry
// ============================================================================

class TypeRegistry {
 public:
  static TypeRegistry& Instance();
  
  void RegisterType(const TypeInfo& type_info);
  std::optional<TypeInfo> GetTypeByOid(Oid oid) const;
  std::optional<TypeInfo> GetTypeByName(const std::string& name) const;
  Oid GetOidByName(const std::string& name) const;
  bool IsRegistered(Oid oid) const;
  
  std::vector<TypeInfo> GetAllTypes() const;
  std::vector<TypeInfo> GetTypesByCategory(TypeCategory category) const;
  
  void Clear();
  size_t Size() const;
  
 private:
  TypeRegistry();
  ~TypeRegistry();
  
  void InitializeBuiltinTypes();
  
  std::unordered_map<Oid, TypeInfo> types_by_oid_;
  std::unordered_map<std::string, Oid> types_by_name_;
};

// ============================================================================
// TypeConversion
// ============================================================================

class TypeConversion {
 public:
  TypeConversion();
  ~TypeConversion();
  
  bool CanConvert(Oid from_oid, Oid to_oid) const;
  
  std::optional<Oid> GetPreferredType(TypeCategory category) const;
  
 private:
  struct ConversionKey {
    Oid from;
    Oid to;
    
    bool operator==(const ConversionKey& other) const {
      return from == other.from && to == other.to;
    }
  };
  
  struct ConversionKeyHash {
    std::size_t operator()(const ConversionKey& key) const {
      return std::hash<Oid>()(key.from) ^ (std::hash<Oid>()(key.to) << 1);
    }
  };
  
  std::unordered_map<ConversionKey, bool, ConversionKeyHash> conversions_;
};

// ============================================================================
// TypeFormatter
// ============================================================================

class TypeFormatter {
 public:
  static std::string FormatTypeName(Oid oid, int32_t typemod = -1);
  static std::string FormatTypeName(const std::string& name, int32_t typemod = -1);
  static std::string FormatArrayTypeName(Oid element_oid);
  static std::string FormatRangeTypeName(Oid subtype_oid);
};

// ============================================================================
// TypeCompatibility
// ============================================================================

class TypeCompatibility {
 public:
  static bool AreCompatible(Oid type1, Oid type2);
  static Oid ChooseCommonType(Oid type1, Oid type2);
  static bool IsCoercible(Oid from_type, Oid to_type);
};

}  // namespace core
}  // namespace scratchrobin
