/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "console.h"
#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QTextStream>
#include <QFile>
#include <QDateTime>

//! [0]
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
//! [0]
    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    console->setParent(ui->widgetConsole);

    QHBoxLayout *layoutConsole = new QHBoxLayout();
    layoutConsole->addWidget(console);
    layoutConsole->setMargin(0);
    ui->widgetConsole->setLayout(layoutConsole);


    //setCentralWidget(console);
//! [1]
    serial = new QSerialPort(this);
//! [1]
    settings = new SettingsDialog;

    listATCmdHistory = new QStringList;
    fileLog = new QFile;

    QFile file("atcommand.txt");
    QTextStream in(&file);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //while (!file.atEnd())
        while (!in.atEnd())
        {
            QString strAsciiLineWithoutLF = QByteArray::fromHex (in.readLine().toLatin1());
            //QString strLineWithLF = file.readLine();
            if(false == listATCmdHistory->contains(strAsciiLineWithoutLF)){
                listATCmdHistory->append(strAsciiLineWithoutLF);
            }
        }
        file.close();
    }
    ui->listWidget->clear();
    ui->listWidget->addItems(*listATCmdHistory);

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);
    ui->actionSave_Command_History->setEnabled(true);

    status = new QLabel;
    ui->statusBar->addWidget(status);
    initActionsConnections();

    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);

//! [2]
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
//! [2]
    connect(console, &Console::getData, this, &MainWindow::writeData);
//! [3]
}
//! [3]

MainWindow::~MainWindow()
{
    delete settings;
    delete ui;
    delete status;
    delete serial;
    delete listATCmdHistory;
    if (true == fileLog->exists())
    {
       fileLog->close();
    }
    delete fileLog;
}

//! [4]
void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        console->setEnabled(true);
        console->setLocalEchoEnabled(p.localEchoEnabled);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        setWindowTitle(p.name);

        QString currentDateTime = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_");
        fileLog->setFileName("logs/" + currentDateTime + p.name + ".txt");
        if (false == fileLog->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        {
            QMessageBox::critical(this, tr("Error"), fileLog->errorString());
        }
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());

        showStatusMessage(tr("Open error"));
    }
}
//! [4]

//! [5]
void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    console->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
    setWindowTitle("Terminal");
}
//! [5]

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Terminal"),
                       tr("God's in his heaven, all is right with the world."));
}

//! [6]
void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
    if(true == ui->crlfcheckBox->isChecked()){
        serial->write("\r\n", 2);
    }
}
//! [6]

//! [7]
void MainWindow::readData()
{
    QByteArray data = serial->readAll();
    console->putData(data);
    //ui->textBrowser->insertPlainText(data);
    //ui->textBrowser->insertPlainText(data);
    //QTextCursor cursor = ui->textBrowser->textCursor();
    //cursor.movePosition(QTextCursor::End);
    //ui->textBrowser->setTextCursor(cursor);
    QTextStream out(fileLog);
    if (true == fileLog->exists())
    {
        out<<data<<endl;
    }
}
//! [7]

//! [8]
void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}
//! [8]
//!
//! [9]
void MainWindow::onClear()
{
    //ui->textBrowser->clear();
    console->clear();
}
//! [9]
//!
//! [10]
void MainWindow::onSaveCommandHistory()
{
    QFile file("atcommand.txt");

    if(false == file.exists()){
        return;
    }
    file.remove();
    QTextStream out(&file);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
    {
        for(int i = 0; i< listATCmdHistory->size(); i++)
        {
            QString stringHex = listATCmdHistory->at(i).toLatin1().toHex();
            out<<stringHex<<endl;
        }
        file.close();
    }
}
//! [10]
void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, settings, &SettingsDialog::show);
    connect(ui->actionClear, &QAction::triggered, this, &MainWindow::onClear);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionSave_Command_History, &QAction::triggered, this, &MainWindow::onSaveCommandHistory);
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}

void MainWindow::on_pushButton_clicked()
{
    QByteArray data;
    if(true == ui->hexCheckBox->isChecked()){
        data = QByteArray::fromHex (ui->textEdit->toPlainText().toLatin1());
    }
    else {
        data = ui->textEdit->toPlainText().toLatin1();
    }

    writeData(data);
    if(false == listATCmdHistory->contains(data)){
        listATCmdHistory->append(data);
        ui->listWidget->clear();
        ui->listWidget->addItems(*listATCmdHistory);
    }

    //ui->textEdit->insertPlainText(ui->textEdit->toPlainText().toLatin1().toHex());
}

void MainWindow::on_hexCheckBox_clicked(bool checked)
{
    if(true == checked) {
        QString strHex = ui->textEdit->toPlainText().toLatin1().toHex();
        QString strHexUpper = strHex.toUpper();
        QString strHexUpperWithSpace;
        // add space
        for(int i = 0;i < strHexUpper.length(); i += 2) {
            QString strTemp = strHexUpper.mid(i,2);
            strHexUpperWithSpace += strTemp;
            strHexUpperWithSpace += " ";
        }
        ui->textEdit->clear();
        ui->textEdit->setText(strHexUpperWithSpace);
    }
    else {
        QString strAscii = QByteArray::fromHex (ui->textEdit->toPlainText().toLatin1());
        ui->textEdit->clear();
        ui->textEdit->setText(strAscii);
    }
}

void MainWindow::on_listWidget_doubleClicked(const QModelIndex &index)
{
    if(false == ui->delCheckBox->isChecked()){
        writeData(ui->listWidget->currentItem()->text().toLatin1());
    }
    else {
        listATCmdHistory->removeOne(ui->listWidget->currentItem()->text().toLatin1());
        ui->listWidget->clear();
        ui->listWidget->addItems(*listATCmdHistory);
    }
}

