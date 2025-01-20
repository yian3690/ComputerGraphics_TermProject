#include "TextureLoader.h"

// 使用 stb_image 載入紋理並生成 OpenGL 紋理對象
GLuint TextureLoader::LoadTexture(const char* filepath) {
    GLuint textureID;
    glGenTextures(1, &textureID); // 產生紋理對象

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

    if (data) {
        // 如果成功載入紋理數據
        std::cout << "Texture loaded successfully: "
            << "File Path = " << filepath
            << ", Width = " << width << ", Height = " << height
            << ", Channels = " << nrChannels << std::endl;

        GLenum format;
        // 根據通道數選擇對應的格式
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        // 綁定紋理
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 傳送影像資料到 OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // 生成 Mipmap
        glGenerateMipmap(GL_TEXTURE_2D);

        // 設置紋理參數
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 釋放紋理數據
        stbi_image_free(data);
    }
    else {
        // 如果載入失敗，輸出錯誤訊息
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        stbi_image_free(data);
    }

    return textureID; // 返回紋理 ID
}
