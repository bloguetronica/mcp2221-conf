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
#include <QStringList>
#include "common.h"
#include "mcp2221.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

// Definitions
const int CENTRAL_HEIGHT = 171;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEditVID->setValidator(new QRegExpValidator(QRegExp("[A-Fa-f\\d]+"), this));
    ui->lineEditPID->setValidator(new QRegExpValidator(QRegExp("[A-Fa-f\\d]+"), this));
    ui->lineEditVID->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    closeAboutDialog();  // See "common.h" and "common.cpp"
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    this->setFixedHeight(ui->menuBar->height() + CENTRAL_HEIGHT);
}

void MainWindow::on_actionAbout_triggered()
{
    showAboutDialog();  // See "common.h" and "common.cpp"
}

void MainWindow::on_lineEditPID_textEdited(const QString &text)
{
    int curPosition = ui->lineEditPID->cursorPosition();
    ui->lineEditPID->setText(text.toLower());
    ui->lineEditPID->setCursorPosition(curPosition);
    validateInput();
}

void MainWindow::on_lineEditVID_textEdited(const QString &text)
{
    int curPosition = ui->lineEditVID->cursorPosition();
    ui->lineEditVID->setText(text.toLower());
    ui->lineEditVID->setCursorPosition(curPosition);
    validateInput();
}

// Refreshes the combo box list
void MainWindow::refresh()
{
    int errcnt = 0;
    QString errstr;
    QStringList comboBoxList = {tr("Select device...")};
    comboBoxList += MCP2221::listDevices(vid_, pid_, errcnt, errstr);
    if (errcnt > 0) {
        QMessageBox::critical(this, tr("Critical Error"), tr("%1\nThis is a critical error and execution will be aborted.").arg(errstr));
        exit(EXIT_FAILURE);  // This error is critical because either libusb failed to initialize, or could not retrieve a list of devices
    } else {
        ui->comboBoxDevices->clear();
        ui->comboBoxDevices->addItems(comboBoxList);
    }
}

// Checks for valid user input, enabling or disabling the combo box and the "Refresh" button, accordingly
void MainWindow::validateInput()
{
    QString vidstr = ui->lineEditVID->text();
    QString pidstr = ui->lineEditPID->text();
    if (vidstr.size() == 4 && pidstr.size() == 4) {
        vid_ = static_cast<quint16>(vidstr.toUInt(nullptr, 16));
        pid_ = static_cast<quint16>(pidstr.toUInt(nullptr, 16));
        refresh();  // This has the "side effect" of disabling the "Open" button - Note that this is the intended behavior!
        ui->comboBoxDevices->setEnabled(true);
        ui->pushButtonRefresh->setEnabled(true);
    } else {
        ui->comboBoxDevices->setCurrentIndex(0);  // This also disables the "Open" button
        ui->comboBoxDevices->setEnabled(false);
        ui->pushButtonRefresh->setEnabled(false);
    }
}
