#include "phases/phase_registry.h"
#include "packaging/packaging_services.h"
#include "release/release_conformance_services.h"
#include "runtime/runtime_services.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

int main(int argc, char** argv) {
    const auto has_arg = [](int argc_local, char** argv_local, std::string_view needle) {
        for (int i = 1; i < argc_local; ++i) {
            if (std::string_view(argv_local[i]) == needle) {
                return true;
            }
        }
        return false;
    };
    const auto arg_value = [](int argc_local, char** argv_local, std::string_view prefix) -> std::optional<std::string> {
        for (int i = 1; i < argc_local; ++i) {
            const std::string_view arg(argv_local[i]);
            if (arg.rfind(prefix, 0) == 0U) {
                return std::string(arg.substr(prefix.size()));
            }
        }
        return std::nullopt;
    };
    const auto find_repo_root = [](const char* argv0) {
        namespace fs = std::filesystem;
        std::vector<fs::path> roots;
        roots.push_back(fs::current_path());
        roots.push_back(fs::current_path().parent_path());
        if (argv0 != nullptr) {
            std::error_code ec;
            fs::path exe = fs::absolute(argv0, ec);
            if (!ec) {
                roots.push_back(exe.parent_path());
                roots.push_back(exe.parent_path().parent_path());
            }
        }
        for (const auto& candidate : roots) {
            if (candidate.empty()) {
                continue;
            }
            if (fs::exists(candidate / "config/scratchrobin.toml.example") &&
                fs::exists(candidate / "config/connections.toml.example")) {
                return candidate;
            }
        }
        return fs::current_path();
    };

    if (argc > 0 && argv != nullptr && has_arg(argc, argv, "--release-gate-check")) {
        namespace fs = std::filesystem;
        const fs::path repo_root = find_repo_root(argc > 0 ? argv[0] : nullptr);
        std::string blocker_register =
            (repo_root.parent_path() /
             "local_work/docs/specifications_beta1b/10_Execution_Tracks_and_Conformance/BLOCKER_REGISTER.csv")
                .string();
        if (const auto value = arg_value(argc, argv, "--blocker-register=")) {
            blocker_register = *value;
        }

        try {
            scratchrobin::release::ReleaseConformanceService service;
            const auto rows = service.LoadBlockerRegister(blocker_register);
            const auto verdict = service.EvaluatePromotability(rows);
            std::cout << service.ExportPromotabilityJson(verdict) << "\n";
            return verdict.promotable ? 0 : 3;
        } catch (const std::exception& ex) {
            std::cerr << "release gate check failed: " << ex.what() << "\n";
            return 2;
        }
    }

    if (argc > 0 && argv != nullptr && arg_value(argc, argv, "--validate-package-manifest=").has_value()) {
        namespace fs = std::filesystem;
        const fs::path repo_root = find_repo_root(argc > 0 ? argv[0] : nullptr);
        const std::string manifest_path = *arg_value(argc, argv, "--validate-package-manifest=");
        std::string registry_path = (repo_root / "resources/schemas/package_surface_id_registry.json").string();
        std::string schema_path = (repo_root / "resources/schemas/package_profile_manifest.schema.json").string();
        if (const auto value = arg_value(argc, argv, "--surface-registry=")) {
            registry_path = *value;
        }
        if (const auto value = arg_value(argc, argv, "--manifest-schema=")) {
            schema_path = *value;
        }

        try {
            scratchrobin::packaging::PackagingService service;
            const auto summary = service.ValidateManifestFile(manifest_path, registry_path, schema_path);
            std::cout << "{\"ok\":" << (summary.ok ? "true" : "false")
                      << ",\"profile_id\":\"" << summary.profile_id << "\"}\n";
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "manifest validation failed: " << ex.what() << "\n";
            return 2;
        }
    }

    if (argc > 0 && argv != nullptr && has_arg(argc, argv, "--runtime-startup")) {
        namespace fs = std::filesystem;
        const fs::path repo_root = find_repo_root(argc > 0 ? argv[0] : nullptr);
        scratchrobin::runtime::ScratchRobinRuntime runtime;
        scratchrobin::runtime::StartupPaths paths;
        paths.app_config_path = (repo_root / "config/scratchrobin.toml").string();
        paths.app_config_example_path = (repo_root / "config/scratchrobin.toml.example").string();
        paths.connections_path = (repo_root / "config/connections.toml").string();
        paths.connections_example_path = (repo_root / "config/connections.toml.example").string();
        paths.session_state_path = (repo_root / "work/session_state.json").string();

        try {
            const auto report = runtime.Startup(paths);
            std::cout << "ScratchRobin runtime startup\n";
            std::cout << "  ok: " << (report.ok ? "true" : "false") << "\n";
            std::cout << "  config_source: " << report.config_source << "\n";
            std::cout << "  profiles: " << report.connection_profile_count << "\n";
            std::cout << "  unavailable_backends: " << report.unavailable_backend_count << "\n";
            std::cout << "  metadata_mode: " << report.metadata_mode << "\n";
            std::cout << "  main_frame_visible: " << (report.main_frame_visible ? "true" : "false") << "\n";
            if (!report.warnings.empty()) {
                std::cout << "  warnings:\n";
                for (const auto& warning : report.warnings) {
                    std::cout << "    - " << warning << "\n";
                }
            }
            runtime.Shutdown(paths);
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "runtime startup failed: " << ex.what() << "\n";
            return 2;
        }
    }

    const auto phases = scratchrobin::phases::BuildPhaseScaffold();

    std::cout << "ScratchRobin beta1b scaffold\n";
    std::cout << "Phase modules: " << phases.size() << "\n\n";

    for (const auto& phase : phases) {
        std::cout << "[Phase " << phase.phase_id << "] " << phase.title << "\n";
        std::cout << "  Spec section: " << phase.spec_section << "\n";
        std::cout << "  Description : " << phase.description << "\n";
        if (phase.dependencies.empty()) {
            std::cout << "  Depends on  : (none)\n";
        } else {
            std::cout << "  Depends on  : ";
            for (std::size_t i = 0; i < phase.dependencies.size(); ++i) {
                if (i != 0) {
                    std::cout << ", ";
                }
                std::cout << phase.dependencies[i];
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}
