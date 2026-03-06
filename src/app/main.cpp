#include <iostream>
#include <QCoreApplication>

#include "backend/parser_port_registry.h"
#include "backend/native_parser_compiler.h"
#include "backend/server_session_gateway.h"
#include "backend/native_adapter_gateway.h"
#include "backend/session_client.h"
#include "backend/scratchbird_runtime_config.h"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  
  app.setApplicationName("ScratchRobin");
  app.setApplicationVersion("0.1.0");
  
  std::cout << "ScratchRobin CLI (Qt6 Edition)" << std::endl;
  std::cout << "Version: " << app.applicationVersion().toStdString() << std::endl;
  std::cout << std::endl;
  
  scratchrobin::backend::ParserPortRegistry registry;
  scratchrobin::backend::NativeParserCompiler compiler;
  scratchrobin::backend::ServerSessionGateway session;
  scratchrobin::backend::NativeAdapterGateway adapter(&registry, &compiler, &session);
  scratchrobin::backend::SessionClient client(&adapter);
  
  auto status = registry.Register(4044, "scratchbird-native");
  if (!status.ok) {
    std::cerr << "Error: " << status.message << std::endl;
    return 1;
  }
  
  scratchrobin::backend::ScratchbirdRuntimeConfig runtime;
  runtime.mode = scratchrobin::backend::TransportMode::kPreview;
  runtime.host = "127.0.0.1";
  runtime.port = 4044;
  runtime.database = "default";
  client.ConfigureRuntime(runtime);
  
  std::cout << "Backend initialized successfully." << std::endl;
  std::cout << "Parser registered on port 4044 (scratchbird-native)" << std::endl;
  
  return 0;
}
