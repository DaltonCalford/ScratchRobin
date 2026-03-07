#include "ssl_certificate_manager.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QProgressBar>
#include <QFile>

namespace scratchrobin::ui {

// ============================================================================
// SSL Certificate Manager Panel
// ============================================================================

SSLCertificateManagerPanel::SSLCertificateManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("ssl_certificate_manager", parent)
    , client_(client) {
    setupUi();
    loadCertificates();
}

void SSLCertificateManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* importBtn = new QPushButton(tr("Import"), this);
    connect(importBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onImportCertificate);
    toolbarLayout->addWidget(importBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onExportCertificate);
    toolbarLayout->addWidget(exportBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onDeleteCertificate);
    toolbarLayout->addWidget(deleteBtn);
    
    auto* generateBtn = new QPushButton(tr("Generate CSR"), this);
    connect(generateBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onGenerateCSR);
    toolbarLayout->addWidget(generateBtn);
    
    toolbarLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onRefreshCertificates);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Certificate list
    certTable_ = new QTableView(this);
    certModel_ = new QStandardItemModel(this);
    certModel_->setHorizontalHeaderLabels({tr("Alias"), tr("Type"), tr("Subject"), tr("Expires"), tr("Status")});
    certTable_->setModel(certModel_);
    certTable_->setAlternatingRowColors(true);
    certTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(certTable_, &QTableView::clicked, this, &SSLCertificateManagerPanel::onCertificateSelected);
    splitter->addWidget(certTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    // Certificate info
    auto* infoGroup = new QGroupBox(tr("Certificate Information"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    subjectLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Subject:"), subjectLabel_);
    
    issuerLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Issuer:"), issuerLabel_);
    
    validFromLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Valid From:"), validFromLabel_);
    
    validUntilLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Valid Until:"), validUntilLabel_);
    
    fingerprintLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Fingerprint:"), fingerprintLabel_);
    
    statusLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Status:"), statusLabel_);
    
    detailsLayout->addWidget(infoGroup);
    
    // Validity bar
    validityBar_ = new QProgressBar(this);
    validityBar_->setRange(0, 100);
    validityBar_->setValue(100);
    validityBar_->setFormat(tr("%p% Valid"));
    detailsLayout->addWidget(validityBar_);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* validateBtn = new QPushButton(tr("Validate"), this);
    connect(validateBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onValidateCertificate);
    btnLayout->addWidget(validateBtn);
    
    auto* detailsBtn = new QPushButton(tr("View Details"), this);
    connect(detailsBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onViewCertificateDetails);
    btnLayout->addWidget(detailsBtn);
    
    auto* renewBtn = new QPushButton(tr("Renew"), this);
    connect(renewBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onRenewCertificate);
    btnLayout->addWidget(renewBtn);
    
    btnLayout->addStretch();
    detailsLayout->addLayout(btnLayout);
    
    // SSL Configuration
    auto* sslGroup = new QGroupBox(tr("SSL Configuration"), this);
    auto* sslLayout = new QFormLayout(sslGroup);
    
    sslEnabledCheck_ = new QCheckBox(tr("Enable SSL"), this);
    sslLayout->addRow(sslEnabledCheck_);
    
    sslModeCombo_ = new QComboBox(this);
    sslModeCombo_->addItems({"disable", "allow", "prefer", "require", "verify-ca", "verify-full"});
    sslLayout->addRow(tr("SSL Mode:"), sslModeCombo_);
    
    verifyServerCheck_ = new QCheckBox(tr("Verify server certificate"), this);
    sslLayout->addRow(verifyServerCheck_);
    
    verifyHostnameCheck_ = new QCheckBox(tr("Verify hostname"), this);
    sslLayout->addRow(verifyHostnameCheck_);
    
    caCertEdit_ = new QLineEdit(this);
    sslLayout->addRow(tr("CA Certificate:"), caCertEdit_);
    
    clientCertEdit_ = new QLineEdit(this);
    sslLayout->addRow(tr("Client Certificate:"), clientCertEdit_);
    
    clientKeyEdit_ = new QLineEdit(this);
    sslLayout->addRow(tr("Client Key:"), clientKeyEdit_);
    
    tlsVersionCombo_ = new QComboBox(this);
    tlsVersionCombo_->addItems({"Auto", "TLS 1.0", "TLS 1.1", "TLS 1.2", "TLS 1.3"});
    sslLayout->addRow(tr("TLS Version:"), tlsVersionCombo_);
    
    detailsLayout->addWidget(sslGroup);
    
    // Apply button
    auto* applyBtn = new QPushButton(tr("Apply Settings"), this);
    connect(applyBtn, &QPushButton::clicked, this, &SSLCertificateManagerPanel::onApplySettings);
    detailsLayout->addWidget(applyBtn);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({350, 350});
    
    mainLayout->addWidget(splitter, 1);
}

void SSLCertificateManagerPanel::loadCertificates() {
    certificates_.clear();
    
    // Simulate certificates
    CertificateInfo cert1;
    cert1.alias = "server_cert";
    cert1.type = CertificateType::Server;
    cert1.subject = "CN=scratchbird.local,O=ScratchBird Inc";
    cert1.issuer = "CN=ScratchBird CA,O=ScratchBird Inc";
    cert1.validFrom = QDateTime::currentDateTime().addYears(-1);
    cert1.validUntil = QDateTime::currentDateTime().addYears(1);
    cert1.fingerprint = "A1:B2:C3:D4:E5:F6:...";
    cert1.isValid = true;
    certificates_.append(cert1);
    
    CertificateInfo cert2;
    cert2.alias = "ca_cert";
    cert2.type = CertificateType::CA;
    cert2.subject = "CN=ScratchBird CA,O=ScratchBird Inc";
    cert2.issuer = "CN=ScratchBird CA,O=ScratchBird Inc";
    cert2.validFrom = QDateTime::currentDateTime().addYears(-2);
    cert2.validUntil = QDateTime::currentDateTime().addYears(8);
    cert2.fingerprint = "B2:C3:D4:E5:F6:A1:...";
    cert2.isValid = true;
    certificates_.append(cert2);
    
    updateCertificateList();
}

void SSLCertificateManagerPanel::updateCertificateList() {
    certModel_->clear();
    certModel_->setHorizontalHeaderLabels({tr("Alias"), tr("Type"), tr("Subject"), tr("Expires"), tr("Status")});
    
    for (const auto& cert : certificates_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(cert.alias);
        
        QString typeStr;
        switch (cert.type) {
            case CertificateType::Server: typeStr = tr("Server"); break;
            case CertificateType::Client: typeStr = tr("Client"); break;
            case CertificateType::CA: typeStr = tr("CA"); break;
            case CertificateType::CRL: typeStr = tr("CRL"); break;
        }
        row << new QStandardItem(typeStr);
        row << new QStandardItem(cert.subject);
        row << new QStandardItem(cert.validUntil.toString("yyyy-MM-dd"));
        
        auto* statusItem = new QStandardItem(cert.isValid ? tr("Valid") : tr("Invalid"));
        if (cert.isValid) {
            statusItem->setForeground(Qt::darkGreen);
        } else {
            statusItem->setForeground(Qt::red);
        }
        row << statusItem;
        
        certModel_->appendRow(row);
    }
}

void SSLCertificateManagerPanel::updateCertificateDetails(const CertificateInfo& cert) {
    subjectLabel_->setText(cert.subject);
    issuerLabel_->setText(cert.issuer);
    validFromLabel_->setText(cert.validFrom.toString());
    validUntilLabel_->setText(cert.validUntil.toString());
    fingerprintLabel_->setText(cert.fingerprint);
    
    if (cert.isValid) {
        statusLabel_->setText(tr("<font color='green'>Valid</font>"));
    } else {
        statusLabel_->setText(tr("<font color='red'>Invalid</font>"));
    }
    
    // Calculate validity percentage
    qint64 total = cert.validFrom.secsTo(cert.validUntil);
    qint64 remaining = QDateTime::currentDateTime().secsTo(cert.validUntil);
    int percent = total > 0 ? qMax(0, static_cast<int>((remaining * 100) / total)) : 0;
    validityBar_->setValue(percent);
    
    if (percent > 50) {
        validityBar_->setStyleSheet("QProgressBar::chunk { background-color: green; }");
    } else if (percent > 20) {
        validityBar_->setStyleSheet("QProgressBar::chunk { background-color: yellow; }");
    } else {
        validityBar_->setStyleSheet("QProgressBar::chunk { background-color: red; }");
    }
}

void SSLCertificateManagerPanel::onImportCertificate() {
    CertificateInfo newCert;
    ImportCertificateDialog dialog(&newCert, this);
    if (dialog.exec() == QDialog::Accepted) {
        certificates_.append(newCert);
        updateCertificateList();
        emit certificateImported(newCert.alias);
    }
}

void SSLCertificateManagerPanel::onExportCertificate() {
    auto index = certTable_->currentIndex();
    if (!index.isValid() || index.row() >= certificates_.size()) {
        QMessageBox::warning(this, tr("Export"), tr("Please select a certificate to export."));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Certificate"),
        certificates_[index.row()].alias + ".pem",
        tr("PEM Files (*.pem);;DER Files (*.der);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Certificate exported to %1").arg(fileName));
    }
}

void SSLCertificateManagerPanel::onDeleteCertificate() {
    auto index = certTable_->currentIndex();
    if (!index.isValid() || index.row() >= certificates_.size()) return;
    
    QString alias = certificates_[index.row()].alias;
    auto reply = QMessageBox::warning(this, tr("Delete Certificate"),
        tr("Are you sure you want to delete certificate '%1'?").arg(alias),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        certificates_.removeAt(index.row());
        updateCertificateList();
        emit certificateDeleted(alias);
    }
}

void SSLCertificateManagerPanel::onViewCertificateDetails() {
    auto index = certTable_->currentIndex();
    if (!index.isValid() || index.row() >= certificates_.size()) return;
    
    CertificateDetailsDialog dialog(certificates_[index.row()], this);
    dialog.exec();
}

void SSLCertificateManagerPanel::onRefreshCertificates() {
    loadCertificates();
}

void SSLCertificateManagerPanel::onValidateCertificate() {
    auto index = certTable_->currentIndex();
    if (!index.isValid() || index.row() >= certificates_.size()) return;
    
    QMessageBox::information(this, tr("Validate"),
        tr("Certificate '%1' validated successfully.").arg(certificates_[index.row()].alias));
}

void SSLCertificateManagerPanel::onRenewCertificate() {
    auto index = certTable_->currentIndex();
    if (!index.isValid() || index.row() >= certificates_.size()) return;
    
    QMessageBox::information(this, tr("Renew"),
        tr("Certificate renewal process would start for '%1'.").arg(certificates_[index.row()].alias));
}

void SSLCertificateManagerPanel::onGenerateCSR() {
    GenerateCSRDialog dialog(this);
    dialog.exec();
}

void SSLCertificateManagerPanel::onCreateSelfSigned() {
    QMessageBox::information(this, tr("Self-Signed"),
        tr("Self-signed certificate generation would start."));
}

void SSLCertificateManagerPanel::onConfigureSSL() {
    SSLConfigurationDialog dialog(&currentConfig_, this);
    dialog.exec();
}

void SSLCertificateManagerPanel::onTestConnection() {
    QMessageBox::information(this, tr("Test Connection"),
        tr("SSL connection test would be performed."));
}

void SSLCertificateManagerPanel::onApplySettings() {
    currentConfig_.sslEnabled = sslEnabledCheck_->isChecked();
    currentConfig_.sslMode = sslModeCombo_->currentText();
    currentConfig_.verifyServerCert = verifyServerCheck_->isChecked();
    currentConfig_.verifyHostname = verifyHostnameCheck_->isChecked();
    currentConfig_.caCertPath = caCertEdit_->text();
    currentConfig_.clientCertPath = clientCertEdit_->text();
    currentConfig_.clientKeyPath = clientKeyEdit_->text();
    currentConfig_.tlsVersion = tlsVersionCombo_->currentIndex();
    
    emit sslConfigured(currentConfig_);
    
    QMessageBox::information(this, tr("Settings"),
        tr("SSL settings applied successfully."));
}

void SSLCertificateManagerPanel::onCertificateSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= certificates_.size()) return;
    updateCertificateDetails(certificates_[index.row()]);
}

