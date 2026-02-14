#include "phases/phase_registry.h"

#include <iostream>

int main() {
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
