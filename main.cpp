#define STB_IMAGE_IMPLEMENTATION    
#define GLEW_STATIC

#include <GL/glew.h>     // GLEW �w�A�Ω�[�� OpenGL �X�i
#include <GL/freeglut.h>      // GLUT �M OpenGL ���Y
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "TextureLoader.h"  //�ɤJ����
#include "Camera.h"         //�ɤJ�۾�
#include "Enemy.h"         //�ɤJ�ĤH

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/*
�ҫ��������������A�N�S�ΤF
���������]�O
*/


/*
drawFunction�Ϊk:
x:�e y:�� z:�`�� xOffset:x�����q, yOffset:x�����q(����), zOffset:z�����q(�`��)
*/


float lastFrame = 0.0f; // �W�@�V���ɶ�

// �K�[�����ܼƨӱ����q�����A
bool flashlightOn = false;

int score = 0; // ���ƪ�l��0
float gameTime = 30.0f; // �˼�30��

bool gameOver = false;//�P�_�C���O�_����

float headshotDisplayTime = 0.0f; // �z�Y��ܮɶ��]��^

TextureLoader textureLoader;    // �ϥ� TextureLoader ���J���z

GLuint floorTexture;
GLuint wallTexture;
GLuint obstacleTexture;

struct Obstacle {
    float width, height, depth;
    float x, y, z;
};


// ���]���@�ӥ����ܼƨ��x�s��ê����T
std::vector<Obstacle> obstacles;


struct Bullet {
    glm::vec3 position; // �l�u��m
    glm::vec3 direction; // �l�u��V
    float speed; // �l�u�t��
    float lifetime; // �l�u�s�b�ɶ��]��^
};

std::vector<Bullet> bullets; // �l�u�C��
float bulletSpeed = 10.0f;   // �l�u�t��
float bulletLifetime = 5.0f; // �l�u�s���ɶ�


int maxBulletsInClip = 12;  // �C�Ӽu�����̤j�l�u�ƶq
int currentBullets = maxBulletsInClip;  // ��e�u�������l�u��
bool isReloading = false;  // �O�_���b���u
float reloadTime = 1.5f;  // ���u�һݮɶ�
float reloadTimer = 0.0f;  // ���u�p�ɾ�





//�ɤJ�Ҳ�

struct Vertex {
    glm::vec3 position;  // ���I��m
    glm::vec3 normal;    // �k�u�V�q
    glm::vec2 texCoords; // ���z����
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

        // �j�w���I�w��
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // �j�w���޽w��
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // �]�m���I�ݩʫ��w
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

        glBindVertexArray(0);
    }
};

std::vector<Mesh> gunMeshes; // �Ω�s�x�j�K�ҫ��������ܼ�




Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // �ѪR���I�ƾ�
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

    // �ѪR���޼ƾ�
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // ��^�ѪR�n�� Mesh
    return Mesh(vertices, indices);
}

void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes) {
    // �B�z��e�`�I���Ҧ�����
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene));
    }

    // ���k�B�z�l�`�I
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
    // �p��j�K���ҫ��x�}
    glm::mat4 gunModelMatrix = camera.getGunModelMatrix();

    // �]�w�T�w�޽u���x�}
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(gunModelMatrix));

    // ��V�j�K�ҫ�
    for (unsigned int i = 0; i < gunMeshes.size(); i++) {
        gunMeshes[i].Draw(); // ���] Mesh �䴩�L�ۦ⾹ø�s���覡
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
    // �N�ҫ���ƸѪR�æs�J gunMeshes
    ProcessAssimpScene(scene, gunMeshes);

    std::cout << "Loading model..." << std::endl;
}



// Player position and orientation
Camera camera; // �ϥ� Camera ���Ӻ޲z�۾�

// Keyboard input movement speed
float speed = 0.1f;

// Mouse position tracking
int lastMouseX, lastMouseY;
bool firstMouse = true;

// Obstacle position and size
const float obstacleX = 2.0f, obstacleZ = 2.0f;
const float obstacleSize = 1.0f;

