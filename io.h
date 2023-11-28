#ifndef IO_H
#define IO_H

#include "definitions.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string>

Image load(std::string file_name);

#endif // IO_H