// ============================================================================
// Import Certificate Dialog
// ============================================================================

ImportCertificateDialog::ImportCertificateDialog(CertificateInfo* cert, QWidget* parent)
    : QDialog(parent)
    , cert_(cert) {
    setupUi();
}

void ImportCertificateDialog::setupUi() {
    setWindowTitle(tr("Import Certificate"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    aliasEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Alias:"), aliasEdit_);
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem(tr("Server"), static_cast<int>(CertificateType::Server));
    typeCombo_->addItem(tr("Client"), static_cast<int>(CertificateType::Client));
    typeCombo_->addItem(tr("CA"), static_cast<int>(CertificateType::CA));
    formLayout->addRow(tr("Type:"), typeCombo_);
    
    auto* fileLayout = new QHBoxLayout();
    fileEdit_ = new QLineEdit(this);
    fileLayout->addWidget(fileEdit_);
    
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &ImportCertificateDialog::onBrowseFile);
    fileLayout->addWidget(browseBtn);
    
    formLayout->addRow(tr("Certificate File:"), fileLayout);
    
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("Password (if encrypted):"), passwordEdit_);
    
    auto* keyLayout = new QHBoxLayout();
    privateKeyEdit_ = new QLineEdit(this);
    keyLayout->addWidget(privateKeyEdit_);
    
    auto* keyBrowseBtn = new QPushButton(tr("Browse..."), this);
    connect(keyBrowseBtn, &QPushButton::clicked, this, &ImportCertificateDialog::onBrowsePrivateKey);
    keyLayout->addWidget(keyBrowseBtn);
    
    formLayout->addRow(tr("Private Key:"), keyLayout);
    
    layout->addLayout(formLayout);
    
    // Preview
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setPlaceholderText(tr("Certificate preview will appear here..."));
    layout->addWidget(new QLabel(tr("Preview:"), this));
    layout->addWidget(previewEdit_, 1);
    
    statusLabel_ = new QLabel(this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* testBtn = new QPushButton(tr("Test Password"), this);
    connect(testBtn, &QPushButton::clicked, this, &ImportCertificateDialog::onTestPassword);
    btnLayout->addWidget(testBtn);
    
    btnLayout->addStretch();
    
    auto* importBtn = new QPushButton(tr("Import"), this);
    connect(importBtn, &QPushButton::clicked, this, &ImportCertificateDialog::onImport);
    btnLayout->addWidget(importBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void ImportCertificateDialog::onBrowseFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Certificate"),
        QString(),
        tr("Certificate Files (*.pem *.crt *.cer *.der);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        fileEdit_->setText(fileName);
    }
}

void ImportCertificateDialog::onBrowsePrivateKey() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Private Key"),
        QString(),
        tr("Key Files (*.key *.pem);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        privateKeyEdit_->setText(fileName);
    }
}

void ImportCertificateDialog::onTestPassword() {
    QMessageBox::information(this, tr("Test Password"),
        tr("Password test would be performed here."));
}

void ImportCertificateDialog::onPreview() {
    // Preview certificate details
}

void ImportCertificateDialog::onImport() {
    cert_->alias = aliasEdit_->text();
    cert_->type = static_cast<CertificateType>(typeCombo_->currentData().toInt());
    cert_->filePath = fileEdit_->text();
    cert_->password = passwordEdit_->text();
    
    accept();
}

// ============================================================================
// Certificate Details Dialog
// ============================================================================

CertificateDetailsDialog::CertificateDetailsDialog(const CertificateInfo& cert, QWidget* parent)
    : QDialog(parent)
    , cert_(cert) {
    setupUi();
}

void CertificateDetailsDialog::setupUi() {
    setWindowTitle(tr("Certificate Details - %1").arg(cert_.alias));
    resize(500, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    subjectLabel_ = new QLabel(cert_.subject, this);
    formLayout->addRow(tr("Subject:"), subjectLabel_);
    
    issuerLabel_ = new QLabel(cert_.issuer, this);
    formLayout->addRow(tr("Issuer:"), issuerLabel_);
    
    serialLabel_ = new QLabel(cert_.serialNumber, this);
    formLayout->addRow(tr("Serial Number:"), serialLabel_);
    
    fingerprintLabel_ = new QLabel(cert_.fingerprint, this);
    formLayout->addRow(tr("Fingerprint (SHA-256):"), fingerprintLabel_);
    
    algorithmLabel_ = new QLabel(cert_.signatureAlgorithm, this);
    formLayout->addRow(tr("Signature Algorithm:"), algorithmLabel_);
    
    validFromLabel_ = new QLabel(cert_.validFrom.toString(), this);
    formLayout->addRow(tr("Valid From:"), validFromLabel_);
    
    validUntilLabel_ = new QLabel(cert_.validUntil.toString(), this);
    formLayout->addRow(tr("Valid Until:"), validUntilLabel_);
    
    keySizeLabel_ = new QLabel(QString::number(cert_.keySize), this);
    formLayout->addRow(tr("Key Size:"), keySizeLabel_);
    
    statusLabel_ = new QLabel(cert_.isValid ? 
        tr("<font color='green'>Valid</font>") : 
        tr("<font color='red'>Invalid</font>"), this);
    formLayout->addRow(tr("Status:"), statusLabel_);
    
    layout->addLayout(formLayout);
    
    // Purposes
    layout->addWidget(new QLabel(tr("Key Usages:"), this));
    purposesEdit_ = new QTextEdit(this);
    purposesEdit_->setReadOnly(true);
    purposesEdit_->setMaximumHeight(60);
    purposesEdit_->setText(cert_.purposes.join("\n"));
    layout->addWidget(purposesEdit_);
    
    // Subject Alternative Names
    layout->addWidget(new QLabel(tr("Subject Alternative Names:"), this));
    sanEdit_ = new QTextEdit(this);
    sanEdit_->setReadOnly(true);
    sanEdit_->setMaximumHeight(60);
    sanEdit_->setText(cert_.subjectAlternativeNames.join("\n"));
    layout->addWidget(sanEdit_);
    
    layout->addStretch();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* chainBtn = new QPushButton(tr("View Chain"), this);
    connect(chainBtn, &QPushButton::clicked, this, &CertificateDetailsDialog::onViewChain);
    btnLayout->addWidget(chainBtn);
    
    auto* verifyBtn = new QPushButton(tr("Verify"), this);
    connect(verifyBtn, &QPushButton::clicked, this, &CertificateDetailsDialog::onVerify);
    btnLayout->addWidget(verifyBtn);
    
    btnLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &CertificateDetailsDialog::onExport);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void CertificateDetailsDialog::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Certificate"),
        cert_.alias + ".pem",
        tr("PEM Files (*.pem);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Certificate exported successfully."));
    }
}

