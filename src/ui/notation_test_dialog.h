/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_NOTATION_TEST_DIALOG_H
#define SCRATCHROBIN_NOTATION_TEST_DIALOG_H

#include <wx/dialog.h>

class wxNotebook;

namespace scratchrobin {

// Dialog showing test diagrams for each ERD notation
class NotationTestDialog : public wxDialog {
public:
    NotationTestDialog(wxWindow* parent);

private:
    void BuildLayout();
    void CreateCrowsFootPage(wxNotebook* notebook);
    void CreateIDEF1XPage(wxNotebook* notebook);
    void CreateUMLPage(wxNotebook* notebook);
    void CreateChenPage(wxNotebook* notebook);

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_NOTATION_TEST_DIALOG_H
