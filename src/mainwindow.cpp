/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>

#include "mainwindow.h"
#include "mdichild.h"
#include "burnconfigdialog.h"
#include "avroutputdialog.h"
#include "donatorsdialog.h"
#include "preferencesdialog.h"
#include "fusesdialog.h"
#include "downloaddialog.h"
#include "stamp-eepe.h"

#define DONATE_STR "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=TGT92W338DPGN&lc=IL&item_name=Erez%20Raviv&item_number=eePe&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted"
#define DNLD_VER_ER9X           0
#define DNLD_VER_ER9X_JETI      1
#define DNLD_VER_ER9X_FRSKY     2
#define DNLD_VER_ER9X_ARDUPILOT 3
#define ER9X_URL   "http://er9x.googlecode.com/svn/trunk/er9x.hex"
#define ER9X_JETI_URL   "http://er9x.googlecode.com/svn/trunk/er9x-jeti.hex"
#define ER9X_FRSKY_URL   "http://er9x.googlecode.com/svn/trunk/er9x-frsky.hex"
#define ER9X_ARDUPILOT_URL   "http://er9x.googlecode.com/svn/trunk/er9x-ardupilot.hex"
#define ER9X_STAMP "http://er9x.googlecode.com/svn/trunk/src/stamp-er9x.h"
#define EEPE_URL   "http://eepe.googlecode.com/svn/trunk/eePeInstall.exe"
#define EEPE_STAMP "http://eepe.googlecode.com/svn/trunk/src/stamp-eepe.h"

MainWindow::MainWindow()
{
    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);
    connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(updateMenus()));
    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateMenus();

    readSettings();

    setWindowTitle(tr("eePe - EEPROM Editor"));
    setUnifiedTitleAndToolBarOnMac(true);
    this->setWindowIcon(QIcon(":/icon.ico"));

    checkForUpdates(false);


    QStringList strl = QApplication::arguments();
    QString str;
    if(strl.count()>1) str = strl[1];
    if(!str.isEmpty())
    {
        MdiChild *child = createMdiChild();
        if (child->loadFile(str)) {
            statusBar()->showMessage(tr("File loaded"), 2000);
            child->show();
            if(!child->parentWidget()->isMaximized() && !child->parentWidget()->isMinimized()) child->parentWidget()->resize(400,500);
        }
    }
}

void MainWindow::checkForUpdates(bool ignoreSettings)
{
    showcheckForUpdatesResult = ignoreSettings;
    check1done = true;
    check2done = true;

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    if(checkER9X || ignoreSettings)
    {
        manager1 = new QNetworkAccessManager(this);
        connect(manager1, SIGNAL(finished(QNetworkReply*)),this, SLOT(reply1Finished(QNetworkReply*)));
        manager1->get(QNetworkRequest(QUrl(ER9X_STAMP)));
        check1done = false;
    }

    if(checkEEPE || ignoreSettings)
    {
        manager2 = new QNetworkAccessManager(this);
        connect(manager2, SIGNAL(finished(QNetworkReply*)),this, SLOT(reply2Finished(QNetworkReply*)));
        manager2->get(QNetworkRequest(QUrl(EEPE_STAMP)));
        check2done = false;
    }
}


