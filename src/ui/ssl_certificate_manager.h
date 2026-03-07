#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QTreeView;
class QProgressBar;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief SSL Certificate Manager - Connection security management
 * 
 * Manage SSL certificates for database connections:
 * - View installed certificates
 * - Import/export certificates
 * - Certificate validation
 * - Connection security settings
 */

// ============================================================================
// Certificate Types
// ============================================================================
enum class CertificateType {
    Server,     // Server certificate
    Client,     // Client certificate
    CA,         // Certificate Authority
    CRL         // Certificate Revocation List
};

struct CertificateInfo {
    QString alias;
    CertificateType type;
    QString subject;
    QString issuer;
    QString serialNumber;
    QString fingerprint;
    QString signatureAlgorithm;
    QDateTime validFrom;
    QDateTime validUntil;
    int keySize = 0;
    bool isValid = true;
    QString filePath;
    QString password;
    QStringList purposes;
    QStringList subjectAlternativeNames;
};

struct ConnectionSecurity {
    bool sslEnabled = false;
    bool verifyServerCert = true;
    bool verifyHostname = true;
    QString sslMode; // disable, allow, prefer, require, verify-ca, verify-full
    QString caCertPath;
    QString clientCertPath;
    QString clientKeyPath;
    QString crlPath;
    QString cipherList;
    int tlsVersion = 0; // 0 = auto, 1 = 1.0, 2 = 1.1, 3 = 1.2, 4 = 1.3
};

// ============================================================================
// SSL Certificate Manager Panel
// ============================================================================
class SSLCertificateManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SSLCertificateManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("SSL Certificate Manager"); }
    QString panelCategory() const override { return "security"; }

public slots:
    // Certificate operations
    void onImportCertificate();
    void onExportCertificate();
    void onDeleteCertificate();
    void onViewCertificateDetails();
    void onRefreshCertificates();
    
    // Certificate actions
    void onValidateCertificate();
    void onRenewCertificate();
    void onGenerateCSR();
    void onCreateSelfSigned();
    
    // Security settings
    void onConfigureSSL();
    void onTestConnection();
    void onApplySettings();
    
    // Selection
    void onCertificateSelected(const QModelIndex& index);

signals:
    void certificateImported(const QString& alias);
    void certificateDeleted(const QString& alias);
    void sslConfigured(const ConnectionSecurity& config);

private:
    void setupUi();
    void setupCertificateList();
    void setupConnectionSettings();
    void loadCertificates();
    void updateCertificateList();
    void updateCertificateDetails(const CertificateInfo& cert);
    
    backend::SessionClient* client_;
    QList<CertificateInfo> certificates_;
    ConnectionSecurity currentConfig_;
    
    // UI
    QTableView* certTable_ = nullptr;
    QStandardItemModel* certModel_ = nullptr;
    
    // Certificate details
    QLabel* subjectLabel_ = nullptr;
    QLabel* issuerLabel_ = nullptr;
    QLabel* validFromLabel_ = nullptr;
    QLabel* validUntilLabel_ = nullptr;
    QLabel* fingerprintLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* validityBar_ = nullptr;
    
    // Connection settings
    QCheckBox* sslEnabledCheck_ = nullptr;
    QComboBox* sslModeCombo_ = nullptr;
    QCheckBox* verifyServerCheck_ = nullptr;
    QCheckBox* verifyHostnameCheck_ = nullptr;
    QLineEdit* caCertEdit_ = nullptr;
    QLineEdit* clientCertEdit_ = nullptr;
    QLineEdit* clientKeyEdit_ = nullptr;
    QComboBox* tlsVersionCombo_ = nullptr;
};

// ============================================================================
// Import Certificate Dialog
// ============================================================================
class ImportCertificateDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportCertificateDialog(CertificateInfo* cert, QWidget* parent = nullptr);

public slots:
    void onBrowseFile();
    void onBrowsePrivateKey();
    void onTestPassword();
    void onPreview();
    void onImport();

private:
    void setupUi();
    void validateCertificate();
    
    CertificateInfo* cert_;
    
    QLineEdit* aliasEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QLineEdit* fileEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLineEdit* privateKeyEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Certificate Details Dialog
// ============================================================================
class CertificateDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit CertificateDetailsDialog(const CertificateInfo& cert, QWidget* parent = nullptr);

public slots:
    void onExport();
    void onViewChain();
    void onVerify();

private:
    void setupUi();
    
    CertificateInfo cert_;
    
    QLabel* subjectLabel_ = nullptr;
    QLabel* issuerLabel_ = nullptr;
    QLabel* serialLabel_ = nullptr;
    QLabel* fingerprintLabel_ = nullptr;
    QLabel* algorithmLabel_ = nullptr;
    QLabel* validFromLabel_ = nullptr;
    QLabel* validUntilLabel_ = nullptr;
    QLabel* keySizeLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTextEdit* purposesEdit_ = nullptr;
    QTextEdit* sanEdit_ = nullptr;
};

// ============================================================================
// Generate CSR Dialog
// ============================================================================
class GenerateCSRDialog : public QDialog {
    Q_OBJECT

public:
    explicit GenerateCSRDialog(QWidget* parent = nullptr);

public slots:
    void onGenerate();
    void onSaveCSR();
    void onSavePrivateKey();
    void onCopyToClipboard();

private:
    void setupUi();
    
    QLineEdit* cnEdit_ = nullptr;
    QLineEdit* orgEdit_ = nullptr;
    QLineEdit* ouEdit_ = nullptr;
    QLineEdit* cityEdit_ = nullptr;
    QLineEdit* stateEdit_ = nullptr;
    QLineEdit* countryEdit_ = nullptr;
    QLineEdit* emailEdit_ = nullptr;
    QComboBox* keySizeCombo_ = nullptr;
    QComboBox* algorithmCombo_ = nullptr;
    QTextEdit* sanEdit_ = nullptr;
    
    QTextEdit* csrEdit_ = nullptr;
    QTextEdit* privateKeyEdit_ = nullptr;
};

// ============================================================================
// SSL Configuration Dialog
// ============================================================================
class SSLConfigurationDialog : public QDialog {
    Q_OBJECT

public:
    explicit SSLConfigurationDialog(ConnectionSecurity* config, QWidget* parent = nullptr);

public slots:
    void onBrowseCA();
    void onBrowseCert();
    void onBrowseKey();
    void onBrowseCRL();
    void onTestConnection();
    void onSave();

private:
    void setupUi();
    void loadSettings();
    
    ConnectionSecurity* config_;
    
    QCheckBox* sslEnabledCheck_ = nullptr;
    QComboBox* sslModeCombo_ = nullptr;
    QCheckBox* verifyServerCheck_ = nullptr;
    QCheckBox* verifyHostnameCheck_ = nullptr;
    QLineEdit* caCertEdit_ = nullptr;
    QLineEdit* clientCertEdit_ = nullptr;
    QLineEdit* clientKeyEdit_ = nullptr;
    QLineEdit* crlEdit_ = nullptr;
    QLineEdit* cipherEdit_ = nullptr;
    QComboBox* tlsVersionCombo_ = nullptr;
};

} // namespace scratchrobin::ui
