#ifndef TIFFFILE_H
#define TIFFFILE_H

#include <QObject>
#include <stdint.h>

class TiffFile : public QObject
{
    Q_OBJECT
public:
    explicit TiffFile(QObject *parent = nullptr);
    static bool saveToTiff32(const char* filePath, const uint size, const int* pixels);
    // const QList<int> thresholds, const bool crossCorrection, const bool spatialCorrectionOnly

signals:

public slots:
};

#endif // TIFFFILE_H
