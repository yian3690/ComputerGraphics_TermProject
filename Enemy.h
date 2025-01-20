#ifndef ENEMY_H
#define ENEMY_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib> // �Ω� std::rand �M std::srand

class Enemy {
public:
    glm::vec3 position; // �ĤH��m
    glm::vec3 size;     // �ĤH�ؤo
    int health;         // �ĤH��q
    float speed;        // �ĤH���ʳt��

    // �c�y��ơA��l�ƼĤH���ݩ�
    Enemy(const glm::vec3& pos, const glm::vec3& sz, int hp, float spd = 0.002f);

    // ø�s�ĤH
    void draw();

    // ���ʼĤH
    void move();

    // �P�_�O�_�Q����
    bool isHit(const glm::vec3& bulletPos, bool& isHeadshot);

private:
    float headshotDisplayTime = 0.0f; // �Ω���� Headshot ���ɶ�
};

// �ĤH�C��
extern std::vector<Enemy> enemies;

// �ͦ��s���ĤH
void spawnEnemy();

// ��V�ĤH���ˬd�䪬�A
void renderEnemies();

#endif // ENEMY_H
#pragma once
