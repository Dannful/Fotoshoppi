#include "image.h"

#define STB_IMAGE_IMPLEMENTATION

void Image::load_image() {
    pixels = stbi_load(file_name.c_str(), &width, &height, &channels, DEFAULT_CHANNEL_COUNT);
}

void Image::unload() {
    stbi_image_free(pixels);
}
