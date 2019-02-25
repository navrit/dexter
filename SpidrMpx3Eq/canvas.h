#ifndef CANVAS_H
#define CANVAS_H
#include <cstddef>
#include <cstdint>

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
    uint8_t* image = nullptr;

    bool saveToTiff(const char* filePath);
};

#endif // CANVAS_H
