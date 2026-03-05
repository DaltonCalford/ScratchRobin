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

#include <memory>
#include <string>

#include "backend/query_response.h"
#include "backend/scratchbird_runtime_config.h"

namespace scratchrobin::backend {

class ScratchbirdSbwpClient {
 public:
  explicit ScratchbirdSbwpClient(ScratchbirdRuntimeConfig config = {});
  ~ScratchbirdSbwpClient();

  ScratchbirdSbwpClient(const ScratchbirdSbwpClient&) = delete;
  ScratchbirdSbwpClient& operator=(const ScratchbirdSbwpClient&) = delete;

  void SetConfig(ScratchbirdRuntimeConfig config);
  const ScratchbirdRuntimeConfig& GetConfig() const;

  QueryResponse ExecuteSql(const std::string& sql);
  QueryResponse BeginTransaction();
  QueryResponse CommitTransaction();
  QueryResponse RollbackTransaction();

  bool IsConnected() const;
  void Disconnect();

 private:
  QueryResponse ConnectIfNeeded();
  QueryResponse ExecuteRemoteSql(const std::string& sql, bool collect_rows,
                                const std::string& execution_path);
  QueryResponse StatusError(const std::string& path,
                           const std::string& message) const;

  ScratchbirdRuntimeConfig config_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::backend
