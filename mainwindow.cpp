#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
const qint64 ONE_READ_CHARS = 1024*1024;
const int MAX_FOUND_POS = 1000;
const int MAX_TRIGRAM_NUM = 40000;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<IndexedFile>("IndexedFile");
    qRegisterMetaType<IndexedFileList>("IndexedFileList");
    qRegisterMetaType<QListInt>("QListInt");
    ui->setupUi(this);
    selectedDir = QDir("");
    model = new QFileSystemModel();
    model->setRootPath(QDir::currentPath());
    ui->fileView->setModel(model);
    ui->fileView->setUniformRowHeights(true);
    ui->outputView->setUniformRowHeights(true);
    ui->fileView->setColumnWidth(0, 400);
    ui->outputView->setColumnCount(3);
    ui->outputView->setHeaderLabels({"File name", "File path", "Position"});
    ui->outputView->setColumnWidth(1, 400);
    indexing = searching = false;
    connect(ui->indexButton, SIGNAL(clicked()), this, SLOT(onStartIndexClicked()), Qt::UniqueConnection);
    connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(onStartSearchClicked()), Qt::UniqueConnection);
    connect(this, SIGNAL(calculatedFileSignal(QFileInfo, QListInt)), this, SLOT(onCalculatedFile(QFileInfo, QListInt)), Qt::UniqueConnection);
    connect(this, SIGNAL(indexingComplete()), this, SLOT(onIndexingComplete()), Qt::UniqueConnection);
    connect(this, SIGNAL(searchComplete()), this, SLOT(onSearchComplete()), Qt::UniqueConnection);
}

MainWindow::~MainWindow()
{
    indexing = false;
    searching = false;
    futureIndexMap.cancel();
    futureSearchMap.cancel();
    future1.waitForFinished(), future2.waitForFinished();
    delete model;
    delete ui;
}

void MainWindow::onStartIndexClicked()
{
    if (searching) { return; }
    if (indexing)
    {
        indexing = false;
        futureIndexMap.cancel();
        future1.waitForFinished();
        indexedFiles.clear();
        return;
    }
    indexing = true;
    ui->statusLabel->setText("Status: indexing...");
    indexedFiles.clear();
    outputMap.clear();
    ui->outputView->clear();
    ui->indexButton->setText("Stop indexing");
    QModelIndexList indexList = ui->fileView->selectionModel()->selectedIndexes();
    if(indexList.empty()) { return; }
    QModelIndex index = indexList.front();
    QVariant data = model->filePath(index);
    selectedDir = QDir(data.toString());
    ui->folderLabel->setText("Current folder: " + data.toString());
    future1 = QtConcurrent::run(this, &MainWindow::indexDirectory, QDir(data.toString()));
}

void MainWindow::indexDirectory(QDir directory)
{
    QDirIterator it(directory, QDirIterator::Subdirectories);
    QFileInfoList files;
    while(it.hasNext() && indexing)
    {
        QString filePath = it.next();
        if(!it.fileInfo().isFile()) { continue; }
        files.push_back(filePath);
    }
    std::function<IndexedFile(QFileInfo)> functor = [this](QFileInfo file) -> IndexedFile { return this->indexFile(file); };
    futureIndexMap = QtConcurrent::mapped(files, functor);
    this->indexedFiles = futureIndexMap.results();
    emit indexingComplete();
}