//�l�u�I������(�|�Ϫ���Q�}�a �i�H���H)
bool checkBulletCollision(const glm::vec3& bulletPos) {
    // �ˬd�l�u�O�_������ê��
    for (auto it = obstacles.begin(); it != obstacles.end(); /* no increment here */) {
        auto& obs = *it;
        if (bulletPos.x >= obs.x - obs.width / 2 && bulletPos.x <= obs.x + obs.width / 2 &&
            bulletPos.y >= obs.y - obs.height / 2 && bulletPos.y <= obs.y + obs.height / 2 &&
            bulletPos.z >= obs.z - obs.depth / 2 && bulletPos.z <= obs.z + obs.depth / 2) {

            // �L�X�Q��������ê����T
            std::cout << "Obstacle hit: "
                << "Position(" << obs.x << ", " << obs.y << ", " << obs.z << "), "
                << "Size(" << obs.width << ", " << obs.height << ", " << obs.depth << ")\n";

            // �����Q��������ê��
            it = obstacles.erase(it);

            // ���}�c�l�[���
            gameTime += 3.0f;

            return true; // �l�u�I���o��
        }
        else {
            ++it;
        }
    }

    // �ˬd�l�u�O�_�����ĤH
    for (auto enemyIt = enemies.begin(); enemyIt != enemies.end(); ) {
        bool isHeadshot = false;
        if (enemyIt->isHit(bulletPos, isHeadshot)) { // �ˬd�O�_�����ĤH
            if (isHeadshot) {
                std::cout << "Headshot! Enemy eliminated instantly!" << std::endl;
                // ���������ĤH
                enemyIt = enemies.erase(enemyIt);
                score += 10;  // �Y�������[10��
            }
            else {
                enemyIt->health--; // ����
                std::cout << "Enemy hit! Remaining health: " << enemyIt->health << std::endl;

                if (enemyIt->health <= 0) { // �p�G�ĤH��q����0
                    std::cout << "Enemy eliminated!\n";
                    score += 10;  // �C�����@�ӼĤH�[10��

                    // �����ĤH
                    enemyIt = enemies.erase(enemyIt);
                }
                else {
                    ++enemyIt;
                }
            }
            return true; // �l�u�I���o��
        }
        else {
            ++enemyIt;
        }
    }

    return false; // �S���I��
}


//�ƹ��I�� �o�g
void mouse(int button, int state, int x, int y) {
    static bool isZoomed = false; // �Ω�O����j���A

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (isReloading || currentBullets <= 0) {
            // �p�G���b���u�Τl�u�ӺɡA�L�k�g��
            return;
        }

        // �Ыؤl�u
        Bullet bullet;
        bullet.position = camera.cameraPos; // �l�u�q�۾���m�o�g
        bullet.direction = camera.cameraFront;  // �l�u��V�P�۾��¦V�@�P
        bullet.speed = bulletSpeed;
        bullet.lifetime = bulletLifetime;

        bullets.push_back(bullet); // �N�l�u�[�J�C��
        currentBullets--; // ��ֳѾl�l�u��

        // �p�G�l�u�ӺɡA�}�l�۰ʴ��u
        if (currentBullets == 0) {
            isReloading = true;
            reloadTimer = 0.0f; // ��l�ƴ��u�p�ɾ�
        }
    }
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        // ������j�P�٭쪬�A
        isZoomed = !isZoomed;

        if (isZoomed) {
            // ��j����
            camera.fov /= 2.0f; // ���] fov �O�۾��������ݩ�
        }
        else {
            // �٭����
            camera.fov *= 2.0f;
        }
    }
}


//��s�e������
void updateProjectionMatrix() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.fov, (float)800 / (float)600, 0.1f, 100.0f); // ���] screenWidth �M screenHeight �O�̹����e��
    glMatrixMode(GL_MODELVIEW);
}





void updateBullets(float deltaTime) {
    for (auto it = bullets.begin(); it != bullets.end();) {
        // ��s�l�u��m
        it->position += it->direction * it->speed * deltaTime;
        it->lifetime -= deltaTime;

        // �����W�ɩμ������l�u
        if (it->lifetime <= 0.0f || checkBulletCollision(it->position)) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }
}

