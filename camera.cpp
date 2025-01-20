#include "Camera.h"
#include <cmath>
#include <iostream>

// �c�y��ơA��l�Ƭ۾����ݩ�
Camera::Camera() {
    cameraPos = glm::vec3(0.0f, 0.5f, 0.0f); // �P�a�O�P��
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);

    mouseSensitivity = 0.1f; // �q�{�����F�ӫ�
    yaw = -90.0f;            // �w�]�� -90 �� (�ݦV -Z �b)
    pitch = 0.0f;
    fov = 45.0f;             // ������l�Ƭ� 45 ��
}

// ��^�۾������ϯx�}
glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

// ��^�۾������� (fov)
float Camera::getFov() const {
    return fov;
}

// �B�z���в��ʡA�p��s�� yaw �M pitch
void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // ���� pitch �d��A�קK�����L�ױ���
    if (constrainPitch) {
        if (pitch > 45.0f) pitch = 45.0f;
        if (pitch < -45.0f) pitch = -45.0f;
    }

    updateCameraVectors();
}

// ��s�۾����e�B�k�B�W�V�q
void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
    cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}

// �B�z��L��J�A���ʬ۾�
void Camera::processKeyboard(const std::string& direction, float velocity) {
    // �p������e�i��V (���� Y ���q)
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

    if (direction == "FORWARD")
        cameraPos += horizontalFront * velocity; // �����e�i
    if (direction == "BACKWARD")
        cameraPos -= horizontalFront * velocity; // ������h
    if (direction == "LEFT")
        cameraPos -= glm::normalize(glm::cross(horizontalFront, cameraUp)) * velocity; // ����
    if (direction == "RIGHT")
        cameraPos += glm::normalize(glm::cross(horizontalFront, cameraUp)) * velocity; // �k��
    if (direction == "UP")
        cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * velocity; // �����V�W
    if (direction == "DOWN")
        cameraPos -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity; // �����V�U
}

// �p��j�K�ҫ��x�}�A�Ω�ø�s�Ĥ@�H�ٵ������j�K
glm::mat4 Camera::getGunModelMatrix() {
    glm::vec3 gunOffset =
        cameraFront * 0.5f - // �Z���۾��y�L�V�e
        cameraRight * 0.3f - // �V�k����
        cameraUp * 0.2f;     // �V�U����

    glm::vec3 gunPosition = cameraPos + gunOffset;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, gunPosition); // �]�m�j�K����m
    model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // ��������
    model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f)); // ��������
    model = glm::scale(model, glm::vec3(0.05f)); // �Y��ܾA��j�p
    return model;
}
