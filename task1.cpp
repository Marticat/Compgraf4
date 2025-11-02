#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

// --- Патч: 16 контрольных точек ---
glm::vec3 controlPoints[16] = {
    {0.0, 0.0, 0.0}, {2.0, 0.0, 1.5}, {4.0, 0.0, 2.9}, {6.0, 0.0, 0.0},
    {0.0, 2.0, 1.1}, {2.0, 2.0, 3.9}, {4.0, 2.0, 3.1}, {6.0, 2.0, 0.7},
    {0.0, 4.0, -0.5},{2.0, 4.0, 2.6},{4.0, 4.0, 2.4},{6.0, 4.0, 0.4},
    {0.0, 6.0, 0.3}, {2.0, 6.0, -1.1},{4.0, 6.0, 1.3},{6.0, 6.0, -0.2}
};

// --- Параметры патча ---
int tessellation = 10;
int selectedPoint = 0;
bool needsUpdate = true; // Флаг для оптимизации обновлений

// --- Камера ---
glm::vec3 camPos = glm::vec3(3.0f, 5.0f, 15.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);
float camYaw = -90.0f;
float camPitch = 0.0f;
float camSpeed = 0.5f;
float camTurnSpeed = 2.0f;

// --- Вершины, нормали и индексы патча ---
std::vector<glm::vec3> vertices;
std::vector<glm::vec3> normals;
std::vector<unsigned int> indices;

// --- OpenGL объекты ---
GLuint patchVAO, patchVBO, patchNBO, patchEBO;
GLuint pointsVAO, pointsVBO, pointsColorVBO;
GLuint axesVAO, axesVBO; // Для осей

// --- Прототипы функций ---
float B(int i, float t);
glm::vec3 evaluateBezier(float u, float v);
void generatePatch();
void setupPatchBuffers();
void setupPointsBuffers();
void setupAxesBuffers();
void drawAxes(GLuint shaderProgram);
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);

// =================== Main ===================
int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 800, "Bezier Patch", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); // Включаем отсечение задних граней
    glPointSize(15.0f);

    // Инициализация объектов
    generatePatch();
    setupPatchBuffers();
    setupPointsBuffers();
    setupAxesBuffers();

    GLuint shaderProgram = createShaderProgram("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // --- Матрицы ---
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1000.0f / 800.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        // --- Свет и цвет ---
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(camPos));
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camPos));

        // --- Отрисовка патча ---
        glBindVertexArray(patchVAO);

        // Рисуем переднюю сторону (обычный режим)
        glUniform1i(glGetUniformLocation(shaderProgram, "isBackFace"), 0);
        glUniform3f(glGetUniformLocation(shaderProgram, "frontColor"), 0.8f, 0.5f, 0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "backColor"), 0.3f, 0.5f, 0.8f);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // --- Отрисовка контрольных точек ---
        glBindVertexArray(pointsVAO);
        glUniform1i(glGetUniformLocation(shaderProgram, "isBackFace"), 0);
        glDrawArrays(GL_POINTS, 0, 16);

        // --- Отрисовка осей ---
        drawAxes(shaderProgram);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Освобождение ресурсов
    glDeleteVertexArrays(1, &patchVAO);
    glDeleteBuffers(1, &patchVBO);
    glDeleteBuffers(1, &patchNBO);
    glDeleteBuffers(1, &patchEBO);
    glDeleteVertexArrays(1, &pointsVAO);
    glDeleteBuffers(1, &pointsVBO);
    glDeleteBuffers(1, &pointsColorVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);

    glfwTerminate();
    return 0;
}

// =================== Функции ===================

// --- Базисные функции Бернштейна ---
float B(int i, float t) {
    int n = 3;
    int C[4] = { 1,3,3,1 };
    return C[i] * pow(t, i) * pow(1 - t, n - i);
}

// --- Оценка патча ---
glm::vec3 evaluateBezier(float u, float v) {
    glm::vec3 p(0, 0, 0);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            p += B(i, u) * B(j, v) * controlPoints[i * 4 + j];
    return p;
}

// --- Генерация патча с улучшенным пересчетом нормалей ---
void generatePatch() {
    vertices.clear();
    normals.clear();
    indices.clear();

    // Генерация вершин
    for (int i = 0; i <= tessellation; i++) {
        float u = i / (float)tessellation;
        for (int j = 0; j <= tessellation; j++) {
            float v = j / (float)tessellation;
            vertices.push_back(evaluateBezier(u, v));
            normals.push_back(glm::vec3(0.0f));
        }
    }

    // Генерация индексов и вычисление нормалей
    for (int i = 0; i < tessellation; i++) {
        for (int j = 0; j < tessellation; j++) {
            int idx = i * (tessellation + 1) + j;
            int idxRight = idx + 1;
            int idxDown = idx + tessellation + 1;
            int idxDiag = idx + tessellation + 2;

            // Первый треугольник
            indices.push_back(idx);
            indices.push_back(idxRight);
            indices.push_back(idxDown);

            // Второй треугольник
            indices.push_back(idxRight);
            indices.push_back(idxDiag);
            indices.push_back(idxDown);

            // Вычисление нормалей для каждого треугольника
            glm::vec3 v1 = vertices[idxRight] - vertices[idx];
            glm::vec3 v2 = vertices[idxDown] - vertices[idx];
            glm::vec3 normal1 = glm::normalize(glm::cross(v1, v2));

            glm::vec3 v3 = vertices[idxDiag] - vertices[idxRight];
            glm::vec3 v4 = vertices[idxDown] - vertices[idxRight];
            glm::vec3 normal2 = glm::normalize(glm::cross(v3, v4));

            // Усреднение нормалей
            normals[idx] += normal1;
            normals[idxRight] += normal1 + normal2;
            normals[idxDown] += normal1 + normal2;
            normals[idxDiag] += normal2;
        }
    }

    // Нормализация всех нормалей
    for (auto& n : normals) {
        if (glm::length(n) > 0.0f) {
            n = glm::normalize(n);
        }
    }
}

