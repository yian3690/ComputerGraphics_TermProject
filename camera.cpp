#include "Camera.h"
#include <cmath>
#include <iostream>

// 構造函數，初始化相機的屬性
Camera::Camera() {
    cameraPos = glm::vec3(0.0f, 0.5f, 0.0f); // 與地板同高
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);

    mouseSensitivity = 0.1f; // 默認鼠標靈敏度
    yaw = -90.0f;            // 預設為 -90 度 (看向 -Z 軸)
    pitch = 0.0f;
    fov = 45.0f;             // 視角初始化為 45 度
}

// 返回相機的視圖矩陣
glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

// 返回相機的視角 (fov)
float Camera::getFov() const {
    return fov;
}

// 處理鼠標移動，計算新的 yaw 和 pitch
void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // 限制 pitch 範圍，避免視角過度旋轉
    if (constrainPitch) {
        if (pitch > 45.0f) pitch = 45.0f;
        if (pitch < -45.0f) pitch = -45.0f;
    }

    updateCameraVectors();
}

// 更新相機的前、右、上向量
void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
    cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}

// 處理鍵盤輸入，移動相機
void Camera::processKeyboard(const std::string& direction, float velocity) {
    // 計算水平前進方向 (忽略 Y 分量)
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

    if (direction == "FORWARD")
        cameraPos += horizontalFront * velocity; // 水平前進
    if (direction == "BACKWARD")
        cameraPos -= horizontalFront * velocity; // 水平後退
    if (direction == "LEFT")
        cameraPos -= glm::normalize(glm::cross(horizontalFront, cameraUp)) * velocity; // 左移
    if (direction == "RIGHT")
        cameraPos += glm::normalize(glm::cross(horizontalFront, cameraUp)) * velocity; // 右移
    if (direction == "UP")
        cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * velocity; // 垂直向上
    if (direction == "DOWN")
        cameraPos -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity; // 垂直向下
}

// 計算槍枝模型矩陣，用於繪製第一人稱視角的槍枝
glm::mat4 Camera::getGunModelMatrix() {
    glm::vec3 gunOffset =
        cameraFront * 0.5f - // 距離相機稍微向前
        cameraRight * 0.3f - // 向右偏移
        cameraUp * 0.2f;     // 向下偏移

    glm::vec3 gunPosition = cameraPos + gunOffset;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, gunPosition); // 設置槍枝的位置
    model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // 水平旋轉
    model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f)); // 垂直旋轉
    model = glm::scale(model, glm::vec3(0.05f)); // 縮放至適當大小
    return model;
}
