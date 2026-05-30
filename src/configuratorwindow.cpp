/* MCP2221 Configurator - Version 1.0.0 for Debian Linux
   Copyright (c) 2026 Samuel Lourenço

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


// Includes
#include <QMessageBox>
#include <QRegExp>
#include <QRegExpValidator>
#include "common.h"
#include "mcp2221limits.h"
#include "serialgeneratordialog.h"
#include "configuratorwindow.h"
#include "ui_configuratorwindow.h"

// The following values are applicable to displayConfiguration()
const bool FULL_UPDATE= true;
const bool PARTIAL_UPDATE = false;

ConfiguratorWindow::ConfiguratorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ConfiguratorWindow)
{
    ui->setupUi(this);
    ui->lineEditVID->setValidator(new QRegExpValidator(QRegExp("[A-Fa-f\\d]+"), this));
    ui->lineEditPID->setValidator(new QRegExpValidator(QRegExp("[A-Fa-f\\d]+"), this));
}

ConfiguratorWindow::~ConfiguratorWindow()
{
    delete ui;
}

// Checks if the device window is currently fully enabled
bool ConfiguratorWindow::isViewEnabled()
{
    return viewEnabled_;
}

// Opens the device and prepares the corresponding window
void ConfiguratorWindow::openDevice(quint16 vid, quint16 pid, const QString &serialString)
{
    int err = mcp2221_.open(vid, pid, serialString);
    if (err == MCP2221::SUCCESS) {  // Device was successfully opened
        err_ = false;
        readDeviceConfiguration();
        if (err_) {  // If an error has occured
            handleError();
            this->deleteLater();  // Close window after the subsequent show() call
        } else {  // Device is now open
            this->setWindowTitle(tr("MCP2221 Device (S/N: %1)").arg(serialString));
            displayConfiguration(deviceConfiguration_, FULL_UPDATE);
            serialString_ = serialString;  // Pass the serial number
            viewEnabled_ = true;
        }
    } else if (err == MCP2221::ERROR_INIT) {  // Failed to initialize libusb
        QMessageBox::critical(this, tr("Critical Error"), tr("Could not initialize libusb.\n\nThis is a critical error and execution will be aborted."));
        exit(EXIT_FAILURE);  // This error is critical because libusb failed to initialize
    } else {
        if (err == MCP2221::ERROR_NOT_FOUND) {  // Failed to find device
            QMessageBox::critical(this, tr("Error"), tr("Could not find device."));
        } else if (err == MCP2221::ERROR_BUSY) {  // Failed to claim interface
            QMessageBox::critical(this, tr("Error"), tr("Device is currently unavailable.\n\nPlease confirm that the device is not in use."));
        }
        this->deleteLater();  // Close window after the subsequent show() call
    }
}

void ConfiguratorWindow::on_actionAbout_triggered()
{
    showAboutDialog();  // See "common.h" and "common.cpp"
}

void ConfiguratorWindow::on_actionSerialGeneratorEnable_toggled(bool checked)
{
    ui->pushButtonGenerateSerial->setEnabled(checked);
}

void ConfiguratorWindow::on_actionSerialGeneratorSettings_triggered()
{
    SerialGeneratorDialog serialGeneratorDialog(this);
    serialGeneratorDialog.setPrototypeSerialLineEditText(serialGeneratorSettings_.serialGenerator.prototypeSerial());
    serialGeneratorDialog.setDigitsCheckBox(serialGeneratorSettings_.serialGenerator.replaceWithDigits());
    serialGeneratorDialog.setUppercaseCheckBox(serialGeneratorSettings_.serialGenerator.replaceWithUppercaseLetters());
    serialGeneratorDialog.setLowercaseCheckBox(serialGeneratorSettings_.serialGenerator.replaceWithLowercaseLetters());
    serialGeneratorDialog.setExportToFileCheckBox(serialGeneratorSettings_.doExport);
    serialGeneratorDialog.setEnableCheckBox(serialGeneratorSettings_.enable);
    serialGeneratorDialog.setAutogenerateCheckBox(serialGeneratorSettings_.autogenerate);
    if (serialGeneratorDialog.exec() == QDialog::Accepted) {  // If the user clicks "OK"
        QString prototypeSerial = serialGeneratorDialog.prototypeSerialLineEditText();
        bool digit = serialGeneratorDialog.digitsCheckBoxIsChecked();
        bool upper = serialGeneratorDialog.uppercaseCheckBoxIsChecked();
        bool lower = serialGeneratorDialog.lowercaseCheckBoxIsChecked();
        if (!SerialGenerator::isValidPrototypeSerial(prototypeSerial) || !SerialGenerator::isValidReplaceMode(digit, upper, lower)) {  // If the user entered invalid settings (i.e. the prototype serial number does not contain a wildcard character or no replacement option was selected)
            QMessageBox::critical(this, tr("Error"), tr("The serial number generator settings are not valid and will not be applied.\n\nPlease verify that the prototype serial number contains at least one wildcard character (?) and that at least one replacement option is selected."));
        } else {  // Valid settings
            serialGeneratorSettings_.serialGenerator.setPrototypeSerial(prototypeSerial);
            serialGeneratorSettings_.serialGenerator.setReplaceMode(digit, upper, lower);
            serialGeneratorSettings_.doExport = serialGeneratorDialog.exportToFileCheckBoxIsChecked();
            serialGeneratorSettings_.enable = serialGeneratorDialog.enableCheckBoxIsChecked();  // No further verification required, because "checkBoxEnable" is automatically unchecked if "checkBoxExportToFile" gets unchecked
            serialGeneratorSettings_.autogenerate = serialGeneratorDialog.autogenerateCheckBoxIsChecked();  // Same as above, because "checkBoxAutogenerate" is automatically unchecked if "checkBoxExportToFile" gets unchecked
        }
    }
}

void ConfiguratorWindow::on_lineEditManufacturer_textEdited(QString text)  // The variable "text" is passed by value here, because it needs to be modified locally!
{
    int curPosition = ui->lineEditManufacturer->cursorPosition();
    ui->lineEditManufacturer->setText(text.replace('\n', ' '));
    ui->lineEditManufacturer->setCursorPosition(curPosition);
}

void ConfiguratorWindow::on_lineEditMaxPower_editingFinished()
{
    ui->lineEditMaxPower->setText(QString::number(2 * (ui->lineEditMaxPower->text().toInt() / 2)));  // This removes any leading zeros and also rounds to the previous even number, if the value is odd
}

void ConfiguratorWindow::on_lineEditMaxPower_textChanged(const QString &text)
{
    if (text.isEmpty()) {
        ui->lineEditMaxPower->setStyleSheet("background: rgb(255, 204, 0);");
    } else {
        ui->lineEditMaxPower->setStyleSheet("");
    }
}

void ConfiguratorWindow::on_lineEditMaxPower_textEdited(QString text)  // The variable "text" is passed by value here, because it needs to be modified locally!
{
    int maxPower = text.toInt();
    if (maxPower > 2 * MCP2221Limits::MAXPOW_MAX) {
        text.chop(1);
        ui->lineEditMaxPower->setText(text);
        maxPower /= 10;
    }
    ui->lineEditMaxPowerHex->setText(QString("%1").arg(maxPower / 2, 2, 16, QChar('0')));  // This will autofill with up to two leading zeros
}

void ConfiguratorWindow::on_lineEditMaxPowerHex_editingFinished()
{
    if (ui->lineEditMaxPowerHex->text().size() < 2) {
        ui->lineEditMaxPowerHex->setText(QString("%1").arg(ui->lineEditMaxPowerHex->text().toInt(nullptr, 16), 2, 16, QChar('0')));  // This will autofill with up to two leading zeros
    }
}

void ConfiguratorWindow::on_lineEditMaxPowerHex_textChanged(const QString &text)
{
    if (text.isEmpty()) {
        ui->lineEditMaxPowerHex->setStyleSheet("background: rgb(255, 204, 0);");
    } else {
        ui->lineEditMaxPowerHex->setStyleSheet("");
    }
}

void ConfiguratorWindow::on_lineEditMaxPowerHex_textEdited(const QString &text)
{
    int curPosition = ui->lineEditMaxPowerHex->cursorPosition();
    ui->lineEditMaxPowerHex->setText(text.toLower());
    int maxPowerHex = text.toInt(nullptr, 16);
    if (maxPowerHex > MCP2221Limits::MAXPOW_MAX) {
        maxPowerHex = MCP2221Limits::MAXPOW_MAX;
        ui->lineEditMaxPowerHex->setText(QString("%1").arg(MCP2221Limits::MAXPOW_MAX, 2, 16, QChar('0')));  // This will autofill with up to two leading zeros
    }
    ui->lineEditMaxPowerHex->setCursorPosition(curPosition);
    ui->lineEditMaxPower->setText(QString::number(2 * maxPowerHex));
}

void ConfiguratorWindow::on_lineEditNewPassword_textChanged(const QString &text)
{
    ui->pushButtonRevealNewPassword->setEnabled(!text.isEmpty());
    if (ui->lineEditNewPassword->text().isEmpty() || ui->lineEditNewPassword->text() != ui->lineEditRepeatPassword->text()) {
        ui->lineEditNewPassword->setStyleSheet("background: rgb(255, 204, 0);");
        ui->lineEditRepeatPassword->setStyleSheet("background: rgb(255, 204, 0);");
    } else {
        ui->lineEditNewPassword->setStyleSheet("");
        ui->lineEditRepeatPassword->setStyleSheet("");
    }
}

void ConfiguratorWindow::on_lineEditPID_textChanged(const QString &text)
{
    if (text.size() < 4 || text == "0000") {
        ui->lineEditPID->setStyleSheet("background: rgb(255, 204, 0);");
    } else {
        ui->lineEditPID->setStyleSheet("");
    }
}

void ConfiguratorWindow::on_lineEditPID_textEdited(const QString &text)
{
    int curPosition = ui->lineEditPID->cursorPosition();
    ui->lineEditPID->setText(text.toLower());
    ui->lineEditPID->setCursorPosition(curPosition);
}

void ConfiguratorWindow::on_lineEditProduct_textEdited(QString text)  // The variable "text" is passed by value here, because it needs to be modified locally!
{
    int curPosition = ui->lineEditProduct->cursorPosition();
    ui->lineEditProduct->setText(text.replace('\n', ' '));
    ui->lineEditProduct->setCursorPosition(curPosition);
}

void ConfiguratorWindow::on_lineEditRepeatPassword_textChanged(const QString &text)
{
    ui->pushButtonRevealRepeatPassword->setEnabled(!text.isEmpty());
    if (ui->lineEditNewPassword->text().isEmpty() || ui->lineEditNewPassword->text() != ui->lineEditRepeatPassword->text()) {
        ui->lineEditNewPassword->setStyleSheet("background: rgb(255, 204, 0);");
        ui->lineEditRepeatPassword->setStyleSheet("background: rgb(255, 204, 0);");
    } else {
        ui->lineEditNewPassword->setStyleSheet("");
        ui->lineEditRepeatPassword->setStyleSheet("");
    }
}

void ConfiguratorWindow::on_lineEditVID_textChanged(const QString &text)
{
    if (text.size() < 4 || text == "0000") {
        ui->lineEditVID->setStyleSheet("background: rgb(255, 204, 0);");
    } else {
        ui->lineEditVID->setStyleSheet("");
    }
}

void ConfiguratorWindow::on_lineEditVID_textEdited(const QString &text)
{
    int curPosition = ui->lineEditVID->cursorPosition();
    ui->lineEditVID->setText(text.toLower());
    ui->lineEditVID->setCursorPosition(curPosition);
}

void ConfiguratorWindow::on_pushButtonGenerateSerial_clicked()
{
    ui->lineEditSerial->setText(serialGeneratorSettings_.serialGenerator.generateSerial());
}

void ConfiguratorWindow::on_pushButtonRevealNewPassword_pressed()
{
    ui->lineEditNewPassword->setEchoMode(QLineEdit::Normal);
}

void ConfiguratorWindow::on_pushButtonRevealNewPassword_released()
{
    ui->lineEditNewPassword->setEchoMode(QLineEdit::Password);
}

void ConfiguratorWindow::on_pushButtonRevealRepeatPassword_pressed()
{
    ui->lineEditRepeatPassword->setEchoMode(QLineEdit::Normal);
}

void ConfiguratorWindow::on_pushButtonRevealRepeatPassword_released()
{
    ui->lineEditRepeatPassword->setEchoMode(QLineEdit::Password);
}

void ConfiguratorWindow::on_radioButtonPasswordProtected_toggled(bool checked)
{
    if (checked == false) {
        ui->lineEditNewPassword->clear();
        ui->lineEditRepeatPassword->clear();
    }
    ui->checkBoxDoNotChangePassword->setChecked(checked && deviceConfiguration_.securityOptions.password && !deviceConfiguration_.securityOptions.lock);
    ui->checkBoxDoNotChangePassword->setEnabled(checked && deviceConfiguration_.securityOptions.password && !deviceConfiguration_.securityOptions.lock);
    ui->lineEditNewPassword->setEnabled(checked && !ui->checkBoxDoNotChangePassword->isChecked());
    ui->lineEditNewPassword->setStyleSheet((checked && !ui->checkBoxDoNotChangePassword->isChecked()) ? "background: rgb(255, 204, 0);" : "");
    ui->pushButtonRevealNewPassword->setEnabled(checked && !ui->checkBoxDoNotChangePassword->isChecked() && !ui->lineEditNewPassword->text().isEmpty());
    ui->lineEditRepeatPassword->setEnabled(checked && !ui->checkBoxDoNotChangePassword->isChecked());
    ui->lineEditRepeatPassword->setStyleSheet((checked && !ui->checkBoxDoNotChangePassword->isChecked()) ? "background: rgb(255, 204, 0);" : "");
    ui->pushButtonRevealRepeatPassword->setEnabled(checked && !ui->checkBoxDoNotChangePassword->isChecked() && !ui->lineEditNewPassword->text().isEmpty());
}

// Partially disables configurator window
void ConfiguratorWindow::disableView()
{
    //ui->actionStatus->setEnabled(false);
    //ui->menuEEPROM->setEnabled(false);
    //ui->actionUsePassword->setEnabled(false);
    //ui->actionLoadConfiguration->setEnabled(false);
    //ui->actionClose->setText(tr("&Close Window"));
    //ui->centralWidget->setEnabled(false);
    viewEnabled_ = false;
}

// Updates all fields pertaining to the MCP2221 chip settings
void ConfiguratorWindow::displayChipSettings(const MCP2221::ChipSettings &chipSettings)
{
    displayUSBParameters(chipSettings.usb);
    // TODO
}

// This is the main display routine, used to display the given configuration, updating all fields accordingly
void ConfiguratorWindow::displayConfiguration(const Configuration &configuration, bool fullUpdate)
{
    displayManufacturer(configuration.manufacturer);
    displayProduct(configuration.product);
    displaySerial(configuration.serial);
    displaySecurityOptions(configuration.securityOptions);
    displayChipSettings(configuration.chipSettings);
    // TODO
    if (fullUpdate) {
        // TODO
        setGeneralSettingsEnabled(!configuration.securityOptions.lock);
        // TODO
    }
}

// Updates the manufacturer descriptor field
void ConfiguratorWindow::displayManufacturer(const QString &manufacturer)
{
    ui->lineEditManufacturer->setText(manufacturer);
}

// Updates the product descriptor field
void ConfiguratorWindow::displayProduct(const QString &product)
{
    ui->lineEditProduct->setText(product);
}

// TODO
void ConfiguratorWindow::displaySecurityOptions(const MCP2221::SecurityOptions &securityOptions)
{
    // TODO
}

// Updates the serial descriptor field
void ConfiguratorWindow::displaySerial(const QString &serial)
{
    ui->lineEditSerial->setText(serial);
}

// Updates all fields pertaining to USB parameters
void ConfiguratorWindow::displayUSBParameters(const MCP2221::USBParameters &usbParameters)
{
    ui->lineEditVID->setText(QString("%1").arg(usbParameters.vid, 4, 16, QChar('0')));  // This will autofill with up to four leading zeros
    ui->lineEditPID->setText(QString("%1").arg(usbParameters.pid, 4, 16, QChar('0')));  // Same as before
    ui->lineEditMaxPower->setText(QString::number(2 * usbParameters.maxpow));
    ui->lineEditMaxPowerHex->setText(QString("%1").arg(usbParameters.maxpow, 2, 16, QChar('0')));  // This will autofill with up to two leading zeros
    ui->comboBoxPowerMode->setCurrentIndex(usbParameters.powmode ? 1 : 0);
    ui->checkBoxRemoteWakeUpCapable->setChecked(usbParameters.rmwakeup);
}

// Determines the type of error and acts accordingly, always showing a message
void ConfiguratorWindow::handleError()
{
    if (mcp2221_.disconnected()) {
        disableView();  // Disable configurator window
        mcp2221_.close();
    }
    QMessageBox::critical(this, tr("Error"), errmsg_);
}

// This is the routine that reads the configuration from the MCP2221 NVRAM
void ConfiguratorWindow::readDeviceConfiguration()
{
    int errcnt = 0;
    QString errstr;
    deviceConfiguration_.manufacturer = mcp2221_.getManufacturerDesc(errcnt, errstr);
    deviceConfiguration_.product = mcp2221_.getProductDesc(errcnt, errstr);
    deviceConfiguration_.serial = mcp2221_.getSerialDesc(errcnt, errstr);
    deviceConfiguration_.chipSettings = mcp2221_.getChipSettings(errcnt, errstr);
    deviceConfiguration_.securityOptions = mcp2221_.getSecurityOptions(errcnt, errstr);
    validateOperation(tr("read device configuration"), errcnt, errstr);
}

// Enables or disables all fields pertaining to general settings
void ConfiguratorWindow::setGeneralSettingsEnabled(bool value)
{
    if (!value) {
        ui->actionSerialGeneratorEnable->setChecked(false);  // This also disables pushButtonGenerateSerial
    }
    ui->actionSerialGeneratorEnable->setEnabled(value);
    ui->lineEditManufacturer->setReadOnly(!value);
    ui->lineEditProduct->setReadOnly(!value);
    ui->lineEditSerial->setReadOnly(!value);
    ui->lineEditVID->setReadOnly(!value);
    ui->lineEditPID->setReadOnly(!value);
    ui->lineEditMaxPower->setReadOnly(!value);
    ui->lineEditMaxPowerHex->setReadOnly(!value);
    ui->comboBoxPowerMode->setEnabled(value);
    ui->checkBoxRemoteWakeUpCapable->setEnabled(value);
    ui->groupBoxSecurityOptions->setEnabled(value);
}

// Checks for errors and validates device operations
void ConfiguratorWindow::validateOperation(const QString &operation, int errcnt, QString errstr)  // The variable "errstr" is passed by value here, because it needs to be modified locally!
{
    if (errcnt > 0) {
        err_ = true;
        if (mcp2221_.disconnected()) {
            errmsg_ = tr("Device disconnected.\n\nPlease reconnect it and try again.");
        } else {
            errstr.chop(1);  // Remove the last character, which is always a newline
            errmsg_ = tr("Failed to %1. The operation returned the following error(s):\n– %2", "", errcnt).arg(operation, errstr.replace("\n", "\n– "));
        }
    }
}
