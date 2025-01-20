#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

// �w�q Camera ���O�A�޲z�۾����ݩʻP�ާ@
class Camera {
public:
    glm::vec3 cameraPos;    // �۾�����m�A�λ��O�۾����u�����v
    glm::vec3 cameraFront;  // �۾��ݪ���V
    glm::vec3 cameraUp;     // �۾����u�W��v
    glm::vec3 cameraRight;  // �۾����u�k��v

    float mouseSensitivity; // �����F�ӫ�
    float yaw;              // �������� (���訤)
    float pitch;            // �������� (������)
    float fov;              // ���� (field of view)

    // �غc�l
    Camera();

    // ������ϯx�}
    glm::mat4 getViewMatrix();

    // ������� (fov)
    float getFov() const;

    // �B�z���в���
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // ��s�۾��V�q
    void updateCameraVectors();

    // �B�z��L��J (WASD)
    void processKeyboard(const std::string& direction, float velocity);

    // �p��j�K�ҫ����x�}
    glm::mat4 getGunModelMatrix();
};

#endif
