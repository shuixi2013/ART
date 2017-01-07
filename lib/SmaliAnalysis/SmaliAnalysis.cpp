//===- SmaliAnalysis.cpp - ART-GUI Analysis engine -------------*- C++ -*-===//
//
//                     ANDROID REVERSE TOOLKIT
//
// This file is distributed under the GNU GENERAL PUBLIC LICENSE
// V3 License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This file defines project tab in workspace tab bar. It helps user to
// choose project to open or change project setting.
//
//===----------------------------------------------------------------------===//

#include "SmaliAnalysis/SmaliAnalysis.h"

#include <utils/ProjectInfo.h>

#include <fstream>
#include <QtCore/QObject>
#include <QDebug>
#include <QDirIterator>
#include <QDir>

using namespace std;

SmaliAnalysis *SmaliAnalysis::instance ()
{
    static SmaliAnalysis* mPtr = nullptr;
    if(mPtr == nullptr) {
        mPtr = new SmaliAnalysis;
    }
    return mPtr;
}

SmaliAnalysis::SmaliAnalysis()
    : m_dirIcon(":/images/treeOpen.png"),
      m_classIcon(":/images/class.png"),
      m_fieldIcon(":/images/field.png"),
      m_methodIcon(":/images/method.png")
{
}

SmaliAnalysis::~SmaliAnalysis() {
}

void SmaliAnalysis::addSourcePath(QString source) {
    m_sourceDir << source;

    SmaliAnalysisThread* thread = new SmaliAnalysisThread(source, this);
    connect(thread, &SmaliAnalysisThread::fileAnalysisFinished, this, &SmaliAnalysis::onFileAnalysisFinished);
    thread->start();
}

void SmaliAnalysis::clear() {
    removeAllSmaliFile();
    removeAllSmaliTree();
    m_sourceDir.clear();
}

void SmaliAnalysis::onFileAnalysisFinished (SmaliFile* file)
{
    qDebug() << "file analysis finished: " << file->sourceFile();
    addSmaliFileinToMap(file);
    addSmaliFileinToTree(file->sourceFile());
    fileAnalysisFinished(file->sourceFile());
}

void SmaliAnalysis::addSmaliFileinToMap(SmaliFile *smaliFile) {
    const QFileInfo fi(smaliFile->sourceFile());
    const QString &path = fi.path();
    if (!m_filenamesMap.contains(path))
        m_filenamesMap.insert(path, new FileNameDatasMap());
    m_filenamesMap.value(path)->insert(fi.fileName(), QSharedPointer<SmaliFile>(smaliFile));


}

bool SmaliAnalysis::removeSmaliFileFromMap(QString fileName) {
    bool found = false;
    const QFileInfo fi(fileName);
    if (FileNameDatasMap *files = m_filenamesMap.value(fi.path())) {
        FileNameDatasMap::iterator i = files->begin();
        auto iEnd = files->end();
        while (i != iEnd) {
            if (i.value()->sourceFile() == fileName) {
                files->erase(i);
                found = true;
                break;
            }
            ++i;
        }
        if (files->count() <= 0) {
            m_filenamesMap.remove(fi.path());
            delete files;
        }
    }
    return found;
}

// SmaliAnalysisThread
//
//int SmaliAnalysis::columnCount(const QModelIndex &parent) const
//{
//    if (parent.isValid())
//        return static_cast<SmaliTreeItem*>(parent.internalPointer())->columnCount();
//    else
//        return rootItem->columnCount();
//}
//
//QVariant SmaliAnalysis::data(const QModelIndex &index, int role) const
//{
//    if (!index.isValid())
//        return QVariant();
//
//    SmaliTreeItem *item = static_cast<SmaliTreeItem*>(index.internalPointer());
//    return item->data(index, role);
//}
//
//Qt::ItemFlags SmaliAnalysis::flags(const QModelIndex &index) const
//{
//    if (!index.isValid())
//        return 0;
//
//    return QAbstractItemModel::flags(index);
//}
//
//QModelIndex SmaliAnalysis::index(int row, int column, const QModelIndex &parent)
//const
//{
//    if (!hasIndex(row, column, parent))
//        return QModelIndex();
//
//    SmaliTreeItem *parentItem;
//
//    if (!parent.isValid())
//        parentItem = rootItem;
//    else
//        parentItem = static_cast<SmaliTreeItem*>(parent.internalPointer());
//
//    SmaliTreeItem *childItem = parentItem->child(row);
//    if (childItem)
//        return createIndex(row, column, childItem);
//    else
//        return QModelIndex();
//}
//
//QModelIndex SmaliAnalysis::parent(const QModelIndex &index) const
//{
//    if (!index.isValid())
//        return QModelIndex();
//
//    SmaliTreeItem *childItem = static_cast<SmaliTreeItem*>(index.internalPointer());
//    SmaliTreeItem *parentItem = childItem->parentItem();
//
//    if (parentItem == rootItem)
//        return QModelIndex();
//
//    return createIndex(parentItem->row(), 0, parentItem);
//}
//
//int SmaliAnalysis::rowCount(const QModelIndex &parent) const
//{
//    SmaliTreeItem *parentItem;
//    if (parent.column() > 0)
//        return 0;
//
//    if (!parent.isValid())
//        parentItem = rootItem;
//    else
//        parentItem = static_cast<SmaliTreeItem*>(parent.internalPointer());
//
//    return parentItem->childCount();
//}