//���u
void updateReload(float deltaTime) {
    if (isReloading) {
        reloadTimer += deltaTime;
        if (reloadTimer >= reloadTime) {
            // ���u����
            currentBullets = maxBulletsInClip;
            isReloading = false;
            reloadTimer = 0.0f;
        }
    }
    else if (currentBullets == 0) {
        // ��e�u���S���l�u�A�۰ʶ}�l���u
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


    float tileWidth = width / tilesX;           //�N���`�e�שM���װ��H���j���ƶq�A�p��C�����j���ؤo�C
    float tileHeight = height / tilesY;

    for (int i = 0; i < tilesX; ++i) {
        for (int j = 0; j < tilesY; ++j) {
            float x = offsetX + i * tileWidth - width / 2.0f;       //�p��C�����j���U���� X �M Y ���СCoffsetX �M offsetY �Ω󰾲��𪺦�m
            float y = offsetY + j * tileHeight - height / 2.0f;

            glBegin(GL_QUADS);
            if (axis == 'z') {                  // ø�s Z �b��V���𭱡A�C�ӳ��I�����쯾�z���y��
                glTexCoord2f(0.0f, 0.0f); glVertex3f(x, y, offsetZ);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(x + tileWidth, y, offsetZ);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(x + tileWidth, y + tileHeight, offsetZ);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(x, y + tileHeight, offsetZ);
            }
            else if (axis == 'x') {             // ø�s X �b��V����
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
    drawTiledFloor(20, 1.0f); // ���� 20 x 20 ������A�C�Ӥ���j�p�� 1.0 ���
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




// �H���P���Ҫ��I��
bool checkBoundaryCollision(float nextX, float nextY, float nextZ) {
    // �ˬd�P�a�O���I��
    if (nextY < 0.5f) return true;

    // �ˬd�P������I��
    if (nextX <= -9.5f || nextX >= 9.5f || nextZ <= -9.5f || nextZ >= 9.5f) return true;

    // �ˬd�P��ê�����I��
    for (const auto& obstacle : obstacles) {
        float xMin = obstacle.x - obstacle.width / 2.0f;
        float xMax = obstacle.x + obstacle.width / 2.0f;
        float yMin = obstacle.y - obstacle.height / 2.0f;
        float yMax = obstacle.y + obstacle.height / 2.0f;
        float zMin = obstacle.z - obstacle.depth / 2.0f;
        float zMax = obstacle.z + obstacle.depth / 2.0f;

        // AABB �I���˴�
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
        // ø�s�l�u�A�o�̥H�p�y���
        glutSolidSphere(0.1f, 16, 16); // �b�|0.1���y
        glPopMatrix();
    }
}



// ��L����
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
    case 'f': // ������q��
        flashlightOn = !flashlightOn;
        break;
    case 'r': // ���u
        if (!isReloading && currentBullets < maxBulletsInClip) {
            isReloading = true;    // �}�l���u
            reloadTimer = 0.0f;    // ��l�ƴ��u�p�ɾ�
        }
        break;
    case 27: // ��������
        exit(0);
    }

    // �ˬd�I��
    if (checkBoundaryCollision(camera.cameraPos.x, camera.cameraPos.y, camera.cameraPos.z)) {
        camera.cameraPos = oldPos; // Revert to old position if collision occurs
    }

    glutPostRedisplay();
}

// Mouse motion callback for camera rotation
void mouseMotion(int x, int y) {
    // �]�w�����������y��
    int windowCenterX = 800 / 2;
    int windowCenterY = 600 / 2;

    if (firstMouse) {
        lastMouseX = windowCenterX;
        lastMouseY = windowCenterY;
        firstMouse = false;
        glutWarpPointer(windowCenterX, windowCenterY); // ��l�Ʒƹ���m
        return;
    }

    // �p��ƹ��������q
    int offsetX = x - lastMouseX;
    int offsetY = lastMouseY - y; // Reversed: y-coordinates go from bottom to top

    // ��s�۾�����
    camera.processMouseMovement(offsetX, offsetY);

    // ���]�ƹ����Ш��������
    glutWarpPointer(windowCenterX, windowCenterY);

    // ��s�̫᪺�ƹ���m����������
    lastMouseX = windowCenterX;
    lastMouseY = windowCenterY;

    glutPostRedisplay();
}

// �H���ͦ��@�ӻ�ê������m�å[�J�V�q
void spawnNewObstacle() {
    float size = 0.5f; // �T�w�j�p
    float y = size / 2.0f; // �O����ê�����a�O�W�Ay = size���@�b
    float x = static_cast<float>(std::rand() % 10 - 5); // �H���ͦ��d�� [-5, 5]
    float z = static_cast<float>(std::rand() % 10 - 5); // �H���ͦ��d�� [-5, 5]

    // �N�s��ê���[�J�V�q
    obstacles.push_back({ size, size, size, x, y, z });
}


// ø�s�Ҧ��{������ê��
void drawObstacles() {
    // �p�G�S����ê���A�H���ͦ��@��
    if (obstacles.empty()) {
        spawnNewObstacle(); // �I�s�ͦ��s��ê�����禡
    }

    // ø�s�Ҧ���ê��
    for (const auto& obstacle : obstacles) {
        drawObstacle(obstacle.width, obstacle.height, obstacle.depth,
            obstacle.x, obstacle.y, obstacle.z);
    }
}

void initializeObstacles() {
    for (int i = 0; i < 2; i++) {
        spawnNewObstacle();
    }

    // ��l��ø�s��ê��
    drawObstacles();
}




int windowWidth = 800; // �w�]�e��
int windowHeight = 600; // �w�]����

//�Ǥ�
void drawCrosshair() {
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT); // �O�s�C��������A

    glMatrixMode(GL_PROJECTION); // �]�m��v�Ҧ�
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1); // �]�m�����v
    glMatrixMode(GL_MODELVIEW); // �]�m�ҫ�����
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST); // �T�β`�״��աA���Ǥ���ܦb�̫e��

    float centerX = windowWidth / 2.0f;
    float centerY = windowHeight / 2.0f;
    float crosshairSize = 10.0f; // �Ǥߪ��j�p

    glColor3f(1.0f, 1.0f, 1.0f); // �]�m���զ�Ǥ�
    glBegin(GL_LINES);
    // �����u
    glVertex2f(centerX - crosshairSize, centerY);
    glVertex2f(centerX + crosshairSize, centerY);
    // �����u
    glVertex2f(centerX, centerY - crosshairSize);
    glVertex2f(centerX, centerY + crosshairSize);
    glEnd();

    glEnable(GL_DEPTH_TEST); // ��_�`�״���

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib(); // ��_�C��������A
}






