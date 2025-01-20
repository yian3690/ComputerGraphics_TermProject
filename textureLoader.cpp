#include "TextureLoader.h"

// �ϥ� stb_image ���J���z�åͦ� OpenGL ���z��H
GLuint TextureLoader::LoadTexture(const char* filepath) {
    GLuint textureID;
    glGenTextures(1, &textureID); // ���ͯ��z��H

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

    if (data) {
        // �p�G���\���J���z�ƾ�
        std::cout << "Texture loaded successfully: "
            << "File Path = " << filepath
            << ", Width = " << width << ", Height = " << height
            << ", Channels = " << nrChannels << std::endl;

        GLenum format;
        // �ھڳq�D�ƿ�ܹ������榡
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        // �j�w���z
        glBindTexture(GL_TEXTURE_2D, textureID);

        // �ǰe�v����ƨ� OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // �ͦ� Mipmap
        glGenerateMipmap(GL_TEXTURE_2D);

        // �]�m���z�Ѽ�
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // ���񯾲z�ƾ�
        stbi_image_free(data);
    }
    else {
        // �p�G���J���ѡA��X���~�T��
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
    }

    return textureID; // ��^���z ID
}
