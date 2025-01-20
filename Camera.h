#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

// 定義 Camera 類別，管理相機的屬性與操作
class Camera {
public:
    glm::vec3 cameraPos;    // 相機的位置，或說是相機的「眼睛」
    glm::vec3 cameraFront;  // 相機看的方向
    glm::vec3 cameraUp;     // 相機的「上方」
    glm::vec3 cameraRight;  // 相機的「右方」

    float mouseSensitivity; // 鼠標靈敏度
    float yaw;              // 水平角度 (偏航角)
    float pitch;            // 垂直角度 (俯仰角)
    float fov;              // 視角 (field of view)

    // 建構子
    Camera();

    // 獲取視圖矩陣
    glm::mat4 getViewMatrix();

    // 獲取視角 (fov)
    float getFov() const;

    // 處理鼠標移動
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // 更新相機向量
    void updateCameraVectors();

    // 處理鍵盤輸入 (WASD)
    void processKeyboard(const std::string& direction, float velocity);

    // 計算槍枝模型的矩陣
    glm::mat4 getGunModelMatrix();
};

#endif
