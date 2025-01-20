#include "Enemy.h"

// 構造函數：初始化敵人位置、尺寸、血量和速度
Enemy::Enemy(const glm::vec3& pos, const glm::vec3& sz, int hp, float spd)
    : position(pos), size(sz), health(hp), speed(spd) {}

// 繪製敵人
void Enemy::draw() {
    glPushAttrib(GL_CURRENT_BIT); // 保存當前顏色屬性
    glPushMatrix();               // 保存當前矩陣

    // 平移至敵人位置
    glTranslatef(position.x, position.y, position.z);

    // 繪製腿部（黑色）
    glPushMatrix();
    glTranslatef(0.0f, -size.y * 3 / 8, 0.0f); // 腿部在底部
    glScalef(size.x * 0.5f, size.y / 4, size.z * 0.5f); // 腿部比例
    glColor3f(0.0f, 0.0f, 0.0f); // 黑色
    glutSolidCube(1.0f);
    glPopMatrix();

    // 繪製身體（藍色）
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f); // 身體位於腿部之上
    glScalef(size.x, size.y / 2, size.z); // 身體比例
    glColor3f(0.0f, 0.0f, 1.0f); // 藍色
    glutSolidCube(1.0f);
    glPopMatrix();

    // 繪製頭部（皮膚色）
    glPushMatrix();
    glTranslatef(0.0f, size.y * 3 / 8, 0.0f); // 頭部位於身體之上
    glScalef(size.x * 0.6f, size.x * 0.6f, size.x * 0.6f); // 頭部為正方體
    glColor3f(1.0f, 0.8f, 0.6f); // 皮膚色
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix(); // 恢復矩陣
    glPopAttrib(); // 恢復顏色屬性
}

// 移動敵人
void Enemy::move() {
    float dx = (std::rand() % 21 - 10) * speed;
    float dz = (std::rand() % 21 - 10) * speed;
    position.x += dx;
    position.z += dz;

    // 限制敵人移動範圍
    position.x = glm::clamp(position.x, -12.0f, 12.0f);
    position.z = glm::clamp(position.z, -12.0f, 12.0f);
}

// 判斷敵人是否被子彈擊中
bool Enemy::isHit(const glm::vec3& bulletPos, bool& isHeadshot) {
    // 計算頭部範圍 (皮膚色區域)
    float headTop = position.y + size.y * 3 / 8 + size.x * 0.6f / 2;
    float headBottom = position.y + size.y * 3 / 8 - size.x * 0.6f / 2;
    float headLeft = position.x - size.x * 0.6f / 2 * 1.8f;
    float headRight = position.x + size.x * 0.6f / 2 * 1.8f;
    float headFront = position.z - size.x * 0.6f / 2 * 1.6f;
    float headBack = position.z + size.x * 0.6f / 2 * 1.6f;

    // 判斷是否命中頭部
    if (bulletPos.x >= headLeft && bulletPos.x <= headRight &&
        bulletPos.y >= headBottom && bulletPos.y <= headTop &&
        bulletPos.z >= headFront && bulletPos.z <= headBack) {
        isHeadshot = true;
        headshotDisplayTime = 1.0f; // 顯示 1 秒
        std::cout << "Headshot hit! Bullet Position: (" << bulletPos.x << ", " << bulletPos.y << ", " << bulletPos.z << ")" << std::endl;
        return true;
    }

    // 判斷是否命中身體
    if (bulletPos.x >= position.x - size.x / 2 && bulletPos.x <= position.x + size.x / 2 &&
        bulletPos.y >= position.y - size.y / 2 && bulletPos.y <= position.y + size.y / 2 &&
        bulletPos.z >= position.z - size.z / 2 && bulletPos.z <= position.z + size.z / 2) {
        isHeadshot = false;
        std::cout << "Body hit! Bullet Position: (" << bulletPos.x << ", " << bulletPos.y << ", " << bulletPos.z << ")" << std::endl;
        return true;
    }

    return false; // 沒有命中
}

// 敵人列表
std::vector<Enemy> enemies;

// 生成新的敵人
void spawnEnemy() {
    glm::vec3 position(
        (std::rand() % 200 - 100) / 10.0f, // x: -10.0 to 10.0
        0.4f,                             // 確保腿部接地
        (std::rand() % 200 - 100) / 10.0f // z: -10.0 to 10.0
    );
    glm::vec3 size(0.4f, 0.8f, 0.4f); // 縮小整體尺寸
    int health = 3;                   // 每個敵人有3點血量
    enemies.emplace_back(position, size, health);
}

// 渲染敵人並檢查其狀態
void renderEnemies() {
    for (auto it = enemies.begin(); it != enemies.end();) {
        if (it->health <= 0) {
            it = enemies.erase(it);  // 刪除敵人
        }
        else {
            it->move();  // 更新敵人位置
            it->draw();  // 繪製敵人
            ++it;
        }
    }

    // 如果敵人數量少於或等於1個，生成新的敵人
    if (enemies.size() <= 1) {
        for (int i = 0; i < 5; ++i) {
            spawnEnemy();
        }
    }
}
