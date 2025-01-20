#define STB_IMAGE_IMPLEMENTATION    
#define GLEW_STATIC

#include <GL/glew.h>     // GLEW 庫，用於加載 OpenGL 擴展
#include <GL/freeglut.h>      // GLUT 和 OpenGL 標頭
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "TextureLoader.h"  //導入材質
#include "Camera.h"         //導入相機
#include "Enemy.h"         //導入敵人

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/*
模型的部分未完成，就沒用了
光的部分也是
*/


/*
drawFunction用法:
x:寬 y:高 z:深度 xOffset:x偏移量, yOffset:x偏移量(高度), zOffset:z偏移量(深度)
*/


float lastFrame = 0.0f; // 上一幀的時間

// 添加全域變數來控制手電筒狀態
bool flashlightOn = false;

int score = 0; // 分數初始為0
float gameTime = 30.0f; // 倒數30秒

bool gameOver = false;//判斷遊戲是否結束

float headshotDisplayTime = 0.0f; // 爆頭顯示時間（秒）

TextureLoader textureLoader;    // 使用 TextureLoader 載入紋理

GLuint floorTexture;
GLuint wallTexture;
GLuint obstacleTexture;

struct Obstacle {
    float width, height, depth;
    float x, y, z;
};


// 假設有一個全域變數來儲存障礙物資訊
std::vector<Obstacle> obstacles;


struct Bullet {
    glm::vec3 position; // 子彈位置
    glm::vec3 direction; // 子彈方向
    float speed; // 子彈速度
    float lifetime; // 子彈存在時間（秒）
};

std::vector<Bullet> bullets; // 子彈列表
float bulletSpeed = 10.0f;   // 子彈速度
float bulletLifetime = 5.0f; // 子彈存活時間


int maxBulletsInClip = 12;  // 每個彈夾的最大子彈數量
int currentBullets = maxBulletsInClip;  // 當前彈夾內的子彈數
bool isReloading = false;  // 是否正在換彈
float reloadTime = 1.5f;  // 換彈所需時間
float reloadTimer = 0.0f;  // 換彈計時器





//導入模組

struct Vertex {
    glm::vec3 position;  // 頂點位置
    glm::vec3 normal;    // 法線向量
    glm::vec2 texCoords; // 紋理坐標
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint vao, vbo, ebo;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
        : vertices(vertices), indices(indices) {
        setupMesh();
    }

    void Draw() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    void setupMesh() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        // 綁定頂點緩衝
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // 綁定索引緩衝
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 設置頂點屬性指針
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

        glBindVertexArray(0);
    }
};

std::vector<Mesh> gunMeshes; // 用於存儲槍枝模型的全局變數




Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 解析頂點數據
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;
        vertex.position = position;

        if (mesh->HasNormals()) {
            glm::vec3 normal;
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;
            vertex.normal = normal;
        }

        if (mesh->mTextureCoords[0]) {
            glm::vec2 texCoords;
            texCoords.x = mesh->mTextureCoords[0][i].x;
            texCoords.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = texCoords;
        }
        else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // 解析索引數據
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 返回解析好的 Mesh
    return Mesh(vertices, indices);
}

void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes) {
    // 處理當前節點的所有網格
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene));
    }

    // 遞歸處理子節點
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, meshes);
    }
}

void ProcessAssimpScene(const aiScene* scene, std::vector<Mesh>& meshes) {
    std::cout << "Processing scene..." << std::endl;
    ProcessNode(scene->mRootNode, scene, meshes);
    std::cout << "Processing completed!" << std::endl;
}



void renderGun(Camera& camera) {
    // 計算槍枝的模型矩陣
    glm::mat4 gunModelMatrix = camera.getGunModelMatrix();

    // 設定固定管線的矩陣
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(gunModelMatrix));

    // 渲染槍枝模型
    for (unsigned int i = 0; i < gunMeshes.size(); i++) {
        gunMeshes[i].Draw(); // 假設 Mesh 支援無著色器繪製的方式
    }
}