//�C����������
void displayGameOver() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // �M���e��

    // ��� "GAME OVER" ��r
    glColor3f(1.0f, 0.0f, 0.0f); // �����r
    glRasterPos2i(350, 300); // �]�m��r��m�]�e�������^
    std::string gameOverText = "GAME OVER";
    for (char c : gameOverText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // ��ܱo���A�N��m�]�m�b "GAME OVER" �U��
    std::string scoreText = "Score: " + std::to_string(score); // ���] score �O����������ܼ�
    glRasterPos2i(350, 250); // �]�m�o����r��m�A�y�L�C�� "GAME OVER"
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // ��ܨC�Ӧr��
    }
}




// ��q�������ݩ�
GLfloat flashlightDiffuse[] = { 1.5f, 1.5f, 1.2f, 1.0f }; // �����G��
GLfloat flashlightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // �I����
GLfloat flashlightDirection[] = { 0.0f, 0.0f, -1.0f };     // �E����V


// ��s��q��������m�M��V
void updateFlashlight() {
    if (flashlightOn) {
        // ��s������m���۾���m
        flashlightPosition[0] = camera.cameraPos.x;
        flashlightPosition[1] = camera.cameraPos.y;
        flashlightPosition[2] = camera.cameraPos.z;

        // ��s������V���۾��e�V�V�q
        glm::vec3 normalizedDirection = glm::normalize(camera.cameraFront);
        flashlightDirection[0] = normalizedDirection.x;
        flashlightDirection[1] = normalizedDirection.y;
        flashlightDirection[2] = normalizedDirection.z;

        // �]�m�� OpenGL ����
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

    // �N�ƥΥ�����m��۾��W�� 10 ���B
    GLfloat backupLightPosition[] = {
        cameraPos.x,
        cameraPos.y + 10.0f, // �W�� 10 ���
        cameraPos.z,
        1.0f
    };

    glLightfv(GL_LIGHT2, GL_POSITION, backupLightPosition);
}


void initBackupLight() {
    glEnable(GL_LIGHT2);
    GLfloat backupLightDiffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f }; // �A�����G��
    GLfloat backupLightPosition[] = { 0.0f, 10.0f, 0.0f, 1.0f }; // ���a�����W�� 10 ���B
    glLightfv(GL_LIGHT2, GL_DIFFUSE, backupLightDiffuse);
    glLightfv(GL_LIGHT2, GL_POSITION, backupLightPosition);

    // ����I��
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.02f);  // �w�C�I��
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.001f); // ���Z���󧡤�
}



