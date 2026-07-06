#pragma once

#include <QString>
#include <QStringList>

class RcloneLocator
{
public:
    static QString findRcloneBinary();
    static QStringList extraSearchPaths();
};