void CertificateDetailsDialog::onViewChain() {
    QMessageBox::information(this, tr("Certificate Chain"),
        tr("Certificate chain would be displayed here."));
}

void CertificateDetailsDialog::onVerify() {
    QMessageBox::information(this, tr("Verify"),
        tr("Certificate verification would be performed."));
}

// ============================================================================
// Generate CSR Dialog
// ============================================================================

GenerateCSRDialog::GenerateCSRDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void GenerateCSRDialog::setupUi() {
    setWindowTitle(tr("Generate Certificate Signing Request"));
    resize(500, 600);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* tabWidget = new QTabWidget(this);
    
    // Subject tab
    auto* subjectWidget = new QWidget(this);
    auto* subjectLayout = new QFormLayout(subjectWidget);
    
    cnEdit_ = new QLineEdit(this);
    subjectLayout->addRow(tr("Common Name (CN):*"), cnEdit_);
    
    orgEdit_ = new QLineEdit(this);
    subjectLayout->addRow(tr("Organization (O):"), orgEdit_);
    
    ouEdit_ = new QLineEdit(this);
    subjectLayout->addRow(tr("Organizational Unit (OU):"), ouEdit_);
    
    cityEdit_ = new QLineEdit(this);
    subjectLayout->addRow(tr("City/Locality (L):"), cityEdit_);
    
    stateEdit_ = new QLineEdit(this);
    subjectLayout->addRow(tr("State/Province (ST):"), stateEdit_);
    
    countryEdit_ = new QLineEdit(this);
    countryEdit_->setMaxLength(2);
    subjectLayout->addRow(tr("Country (C):"), countryEdit_);
    
    emailEdit_ = new QLineEdit(this);
    subjectLayout->addRow(tr("Email:"), emailEdit_);
    
    tabWidget->addTab(subjectWidget, tr("Subject"));
    
    // Key tab
    auto* keyWidget = new QWidget(this);
    auto* keyLayout = new QFormLayout(keyWidget);
    
    keySizeCombo_ = new QComboBox(this);
    keySizeCombo_->addItems({"2048", "3072", "4096"});
    keySizeCombo_->setCurrentIndex(1);
    keyLayout->addRow(tr("Key Size:"), keySizeCombo_);
    
    algorithmCombo_ = new QComboBox(this);
    algorithmCombo_->addItems({"RSA", "ECDSA", "Ed25519"});
    keyLayout->addRow(tr("Algorithm:"), algorithmCombo_);
    
    sanEdit_ = new QTextEdit(this);
    sanEdit_->setPlaceholderText(tr("Enter one SAN per line\nExample:\nDNS:example.com\nIP:192.168.1.1"));
    keyLayout->addRow(tr("Subject Alt Names:"), sanEdit_);
    
    tabWidget->addTab(keyWidget, tr("Key Options"));
    
    layout->addWidget(tabWidget);
    
    // Results
    auto* resultGroup = new QGroupBox(tr("Generated CSR and Private Key"), this);
    auto* resultLayout = new QVBoxLayout(resultGroup);
    
    csrEdit_ = new QTextEdit(this);
    csrEdit_->setPlaceholderText(tr("CSR will appear here..."));
    resultLayout->addWidget(new QLabel(tr("Certificate Signing Request:"), this));
    resultLayout->addWidget(csrEdit_);
    
    privateKeyEdit_ = new QTextEdit(this);
    privateKeyEdit_->setPlaceholderText(tr("Private key will appear here..."));
    resultLayout->addWidget(new QLabel(tr("Private Key:"), this));
    resultLayout->addWidget(privateKeyEdit_);
    
    layout->addWidget(resultGroup);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* generateBtn = new QPushButton(tr("Generate"), this);
    connect(generateBtn, &QPushButton::clicked, this, &GenerateCSRDialog::onGenerate);
    btnLayout->addWidget(generateBtn);
    
    btnLayout->addStretch();
    
    auto* copyBtn = new QPushButton(tr("Copy CSR"), this);
    connect(copyBtn, &QPushButton::clicked, this, &GenerateCSRDialog::onCopyToClipboard);
    btnLayout->addWidget(copyBtn);
    
    auto* saveCsrBtn = new QPushButton(tr("Save CSR"), this);
    connect(saveCsrBtn, &QPushButton::clicked, this, &GenerateCSRDialog::onSaveCSR);
    btnLayout->addWidget(saveCsrBtn);
    
    auto* saveKeyBtn = new QPushButton(tr("Save Key"), this);
    connect(saveKeyBtn, &QPushButton::clicked, this, &GenerateCSRDialog::onSavePrivateKey);
    btnLayout->addWidget(saveKeyBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void GenerateCSRDialog::onGenerate() {
    // Simulate CSR generation
    csrEdit_->setText("-----BEGIN CERTIFICATE REQUEST-----\n"
                      "MIICvDCCAaQCAQAwdzELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3JuaWEx\n"
                      "FjAUBgNVBAcMDU1vdW50YWluIFZpZXcxEzARBgNVBAoMClNjcmF0Y2hCaXJkMRMw\n"
                      "EQYDVQQLDApEYXRhYmFzZXMxEzARBgNVBAMMCnNjcmF0Y2hiaXJkMIIBIjANBgkq\n"
                      "...\n"
                      "-----END CERTIFICATE REQUEST-----");
    
    privateKeyEdit_->setText("-----BEGIN PRIVATE KEY-----\n"
                              "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC7JLJHM+CIq1r0\n"
                              "...\n"
                              "-----END PRIVATE KEY-----");
}

void GenerateCSRDialog::onSaveCSR() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save CSR"),
        "request.csr",
        tr("CSR Files (*.csr *.pem);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Save"), tr("CSR saved."));
    }
}