void MainWindow::reply1Finished(QNetworkReply * reply)
{
    check1done = true;
    QByteArray qba = reply->readAll();
    int i = qba.indexOf("SVN_VERS");

    if(i>0)
    {
        bool cres;
        int rev = QString::fromAscii(qba.mid(i+17,3)).toInt(&cres);

        if(!cres)
        {
            QMessageBox::warning(this, "eePe", tr("Unable to check for updates."));
            return;
        }

        if(rev>currentER9Xrev)
        {
            QSettings settings("er9x-eePe", "eePe");

            QString dnldURL, baseFileName;
            switch (settings.value("download-version", 0).toInt())
            {
            case (DNLD_VER_ER9X_JETI):
                dnldURL = ER9X_JETI_URL;
                baseFileName = "er9x-jeti.hex";
                break;
            case (DNLD_VER_ER9X_FRSKY):
                dnldURL = ER9X_FRSKY_URL;
                baseFileName = "er9x-frsky.hex";
                break;
            case (DNLD_VER_ER9X_ARDUPILOT):
                dnldURL = ER9X_ARDUPILOT_URL;
                baseFileName = "er9x-ardupilot.hex";
                break;
            default:
                dnldURL = ER9X_URL;
                baseFileName = "er9x.hex";
                break;
            }

            showcheckForUpdatesResult = false; // update is available - do not show dialog
            int ret = QMessageBox::question(this, "eePe",tr("A new version of ER9x (%2) is available (r%1)<br>"
                                                                "Would you like to download it?").arg(rev).arg(baseFileName) ,
                                            QMessageBox::Yes | QMessageBox::No);

            if (ret == QMessageBox::Yes)
            {
                QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),settings.value("lastDir").toString() + "/" + baseFileName,tr("EEPROM hex files (*.hex);;EEPROM bin files (*.bin)"));
                if (fileName.isEmpty()) return;
                settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

                downloadDialog * dd = new downloadDialog(this,dnldURL,fileName);
                int res = dd->exec();
                if(res == QDialog::Accepted)
                {
                    currentER9Xrev = rev;
                    settings.setValue("currentER9Xrev", rev);
                }
            }

            if(ret == QMessageBox::No)
            {
                int res = QMessageBox::question(this, "eePe",tr("Ignore this version (r%1)?").arg(rev) ,
                                                QMessageBox::Yes | QMessageBox::No);
                if(res == QMessageBox::Yes)
                {
                    currentER9Xrev = rev;
                    settings.setValue("currentER9Xrev", rev);
                }
            }
        }
        else
        {
            if(showcheckForUpdatesResult && check1done && check2done)
                QMessageBox::information(this, "eePe", tr("No updates available at this time."));
        }
    }
    else
    {
        if(check1done && check2done)
            QMessageBox::warning(this, "eePe", tr("Unable to check for updates."));
    }
}

void MainWindow::reply2Finished(QNetworkReply * reply)
{
    check2done = true;

    QByteArray qba = reply->readAll();
    int i = qba.indexOf("SVN_VERS");

    if(i>0)
    {
        bool cres;
        int rev = QString::fromAscii(qba.mid(i+17,3)).toInt(&cres);

        if(!cres)
        {
            QMessageBox::warning(this, "eePe", tr("Unable to check for updates."));
            return;
        }

        if(rev>currentEEPErev)
        {
            showcheckForUpdatesResult = false; // update is available - do not show dialog
            int ret = QMessageBox::question(this, "eePe", tr("A new version of eePe is available (r%1)<br>"
                                                                "Would you like to download it?").arg(rev) ,
                                            QMessageBox::Yes | QMessageBox::No);

            QSettings settings("er9x-eePe", "eePe");

            if (ret == QMessageBox::Yes)
            {
                QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),settings.value("lastDir").toString() + "/eePeInstall.exe",tr("Executable (*.exe)"));
                if (fileName.isEmpty()) return;
                settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

                downloadDialog * dd = new downloadDialog(this,EEPE_URL,fileName);
                int res = dd->exec();
                if(res == QDialog::Accepted)
                {
                    int ret2 = QMessageBox::question(this, "eePe",tr("Would you like to launch the installer?") ,
                                                     QMessageBox::Yes | QMessageBox::No);
                    if (ret2 == QMessageBox::Yes)
                    {
                        if(QDesktopServices::openUrl(QUrl(QFileInfo(fileName).absoluteFilePath(), QUrl::TolerantMode)))
                            QApplication::exit();
                    }
                }
            }
        }
        else
        {
            if(showcheckForUpdatesResult && check1done && check2done)
                QMessageBox::information(this, "eePe", tr("No updates available at this time."));
        }
    }
    else
    {
        if(check1done && check2done)
            QMessageBox::warning(this, "eePe", tr("Unable to check for updates."));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        writeSettings();
        event->accept();
    }
}