void LoadGunModel() {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("assets/AK47.obj",
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
    return;
} else {
    std::cout << "Model loaded successfully!" << std::endl;
}
    // 將模型資料解析並存入 gunMeshes
    ProcessAssimpScene(scene, gunMeshes);

    std::cout << "Loading model..." << std::endl;
}



// Player position and orientation
Camera camera; // 使用 Camera 類來管理相機

// Keyboard input movement speed
float speed = 0.1f;

// Mouse position tracking
int lastMouseX, lastMouseY;
bool firstMouse = true;

// Obstacle position and size
const float obstacleX = 2.0f, obstacleZ = 2.0f;
const float obstacleSize = 1.0f;

//子彈碰撞物體(會使物體被破壞 可以打人)
bool checkBulletCollision(const glm::vec3& bulletPos) {
    // 檢查子彈是否擊中障礙物
    for (auto it = obstacles.begin(); it != obstacles.end(); /* no increment here */) {
        auto& obs = *it;
        if (bulletPos.x >= obs.x - obs.width / 2 && bulletPos.x <= obs.x + obs.width / 2 &&
            bulletPos.y >= obs.y - obs.height / 2 && bulletPos.y <= obs.y + obs.height / 2 &&
            bulletPos.z >= obs.z - obs.depth / 2 && bulletPos.z <= obs.z + obs.depth / 2) {

            // 印出被擊中的障礙物資訊
            std::cout << "Obstacle hit: "
                << "Position(" << obs.x << ", " << obs.y << ", " << obs.z << "), "
                << "Size(" << obs.width << ", " << obs.height << ", " << obs.depth << ")\n";

            // 移除被擊中的障礙物
            it = obstacles.erase(it);

            // 打破箱子加秒數
            gameTime += 3.0f;

            return true; // 子彈碰撞發生
        }
        else {
            ++it;
        }
    }

    // 檢查子彈是否擊中敵人
    for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ) {
        bool isHeadshot = false;
        if (enemyIt->isHit(bulletPos, isHeadshot)) { // 檢查是否擊中敵人
            if (isHeadshot) {
                std::cout << "Headshot! Enemy eliminated instantly!" << std::endl;
                // 直接移除敵人
                enemyIt = enemies.erase(enemyIt);
                score += 10;  // 頭部擊殺加10分
            }
            else {
                enemyIt->health--; // 扣血
                std::cout << "Enemy hit! Remaining health: " << enemyIt->health << std::endl;

                if (enemyIt->health <= 0) { // 如果敵人血量降至0
                    std::cout << "Enemy eliminated!\n";
                    score += 10;  // 每擊殺一個敵人加10分

                    // 移除敵人
                    enemyIt = enemies.erase(enemyIt);
                }
                else {
                    ++enemyIt;
                }
            }
            return true; // 子彈碰撞發生
        }
        else {
            ++enemyIt;
        }
    }

    return false; // 沒有碰撞
}


//滑鼠點擊 發射
void mouse(int button, int state, int x, int y) {
    static bool isZoomed = false; // 用於記錄放大狀態

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (isReloading || currentBullets <= 0) {
            // 如果正在換彈或子彈耗盡，無法射擊
            return;
        }

        // 創建子彈
        Bullet bullet;
        bullet.position = camera.cameraPos; // 子彈從相機位置發射
        bullet.direction = camera.cameraFront;  // 子彈方向與相機朝向一致
        bullet.speed = bulletSpeed;
        bullet.lifetime = bulletLifetime;

        bullets.push_back(bullet); // 將子彈加入列表
        currentBullets--; // 減少剩餘子彈數

        // 如果子彈耗盡，開始自動換彈
        if (currentBullets == 0) {
            isReloading = true;
            reloadTimer = 0.0f; // 初始化換彈計時器
        }
    }
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        // 切換放大與還原狀態
        isZoomed = !isZoomed;

        if (isZoomed) {
            // 放大視角
            camera.fov /= 2.0f; // 假設 fov 是相機的視角屬性
        }
        else {
            // 還原視角
            camera.fov *= 2.0f;
        }
    }
}