void SmaliAnalysis::removeAllSmaliFile() {
    auto end = m_filenamesMap.constEnd();
    for (auto it = m_filenamesMap.constBegin(); it != end; ++it) {
        FileNameDatasMap *files = it.value();
        delete files;
    }
    m_filenamesMap.clear();
}

void SmaliAnalysis::addSmaliFileinToTree(QString filepath) {
    QFileInfo fi(filepath);
    auto filedata = getSmaliFile(fi.absoluteFilePath());
    if(filedata.isNull()) {
        return;
    }

    auto parent = findChildByFullPath(fi.absolutePath(), true);
    auto child = findChild(parent, fi.baseName(), true);
    child->setIcon(m_classIcon);
    child->removeRows(0, child->rowCount());

    for(auto i = 0, count = filedata->fieldCount(); i < count; i++) {
        auto field = filedata->field(i);
        auto item = new QStandardItem(m_fieldIcon, field->m_name);
        item->setEditable(false);
        child->appendRow(item);
    }
    for(auto i = 0, count = filedata->methodCount(); i < count; i++) {
        auto method = filedata->method(i);
        auto item = new QStandardItem(m_methodIcon, method->m_name);
        item->setEditable(false);
        child->appendRow(item);
    }
}

QSharedPointer<SmaliFile> SmaliAnalysis::getSmaliFile(QString filepath) {
    const QFileInfo fi(filepath);
    if (FileNameDatasMap *files = m_filenamesMap.value(fi.path())) {
        FileNameDatasMap::iterator i = files->begin();
        auto iEnd = files->end();
        while (i != iEnd) {
            return i.value();
        }
    }
    return QSharedPointer<SmaliFile>();
}

QStandardItem *SmaliAnalysis::findChildByFullPath(QString filepath, bool gen) {
    QString projectroot = QFileInfo(ProjectInfo::current()->getSourcePath()).absoluteFilePath();
    QDir dir(filepath);
    QStringList midpaths;
    while(dir.absolutePath() != projectroot) {
        midpaths << dir.dirName();
        dir.cdUp();
    }
    if(dir.absolutePath() != projectroot) {
        return nullptr;
    }
    auto parent = invisibleRootItem();
    for(auto it = midpaths.rbegin(), itEnd = midpaths.rend(); it != itEnd; it ++) {
        auto child = findChild(parent, *it, gen);
        if(child == nullptr) {
            return child;
        }
        parent = child;
    }
    return parent;
}

QStandardItem *SmaliAnalysis::findChild(QStandardItem *parent, QString name, bool gen) {
    for(auto i = 0, count = parent->rowCount(); i < count; i++) {
        auto child = parent->child(i, 0);
        if(child->text() == name) {
            return child;
        }
    }
    // chld not found
    if(!gen) {
        return nullptr;
    }
    auto child = new QStandardItem(m_dirIcon, name);
    child->setEditable(false);
    parent->appendRow(child);
    return child;
}


void SmaliAnalysis::removeSmaliFromTree(QString filepath) {
    QFileInfo fi(filepath);
    auto parent = findChildByFullPath(fi.absolutePath(), false);
    if(parent == nullptr) {
        return;
    }
    auto child = findChild(parent, fi.baseName(), false);
    if(child == nullptr) {
        return;
    }
    parent->removeRow(child->row());
}

void SmaliAnalysis::removeAllSmaliTree() {
    auto root = invisibleRootItem();
    root->removeRows(0, root->rowCount());
}

SmaliAnalysisThread::SmaliAnalysisThread (QString path, QObject *parent)
        : QThread (parent)
{
    m_src = path;
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void SmaliAnalysisThread::run ()
{
    QFileInfo fi(m_src);
    if(fi.isDir()) {
        parseDirectory(m_src);
    } else {
        parseFile(m_src);
    }
}

void SmaliAnalysisThread::parseFile(QString path) {
    auto* filedata = new SmaliFile(path);
    if(!filedata->isValid()) {
        delete filedata;
        return;
    }
    fileAnalysisFinished(filedata);
}

void SmaliAnalysisThread::parseDirectory(QString path) {
    QDirIterator it(path, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto filePath = it.next();
        auto info = QFileInfo(filePath);
        if(info.isFile () && info.fileName ().endsWith (".smali")) {
            parseFile(info.absoluteFilePath());
        }
    }
}