void GenerateCSRDialog::onSavePrivateKey() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Private Key"),
        "private.key",
        tr("Key Files (*.key *.pem);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Save"), tr("Private key saved."));
    }
}

void GenerateCSRDialog::onCopyToClipboard() {
    QMessageBox::information(this, tr("Copy"), tr("CSR copied to clipboard."));
}

// ============================================================================
// SSL Configuration Dialog
// ============================================================================

SSLConfigurationDialog::SSLConfigurationDialog(ConnectionSecurity* config, QWidget* parent)
    : QDialog(parent)
    , config_(config) {
    setupUi();
}

void SSLConfigurationDialog::setupUi() {
    setWindowTitle(tr("SSL Configuration"));
    resize(450, 450);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    sslEnabledCheck_ = new QCheckBox(tr("Enable SSL/TLS"), this);
    sslEnabledCheck_->setChecked(config_->sslEnabled);
    formLayout->addRow(sslEnabledCheck_);
    
    sslModeCombo_ = new QComboBox(this);
    sslModeCombo_->addItems({"disable", "allow", "prefer", "require", "verify-ca", "verify-full"});
    sslModeCombo_->setCurrentText(config_->sslMode);
    formLayout->addRow(tr("SSL Mode:"), sslModeCombo_);
    
    verifyServerCheck_ = new QCheckBox(tr("Verify server certificate"), this);
    verifyServerCheck_->setChecked(config_->verifyServerCert);
    formLayout->addRow(verifyServerCheck_);
    
    verifyHostnameCheck_ = new QCheckBox(tr("Verify hostname matches certificate"), this);
    verifyHostnameCheck_->setChecked(config_->verifyHostname);
    formLayout->addRow(verifyHostnameCheck_);
    
    // CA Certificate
    auto* caLayout = new QHBoxLayout();
    caCertEdit_ = new QLineEdit(config_->caCertPath, this);
    caLayout->addWidget(caCertEdit_);
    
    auto* caBtn = new QPushButton(tr("Browse..."), this);
    connect(caBtn, &QPushButton::clicked, this, &SSLConfigurationDialog::onBrowseCA);
    caLayout->addWidget(caBtn);
    
    formLayout->addRow(tr("CA Certificate:"), caLayout);
    
    // Client Certificate
    auto* certLayout = new QHBoxLayout();
    clientCertEdit_ = new QLineEdit(config_->clientCertPath, this);
    certLayout->addWidget(clientCertEdit_);
    
    auto* certBtn = new QPushButton(tr("Browse..."), this);
    connect(certBtn, &QPushButton::clicked, this, &SSLConfigurationDialog::onBrowseCert);
    certLayout->addWidget(certBtn);
    
    formLayout->addRow(tr("Client Certificate:"), certLayout);
    
    // Client Key
    auto* keyLayout = new QHBoxLayout();
    clientKeyEdit_ = new QLineEdit(config_->clientKeyPath, this);
    keyLayout->addWidget(clientKeyEdit_);
    
    auto* keyBtn = new QPushButton(tr("Browse..."), this);
    connect(keyBtn, &QPushButton::clicked, this, &SSLConfigurationDialog::onBrowseKey);
    keyLayout->addWidget(keyBtn);
    
    formLayout->addRow(tr("Client Private Key:"), keyLayout);
    
    // CRL
    auto* crlLayout = new QHBoxLayout();
    crlEdit_ = new QLineEdit(config_->crlPath, this);
    crlLayout->addWidget(crlEdit_);
    
    auto* crlBtn = new QPushButton(tr("Browse..."), this);
    connect(crlBtn, &QPushButton::clicked, this, &SSLConfigurationDialog::onBrowseCRL);
    crlLayout->addWidget(crlBtn);
    
    formLayout->addRow(tr("CRL File:"), crlLayout);
    
    // Cipher list
    cipherEdit_ = new QLineEdit(config_->cipherList, this);
    formLayout->addRow(tr("Cipher List:"), cipherEdit_);
    
    // TLS Version
    tlsVersionCombo_ = new QComboBox(this);
    tlsVersionCombo_->addItems({tr("Auto (Recommended)"), "TLS 1.0", "TLS 1.1", "TLS 1.2", "TLS 1.3"});
    tlsVersionCombo_->setCurrentIndex(config_->tlsVersion);
    formLayout->addRow(tr("Minimum TLS Version:"), tlsVersionCombo_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* testBtn = new QPushButton(tr("Test Connection"), this);
    connect(testBtn, &QPushButton::clicked, this, &SSLConfigurationDialog::onTestConnection);
    btnLayout->addWidget(testBtn);
    
    btnLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &SSLConfigurationDialog::onSave);
    btnLayout->addWidget(saveBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void SSLConfigurationDialog::onBrowseCA() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select CA Certificate"),
        QString(),
        tr("Certificate Files (*.pem *.crt);;All Files (*.*)"));
    if (!fileName.isEmpty()) caCertEdit_->setText(fileName);
}

