#include <iostream>

#include "backend/native_adapter_gateway.h"
#include "backend/native_parser_compiler.h"
#include "backend/parser_port_registry.h"
#include "backend/session_client.h"
#include "backend/server_session_gateway.h"
#include "ui/main_window.h"

int main() {
  scratchrobin::backend::ParserPortRegistry registry;
  registry.Register(4044, "scratchbird-native");

  scratchrobin::backend::NativeParserCompiler compiler;
  scratchrobin::backend::ServerSessionGateway session;
  scratchrobin::backend::NativeAdapterGateway adapter(&registry, &compiler, &session);
  scratchrobin::backend::SessionClient client(&adapter);

  scratchrobin::ui::MainWindow main_window;

  std::cout << main_window.GetTitle() << '\n';
  std::cout << "Layout: " << main_window.GetLayoutModel() << '\n';

  const auto response = client.ExecuteSql(
      4044,
      "scratchbird-native",
      "select id, name from projects");

  if (!response.status.ok) {
    std::cout << "Execution failed: " << response.status.message << '\n';
    return 1;
  }

  std::cout << "Execution path: " << response.execution_path << '\n';
  for (const auto& row : response.result_set.rows) {
    if (row.size() >= 2) {
      std::cout << row[0] << " | " << row[1] << '\n';
    }
  }

  return 0;
}
