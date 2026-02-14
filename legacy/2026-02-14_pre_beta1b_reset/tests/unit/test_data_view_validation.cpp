/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "core/data_view_validation.h"

#include <gtest/gtest.h>

namespace scratchrobin {

TEST(DataViewValidationTest, AcceptsValidObject) {
    std::string error;
    EXPECT_TRUE(ValidateDataViewJson(R"({"name":"Sample","columns":[],"rows":[]})", &error));
    EXPECT_TRUE(error.empty());
}

TEST(DataViewValidationTest, RejectsInvalidJson) {
    std::string error;
    EXPECT_FALSE(ValidateDataViewJson("{bad", &error));
    EXPECT_FALSE(error.empty());
}

TEST(DataViewValidationTest, RejectsNonObjectRoot) {
    std::string error;
    EXPECT_FALSE(ValidateDataViewJson(R"(["not","object"])", &error));
    EXPECT_FALSE(error.empty());
}

} // namespace scratchrobin