// ��l�ƥ���
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);

    // ��q���ݩ�
    glLightfv(GL_LIGHT1, GL_DIFFUSE, flashlightDiffuse);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 90.0f);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 10.0f);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.005f);

    // �վ����ҥ��j��
    GLfloat ambientLight[] = { 0.4f, 0.4f, 0.4f, 1.0f }; // �����j��
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

    // ��l�ƳƥΥ���
    initBackupLight();

    // �T�Ψ�L����
    glDisable(GL_LIGHT0);
}








// ��l�ƨ��
void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D); // �}�ү��z�\��

    glMatrixMode(GL_PROJECTION);
    gluPerspective(45.0, 800.0 / 600.0, 0.1, 100.0);

    // �ϥ� stb_image ���J���z
    wallTexture = textureLoader.LoadTexture("img/brick.jpg");
    floorTexture = textureLoader.LoadTexture("img/metal.jpg");
    obstacleTexture = textureLoader.LoadTexture("img/box.jpg");

    // ��l�ƻ�ê��
    initializeObstacles();


    // ��l�ƥ���
    //initLighting();


    // ���÷ƹ�����
    glutSetCursor(GLUT_CURSOR_NONE);

    // �ͦ���l�ĤH
    for (int i = 0; i < 5; ++i) { // ��l�ͦ� 5 �ӼĤH
        spawnEnemy();
    }
}