void MainWindow::onIndexingComplete()
{
    if(indexing)
    {
        ui->statusLabel->setText("Status: indexing finished!");
    }
    else
    {
        ui->folderLabel->setText("Current folder: none");
        ui->statusLabel->setText("Status: indexing cancelled");
    }
    ui->indexButton->setText("Start indexing");
    indexing = false;
}
IndexedFile MainWindow::indexFile(QFileInfo file)
{
    IndexedFile indexedFile(file);
    indexedFile.valid = false;
    if(!indexing) { return indexedFile ; }
    QFile fileRead(file.absoluteFilePath());
    fileRead.open(QIODevice::ReadOnly);
    QTextStream stream(&fileRead);
    QString str = stream.read(2);
    while(!stream.atEnd())
    {
        if (!indexing || indexedFile.trigrams.size() > MAX_TRIGRAM_NUM)  { return indexedFile; }
        QString buffer = stream.read(ONE_READ_CHARS);
        for(int i = 0; i < buffer.size(); i++)
        {
            if (!indexing || indexedFile.trigrams.size() > MAX_TRIGRAM_NUM)  { return indexedFile; }
            str.append(buffer.at(i));
            if(str.back() == 0) { return indexedFile; }
            indexedFile.trigrams.insert(str);
            str = str.right(2);
        }
    }
    indexedFile.valid = true;
    return indexedFile;
}
void MainWindow::onStartSearchClicked()
{
    if(indexing) { return; }
    if(searching)
    {
        searching = false;
        futureSearchMap.cancel();
        future2.waitForFinished();
        return;
    }
    if(ui->lineEdit->text().size() == 0)
    {
        ui->statusLabel->setText("Status: please enter a string.");
        return;
    }
    searching = true;
    outputMap.clear();
    ui->outputView->clear();
    ui->statusLabel->setText("Status: searching...");
    ui->searchButton->setText("Stop searching");
    QString string = ui->lineEdit->text();
    future2 = QtConcurrent::run(this, &MainWindow::searchDirectory, string);
}

void MainWindow::searchDirectory(QString string)
{
    auto functor = [this, string](IndexedFile file) { searchFile(file, string); };
    futureSearchMap = QtConcurrent::map(indexedFiles, functor);
    futureSearchMap.waitForFinished();
    emit searchComplete();
}

void MainWindow::onSearchComplete()
{
    if(searching) { ui->statusLabel->setText("Status: Search finished!"); }
    else { ui->statusLabel->setText("Status: search cancelled"); }
    searching = false;
    ui->searchButton->setText("Start searching");
}

void MainWindow::searchFile(IndexedFile file, QString searchedStr)
{
    if(!file.valid || !searching) { return; }
    QString trigram = searchedStr.left(2);
    for(int i = 2; i < searchedStr.size(); i++)
    {
        if(!searching) { return; }
        trigram.append(searchedStr.at(i));
        if(!file.trigrams.contains(trigram)) { return; }
        trigram = trigram.right(2);
    }
    QList<qint64> positions;
    QFile fileRead(file.file.absoluteFilePath());
    fileRead.open(QIODevice::ReadOnly);
    QTextStream stream(&fileRead);
    QString candidate = stream.read(searchedStr.length() - 1);
    qint64 pos = 1;
    while(!stream.atEnd())
    {
        if(!searching) { return; }
        QString buffer = stream.read(ONE_READ_CHARS);
        for(int i = 0; i < buffer.size(); i++)
        {
            if(!searching) { return; }
            candidate.append(buffer.at(i));
            if(candidate == searchedStr)
            {
                positions.append(pos);
                if(positions.size() >= MAX_FOUND_POS)
                {
                    emit calculatedFileSignal(file.file, positions);
                    positions.clear();
                }
            }
            pos++;
            candidate = candidate.right(candidate.length() - 1);
        }
    }
    if(!positions.empty())
    {
        emit calculatedFileSignal(file.file, positions);
    }
}

void MainWindow::onCalculatedFile(QFileInfo file, QListInt positions)
{
    QTreeWidgetItem *fileItem;
    if(outputMap.contains(file.absoluteFilePath()))
    {
        fileItem = *outputMap.find(file.absoluteFilePath());
    }
    else
    {
        fileItem = new QTreeWidgetItem({ file.fileName(), file.absoluteFilePath(), ""});
        outputMap.insert(file.absoluteFilePath(), fileItem);
        ui->outputView->addTopLevelItem(fileItem);
    }
    for(auto it: positions)
    {
        QTreeWidgetItem *positionItem = new QTreeWidgetItem({"", "", QString::number(it)});
        fileItem->addChild(positionItem);
    }
}


