#include "io.h"

Image load(std::string file_name) {
    auto result = Image();
    result.pixels = stbi_load(file_name.c_str(), &result.width, &result.height, &result.channels, DEFAULT_CHANNEL_COUNT);
    return result;
}