// �Ŷ���ơA���_��ø
void idle() {
    // �b�o�̱z�i�H�[�J�ݭn�����s���޿�
    glutPostRedisplay();  // �ШD���sø�s�̹�
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    // �]�m�۾�����
    glm::mat4 view = camera.getViewMatrix();
    glLoadMatrixf(glm::value_ptr(view));

    // �k���j���Y
    updateProjectionMatrix();


    // ��s��q��
    // ��s����
    updateBackupLight();
    updateFlashlight();
    

    drawFloor();
    drawWalls(); // Draw walls

    // ø�s�Ҧ��ثe�s�b����ê��
    drawObstacles();

    // ø�s�ĤH
    renderEnemies();

    

    // ��V�Ǥ�
    drawCrosshair();

    // ��s�l�u
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;  // �H�����
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;


    updateBullets(deltaTime); // ��s�l�u��m

    // ø�s�l�u
    drawBullets();

    // ���u
    updateReload(deltaTime);



    // ��V�j�K
    //renderGun(camera);

    /*������ܳo�����|�����A�Y�N�r��אּ��L�C��*/
    // ��ܤ���
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1); // �]�m���g��v�A�N�e���]�m��2D

    glColor3f(1.0f, 1.0f, 1.0f); // �]�m����r���C��
    glRasterPos2i(10, 20); // �]�m��r��ܦ�m�]���W���^

    // ��ܤ���
    std::string scoreText = "Score: " + std::to_string(score);
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // ��ܨC�Ӧr��
    }

    // ��ܭ˼Ʈɶ�
    glColor3f(1.0f, 1.0f, 1.0f); // �]�m�զ�r���C��
    glRasterPos2i(700, 20); // �]�m��r��ܦ�m�]�k�W���^
    std::string timeText = "Time: " + std::to_string(static_cast<int>(gameTime));
    for (char c : timeText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c); // ��ܨC�Ӧr��
    }
    gameTime -= deltaTime; // ��֭˼Ʈɶ�



    // �b�ù��W�褤ø�s "Headshot"
    if (headshotDisplayTime > 0.0f) {
        // �O�s��e�x�}�M�ݩ�
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

        // �����ܥ��g��v
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, 800, 600, 0, -1, 1); // �]�m���g��v

        // �����ܼҫ����ϯx�}
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // �]�m��r�C���ø�s
        glDisable(GL_LIGHTING); // �T�οO���]����v�T�C��^
        glColor3f(1.0f, 0.0f, 0.0f); // ����r��
        glRasterPos2i(400 - 50, 50); // �e���W�褤��
        std::string headshotText = "HEADSHOT!";
        for (char c : headshotText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        // ��_�x�}�M�ݩ�
        glPopMatrix();          // ��_�ҫ����ϯx�}
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();          // ��_��v�x�}
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();          // ��_�Ҧ��O�s���ݩ�

        // �����ܮɶ�
        headshotDisplayTime -= deltaTime;
    }


    if (isReloading) {
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT); // �O�s��e���A

        // �����ܥ��g��v
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, 800, 600, 0, -1, 1); // �]�m���g��v

        // �����ܼҫ����ϯx�}
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // ��ܴ��u��r
        glDisable(GL_LIGHTING); // �T�οO���A�קK�v�T�C��
        glColor3f(1.0f, 1.0f, 0.0f); // �]�m�����r
        glRasterPos2i(400 - 50, 100); // �e�������W��
        std::string reloadText = "Reloading...";
        for (char c : reloadText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        // ��_�x�}�P���A
        glPopMatrix();          // ��_�ҫ����ϯx�}
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();          // ��_��v�x�}
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();          // ��_�Ҧ��O�s���ݩ�
    }

    // ��ܳѾl�l�u/�̤j�l�u��
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT); // �O�s���A

    // �����ܥ��g��v
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1); // �]�m���g��v

    // �����ܼҫ����ϯx�}
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // ��ܳѾl�l�u��
    glDisable(GL_LIGHTING); // �T�οO���A�קK�v�T��r�C��
    glColor3f(1.0f, 1.0f, 0.0f); // �]�m�����r
    glRasterPos2i(10, 580); // ���U�����
    std::string ammoText = "Ammo: " + std::to_string(currentBullets) + "/" + std::to_string(maxBulletsInClip);
    for (char c : ammoText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // ��_�x�}�P���A
    glPopMatrix();          // ��_�ҫ����ϯx�}
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();          // ��_��v�x�}
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();          // ��_�Ҧ��O�s���ݩ�




    // �C�������ˬd
    if (gameTime <= 0.0f) {
        gameOver = true;
    }
    
    //�C������
    if (gameOver) { 
       displayGameOver(); // ��� Game Over �e��
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

    glewExperimental = GL_TRUE; // �T�O GLEW �䴩�֤߼Ҧ�
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW ��l�ƥ��ѡI" << std::endl;
        return 0;
    }


    GLenum err = glGetError();  //���~�˴�
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL Error: " << err << std::endl;
    }
    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);        //��L�ʱ�

    glutPassiveMotionFunc(mouseMotion);

    glutMouseFunc(mouse);           // **���U���Шƥ�^�I���**

    glutIdleFunc(idle);  // ���U�Ŷ����

    glutMainLoop();
    return 0;
}