//更新畫面視角
void updateProjectionMatrix() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.fov, (float)800 / (float)600, 0.1f, 100.0f); // 假設 screenWidth 和 screenHeight 是屏幕的寬高
    glMatrixMode(GL_MODELVIEW);
}





void updateBullets(float deltaTime) {
    for (auto it = bullets.begin(); it != bullets.end();) {
        // 更新子彈位置
        it->position += it->direction * it->speed * deltaTime;
        it->lifetime -= deltaTime;

        // 移除超時或撞擊的子彈
        if (it->lifetime <= 0.0f || checkBulletCollision(it->position)) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }
}

//換彈
void updateReload(float deltaTime) {
    if (isReloading) {
        reloadTimer += deltaTime;
        if (reloadTimer >= reloadTime) {
            // 換彈完成
            currentBullets = maxBulletsInClip;
            isReloading = false;
            reloadTimer = 0.0f;
        }
    }
    else if (currentBullets == 0) {
        // 當前彈夾沒有子彈，自動開始換彈
        isReloading = true;
        reloadTimer = 0.0f;
    }
}




// Function to draw a tiled floor
void drawTiledFloor(int tiles, float tileSize) {
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    float halfSize = tiles * tileSize / 2.0f;
    for (int i = 0; i < tiles; ++i) {
        for (int j = 0; j < tiles; ++j) {
            float x = -halfSize + i * tileSize;
            float z = -halfSize + j * tileSize;

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(x, 0.0f, z);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(x + tileSize, 0.0f, z);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(x + tileSize, 0.0f, z + tileSize);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(x, 0.0f, z + tileSize);
            glEnd();
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// Function to draw a tiled wall
void drawTiledWall(float width, float height, int tilesX, int tilesY, float offsetX, float offsetY, float offsetZ, char axis) {
    glBindTexture(GL_TEXTURE_2D, wallTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    float tileWidth = width / tilesX;           //將牆的總寬度和高度除以瓷磚的數量，計算每塊瓷磚的尺寸。
    float tileHeight = height / tilesY;

    for (int i = 0; i < tilesX; ++i) {
        for (int j = 0; j < tilesY; ++j) {
            float x = offsetX + i * tileWidth - width / 2.0f;       //計算每塊瓷磚左下角的 X 和 Y 坐標。offsetX 和 offsetY 用於偏移牆的位置
            float y = offsetY + j * tileHeight - height / 2.0f;

            glBegin(GL_QUADS);
            if (axis == 'z') {                  // 繪製 Z 軸方向的牆面，每個頂點對應到紋理的座標
                glTexCoord2f(0.0f, 0.0f); glVertex3f(x, y, offsetZ);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(x + tileWidth, y, offsetZ);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(x + tileWidth, y + tileHeight, offsetZ);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(x, y + tileHeight, offsetZ);
            }
            else if (axis == 'x') {             // 繪製 X 軸方向的牆面
                glTexCoord2f(0.0f, 0.0f); glVertex3f(offsetX, y, x);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(offsetX, y, x + tileWidth);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(offsetX, y + tileHeight, x + tileWidth);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(offsetX, y + tileHeight, x);
            }
            glEnd();
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}




// Function to draw the floor
void drawFloor() {
    drawTiledFloor(20, 1.0f); // 分成 20 x 20 的方塊，每個方塊大小為 1.0 單位
}



// Function to draw an obstacle with texture
void drawObstacle(float width, float height, float depth, float posX, float posY, float posZ) {
    glBindTexture(GL_TEXTURE_2D, obstacleTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float x1 = posX - width / 2.0f;
    float x2 = posX + width / 2.0f;
    float y1 = posY - height / 2.0f;
    float y2 = posY + height / 2.0f;
    float z1 = posZ - depth / 2.0f;
    float z2 = posZ + depth / 2.0f;

    glBegin(GL_QUADS);

    // Front face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, z2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y1, z2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, z2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y2, z2);

    // Back face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, z1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y1, z1);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, z1);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y2, z1);

    // Left face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, z1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x1, y1, z2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x1, y2, z2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y2, z1);

    // Right face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x2, y1, z1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y1, z2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, z2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x2, y2, z1);

    // Top face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y2, z1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y2, z1);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, z2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y2, z2);

    // Bottom face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, z1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y1, z1);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y1, z2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y1, z2);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

// Function to draw the walls
void drawWalls() {
    // Front wall
    drawTiledWall(20.0f, 2.0f, 10, 5, 0.0f, 1.0f, -10.0f, 'z');

    // Back wall
    drawTiledWall(20.0f, 2.0f, 10, 5, 0.0f, 1.0f, 10.0f, 'z');

    // Left wall
    drawTiledWall(40.0f, 2.0f, 10, 5, -10.0f, 1.0f, 0.0f, 'x');

    // Right wall
    drawTiledWall(40.0f, 2.0f, 10, 5, 10.0f, 1.0f, 0.0f, 'x');
}




// 人物與環境的碰撞
bool checkBoundaryCollision(float nextX, float nextY, float nextZ) {
    // 檢查與地板的碰撞
    if (nextY < 0.5f) return true;

    // 檢查與牆壁的碰撞
    if (nextX <= -9.5f || nextX >= 9.5f || nextZ <= -9.5f || nextZ >= 9.5f) return true;

    // 檢查與障礙物的碰撞
    for (const auto& obstacle : obstacles) {
        float xMin = obstacle.x - obstacle.width / 2.0f;
        float xMax = obstacle.x + obstacle.width / 2.0f;
        float yMin = obstacle.y - obstacle.height / 2.0f;
        float yMax = obstacle.y + obstacle.height / 2.0f;
        float zMin = obstacle.z - obstacle.depth / 2.0f;
        float zMax = obstacle.z + obstacle.depth / 2.0f;

        // AABB 碰撞檢測
        if (nextX >= xMin && nextX <= xMax &&
            nextY >= yMin && nextY <= yMax &&
            nextZ >= zMin && nextZ <= zMax) {
            std::cout << "Checking obstacle: x[" << xMin << ", " << xMax
                << "], y[" << yMin << ", " << yMax
                << "], z[" << zMin << ", " << zMax << "]\n";
            std::cout << "Object position: (" << nextX << ", " << nextY << ", " << nextZ << ")\n";

            return true;
        }

    }
    return false;
}


void drawBullets() {
    for (const auto& bullet : bullets) {
        glPushMatrix();
        glTranslatef(bullet.position.x, bullet.position.y, bullet.position.z);
        // 繪製子彈，這裡以小球表示
        glutSolidSphere(0.1f, 16, 16); // 半徑0.1的球
        glPopMatrix();
    }
}



// 鍵盤控制
void keyboard(unsigned char key, int x, int y) {
    glm::vec3 oldPos = camera.cameraPos; // Save current position

    switch (key) {
    case 'w': // Move forward
        camera.processKeyboard("FORWARD", speed);
        break;
    case 's': // Move backward
        camera.processKeyboard("BACKWARD", speed);
        break;
    case 'a': // Strafe left
        camera.processKeyboard("LEFT", speed);
        break;
    case 'd': // Strafe right
        camera.processKeyboard("RIGHT", speed);
        break;
    case 'f': // 切換手電筒
        flashlightOn = !flashlightOn;
        break;
    case 'r': // 換彈
        if (!isReloading && currentBullets < maxBulletsInClip) {
            isReloading = true;    // 開始換彈
            reloadTimer = 0.0f;    // 初始化換彈計時器
        }
        break;
    case 27: // 關閉視窗
        exit(0);
    }

    // 檢查碰撞
    if (checkBoundaryCollision(camera.cameraPos.x, camera.cameraPos.y, camera.cameraPos.z)) {
        camera.cameraPos = oldPos; // Revert to old position if collision occurs
    }

    glutPostRedisplay();
}

// Mouse motion callback for camera rotation
void mouseMotion(int x, int y) {
    // 設定視窗中央的座標
    int windowCenterX = 800 / 2;
    int windowCenterY = 600 / 2;

    if (firstMouse) {
        lastMouseX = windowCenterX;
        lastMouseY = windowCenterY;
        firstMouse = false;
        glutWarpPointer(windowCenterX, windowCenterY); // 初始化滑鼠位置
        return;
    }

    // 計算滑鼠的偏移量
    int offsetX = x - lastMouseX;
    int offsetY = lastMouseY - y; // Reversed: y-coordinates go from bottom to top

    // 更新相機視角
    camera.processMouseMovement(offsetX, offsetY);

    // 重設滑鼠指標到視窗中央
    glutWarpPointer(windowCenterX, windowCenterY);

    // 更新最後的滑鼠位置為視窗中央
    lastMouseX = windowCenterX;
    lastMouseY = windowCenterY;

    glutPostRedisplay();
}

// 隨機生成一個障礙物的位置並加入向量
void spawnNewObstacle() {
    float size = 0.5f; // 固定大小
    float y = size / 2.0f; // 保持障礙物位於地板上，y = size的一半
    float x = static_cast<float>(std::rand() % 10 - 5); // 隨機生成範圍 [-5, 5]
    float z = static_cast<float>(std::rand() % 10 - 5); // 隨機生成範圍 [-5, 5]

    // 將新障礙物加入向量
    obstacles.push_back({ size, size, size, x, y, z });
}


// 繪製所有現有的障礙物
void drawObstacles() {
    // 如果沒有障礙物，隨機生成一個
    if (obstacles.empty()) {
        spawnNewObstacle(); // 呼叫生成新障礙物的函式
    }

    // 繪製所有障礙物
    for (const auto& obstacle : obstacles) {
        drawObstacle(obstacle.width, obstacle.height, obstacle.depth,
            obstacle.x, obstacle.y, obstacle.z);
    }
}

void initializeObstacles() {
    for (int i = 0; i < 2; i++) {
        spawnNewObstacle();
    }

    // 初始時繪製障礙物
    drawObstacles();
}




int windowWidth = 800; // 預設寬度
int windowHeight = 600; // 預設高度

//準心
void drawCrosshair() {
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT); // 保存顏色相關狀態

    glMatrixMode(GL_PROJECTION); // 設置投影模式
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1); // 設置正交投影
    glMatrixMode(GL_MODELVIEW); // 設置模型視圖
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST); // 禁用深度測試，讓準心顯示在最前面

    float centerX = windowWidth / 2.0f;
    float centerY = windowHeight / 2.0f;
    float crosshairSize = 10.0f; // 準心的大小

    glColor3f(1.0f, 1.0f, 1.0f); // 設置為白色準心
    glBegin(GL_LINES);
    // 水平線
    glVertex2f(centerX - crosshairSize, centerY);
    glVertex2f(centerX + crosshairSize, centerY);
    // 垂直線
    glVertex2f(centerX, centerY - crosshairSize);
    glVertex2f(centerX, centerY + crosshairSize);
    glEnd();

    glEnable(GL_DEPTH_TEST); // 恢復深度測試

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib(); // 恢復顏色相關狀態
}






//遊戲結束頁面
void displayGameOver() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清除畫面

    // 顯示 "GAME OVER" 文字
    glColor3f(1.0f, 0.0f, 0.0f); // 紅色文字
    glRasterPos2i(350, 300); // 設置文字位置（畫面中間）
    std::string gameOverText = "GAME OVER";
    for (char c : gameOverText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // 顯示得分，將位置設置在 "GAME OVER" 下方
    std::string scoreText = "Score: " + std::to_string(score); // 假設 score 是整數類型的變數
    glRasterPos2i(350, 250); // 設置得分文字位置，稍微低於 "GAME OVER"
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // 顯示每個字符
    }
}




// 手電筒光源屬性
GLfloat flashlightDiffuse[] = { 1.5f, 1.5f, 1.2f, 1.0f }; // 提高亮度
GLfloat flashlightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 點光源
GLfloat flashlightDirection[] = { 0.0f, 0.0f, -1.0f };     // 聚光方向


// 更新手電筒光源位置和方向
void updateFlashlight() {
    if (flashlightOn) {
        // 更新光源位置為相機位置
        flashlightPosition[0] = camera.cameraPos.x;
        flashlightPosition[1] = camera.cameraPos.y;
        flashlightPosition[2] = camera.cameraPos.z;

        // 更新光源方向為相機前向向量
        glm::vec3 normalizedDirection = glm::normalize(camera.cameraFront);
        flashlightDirection[0] = normalizedDirection.x;
        flashlightDirection[1] = normalizedDirection.y;
        flashlightDirection[2] = normalizedDirection.z;

        // 設置到 OpenGL 光源
        glEnable(GL_LIGHT1);
        glLightfv(GL_LIGHT1, GL_POSITION, flashlightPosition);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, flashlightDirection);
    }
    else {
        glDisable(GL_LIGHT1);
    }
}


void updateBackupLight() {
    glm::vec3 cameraPos = camera.cameraPos;

    // 將備用光源放置於相機上方 10 單位處
    GLfloat backupLightPosition[] = {
        cameraPos.x,
        cameraPos.y + 10.0f, // 上方 10 單位
        cameraPos.z,
        1.0f
    };

    glLightfv(GL_LIGHT2, GL_POSITION, backupLightPosition);
}


void initBackupLight() {
    glEnable(GL_LIGHT2);
    GLfloat backupLightDiffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // 適中的亮度
    GLfloat backupLightPosition[] = { 0.0f, 10.0f, 0.0f, 1.0f }; // 位於地面正上方 10 單位處
    glLightfv(GL_LIGHT2, GL_DIFFUSE, backupLightDiffuse);
    glLightfv(GL_LIGHT2, GL_POSITION, backupLightPosition);

    // 控制衰減
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.02f);  // 緩慢衰減
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.001f); // 遠距離更均勻
}



// 初始化光照
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);

    // 手電筒屬性
    glLightfv(GL_LIGHT1, GL_DIFFUSE, flashlightDiffuse);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 90.0f);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 10.0f);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.005f);

    // 調整環境光強度
    GLfloat ambientLight[] = { 0.4f, 0.4f, 0.4f, 1.0f }; // 提高強度
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

    // 初始化備用光源
    initBackupLight();

    // 禁用其他光源
    glDisable(GL_LIGHT0);
}








