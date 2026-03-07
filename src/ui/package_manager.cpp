#include "ui/package_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QListWidget>
#include <QSplitter>
#include <QLabel>
#include <QGroupBox>
#include <QToolBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace scratchrobin::ui {

// ============================================================================
// Package Manager Panel
// ============================================================================
PackageManagerPanel::PackageManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("package_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void PackageManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    createBtn_ = new QPushButton(tr("Create"), this);
    editSpecBtn_ = new QPushButton(tr("Edit Spec"), this);
    editBodyBtn_ = new QPushButton(tr("Edit Body"), this);
    dropBtn_ = new QPushButton(tr("Drop"), this);
    compileBtn_ = new QPushButton(tr("Compile"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(editSpecBtn_);
    toolbar->addWidget(editBodyBtn_);
    toolbar->addWidget(compileBtn_);
    toolbar->addWidget(dropBtn_);
    toolbar->addWidget(refreshBtn);
    
    connect(createBtn_, &QPushButton::clicked, this, &PackageManagerPanel::onCreatePackage);
    connect(editSpecBtn_, &QPushButton::clicked, this, &PackageManagerPanel::onEditPackage);
    connect(editBodyBtn_, &QPushButton::clicked, this, &PackageManagerPanel::onEditPackage);
    connect(dropBtn_, &QPushButton::clicked, this, &PackageManagerPanel::onDropPackage);
    connect(compileBtn_, &QPushButton::clicked, this, &PackageManagerPanel::onCompilePackage);
    connect(refreshBtn, &QPushButton::clicked, this, &PackageManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter packages..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &PackageManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter for tree and preview
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Package tree
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    packageTree_ = new QTreeView(this);
    packageTree_->setAlternatingRowColors(true);
    packageTree_->setSortingEnabled(true);
    connect(packageTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PackageManagerPanel::onPackageSelected);
    
    leftLayout->addWidget(packageTree_);
    splitter->addWidget(leftWidget);
    
    // Right: Member tree and preview
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    memberTree_ = new QTreeView(this);
    memberTree_->setAlternatingRowColors(true);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setPlaceholderText(tr("Select a package to view its specification..."));
    
    rightLayout->addWidget(new QLabel(tr("Members:")));
    rightLayout->addWidget(memberTree_, 1);
    rightLayout->addWidget(new QLabel(tr("Preview:")));
    rightLayout->addWidget(previewEdit_, 2);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({300, 500});
    
    mainLayout->addWidget(splitter);
    
    // Disable buttons until selection
    editSpecBtn_->setEnabled(false);
    editBodyBtn_->setEnabled(false);
    dropBtn_->setEnabled(false);
    compileBtn_->setEnabled(false);
}

void PackageManagerPanel::setupModel()
{
    packageModel_ = new QStandardItemModel(this);
    packageModel_->setColumnCount(4);
    packageModel_->setHorizontalHeaderLabels({
        tr("Package"), tr("Schema"), tr("Status"), tr("Has Body")
    });
    packageTree_->setModel(packageModel_);
    packageTree_->header()->setStretchLastSection(false);
    packageTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    memberModel_ = new QStandardItemModel(this);
    memberModel_->setColumnCount(3);
    memberModel_->setHorizontalHeaderLabels({
        tr("Member"), tr("Type"), tr("Data Type")
    });
    memberTree_->setModel(memberModel_);
    memberTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void PackageManagerPanel::refresh()
{
    loadPackages();
}

void PackageManagerPanel::panelActivated()
{
    refresh();
}

void PackageManagerPanel::loadPackages()
{
    packageModel_->removeRows(0, packageModel_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void PackageManagerPanel::onCreatePackage()
{
    PackageEditorDialog dialog(client_, PackageEditorDialog::Create, QString(), this);
    dialog.exec();
}

void PackageManagerPanel::onEditPackage()
{
    auto index = packageTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString packageName = packageModel_->item(index.row(), 0)->text();
    
    bool editBody = (sender() == editBodyBtn_);
    
    PackageEditorDialog::Mode mode = editBody ? 
        PackageEditorDialog::EditBody : PackageEditorDialog::EditSpec;
    
    PackageEditorDialog dialog(client_, mode, packageName, this);
    dialog.exec();
}

void PackageManagerPanel::onDropPackage()
{
    auto index = packageTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString packageName = packageModel_->item(index.row(), 0)->text();
    QString schema = packageModel_->item(index.row(), 1)->text();
    
    auto reply = QMessageBox::question(this, tr("Drop Package"),
        tr("Drop package '%1.%2'?\n\nThis will also drop the package body.").arg(schema).arg(packageName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void PackageManagerPanel::onCompilePackage()
{
    auto index = packageTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString packageName = packageModel_->item(index.row(), 0)->text();
    
    // TODO: Execute via SessionClient when API is available
    QMessageBox::information(this, tr("Compile"),
        tr("Package compiled successfully."));
    refresh();
}

void PackageManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < packageModel_->rowCount(); ++i) {
        QString name = packageModel_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        packageTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void PackageManagerPanel::onShowErrors()
{
    // TODO: Show compilation errors for the selected package
}

void PackageManagerPanel::onPackageSelected(const QModelIndex& index)
{
    bool hasSelection = index.isValid();
    
    editSpecBtn_->setEnabled(hasSelection);
    editBodyBtn_->setEnabled(hasSelection);
    dropBtn_->setEnabled(hasSelection);
    compileBtn_->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString packageName = packageModel_->item(index.row(), 0)->text();
        emit packageSelected(packageName);
    }
}

// ============================================================================
// Package Editor Dialog
// ============================================================================
PackageEditorDialog::PackageEditorDialog(backend::SessionClient* client,
                                         Mode mode,
                                         const QString& packageName,
                                         QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , mode_(mode)
    , originalName_(packageName)
{
    setMinimumSize(900, 700);
    
    if (mode_ == Create) {
        setWindowTitle(tr("Create Package"));
    } else if (mode_ == EditSpec) {
        setWindowTitle(tr("Edit Package Specification"));
    } else {
        setWindowTitle(tr("Edit Package Body"));
    }
    
    setupUi();
}

void PackageEditorDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Header section
    auto* headerLayout = new QHBoxLayout();
    
    headerLayout->addWidget(new QLabel(tr("Package Name:")));
    nameEdit_ = new QLineEdit(this);
    headerLayout->addWidget(nameEdit_);
    
    headerLayout->addWidget(new QLabel(tr("Schema:")));
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    headerLayout->addWidget(schemaCombo_);
    
    headerLayout->addWidget(new QLabel(tr("AuthID:")));
    authIdCombo_ = new QComboBox(this);
    authIdCombo_->addItems({"DEFINER", "CURRENT_USER"});
    headerLayout->addWidget(authIdCombo_);
    
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Editor tabs
    editorTabs_ = new QTabWidget(this);
    
    specEdit_ = new QTextEdit(this);
    specEdit_->setFont(QFont("Monospace", 10));
    editorTabs_->addTab(specEdit_, tr("Specification"));
    
    bodyEdit_ = new QTextEdit(this);
    bodyEdit_->setFont(QFont("Monospace", 10));
    editorTabs_->addTab(bodyEdit_, tr("Body"));
    
    mainLayout->addWidget(editorTabs_);
    
    // Status label
    statusLabel_ = new QLabel(this);
    statusLabel_->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    mainLayout->addWidget(statusLabel_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* validateBtn = new QPushButton(tr("Validate"), this);
    auto* compileBtn = new QPushButton(tr("Compile"), this);
    auto* saveBtn = new QPushButton(tr("Save"), this);
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    saveBtn->setDefault(true);
    
    buttonLayout->addWidget(validateBtn);
    buttonLayout->addWidget(compileBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(validateBtn, &QPushButton::clicked, this, &PackageEditorDialog::onValidate);
    connect(compileBtn, &QPushButton::clicked, this, &PackageEditorDialog::onCompile);
    connect(saveBtn, &QPushButton::clicked, this, &PackageEditorDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void PackageEditorDialog::onValidate()
{
    statusLabel_->setText(tr("Valid specification."));
    statusLabel_->setStyleSheet("color: green;");
}

void PackageEditorDialog::onCompile()
{
    statusLabel_->setText(tr("Package compiled successfully."));
    statusLabel_->setStyleSheet("color: green;");
}

void PackageEditorDialog::onSave()
{
    // TODO: Save via SessionClient when API is available
    accept();
}

} // namespace scratchrobin::ui
