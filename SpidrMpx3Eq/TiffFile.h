#ifndef TIFFFILE_H
#define TIFFFILE_H

#include <QObject>
#include <stdint.h>

class TiffFile : public QObject
{
    Q_OBJECT
public:
    explicit TiffFile(QObject *parent = nullptr);
    static bool saveToTiff32_512(const char* filePath, const int* pixels);
    static bool saveToTiff32_256(const char* filePath, const int* pixels);
    static bool saveToTiff32_516(const char* filePath, const int* pixels);
    static bool saveToTiff32_260(const char* filePath, const int* pixels);
    // const QList<int> thresholds, const bool crossCorrection, const bool spatialCorrectionOnly

private:
    const static int SAMPLES_PER_PIXEL = 1; // This is greyscale - 3 for RGB, 4 for RGBA
    const static int BPP = 32; // Bits per pixel - gives ~4 billion per pixel upper limit before loss of signal

};

#endif // TIFFFILE_H
