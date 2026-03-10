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


#ifndef CONFIGURATORWINDOW_H
#define CONFIGURATORWINDOW_H

// Includes
#include <QMainWindow>
#include "mcp2221.h"

namespace Ui {
class ConfiguratorWindow;
}

class ConfiguratorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConfiguratorWindow(QWidget *parent = nullptr);
    ~ConfiguratorWindow();

    bool isViewEnabled();
    void openDevice(quint16 vid, quint16 pid, const QString &serialString);

private slots:
    void on_actionAbout_triggered();

private:
    Ui::ConfiguratorWindow *ui;
    MCP2221 mcp2221_;
    QString errmsg_, serialString_;
    bool err_, viewEnabled_ = false;

    void disableView();
    void handleError();
    void readDeviceConfiguration();
    void validateOperation(const QString &operation, int errcnt, QString errstr);
};

#endif  // CONFIGURATORWINDOW_H