void MainWindow::newFile()
{
    MdiChild *child = createMdiChild();
    child->newFile();
    child->show();

    if(!child->parentWidget()->isMaximized() && !child->parentWidget()->isMinimized()) child->parentWidget()->resize(400,500);
}

void MainWindow::open()
{
    QSettings settings("er9x-eePe", "eePe");
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open"),settings.value("lastDir").toString(),tr("EEPROM files (*.bin *.hex);;BIN files (*.bin);;HEX files (*.hex)"));
    if (!fileName.isEmpty())
    {
        settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

        QMdiSubWindow *existing = findMdiChild(fileName);
        if (existing) {
            mdiArea->setActiveSubWindow(existing);
            return;
        }

        MdiChild *child = createMdiChild();
        if (child->loadFile(fileName))
        {
            statusBar()->showMessage(tr("File loaded"), 2000);
            child->show();
            if(!child->parentWidget()->isMaximized() && !child->parentWidget()->isMinimized()) child->parentWidget()->resize(400,500);
        }
    }
}

void MainWindow::save()
{
    if (activeMdiChild() && activeMdiChild()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs()
{
    if (activeMdiChild() && activeMdiChild()->saveAs())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::preferences()
{
    preferencesDialog *pd = new preferencesDialog(this);
    pd->exec();
}

void MainWindow::cut()
{
    if (activeMdiChild())
    {
        activeMdiChild()->cut();
        updateMenus();
    }
}

void MainWindow::copy()
{
    if (activeMdiChild())
    {
        activeMdiChild()->copy();
        updateMenus();
    }
}

void MainWindow::paste()
{
    if (activeMdiChild())
        activeMdiChild()->paste();
}

void MainWindow::burnTo()
{
    if (activeMdiChild())
        activeMdiChild()->burnTo();
}

void MainWindow::simulate()
{
    if (activeMdiChild())
        activeMdiChild()->simulate();
}


void MainWindow::print()
{
    if (activeMdiChild())
        activeMdiChild()->print();
}

void MainWindow::burnFrom()
{
    burnConfigDialog bcd;
    QString avrdudeLoc = bcd.getAVRDUDE();
    QString tempDir    = QDir::tempPath();
    QString programmer = bcd.getProgrammer();
    QStringList args   = bcd.getAVRArgs();
    if(!bcd.getPort().isEmpty()) args << "-P" << bcd.getPort();


    QString tempFile = tempDir + "/temp.hex";
    QString str = "eeprom:r:" + tempFile + ":i"; // writing eeprom -> MEM:OPR:FILE:FTYPE"

    QStringList arguments;
    arguments << "-c" << programmer << "-p" << "m64" << args << "-U" << str;

    avrOutputDialog *ad = new avrOutputDialog(this, avrdudeLoc, arguments,tr("Read EEPROM From Tx")); //, AVR_DIALOG_KEEP_OPEN);
    ad->setWindowIcon(QIcon(":/images/read_eeprom.png"));
    int res = ad->exec();

    if(QFileInfo(tempFile).exists() && res)
    {
        MdiChild *child = createMdiChild();
        child->newFile();
        child->loadFile(tempFile,false);
        child->setModified();
        child->show();
        if(!child->parentWidget()->isMaximized() && !child->parentWidget()->isMinimized()) child->parentWidget()->resize(400,500);
    }
}

void MainWindow::burnExtenalToEEPROM()
{
    QSettings settings("er9x-eePe", "eePe");
    QString fileName = QFileDialog::getOpenFileName(this,tr("Choose file to write to EEPROM memory"),settings.value("lastDir").toString(),tr("FLASH files (*.bin *.hex);;BIN files (*.bin);;HEX files (*.hex)"));
    if (!fileName.isEmpty())
    {
        settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

        int ret = QMessageBox::question(this, "eePe", tr("Write %1 to EEPROM memory?").arg(QFileInfo(fileName).fileName()), QMessageBox::Yes | QMessageBox::No);
        if(ret!=QMessageBox::Yes) return;

        burnConfigDialog bcd;
        QString avrdudeLoc = bcd.getAVRDUDE();
        QString programmer = bcd.getProgrammer();
        QStringList args   = bcd.getAVRArgs();
        if(!bcd.getPort().isEmpty()) args << "-P" << bcd.getPort();

        QString str = "eeprom:w:" + fileName; // writing eeprom -> MEM:OPR:FILE:FTYPE"
        if(QFileInfo(fileName).suffix().toUpper()=="HEX") str += ":i";
        else if(QFileInfo(fileName).suffix().toUpper()=="BIN") str += ":r";
        else str += ":a";

        QStringList arguments;
        arguments << "-c" << programmer << "-p" << "m64" << args << "-U" << str;

        avrOutputDialog *ad = new avrOutputDialog(this, avrdudeLoc, arguments, "Write EEPROM To Tx", AVR_DIALOG_SHOW_DONE);
        ad->setWindowIcon(QIcon(":/images/write_eeprom.png"));
        ad->show();
    }
}

void MainWindow::burnToFlash()
{
    QSettings settings("er9x-eePe", "eePe");
    QString fileName = QFileDialog::getOpenFileName(this,tr("Choose file to write to flash memory"),settings.value("lastDir").toString(),tr("FLASH files (*.bin *.hex);;BIN files (*.bin);;HEX files (*.hex)"));
    if (!fileName.isEmpty())
    {
        settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

        int ret = QMessageBox::question(this, "eePe", tr("Write %1 to flash memory?").arg(QFileInfo(fileName).fileName()), QMessageBox::Yes | QMessageBox::No);
        if(ret!=QMessageBox::Yes) return;

        burnConfigDialog bcd;
        QString avrdudeLoc = bcd.getAVRDUDE();
        QString programmer = bcd.getProgrammer();
        QStringList args   = bcd.getAVRArgs();
        if(!bcd.getPort().isEmpty()) args << "-P" << bcd.getPort();

        QString str = "flash:w:" + fileName; // writing eeprom -> MEM:OPR:FILE:FTYPE"
        if(QFileInfo(fileName).suffix().toUpper()=="HEX") str += ":i";
        else if(QFileInfo(fileName).suffix().toUpper()=="BIN") str += ":r";
        else str += ":a";

        QStringList arguments;
        arguments << "-c" << programmer << "-p" << "m64" << args << "-U" << str;

        avrOutputDialog *ad = new avrOutputDialog(this, avrdudeLoc, arguments, "Write Flash To Tx", AVR_DIALOG_SHOW_DONE);
        ad->setWindowIcon(QIcon(":/images/write_flash.png"));
        ad->show();
    }
}


void MainWindow::burnExtenalFromEEPROM()
{
    QSettings settings("er9x-eePe", "eePe");
    QString fileName = QFileDialog::getSaveFileName(this,tr("Read EEPROM memory to File"),settings.value("lastDir").toString(),tr("HEX files (*.hex);;BIN files (*.bin);;FLASH files (*.bin *.hex)"));
    if (!fileName.isEmpty())
    {
        settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

        burnConfigDialog bcd;
        QString avrdudeLoc = bcd.getAVRDUDE();
        QString programmer = bcd.getProgrammer();
        QStringList args   = bcd.getAVRArgs();
        if(!bcd.getPort().isEmpty()) args << "-P" << bcd.getPort();


        QString str = "eeprom:r:" + fileName;
        if(QFileInfo(fileName).suffix().toUpper()=="HEX") str += ":i";
        else if(QFileInfo(fileName).suffix().toUpper()=="BIN") str += ":r";
        else str += ":a";

        QStringList arguments;
        arguments << "-c" << programmer << "-p" << "m64" << args << "-U" << str;

        avrOutputDialog *ad = new avrOutputDialog(this, avrdudeLoc, arguments, "Read EEPROM From Tx");
        ad->setWindowIcon(QIcon(":/images/read_eeprom.png"));
        ad->show();
    }

}

void MainWindow::burnFromFlash()
{
    QSettings settings("er9x-eePe", "eePe");
    QString fileName = QFileDialog::getSaveFileName(this,tr("Read Flash to File"),settings.value("lastDir").toString(),tr("HEX files (*.hex);;BIN files (*.bin);;FLASH files (*.bin *.hex)"));
    if (!fileName.isEmpty())
    {
        settings.setValue("lastDir",QFileInfo(fileName).dir().absolutePath());

        burnConfigDialog bcd;
        QString avrdudeLoc = bcd.getAVRDUDE();
        QString programmer = bcd.getProgrammer();
        QStringList args   = bcd.getAVRArgs();
        if(!bcd.getPort().isEmpty()) args << "-P" << bcd.getPort();


        QString str = "flash:r:" + fileName; // writing eeprom -> MEM:OPR:FILE:FTYPE"
        if(QFileInfo(fileName).suffix().toUpper()=="HEX") str += ":i";
        else if(QFileInfo(fileName).suffix().toUpper()=="BIN") str += ":r";
        else str += ":a";

        QStringList arguments;
        arguments << "-c" << programmer << "-p" << "m64" << args << "-U" << str;

        avrOutputDialog *ad = new avrOutputDialog(this, avrdudeLoc, arguments, "Read Flash From Tx");
        ad->setWindowIcon(QIcon(":/images/read_flash.png"));
        ad->show();
    }

}

void MainWindow::burnConfig()
{
    burnConfigDialog *bcd = new burnConfigDialog(this);
    bcd->exec();
}

void MainWindow::burnList()
{
    burnConfigDialog *bcd = new burnConfigDialog(this);
    bcd->listProgrammers();
}

void MainWindow::burnFuses()
{
    fusesDialog *fd = new fusesDialog(this);
    fd->exec();
}

void MainWindow::donators()
{
    donatorsDialog *dd = new donatorsDialog(this);
    dd->exec();
}

void MainWindow::about()
{
    QString aboutStr = "<center><img src=\":/images/eepe-title.png\"><br>";
    aboutStr.append(tr("Copyright") +" Erez Raviv &copy;2010<br>");
    aboutStr.append(QString("<a href='http://code.google.com/p/eepe/'>http://code.google.com/p/eepe/</a><br>Revision: %1, %2<br><br>").arg(currentEEPErev).arg(__DATE__));
    aboutStr.append(tr("If you've found this program and/or the er9x firmware useful please support by"));
    aboutStr.append(" <a href='" DONATE_STR "'>");
    aboutStr.append(tr("donating") + "</a></center>");

    QMessageBox::about(this, tr("About eePe"),aboutStr);
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeMdiChild() != 0);
    saveAct->setEnabled(hasMdiChild);
    saveAsAct->setEnabled(hasMdiChild);
    pasteAct->setEnabled(hasMdiChild ? activeMdiChild()->hasPasteData() : false);
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    burnToAct->setEnabled(hasMdiChild);
    separatorAct->setVisible(hasMdiChild);

    bool hasSelection = (activeMdiChild() &&
                         activeMdiChild()->hasSelection());
    cutAct->setEnabled(hasSelection);
    copyAct->setEnabled(hasSelection);
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        MdiChild *child = qobject_cast<MdiChild *>(windows.at(i)->widget());

        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1)
                               .arg(child->userFriendlyCurrentFile());
        } else {
            text = tr("%1 %2").arg(i + 1)
                              .arg(child->userFriendlyCurrentFile());
        }
        QAction *action  = windowMenu->addAction(text);
        action->setCheckable(true);
        action ->setChecked(child == activeMdiChild());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i));
    }
}

