#ifndef INDEXEDFILE_H
#define INDEXEDFILE_H

#include <QFileInfo>
#include <QSet>
struct IndexedFile
{
    IndexedFile() {}
    IndexedFile(QFileInfo file): file(file) {}
    IndexedFile(QFileInfo file, QSet<unsigned int> trigrams): file(file), trigrams(trigrams) {}
    QFileInfo file;
    QSet<unsigned int> trigrams;
    bool valid;
};
#endif // INDEXEDFILE_H

