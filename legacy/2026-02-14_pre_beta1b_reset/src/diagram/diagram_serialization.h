/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_SERIALIZATION_H
#define SCRATCHROBIN_DIAGRAM_SERIALIZATION_H

#include <string>

#include "../ui/diagram_model.h"

namespace scratchrobin {
namespace diagram {

struct DiagramDocument {
    std::string diagram_id;
    std::string name;
    double zoom = 1.0;
    double pan_x = 0.0;
    double pan_y = 0.0;
};

class DiagramSerializer {
public:
    static std::string ToJson(const DiagramModel& model, const DiagramDocument& doc);
    static bool FromJson(const std::string& json, DiagramModel* model, DiagramDocument* doc,
                         std::string* error);

    static bool SaveToFile(const DiagramModel& model, const DiagramDocument& doc,
                           const std::string& path, std::string* error);
    static bool LoadFromFile(DiagramModel* model, DiagramDocument* doc,
                             const std::string& path, std::string* error);
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_SERIALIZATION_H
