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
#include "common.h"
#include "configuratorwindow.h"
#include "ui_configuratorwindow.h"

ConfiguratorWindow::ConfiguratorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ConfiguratorWindow)
{
    ui->setupUi(this);
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
            //displayConfiguration(deviceConfiguration_, FULL_UPDATE);
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
    //deviceConfiguration_.manufacturer = mcp2221_.getManufacturerDesc(errcnt, errstr);
    //deviceConfiguration_.product = mcp2221_.getProductDesc(errcnt, errstr);
    validateOperation(tr("read device configuration"), errcnt, errstr);
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
