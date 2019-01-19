#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/QtConcurrentRun>
#include <QtConcurrent/QtConcurrentMap>
#include <QSet>
#include "indexedfile.h"
#include <QTreeWidgetItem>
#include <atomic>

using IndexedFileList = QList<IndexedFile>;
using QListInt = QList<qint64>;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QDir selectedDir;
    QFileSystemModel *model;
    QList<IndexedFile> indexedFiles;
    QMap<QString, QTreeWidgetItem*> outputMap;
    IndexedFile indexFile(QFileInfo file);
    std::atomic_bool indexing, searching;
    QFuture<void> future1, future2;
    QFuture<IndexedFile> futureIndexMap;
    QFuture<void> futureSearchMap;
    unsigned int trigramHash(QString trigram);
    QFileSystemWatcher systemWatcher;
    void indexDirectory(QDir directory);
    void searchDirectory(QString string);
    void searchFile(IndexedFile file, QString searchedStr);
    void clearWatcher();
private slots:
    void onStartIndexClicked();
    void onStartSearchClicked();
    void onCalculatedFile(QFileInfo file, QListInt positions);
    void onIndexingComplete();
    void onSearchComplete();
    void addToWatcher(QString filePath);
    void onSystemWatcherAlert(QString path);
signals:
    void calculatedFileSignal(QFileInfo file, QListInt positions);
    void indexingComplete();
    void fileIndexingComplete(QString filePath);
    void searchComplete();
};

#endif // MAINWINDOW_H
