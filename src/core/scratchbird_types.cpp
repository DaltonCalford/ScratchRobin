/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "scratchbird_types.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace scratchrobin {
namespace core {

namespace {

// Type name normalization for comparison
std::string NormalizeTypeName(const std::string& name) {
  std::string result;
  result.reserve(name.length());
  for (char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
      result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
  }
  return result;
}

}  // anonymous namespace

// ============================================================================
// Oid Type Definitions
// ============================================================================

const Oid kOidInvalid = 0;
const Oid kOidBoolean = 16;
const Oid kOidBytea = 17;
const Oid kOidChar = 18;
const Oid kOidName = 19;
const Oid kOidInt8 = 20;
const Oid kOidInt2 = 21;
const Oid kOidInt2Vector = 22;
const Oid kOidInt4 = 23;
const Oid kOidRegproc = 24;
const Oid kOidText = 25;
const Oid kOidOid = 26;
const Oid kOidTid = 27;
const Oid kOidXid = 28;
const Oid kOidCid = 29;
const Oid kOidOidVector = 30;
const Oid kOidJson = 114;
const Oid kOidXml = 142;
const Oid kOidPgNodeTree = 194;
const Oid kOidFloat4 = 700;
const Oid kOidFloat8 = 701;
const Oid kOidAbstime = 702;
const Oid kOidReltime = 703;
const Oid kOidTinterval = 704;
const Oid kOidUnknown = 705;
const Oid kOidPolygon = 604;
const Oid kOidOidArray = 1028;
const Oid kOidPoint = 600;
const Oid kOidLseg = 601;
const Oid kOidPath = 602;
const Oid kOidBox = 603;
const Oid kOidLine = 628;
const Oid kOidLineArray = 629;
const Oid kOidFloat4Array = 1021;
const Oid kOidFloat8Array = 1022;
const Oid kOidAbstimeArray = 1023;
const Oid kOidReltimeArray = 1024;
const Oid kOidTintervalArray = 1025;
const Oid kOidUnknownArray = 1026;
const Oid kOidPolygonArray = 1027;
const Oid kOidAclItem = 1033;
const Oid kOidAclItemArray = 1034;
const Oid kOidMacAddr = 829;
const Oid kOidInet = 869;
const Oid kOidCidr = 650;
const Oid kOidMacAddrArray = 1040;
const Oid kOidInetArray = 1041;
const Oid kOidBpchar = 1042;
const Oid kOidVarchar = 1043;
const Oid kOidDate = 1082;
const Oid kOidTime = 1083;
const Oid kOidTimestamp = 1114;
const Oid kOidTimestamptz = 1184;
const Oid kOidInterval = 1186;
const Oid kOidTimetz = 1266;
const Oid kOidBit = 1560;
const Oid kOidVarbit = 1562;
const Oid kOidNumeric = 1700;
const Oid kOidRefcursor = 1790;
const Oid kOidRegprocedure = 2202;
const Oid kOidRegoper = 2203;
const Oid kOidRegoperator = 2204;
const Oid kOidRegclass = 2205;
const Oid kOidRegtype = 2206;
const Oid kOidRecord = 2249;
const Oid kOidCstring = 2275;
const Oid kOidAny = 2276;
const Oid kOidAnyArray = 2277;
const Oid kOidVoid = 2278;
const Oid kOidTrigger = 2279;
const Oid kOidLanguageHandler = 2280;
const Oid kOidInternal = 2281;
const Oid kOidOpaque = 2282;
const Oid kOidAnyelement = 2283;
const Oid kOidRecordArray = 2287;
const Oid kOidAnynonarray = 2776;
const Oid kOidUuid = 2950;
const Oid kOidTxidSnapshot = 2970;
const Oid kOidUuidArray = 2951;
const Oid kOidTxidSnapshotArray = 2949;
const Oid kOidFdwHandler = 3115;
const Oid kOidJsonb = 3802;
const Oid kOidAnyrange = 3831;
const Oid kOidInt4Range = 3904;
const Oid kOidNumRange = 3906;
const Oid kOidTsRange = 3908;
const Oid kOidTstzRange = 3910;
const Oid kOidDateRange = 3912;
const Oid kOidInt8Range = 3926;
const Oid kOidInt4RangeArray = 3905;
const Oid kOidNumRangeArray = 3907;
const Oid kOidTsRangeArray = 3909;
const Oid kOidTstzRangeArray = 3911;
const Oid kOidDateRangeArray = 3913;
const Oid kOidInt8RangeArray = 3927;
const Oid kOidAnyElement = 2283;  // PostgreSQL pg_type OID for anyelement

// ============================================================================
// TypeCategory Implementation
// ============================================================================

TypeCategory GetTypeCategory(Oid oid) {
  switch (oid) {
    case kOidBoolean:
      return TypeCategory::kBoolean;
    case kOidInt2:
    case kOidInt4:
    case kOidInt8:
      return TypeCategory::kInteger;
    case kOidFloat4:
    case kOidFloat8:
      return TypeCategory::kFloatingPoint;
    case kOidNumeric:
      return TypeCategory::kNumeric;
    case kOidChar:
    case kOidVarchar:
    case kOidBpchar:
    case kOidText:
    case kOidName:
      return TypeCategory::kString;
    case kOidBytea:
      return TypeCategory::kBinary;
    case kOidDate:
      return TypeCategory::kDate;
    case kOidTime:
    case kOidTimetz:
      return TypeCategory::kTime;
    case kOidTimestamp:
    case kOidTimestamptz:
      return TypeCategory::kTimestamp;
    case kOidInterval:
      return TypeCategory::kInterval;
    case kOidOid:
    case kOidRegproc:
    case kOidRegprocedure:
    case kOidRegoper:
    case kOidRegoperator:
    case kOidRegclass:
    case kOidRegtype:
      return TypeCategory::kOid;
    case kOidUuid:
      return TypeCategory::kUuid;
    case kOidXml:
    case kOidJson:
    case kOidJsonb:
      return TypeCategory::kComplex;
    case kOidInet:
    case kOidCidr:
      return TypeCategory::kNetwork;
    case kOidBit:
    case kOidVarbit:
      return TypeCategory::kBitString;
    case kOidPoint:
    case kOidLine:
    case kOidLseg:
    case kOidBox:
    case kOidPath:
    case kOidPolygon:
      return TypeCategory::kGeometric;
    default:
      if (oid >= kOidInt4Range && oid <= kOidInt8RangeArray) {
        return TypeCategory::kRange;
      }
      if (oid >= kOidOidArray && oid <= kOidOidVector) {
        return TypeCategory::kArray;
      }
      return TypeCategory::kUserDefined;
  }
}

std::string TypeCategoryToString(TypeCategory category) {
  switch (category) {
    case TypeCategory::kBoolean:
      return "BOOLEAN";
    case TypeCategory::kInteger:
      return "INTEGER";
    case TypeCategory::kFloatingPoint:
      return "FLOATING_POINT";
    case TypeCategory::kNumeric:
      return "NUMERIC";
    case TypeCategory::kString:
      return "STRING";
    case TypeCategory::kBinary:
      return "BINARY";
    case TypeCategory::kDate:
      return "DATE";
    case TypeCategory::kTime:
      return "TIME";
    case TypeCategory::kTimestamp:
      return "TIMESTAMP";
    case TypeCategory::kInterval:
      return "INTERVAL";
    case TypeCategory::kOid:
      return "OID";
    case TypeCategory::kUuid:
      return "UUID";
    case TypeCategory::kComplex:
      return "COMPLEX";
    case TypeCategory::kNetwork:
      return "NETWORK";
    case TypeCategory::kBitString:
      return "BIT_STRING";
    case TypeCategory::kGeometric:
      return "GEOMETRIC";
    case TypeCategory::kArray:
      return "ARRAY";
    case TypeCategory::kRange:
      return "RANGE";
    case TypeCategory::kComposite:
      return "COMPOSITE";
    case TypeCategory::kEnum:
      return "ENUM";
    case TypeCategory::kPseudo:
      return "PSEUDO";
    case TypeCategory::kUserDefined:
      return "USER_DEFINED";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// TypeInfo Implementation
// ============================================================================

TypeInfo::TypeInfo() = default;

TypeInfo::TypeInfo(Oid oid_val, const std::string& name_val, int32_t size_val,
                   bool by_val_val, char align_val, char storage_val)
    : oid(oid_val),
      name(name_val),
      size(size_val),
      by_val(by_val_val),
      align(align_val),
      storage(storage_val) {}

bool TypeInfo::IsFixedLength() const {
  return size > 0;
}

bool TypeInfo::IsVariableLength() const {
  return size < 0;
}

TypeCategory TypeInfo::GetCategory() const {
  return GetTypeCategory(oid);
}

bool TypeInfo::IsArray() const {
  return GetCategory() == TypeCategory::kArray;
}

bool TypeInfo::IsNumeric() const {
  auto cat = GetCategory();
  return cat == TypeCategory::kInteger || 
         cat == TypeCategory::kFloatingPoint || 
         cat == TypeCategory::kNumeric;
}

bool TypeInfo::IsTextual() const {
  return GetCategory() == TypeCategory::kString;
}

bool TypeInfo::IsTemporal() const {
  auto cat = GetCategory();
  return cat == TypeCategory::kDate || 
         cat == TypeCategory::kTime || 
         cat == TypeCategory::kTimestamp || 
         cat == TypeCategory::kInterval;
}

// ============================================================================
// TypeRegistry Implementation
// ============================================================================

TypeRegistry::TypeRegistry() {
  InitializeBuiltinTypes();
}

TypeRegistry::~TypeRegistry() = default;

TypeRegistry& TypeRegistry::Instance() {
  static TypeRegistry instance;
  return instance;
}

void TypeRegistry::InitializeBuiltinTypes() {
  // Boolean types
  RegisterType(TypeInfo(kOidBoolean, "bool", 1, true, 'c', 'p'));
  
  // Integer types
  RegisterType(TypeInfo(kOidInt2, "int2", 2, true, 's', 'p'));
  RegisterType(TypeInfo(kOidInt4, "int4", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidInt8, "int8", 8, true, 'd', 'p'));
  
  // Floating point types
  RegisterType(TypeInfo(kOidFloat4, "float4", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidFloat8, "float8", 8, true, 'd', 'p'));
  
  // Numeric type
  RegisterType(TypeInfo(kOidNumeric, "numeric", -1, false, 'i', 'm'));
  
  // Character types
  RegisterType(TypeInfo(kOidChar, "char", 1, true, 'c', 'p'));
  RegisterType(TypeInfo(kOidBpchar, "bpchar", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidVarchar, "varchar", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidText, "text", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidName, "name", 64, false, 'c', 'c'));
  
  // Binary types
  RegisterType(TypeInfo(kOidBytea, "bytea", -1, false, 'i', 'x'));
  
  // Temporal types
  RegisterType(TypeInfo(kOidDate, "date", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidTime, "time", 8, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidTimetz, "timetz", 12, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidTimestamp, "timestamp", 8, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidTimestamptz, "timestamptz", 8, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidInterval, "interval", 16, true, 'd', 'p'));
  
  // OID types
  RegisterType(TypeInfo(kOidOid, "oid", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidRegproc, "regproc", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidRegprocedure, "regprocedure", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidRegoper, "regoper", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidRegoperator, "regoperator", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidRegclass, "regclass", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidRegtype, "regtype", 4, true, 'i', 'p'));
  
  // UUID type
  RegisterType(TypeInfo(kOidUuid, "uuid", 16, true, 'c', 'p'));
  
  // Network types
  RegisterType(TypeInfo(kOidInet, "inet", -1, false, 'i', 'm'));
  RegisterType(TypeInfo(kOidCidr, "cidr", -1, false, 'i', 'm'));
  RegisterType(TypeInfo(kOidMacAddr, "macaddr", 6, true, 's', 'p'));
  
  // Bit string types
  RegisterType(TypeInfo(kOidBit, "bit", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidVarbit, "varbit", -1, false, 'i', 'x'));
  
  // Complex types
  RegisterType(TypeInfo(kOidJson, "json", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidJsonb, "jsonb", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidXml, "xml", -1, false, 'i', 'x'));
  
  // Geometric types
  RegisterType(TypeInfo(kOidPoint, "point", 16, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidLine, "line", 24, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidLseg, "lseg", 32, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidBox, "box", 32, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidPath, "path", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidPolygon, "polygon", -1, false, 'i', 'x'));
  
  // Range types
  RegisterType(TypeInfo(kOidInt4Range, "int4range", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidInt8Range, "int8range", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidNumRange, "numrange", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidTsRange, "tsrange", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidTstzRange, "tstzrange", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidDateRange, "daterange", -1, false, 'i', 'x'));
  
  // Pseudo types
  RegisterType(TypeInfo(kOidRecord, "record", -1, false, 'd', 'x'));
  RegisterType(TypeInfo(kOidVoid, "void", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidAny, "any", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidAnyArray, "anyarray", -1, false, 'd', 'x'));
  RegisterType(TypeInfo(kOidAnyElement, "anyelement", -1, false, 'd', 'x'));
  RegisterType(TypeInfo(kOidAnynonarray, "anynonarray", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidAnyrange, "anyrange", -1, false, 'i', 'x'));
  RegisterType(TypeInfo(kOidCstring, "cstring", -2, false, 'c', 'p'));
  RegisterType(TypeInfo(kOidInternal, "internal", 8, true, 'd', 'p'));
  RegisterType(TypeInfo(kOidLanguageHandler, "language_handler", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidFdwHandler, "fdw_handler", 4, true, 'i', 'p'));
  RegisterType(TypeInfo(kOidTrigger, "trigger", 4, true, 'i', 'p'));
}

void TypeRegistry::RegisterType(const TypeInfo& type_info) {
  types_by_oid_[type_info.oid] = type_info;
  types_by_name_[NormalizeTypeName(type_info.name)] = type_info.oid;
}

std::optional<TypeInfo> TypeRegistry::GetTypeByOid(Oid oid) const {
  auto it = types_by_oid_.find(oid);
  if (it != types_by_oid_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<TypeInfo> TypeRegistry::GetTypeByName(const std::string& name) const {
  auto it = types_by_name_.find(NormalizeTypeName(name));
  if (it != types_by_name_.end()) {
    return GetTypeByOid(it->second);
  }
  return std::nullopt;
}

Oid TypeRegistry::GetOidByName(const std::string& name) const {
  auto it = types_by_name_.find(NormalizeTypeName(name));
  if (it != types_by_name_.end()) {
    return it->second;
  }
  return kOidInvalid;
}

bool TypeRegistry::IsRegistered(Oid oid) const {
  return types_by_oid_.find(oid) != types_by_oid_.end();
}

std::vector<TypeInfo> TypeRegistry::GetAllTypes() const {
  std::vector<TypeInfo> result;
  result.reserve(types_by_oid_.size());
  for (const auto& [oid, type_info] : types_by_oid_) {
    result.push_back(type_info);
  }
  return result;
}

std::vector<TypeInfo> TypeRegistry::GetTypesByCategory(TypeCategory category) const {
  std::vector<TypeInfo> result;
  for (const auto& [oid, type_info] : types_by_oid_) {
    if (type_info.GetCategory() == category) {
      result.push_back(type_info);
    }
  }
  return result;
}

void TypeRegistry::Clear() {
  types_by_oid_.clear();
  types_by_name_.clear();
}

size_t TypeRegistry::Size() const {
  return types_by_oid_.size();
}

// ============================================================================
// TypeConversion Implementation
// ============================================================================

TypeConversion::TypeConversion() {
  // Initialize with default implicit conversions
  // Integer to integer
  conversions_[{kOidInt2, kOidInt4}] = true;
  conversions_[{kOidInt2, kOidInt8}] = true;
  conversions_[{kOidInt4, kOidInt8}] = true;
  
  // Integer to float
  conversions_[{kOidInt2, kOidFloat4}] = true;
  conversions_[{kOidInt2, kOidFloat8}] = true;
  conversions_[{kOidInt4, kOidFloat4}] = true;
  conversions_[{kOidInt4, kOidFloat8}] = true;
  conversions_[{kOidInt8, kOidFloat4}] = true;
  conversions_[{kOidInt8, kOidFloat8}] = true;
  
  // Float to float
  conversions_[{kOidFloat4, kOidFloat8}] = true;
  
  // String types
  conversions_[{kOidChar, kOidVarchar}] = true;
  conversions_[{kOidChar, kOidText}] = true;
  conversions_[{kOidVarchar, kOidText}] = true;
  
  // Date/time
  conversions_[{kOidDate, kOidTimestamp}] = true;
  conversions_[{kOidDate, kOidTimestamptz}] = true;
}

TypeConversion::~TypeConversion() = default;

bool TypeConversion::CanConvert(Oid from_oid, Oid to_oid) const {
  // Same type always convertible
  if (from_oid == to_oid) {
    return true;
  }
  
  // Check explicit conversion map
  ConversionKey key{from_oid, to_oid};
  auto it = conversions_.find(key);
  if (it != conversions_.end()) {
    return it->second;
  }
  
  // Category-based conversion
  auto from_type = TypeRegistry::Instance().GetTypeByOid(from_oid);
  auto to_type = TypeRegistry::Instance().GetTypeByOid(to_oid);
  
  if (!from_type || !to_type) {
    return false;
  }
  
  auto from_cat = from_type->GetCategory();
  auto to_cat = to_type->GetCategory();
  
  // Numeric types are generally convertible
  if (from_cat == TypeCategory::kInteger && to_cat == TypeCategory::kInteger) {
    return true;
  }
  if ((from_cat == TypeCategory::kInteger || from_cat == TypeCategory::kFloatingPoint) &&
      (to_cat == TypeCategory::kInteger || to_cat == TypeCategory::kFloatingPoint || 
       to_cat == TypeCategory::kNumeric)) {
    return true;
  }
  
  // String types are convertible
  if (from_cat == TypeCategory::kString && to_cat == TypeCategory::kString) {
    return true;
  }
  
  // Temporal types
  if ((from_cat == TypeCategory::kTimestamp && to_cat == TypeCategory::kDate) ||
      (from_cat == TypeCategory::kDate && to_cat == TypeCategory::kTimestamp)) {
    return true;
  }
  
  return false;
}

std::optional<Oid> TypeConversion::GetPreferredType(TypeCategory category) const {
  switch (category) {
    case TypeCategory::kInteger:
      return kOidInt8;
    case TypeCategory::kFloatingPoint:
      return kOidFloat8;
    case TypeCategory::kString:
      return kOidText;
    case TypeCategory::kTimestamp:
      return kOidTimestamptz;
    default:
      return std::nullopt;
  }
}

// ============================================================================
// TypeFormatter Implementation
// ============================================================================

std::string TypeFormatter::FormatTypeName(Oid oid, int32_t typemod) {
  auto type_info = TypeRegistry::Instance().GetTypeByOid(oid);
  if (!type_info) {
    return "unknown";
  }
  
  std::string result = type_info->name;
  
  // Add type modifier for sized types
  if (typemod >= 0) {
    if (oid == kOidVarchar || oid == kOidBpchar) {
      result += "(" + std::to_string(typemod) + ")";
    } else if (oid == kOidNumeric) {
      int precision = (typemod >> 16) & 0xffff;
      int scale = typemod & 0xffff;
      result += "(" + std::to_string(precision) + "," + std::to_string(scale) + ")";
    } else if (oid == kOidBit || oid == kOidVarbit) {
      result += "(" + std::to_string(typemod) + ")";
    }
  }
  
  return result;
}

std::string TypeFormatter::FormatTypeName(const std::string& name, int32_t typemod) {
  auto type_info = TypeRegistry::Instance().GetTypeByName(name);
  if (type_info) {
    return FormatTypeName(type_info->oid, typemod);
  }
  return name;
}

std::string TypeFormatter::FormatArrayTypeName(Oid element_oid) {
  auto type_info = TypeRegistry::Instance().GetTypeByOid(element_oid);
  if (!type_info) {
    return "unknown[]";
  }
  return type_info->name + "[]";
}

std::string TypeFormatter::FormatRangeTypeName(Oid subtype_oid) {
  auto type_info = TypeRegistry::Instance().GetTypeByOid(subtype_oid);
  if (!type_info) {
    return "unknownrange";
  }
  return type_info->name + "range";
}

// ============================================================================
// TypeCompatibility Implementation
// ============================================================================

bool TypeCompatibility::AreCompatible(Oid type1, Oid type2) {
  if (type1 == type2) {
    return true;
  }
  
  auto cat1 = GetTypeCategory(type1);
  auto cat2 = GetTypeCategory(type2);
  
  // Same category types are generally compatible
  if (cat1 == cat2) {
    return true;
  }
  
  // Numeric categories are compatible with each other
  if ((cat1 == TypeCategory::kInteger || cat1 == TypeCategory::kFloatingPoint || 
       cat1 == TypeCategory::kNumeric) &&
      (cat2 == TypeCategory::kInteger || cat2 == TypeCategory::kFloatingPoint || 
       cat2 == TypeCategory::kNumeric)) {
    return true;
  }
  
  return false;
}

Oid TypeCompatibility::ChooseCommonType(Oid type1, Oid type2) {
  if (type1 == type2) {
    return type1;
  }
  
  auto cat1 = GetTypeCategory(type1);
  auto cat2 = GetTypeCategory(type2);
  
  // Same category - choose the "larger" type
  if (cat1 == cat2) {
    auto type_info1 = TypeRegistry::Instance().GetTypeByOid(type1);
    auto type_info2 = TypeRegistry::Instance().GetTypeByOid(type2);
    
    if (type_info1 && type_info2) {
      // Prefer larger size for fixed-length types
      if (type_info1->IsFixedLength() && type_info2->IsFixedLength()) {
        return type_info1->size >= type_info2->size ? type1 : type2;
      }
      // Prefer variable-length types
      if (type_info1->IsVariableLength() && !type_info2->IsVariableLength()) {
        return type1;
      }
      if (type_info2->IsVariableLength() && !type_info1->IsVariableLength()) {
        return type2;
      }
    }
  }
  
  // Numeric categories
  if (cat1 == TypeCategory::kNumeric || cat2 == TypeCategory::kNumeric) {
    return kOidNumeric;
  }
  if (cat1 == TypeCategory::kFloatingPoint || cat2 == TypeCategory::kFloatingPoint) {
    return kOidFloat8;
  }
  if (cat1 == TypeCategory::kInteger || cat2 == TypeCategory::kInteger) {
    return kOidInt8;
  }
  
  // Default to first type
  return type1;
}

bool TypeCompatibility::IsCoercible(Oid from_type, Oid to_type) {
  return TypeConversion().CanConvert(from_type, to_type);
}

}  // namespace core
}  // namespace scratchrobin
