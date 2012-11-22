// Copyright (c) 2012 Oliver Lau <ola@ct.de>
// All rights reserved.

#include <QApplication>
#include "mainwindow.h"
#include "main.h"

const QString Company = "c't";
const QString AppName = "W-1";
const QString AppUrl = "https://github.com/ola-ct/w-1";
const QString AppAuthor = "Oliver Lau";
const QString AppAuthorMail = "ola@ct.de";
const QString AppVersionNoDebug = "0.2";
const QString AppMinorVersion = "";
#ifdef QT_NO_DEBUG
const QString AppVersion = AppVersionNoDebug + AppMinorVersion;
#else
const QString AppVersion = AppVersionNoDebug + AppMinorVersion + " [DEBUG]";
#endif


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
