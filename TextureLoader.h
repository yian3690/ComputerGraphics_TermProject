#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <GL/glew.h>
#include "stb_image.h"
#include <iostream>
#include <string>

// TextureLoader ���O�t�d���J���z�óB�z OpenGL ���z�ާ@
class TextureLoader {
public:
    // ���J���z��ơA�ϥ� stb_image.h
    GLuint LoadTexture(const char* filepath);
};

#endif // TEXTURE_LOADER_H
