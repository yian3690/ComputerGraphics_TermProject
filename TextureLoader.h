#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <GL/glew.h>
#include "stb_image.h"
#include <iostream>
#include <string>

// TextureLoader 類別負責載入紋理並處理 OpenGL 紋理操作
class TextureLoader {
public:
    // 載入紋理函數，使用 stb_image.h
    GLuint LoadTexture(const char* filepath);
};

#endif // TEXTURE_LOADER_H
