#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QCheckBox;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Synonym Manager - Database synonym administration
 * 
 * Supports Oracle-style synonyms:
 * - PUBLIC synonyms: Accessible to all users
 * - PRIVATE synonyms: Accessible only to the owner
 * 
 * DDL Syntax:
 *   CREATE [PUBLIC] SYNONYM [schema.]name FOR [schema.]object[@dblink];
 *   DROP [PUBLIC] SYNONYM [schema.]name;
 */

// ============================================================================
// Synonym Info
// ============================================================================
struct SynonymInfo {
    QString name;
    QString schema;
    bool isPublic = false;
    QString targetSchema;
    QString targetName;
    QString targetDbLink;  // For remote objects
    QString objectType;    // TABLE, VIEW, PROCEDURE, etc.
    bool isValid = true;   // If target object exists
    QDateTime created;
};

// ============================================================================
// Synonym Manager Panel
// ============================================================================
class SynonymManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SynonymManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Synonym Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateSynonym();
    void onEditSynonym();
    void onDropSynonym();
    void onDropPublicSynonym();
    void onValidateSynonym();
    void onFilterChanged(const QString& filter);
    void onSynonymSelected(const QModelIndex& index);

signals:
    void synonymSelected(const QString& name);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadSynonyms();
    void updateDetails(const SynonymInfo& info);
    
    backend::SessionClient* client_;
    
    QTreeView* synonymTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    
    // Details panel
    QLabel* nameLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLabel* targetLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Synonym Editor Dialog
// ============================================================================
class SynonymEditorDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, Edit };
    enum SynonymType { Private, Public };
    
    explicit SynonymEditorDialog(backend::SessionClient* client,
                                 Mode mode,
                                 const QString& synonymName = QString(),
                                 bool isPublic = false,
                                 QWidget* parent = nullptr);

public slots:
    void onBrowseTarget();
    void onValidate();
    void onPreview();
    void onSave();

private:
    void setupUi();
    void loadSynonym(const QString& name, bool isPublic);
    void updateTargetInfo();
    SynonymInfo collectInfo() const;
    QString generateDdl(bool forDrop = false) const;
    bool validateTarget(const QString& schema, const QString& name, QString& objectType);
    
    backend::SessionClient* client_;
    Mode mode_;
    bool originalIsPublic_ = false;
    
    // Identity
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QCheckBox* publicCheck_ = nullptr;
    
    // Target
    QComboBox* targetSchemaCombo_ = nullptr;
    QLineEdit* targetNameEdit_ = nullptr;
    QComboBox* targetDbLinkCombo_ = nullptr;
    QPushButton* browseBtn_ = nullptr;
    
    // Info
    QLabel* targetTypeLabel_ = nullptr;
    QLabel* targetStatusLabel_ = nullptr;
    
    // Preview
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Target Browser Dialog
// ============================================================================
class TargetBrowserDialog : public QDialog {
    Q_OBJECT

public:
    explicit TargetBrowserDialog(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString selectedSchema() const;
    QString selectedName() const;
    QString selectedType() const;

private:
    void setupUi();
    void loadSchemas();
    void loadObjects(const QString& schema);
    
    backend::SessionClient* client_;
    
    QComboBox* schemaCombo_ = nullptr;
    QTreeView* objectTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    QString selectedSchema_;
    QString selectedName_;
    QString selectedType_;
};

} // namespace scratchrobin::ui