// --- Настройка VAO/VBO патча ---
void setupPatchBuffers() {
    if (patchVAO == 0) {
        glGenVertexArrays(1, &patchVAO);
        glGenBuffers(1, &patchVBO);
        glGenBuffers(1, &patchNBO);
        glGenBuffers(1, &patchEBO);
    }

    glBindVertexArray(patchVAO);

    glBindBuffer(GL_ARRAY_BUFFER, patchVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, patchNBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, patchEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}

// --- Настройка VAO/VBO контрольных точек ---
void setupPointsBuffers() {
    if (pointsVAO == 0) {
        glGenVertexArrays(1, &pointsVAO);
        glGenBuffers(1, &pointsVBO);
        glGenBuffers(1, &pointsColorVBO);
    }

    glm::vec3 pointColors[16];
    for (int i = 0; i < 16; i++)
        pointColors[i] = (i == selectedPoint) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);

    glBindVertexArray(pointsVAO);

    glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(controlPoints), controlPoints, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, pointsColorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), pointColors, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// --- Настройка VAO/VBO для осей (однократное создание) ---
void setupAxesBuffers() {
    // Сдвигаем оси по оси Z на небольшое значение
    glm::vec3 axes[] = {
        {0.0f, 0.0f, 2.0f}, {1.0f, 0.0f, 2.0f}, // X ось
        {0.0f, 0.0f, 2.0f}, {0.0f, 1.0f, 2.0f}, // Y ось  
        {0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, 3.0f}  // Z ось
    };

    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// --- Отрисовка осей ---
void drawAxes(GLuint shaderProgram) {
    glBindVertexArray(axesVAO);

    // X - красный
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Y - зеленый
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_LINES, 2, 2);

    // Z - синий
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
}

// --- Обработка клавиш ---
void processInput(GLFWwindow* window) {
    // --- Выход ---
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- Камера ---
    float speed = camSpeed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;

    // --- Сброс вида ---
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camPos = glm::vec3(3.0f, 5.0f, 15.0f);
        camFront = glm::vec3(0.0f, 0.0f, -1.0f);
        camUp = glm::vec3(0.0f, 1.0f, 0.0f);
        camYaw = -90.0f; camPitch = 0.0f;
    }

    // --- Выбор точки (стрелки влево/вправо) ---
    static bool leftPressedLast = false, rightPressedLast = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !leftPressedLast) {
        selectedPoint = (selectedPoint + 15) % 16;
        needsUpdate = true;
        leftPressedLast = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE) leftPressedLast = false;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !rightPressedLast) {
        selectedPoint = (selectedPoint + 1) % 16;
        needsUpdate = true;
        rightPressedLast = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE) rightPressedLast = false;

    // --- Движение выбранной точки ---
    float moveStep = 0.1f;
    bool pointMoved = false;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) { controlPoints[selectedPoint].y += moveStep; pointMoved = true; }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) { controlPoints[selectedPoint].y -= moveStep; pointMoved = true; }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) { controlPoints[selectedPoint].x -= moveStep; pointMoved = true; }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) { controlPoints[selectedPoint].x += moveStep; pointMoved = true; }
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) { controlPoints[selectedPoint].z += moveStep; pointMoved = true; }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) { controlPoints[selectedPoint].z -= moveStep; pointMoved = true; }

    if (pointMoved) needsUpdate = true;

    // --- Изменение разрешения патча ---
    static bool plusPressedLast = false, minusPressedLast = false;
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && !plusPressedLast) {
        tessellation = std::min(tessellation + 1, 50);
        needsUpdate = true;
        plusPressedLast = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_RELEASE) plusPressedLast = false;

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS && !minusPressedLast) {
        tessellation = std::max(tessellation - 1, 1);
        needsUpdate = true;
        minusPressedLast = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_RELEASE) minusPressedLast = false;

    // --- Обновление только при необходимости ---
    if (needsUpdate) {
        // Обновление цветов контрольных точек
        setupPointsBuffers();

        // Пересчет патча
        generatePatch();
        setupPatchBuffers();

        needsUpdate = false;
    }
}

// --- Resize окна ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// --- Создание шейдерной программы ---
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile(vertexPath), fShaderFile(fragmentPath);

    if (!vShaderFile.is_open() || !fShaderFile.is_open()) {
        std::cout << "Failed to open shader files\n";
        return 0;
    }

    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vertex, 512, NULL, infoLog); std::cout << "Vertex shader error: " << infoLog << "\n"; }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fragment, 512, NULL, infoLog); std::cout << "Fragment shader error: " << infoLog << "\n"; }

    GLuint ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(ID, 512, NULL, infoLog); std::cout << "Shader program error: " << infoLog << "\n"; }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return ID;
}