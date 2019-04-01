#ifndef CANVAS_H
#define CANVAS_H
#include <memory>
#include <cstddef>
#include <cstdint>
#include <QMutex>

class Dataset;

class Canvas
{
public:
    Canvas();
    Canvas(Dataset *ds, int key, int gap, int bytesPerPixel);
    int width = 0, height = 0;
    size_t pixelStride = 0;
    size_t rowStride = 0;
    size_t size = 0;
    std::shared_ptr<uint8_t> image = nullptr;
    static QMutex _mutex;

    bool saveToTiff(const char* filePath);
    bool saveToPGM16(const char* filePath);
};

#endif // CANVAS_H
