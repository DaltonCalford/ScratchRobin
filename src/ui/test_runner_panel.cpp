/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "test_runner_panel.h"

namespace scratchrobin {

TestRunnerPanel::TestRunnerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    // Stub implementation
}

TestRunnerPanel::~TestRunnerPanel() = default;

void TestRunnerPanel::LoadTestSuite(const TestSuite& suite) {}
void TestRunnerPanel::RunSelectedTests() {}
void TestRunnerPanel::RunAllTests() {}

wxBEGIN_EVENT_TABLE(TestRunnerPanel, wxPanel)
wxEND_EVENT_TABLE()

} // namespace scratchrobin
