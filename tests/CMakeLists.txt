add_executable(backend_contract_tests
  backend_contract_tests.cpp
)

target_include_directories(backend_contract_tests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(backend_contract_tests
  PRIVATE
    scratchrobin_backend
)

add_test(NAME backend_contract_tests COMMAND backend_contract_tests)