MdiChild *MainWindow::createMdiChild()
{
    MdiChild *child = new MdiChild;
    mdiArea->addSubWindow(child);

    connect(child, SIGNAL(copyAvailable(bool)),cutAct, SLOT(setEnabled(bool)));
    connect(child, SIGNAL(copyAvailable(bool)),copyAct, SLOT(setEnabled(bool)));
    connect(child, SIGNAL(copyAvailable(bool)),simulateAct, SLOT(setEnabled(bool)));
    connect(child, SIGNAL(copyAvailable(bool)),printAct, SLOT(setEnabled(bool)));

    return child;
}

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    preferencesAct = new QAction(QIcon(":/images/preferences.png"), tr("&Preferences..."), this);
    preferencesAct->setStatusTip(tr("Edit general preferences"));
    connect(preferencesAct, SIGNAL(triggered()), this, SLOT(preferences()));

    checkForUpdatesAct = new QAction(QIcon(":/images/update.png"), tr("&Check for updates..."), this);
    checkForUpdatesAct->setStatusTip(tr("Check for new version of eePe/er9x"));
    connect(checkForUpdatesAct, SIGNAL(triggered()), this, SLOT(checkForUpdates()));

//! [0]
    exitAct = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
//! [0]

    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));


    burnToAct = new QAction(QIcon(":/images/write_eeprom.png"), tr("&Write EEPROM To Tx"), this);
    burnToAct->setShortcut(tr("Ctrl+Alt+W"));
    burnToAct->setStatusTip(tr("Write EEPROM to transmitter"));
    connect(burnToAct,SIGNAL(triggered()),this,SLOT(burnTo()));

    burnFromAct = new QAction(QIcon(":/images/read_eeprom.png"), tr("&Read EEPROM From Tx"), this);
    burnFromAct->setShortcut(tr("Ctrl+Alt+R"));
    burnFromAct->setStatusTip(tr("Read EEPROM from transmitter"));
    connect(burnFromAct,SIGNAL(triggered()),this,SLOT(burnFrom()));

    burnToFlashAct = new QAction(QIcon(":/images/write_flash.png"), tr("Write Flash memory"), this);
    burnToFlashAct->setStatusTip(tr("Write flash memory to transmitter"));
    connect(burnToFlashAct,SIGNAL(triggered()),this,SLOT(burnToFlash()));

    burnExtenalToEEPROMAct = new QAction(QIcon(":/images/write_eeprom_file.png"), tr("Write EEPROM memory from file"), this);
    burnExtenalToEEPROMAct->setStatusTip(tr("Write EEPROM memory from file to transmitter"));
    connect(burnExtenalToEEPROMAct,SIGNAL(triggered()),this,SLOT(burnExtenalToEEPROM()));

    burnExtenalFromEEPROMAct = new QAction(QIcon(":/images/read_eeprom_file.png"), tr("Read EEPROM memory to file"), this);
    burnExtenalFromEEPROMAct->setStatusTip(tr("Read EEPROM memory from transmitter to file"));
    connect(burnExtenalFromEEPROMAct,SIGNAL(triggered()),this,SLOT(burnExtenalFromEEPROM()));

    burnFromFlashAct = new QAction(QIcon(":/images/read_flash.png"), tr("Read Flash memory"), this);
    burnFromFlashAct->setStatusTip(tr("Read flash memory to transmitter"));
    connect(burnFromFlashAct,SIGNAL(triggered()),this,SLOT(burnFromFlash()));

    burnConfigAct = new QAction(QIcon(":/images/configure.png"), tr("&Configure..."), this);
    burnConfigAct->setStatusTip(tr("Configure burning software"));
    connect(burnConfigAct,SIGNAL(triggered()),this,SLOT(burnConfig()));

    burnListAct = new QAction(QIcon(":/images/list.png"), tr("&List programmers"), this);
    burnListAct->setStatusTip(tr("List available programmers"));
    connect(burnListAct,SIGNAL(triggered()),this,SLOT(burnList()));

    burnFusesAct = new QAction(QIcon(":/images/fuses.png"), tr("&Fuses..."), this);
    burnFusesAct->setStatusTip(tr("Show fuses dialog"));
    connect(burnFusesAct,SIGNAL(triggered()),this,SLOT(burnFuses()));

    simulateAct = new QAction(QIcon(":/images/simulate.png"), tr("&Simulate"), this);
    simulateAct->setShortcut(tr("Alt+S"));
    simulateAct->setStatusTip(tr("Simulate selected model."));
    simulateAct->setEnabled(false);
    connect(simulateAct,SIGNAL(triggered()),this,SLOT(simulate()));

    printAct = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
    printAct->setShortcut(tr("Ctrl+P"));
    printAct->setStatusTip(tr("Print current model."));
    printAct->setEnabled(false);
    connect(printAct,SIGNAL(triggered()),this,SLOT(print()));

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeActiveSubWindow()));

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeAllSubWindows()));

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()),
            mdiArea, SLOT(activateNextSubWindow()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                 "window"));
    connect(previousAct, SIGNAL(triggered()),
            mdiArea, SLOT(activatePreviousSubWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(QIcon(":/icon.ico"), tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    donatorsAct = new QAction(QIcon(":/images/contributors.png"), tr("&Contributors"), this);
    donatorsAct->setStatusTip(tr("List er9x/eePe Contributors"));
    connect(donatorsAct, SIGNAL(triggered()), this, SLOT(donators()));

    switchLayoutDirectionAct = new QAction(QIcon(":/images/switch_dir.png"),  tr("Switch layout direction"), this);
    switchLayoutDirectionAct->setStatusTip(tr("Switch layout Left/Right"));
    connect(switchLayoutDirectionAct, SIGNAL(triggered()), this, SLOT(switchLayoutDirection()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(simulateAct);
    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(preferencesAct);
    fileMenu->addAction(switchLayoutDirectionAct);
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    burnMenu = menuBar()->addMenu(tr("&Burn"));
    burnMenu->addAction(burnToAct);
    burnMenu->addAction(burnFromAct);
    burnMenu->addSeparator();
    burnMenu->addAction(burnExtenalToEEPROMAct);
    burnMenu->addAction(burnExtenalFromEEPROMAct);
    burnMenu->addSeparator();
    burnMenu->addAction(burnToFlashAct);
    burnMenu->addAction(burnFromFlashAct);
    burnMenu->addSeparator();
    burnMenu->addAction(burnConfigAct);
    burnMenu->addAction(burnFusesAct);
    burnMenu->addAction(burnListAct);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    updateWindowMenu();
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(donatorsAct);
    helpMenu->addSeparator();
    helpMenu->addAction(checkForUpdatesAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addSeparator();
    fileToolBar->addAction(simulateAct);
    fileToolBar->addAction(printAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);

    burnToolBar = addToolBar(tr("Burn"));
    burnToolBar->addAction(burnToAct);
    burnToolBar->addAction(burnFromAct);
    burnToolBar->addSeparator();
    burnToolBar->addAction(burnExtenalToEEPROMAct);
    burnToolBar->addAction(burnExtenalFromEEPROMAct);
    burnToolBar->addSeparator();
    burnToolBar->addAction(burnToFlashAct);
    burnToolBar->addAction(burnFromFlashAct);
    burnToolBar->addSeparator();
    burnToolBar->addAction(burnConfigAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings("er9x-eePe", "eePe");
    bool maximized = settings.value("maximized", false).toBool();
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();

    currentER9Xrev = settings.value("currentER9Xrev", 1).toInt();
    currentEEPErev = SVN_VER_NUM;

    checkER9X = settings.value("startup_check_er9x", true).toBool();
    checkEEPE = settings.value("startup_check_eepe", true).toBool();

    if(maximized)
    {
         setWindowState(Qt::WindowMaximized);
    }
    else
    {
        move(pos);
        resize(size);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings("er9x-eePe", "eePe");

    settings.setValue("maximized", isMaximized());
    if(!isMaximized())
    {
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }
}

MdiChild *MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<MdiChild *>(activeSubWindow->widget());
    return 0;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
        if (mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return 0;
}

void MainWindow::switchLayoutDirection()
{
    if (layoutDirection() == Qt::LeftToRight)
        qApp->setLayoutDirection(Qt::RightToLeft);
    else
        qApp->setLayoutDirection(Qt::LeftToRight);
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}
