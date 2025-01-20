#ifndef ENEMY_H
#define ENEMY_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib> // 用於 std::rand 和 std::srand

class Enemy {
public:
    glm::vec3 position; // 敵人位置
    glm::vec3 size;     // 敵人尺寸
    int health;         // 敵人血量
    float speed;        // 敵人移動速度

    // 構造函數，初始化敵人的屬性
    Enemy(const glm::vec3& pos, const glm::vec3& sz, int hp, float spd = 0.002f);

    // 繪製敵人
    void draw();

    // 移動敵人
    void move();

    // 判斷是否被擊中
    bool isHit(const glm::vec3& bulletPos, bool& isHeadshot);

private:
    float headshotDisplayTime = 0.0f; // 用於顯示 Headshot 的時間
};

// 敵人列表
extern std::vector<Enemy> enemies;

// 生成新的敵人
void spawnEnemy();

// 渲染敵人並檢查其狀態
void renderEnemies();

#endif // ENEMY_H
#pragma once
