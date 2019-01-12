#ifndef INDEXEDFILE_H
#define INDEXEDFILE_H

#include <QFileInfo>
#include <QSet>
struct IndexedFile
{
    IndexedFile() {}
    IndexedFile(QFileInfo file): file(file) {}
    IndexedFile(QFileInfo file, QSet<QString> trigrams): file(file), trigrams(trigrams) {}
    QFileInfo file;
    QSet<QString> trigrams;
    bool valid;
};
#endif // INDEXEDFILE_H

