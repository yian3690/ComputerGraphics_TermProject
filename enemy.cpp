#include "Enemy.h"

// �c�y��ơG��l�ƼĤH��m�B�ؤo�B��q�M�t��
Enemy::Enemy(const glm::vec3& pos, const glm::vec3& sz, int hp, float spd)
    : position(pos), size(sz), health(hp), speed(spd) {}

// ø�s�ĤH
void Enemy::draw() {
    glPushAttrib(GL_CURRENT_BIT); // �O�s��e�C���ݩ�
    glPushMatrix();               // �O�s��e�x�}

    // �����ܼĤH��m
    glTranslatef(position.x, position.y, position.z);

    // ø�s�L���]�¦�^
    glPushMatrix();
    glTranslatef(0.0f, -size.y * 3 / 8, 0.0f); // �L���b����
    glScalef(size.x * 0.5f, size.y / 4, size.z * 0.5f); // �L�����
    glColor3f(0.0f, 0.0f, 0.0f); // �¦�
    glutSolidCube(1.0f);
    glPopMatrix();

    // ø�s����]�Ŧ�^
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f); // ������L�����W
    glScalef(size.x, size.y / 2, size.z); // ������
    glColor3f(0.0f, 0.0f, 1.0f); // �Ŧ�
    glutSolidCube(1.0f);
    glPopMatrix();

    // ø�s�Y���]�ֽ���^
    glPushMatrix();
    glTranslatef(0.0f, size.y * 3 / 8, 0.0f); // �Y������餧�W
    glScalef(size.x * 0.6f, size.x * 0.6f, size.x * 0.6f); // �Y����������
    glColor3f(1.0f, 0.8f, 0.6f); // �ֽ���
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix(); // ��_�x�}
    glPopAttrib(); // ��_�C���ݩ�
}

// ���ʼĤH
void Enemy::move() {
    float dx = (std::rand() % 21 - 10) * speed;
    float dz = (std::rand() % 21 - 10) * speed;
    position.x += dx;
    position.z += dz;

    // ����ĤH���ʽd��
    position.x = glm::clamp(position.x, -12.0f, 12.0f);
    position.z = glm::clamp(position.z, -12.0f, 12.0f);
}

// �P�_�ĤH�O�_�Q�l�u����
bool Enemy::isHit(const glm::vec3& bulletPos, bool& isHeadshot) {
    // �p���Y���d�� (�ֽ���ϰ�)
    float headTop = position.y + size.y * 3 / 8 + size.x * 0.6f / 2;
    float headBottom = position.y + size.y * 3 / 8 - size.x * 0.6f / 2;
    float headLeft = position.x - size.x * 0.6f / 2 * 1.8f;
    float headRight = position.x + size.x * 0.6f / 2 * 1.8f;
    float headFront = position.z - size.x * 0.6f / 2 * 1.6f;
    float headBack = position.z + size.x * 0.6f / 2 * 1.6f;

    // �P�_�O�_�R���Y��
    if (bulletPos.x >= headLeft && bulletPos.x <= headRight &&
        bulletPos.y >= headBottom && bulletPos.y <= headTop &&
        bulletPos.z >= headFront && bulletPos.z <= headBack) {
        isHeadshot = true;
        headshotDisplayTime = 1.0f; // ��� 1 ��
        std::cout << "Headshot hit! Bullet Position: (" << bulletPos.x << ", " << bulletPos.y << ", " << bulletPos.z << ")" << std::endl;
        return true;
    }

    // �P�_�O�_�R������
    if (bulletPos.x >= position.x - size.x / 2 && bulletPos.x <= position.x + size.x / 2 &&
        bulletPos.y >= position.y - size.y / 2 && bulletPos.y <= position.y + size.y / 2 &&
        bulletPos.z >= position.z - size.z / 2 && bulletPos.z <= position.z + size.z / 2) {
        isHeadshot = false;
        std::cout << "Body hit! Bullet Position: (" << bulletPos.x << ", " << bulletPos.y << ", " << bulletPos.z << ")" << std::endl;
        return true;
    }

    return false; // �S���R��
}

// �ĤH�C��
std::vector<Enemy> enemies;

// �ͦ��s���ĤH
void spawnEnemy() {
    glm::vec3 position(
        (std::rand() % 200 - 100) / 10.0f, // x: -10.0 to 10.0
        0.4f,                             // �T�O�L�����a
        (std::rand() % 200 - 100) / 10.0f // z: -10.0 to 10.0
    );
    glm::vec3 size(0.4f, 0.8f, 0.4f); // �Y�p����ؤo
    int health = 3;                   // �C�ӼĤH��3�I��q
    enemies.emplace_back(position, size, health);
}

// ��V�ĤH���ˬd�䪬�A
void renderEnemies() {
    for (auto it = enemies.begin(); it != enemies.end();) {
        if (it->health <= 0) {
            it = enemies.erase(it);  // �R���ĤH
        }
        else {
            it->move();  // ��s�ĤH��m
            it->draw();  // ø�s�ĤH
            ++it;
        }
    }

    // �p�G�ĤH�ƶq�֩�ε���1�ӡA�ͦ��s���ĤH
    if (enemies.size() <= 1) {
        for (int i = 0; i < 5; ++i) {
            spawnEnemy();
        }
    }
}