// 初始化函數
void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D); // 開啟紋理功能

    glMatrixMode(GL_PROJECTION);
    gluPerspective(45.0, 800.0 / 600.0, 0.1, 100.0);

    // 使用 stb_image 載入紋理
    wallTexture = textureLoader.LoadTexture("img/brick.jpg");
    floorTexture = textureLoader.LoadTexture("img/metal.jpg");
    obstacleTexture = textureLoader.LoadTexture("img/box.jpg");

    // 初始化障礙物
    initializeObstacles();


    // 初始化光照
    //initLighting();


    // 隱藏滑鼠指標
    glutSetCursor(GLUT_CURSOR_NONE);

    // 生成初始敵人
    for (int i = 0; i < 5; ++i) { // 初始生成 5 個敵人
        spawnEnemy();
    }
}


// 空閒函數，不斷重繪
void idle() {
    // 在這裡您可以加入需要持續更新的邏輯
    glutPostRedisplay();  // 請求重新繪製屏幕
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    // 設置相機視角
    glm::mat4 view = camera.getViewMatrix();
    glLoadMatrixf(glm::value_ptr(view));

    // 右鍵放大鏡頭
    updateProjectionMatrix();


    // 更新手電筒
    // 更新光源
    updateBackupLight();
    updateFlashlight();
    

    drawFloor();
    drawWalls(); // Draw walls

    // 繪製所有目前存在的障礙物
    drawObstacles();

    // 繪製敵人
    renderEnemies();

    

    // 渲染準心
    drawCrosshair();

    // 更新子彈
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;  // 以秒為單位
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;


    updateBullets(deltaTime); // 更新子彈位置

    // 繪製子彈
    drawBullets();

    // 換彈
    updateReload(deltaTime);



    // 渲染槍枝
    //renderGun(camera);

    /*分數顯示這部分會有錯，若將字體改為其他顏色*/
    // 顯示分數
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1); // 設置正射投影，將畫布設置為2D

    glColor3f(1.0f, 1.0f, 1.0f); // 設置黃色字體顏色
    glRasterPos2i(10, 20); // 設置文字顯示位置（左上角）

    // 顯示分數
    std::string scoreText = "Score: " + std::to_string(score);
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // 顯示每個字符
    }

    // 顯示倒數時間
    glColor3f(1.0f, 1.0f, 1.0f); // 設置白色字體顏色
    glRasterPos2i(700, 20); // 設置文字顯示位置（右上角）
    std::string timeText = "Time: " + std::to_string(static_cast<int>(gameTime));
    for (char c : timeText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // 顯示每個字符
    }
    gameTime -= deltaTime; // 減少倒數時間



    // 在螢幕上方中繪製 "Headshot"
    if (headshotDisplayTime > 0.0f) {
        // 保存當前矩陣和屬性
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

        // 切換至正射投影
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, 800, 600, 0, -1, 1); // 設置正射投影

        // 切換至模型視圖矩陣
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // 設置文字顏色並繪製
        glDisable(GL_LIGHTING); // 禁用燈光（防止影響顏色）
        glColor3f(1.0f, 0.0f, 0.0f); // 紅色字體
        glRasterPos2i(400 - 50, 50); // 畫面上方中間
        std::string headshotText = "HEADSHOT!";
        for (char c : headshotText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        // 恢復矩陣和屬性
        glPopMatrix();          // 恢復模型視圖矩陣
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();          // 恢復投影矩陣
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();          // 恢復所有保存的屬性

        // 減少顯示時間
        headshotDisplayTime -= deltaTime;
    }


    if (isReloading) {
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT); // 保存當前狀態

        // 切換至正射投影
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, 800, 600, 0, -1, 1); // 設置正射投影

        // 切換至模型視圖矩陣
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // 顯示換彈文字
        glDisable(GL_LIGHTING); // 禁用燈光，避免影響顏色
        glColor3f(1.0f, 1.0f, 0.0f); // 設置黃色文字
        glRasterPos2i(400 - 50, 100); // 畫面中央上方
        std::string reloadText = "Reloading...";
        for (char c : reloadText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        // 恢復矩陣與狀態
        glPopMatrix();          // 恢復模型視圖矩陣
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();          // 恢復投影矩陣
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();          // 恢復所有保存的屬性
    }

    // 顯示剩餘子彈/最大子彈數
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT); // 保存狀態

    // 切換至正射投影
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1); // 設置正射投影

    // 切換至模型視圖矩陣
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 顯示剩餘子彈數
    glDisable(GL_LIGHTING); // 禁用燈光，避免影響文字顏色
    glColor3f(1.0f, 1.0f, 0.0f); // 設置黃色文字
    glRasterPos2i(10, 580); // 左下角顯示
    std::string ammoText = "Ammo: " + std::to_string(currentBullets) + "/" + std::to_string(maxBulletsInClip);
    for (char c : ammoText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // 恢復矩陣與狀態
    glPopMatrix();          // 恢復模型視圖矩陣
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();          // 恢復投影矩陣
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();          // 恢復所有保存的屬性




    // 遊戲結束檢查
    if (gameTime <= 0.0f) {
        gameOver = true;
    }
    
    //遊戲結束
    if (gameOver) { 
       displayGameOver(); // 顯示 Game Over 畫面
    }

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glutSwapBuffers();
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(800, 600);
    glutCreateWindow("First-Person Camera with GLUT");

    glewExperimental = GL_TRUE; // 確保 GLEW 支援核心模式
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 初始化失敗！" << std::endl;
        return 0;
    }


    GLenum err = glGetError();  //錯誤檢測
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL Error: " << err << std::endl;
    }
    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);        //鍵盤監控

    glutPassiveMotionFunc(mouseMotion);

    glutMouseFunc(mouse);           // **註冊鼠標事件回呼函數**

    glutIdleFunc(idle);  // 註冊空閒函數

    glutMainLoop();
    return 0;
}