void SSLConfigurationDialog::onBrowseCert() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Client Certificate"),
        QString(),
        tr("Certificate Files (*.pem *.crt);;All Files (*.*)"));
    if (!fileName.isEmpty()) clientCertEdit_->setText(fileName);
}

void SSLConfigurationDialog::onBrowseKey() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Private Key"),
        QString(),
        tr("Key Files (*.pem *.key);;All Files (*.*)"));
    if (!fileName.isEmpty()) clientKeyEdit_->setText(fileName);
}

void SSLConfigurationDialog::onBrowseCRL() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select CRL File"),
        QString(),
        tr("CRL Files (*.crl *.pem);;All Files (*.*)"));
    if (!fileName.isEmpty()) crlEdit_->setText(fileName);
}

void SSLConfigurationDialog::onTestConnection() {
    QMessageBox::information(this, tr("Test Connection"),
        tr("SSL connection test would be performed with current settings."));
}

void SSLConfigurationDialog::onSave() {
    config_->sslEnabled = sslEnabledCheck_->isChecked();
    config_->sslMode = sslModeCombo_->currentText();
    config_->verifyServerCert = verifyServerCheck_->isChecked();
    config_->verifyHostname = verifyHostnameCheck_->isChecked();
    config_->caCertPath = caCertEdit_->text();
    config_->clientCertPath = clientCertEdit_->text();
    config_->clientKeyPath = clientKeyEdit_->text();
    config_->crlPath = crlEdit_->text();
    config_->cipherList = cipherEdit_->text();
    config_->tlsVersion = tlsVersionCombo_->currentIndex();
    accept();
}

} // namespace scratchrobin::ui
