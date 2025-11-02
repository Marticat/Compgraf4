#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <string>

// ==================== CONSTANTS AND GLOBALS ====================

const float PI = 3.14159265358979323846f;
const float TWO_PI = 2.0f * PI;

struct GameObject {
    GLuint VAO, VBO, EBO;
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 color;
    glm::vec3 position;
    int objectID;
};

struct TexturedBezierPatch {
    GLuint VAO, VBO, EBO, textureVBO;
    GLuint texture;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;
    int tessellation = 12;
};

// ==================== CAMERA SYSTEM ====================

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f, float pitch = 0.0f)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f),
        MouseSensitivity(0.1f), Zoom(45.0f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(int direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == 0) // FORWARD
            Position += Front * velocity;
        if (direction == 1) // BACKWARD
            Position -= Front * velocity;
        if (direction == 2) // LEFT
            Position -= Right * velocity;
        if (direction == 3) // RIGHT
            Position += Right * velocity;
        if (direction == 4) // UP
            Position += WorldUp * velocity;
        if (direction == 5) // DOWN
            Position -= WorldUp * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

    void Reset() {
        Position = glm::vec3(0.0f, 2.0f, 8.0f);
        Yaw = -90.0f;
        Pitch = 0.0f;
        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

// Application state
std::vector<GameObject> objects;
TexturedBezierPatch texturedPatch;
Camera camera;
bool antiAliasingEnabled = true;
bool textureMappingEnabled = false;
bool proceduralTexturingEnabled = false;
GLuint FBO, pickingTexture;
GLuint mainShader, pickingShader, textureShader, proceduralShader;

// Mouse state
double lastX = 400.0, lastY = 300.0;
bool firstMouse = true;
bool mousePressed = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Bezier control points
glm::vec3 controlPoints[16] = {
    {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 1.5f}, {4.0f, 0.0f, 2.9f}, {6.0f, 0.0f, 0.0f},
    {0.0f, 2.0f, 1.1f}, {2.0f, 2.0f, 3.9f}, {4.0f, 2.0f, 3.1f}, {6.0f, 2.0f, 0.7f},
    {0.0f, 4.0f, -0.5f},{2.0f, 4.0f, 2.6f},{4.0f, 4.0f, 2.4f},{6.0f, 4.0f, 0.4f},
    {0.0f, 6.0f, 0.3f}, {2.0f, 6.0f, -1.1f},{4.0f, 6.0f, 1.3f},{6.0f, 6.0f, -0.2f}
};

// ==================== SHADER SOURCES ====================

const char* mainVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0f));
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
)";

const char* mainFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main() {
    // Simple lighting without normals
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Use a fixed normal (approximation)
    vec3 normal = vec3(0.0f, 1.0f, 0.0f);
    
    // Ambient
    float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * lightColor;
    
    // Specular (simple approximation)
    float specularStrength = 0.3f;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 16.0f);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse) * objectColor + specular;
    FragColor = vec4(result, 1.0f);
}
)";

const char* pickingVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
)";

const char* pickingFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 objectColor;
void main() {
    FragColor = vec4(objectColor, 1.0f);
}
)";

const char* textureVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0f));
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
)";

const char* textureFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec2 TexCoord;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D texture1;

void main() {
    // Simple lighting for textured objects
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 normal = vec3(0.0f, 1.0f, 0.0f); // Fixed normal
    
    // Ambient
    float ambientStrength = 0.2f;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * lightColor;
    
    // Texture color
    vec3 textureColor = texture(texture1, TexCoord).rgb;
    
    vec3 result = (ambient + diffuse) * textureColor;
    FragColor = vec4(result, 1.0f);
}
)";

const char* proceduralVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0f));
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
)";

const char* proceduralFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

vec3 procedural3DTexture(vec3 worldPos) {
    // Create marble-like 3D pattern
    float scale = 2.0f;
    vec3 p = worldPos * scale;
    
    float turbulence = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    
    for (int i = 0; i < 6; i++) {
        turbulence += amplitude * abs(sin(p.x * frequency + sin(p.y * frequency * 0.7f) + sin(p.z * frequency * 1.3f)));
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    
    turbulence = 0.5f * sin(8.0f * turbulence) + 0.5f;
    
    vec3 color1 = vec3(0.7f, 0.7f, 0.9f);
    vec3 color2 = vec3(0.1f, 0.1f, 0.3f);
    vec3 color3 = vec3(0.9f, 0.9f, 0.7f);
    
    if (turbulence < 0.4f) {
        return mix(color1, color2, turbulence / 0.4f);
    } else if (turbulence < 0.7f) {
        return mix(color2, color3, (turbulence - 0.4f) / 0.3f);
    } else {
        return color3;
    }
}

void main() {
    // Simple lighting for procedural texture
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 normal = vec3(0.0f, 1.0f, 0.0f); // Fixed normal
    
    float diff = max(dot(normal, lightDir), 0.0f);
    
    // 3D procedural texture color with lighting
    vec3 textureColor = procedural3DTexture(FragPos);
    vec3 ambient = 0.3f * textureColor;
    vec3 diffuse = diff * lightColor * textureColor;
    
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0f);
}
)";

// ==================== UTILITY FUNCTIONS ====================

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Shader program linking failed:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

float B(int i, float t) {
    int n = 3;
    int C[4] = { 1, 3, 3, 1 };
    return static_cast<float>(C[i]) * powf(t, static_cast<float>(i)) * powf(1.0f - t, static_cast<float>(n - i));
}

glm::vec3 evaluateBezier(float u, float v) {
    glm::vec3 p(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            p += B(i, u) * B(j, v) * controlPoints[i * 4 + j];
    return p;
}

glm::vec3 generateRandomColor() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return glm::vec3(dis(gen), dis(gen), dis(gen));
}

// ==================== GEOMETRY GENERATION ====================

void generateSphere(GameObject& obj, float radius, int sectors, int stacks) {
    obj.vertices.clear();
    obj.indices.clear();

    float sectorStep = TWO_PI / static_cast<float>(sectors);
    float stackStep = PI / static_cast<float>(stacks);

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2.0f - static_cast<float>(i) * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = static_cast<float>(j) * sectorStep;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            obj.vertices.push_back(glm::vec3(x, y, z));
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                obj.indices.push_back(static_cast<unsigned int>(k1));
                obj.indices.push_back(static_cast<unsigned int>(k2));
                obj.indices.push_back(static_cast<unsigned int>(k1 + 1));
            }
            if (i != (stacks - 1)) {
                obj.indices.push_back(static_cast<unsigned int>(k1 + 1));
                obj.indices.push_back(static_cast<unsigned int>(k2));
                obj.indices.push_back(static_cast<unsigned int>(k2 + 1));
            }
        }
    }
}

void generateCube(GameObject& obj, float size) {
    float half = size / 2.0f;
    obj.vertices = {
        {-half, -half, half}, {half, -half, half}, {half, half, half}, {-half, half, half},
        {-half, -half, -half}, {-half, half, -half}, {half, half, -half}, {half, -half, -half},
        {-half, half, -half}, {-half, half, half}, {half, half, half}, {half, half, -half},
        {-half, -half, -half}, {half, -half, -half}, {half, -half, half}, {-half, -half, half},
        {half, -half, -half}, {half, half, -half}, {half, half, half}, {half, -half, half},
        {-half, -half, -half}, {-half, -half, half}, {-half, half, half}, {-half, half, -half}
    };

    obj.indices = {
        0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,10, 10,11,8,
        12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
    };
}

void generateCone(GameObject& obj, float radius, float height, int sectors) {
    obj.vertices.clear();
    obj.indices.clear();

    // Base vertices
    obj.vertices.push_back(glm::vec3(0.0f, -height / 2.0f, 0.0f)); // Center of base
    for (int i = 0; i <= sectors; ++i) {
        float sectorAngle = TWO_PI * static_cast<float>(i) / static_cast<float>(sectors);
        float x = radius * cosf(sectorAngle);
        float z = radius * sinf(sectorAngle);
        obj.vertices.push_back(glm::vec3(x, -height / 2.0f, z));
    }

    // Apex
    obj.vertices.push_back(glm::vec3(0.0f, height / 2.0f, 0.0f));

    // Base indices
    for (int i = 1; i <= sectors; ++i) {
        obj.indices.push_back(0);
        obj.indices.push_back(static_cast<unsigned int>(i));
        obj.indices.push_back(static_cast<unsigned int>(i + 1));
    }

    // Side indices
    int apexIndex = sectors + 2;
    for (int i = 1; i <= sectors; ++i) {
        obj.indices.push_back(static_cast<unsigned int>(i));
        obj.indices.push_back(static_cast<unsigned int>(apexIndex));
        obj.indices.push_back(static_cast<unsigned int>(i + 1));
    }
}

void generateTexturedBezierPatch(TexturedBezierPatch& patch) {
    patch.vertices.clear();
    patch.texCoords.clear();
    patch.indices.clear();

    // Generate vertices with texture coordinates
    for (int i = 0; i <= patch.tessellation; i++) {
        float u = static_cast<float>(i) / static_cast<float>(patch.tessellation);
        for (int j = 0; j <= patch.tessellation; j++) {
            float v = static_cast<float>(j) / static_cast<float>(patch.tessellation);
            patch.vertices.push_back(evaluateBezier(u, v));
            patch.texCoords.push_back(glm::vec2(u, v));
        }
    }

    // Generate indices
    for (int i = 0; i < patch.tessellation; i++) {
        for (int j = 0; j < patch.tessellation; j++) {
            int idx = i * (patch.tessellation + 1) + j;
            int idxRight = idx + 1;
            int idxDown = idx + patch.tessellation + 1;
            int idxDiag = idx + patch.tessellation + 2;

            patch.indices.push_back(static_cast<unsigned int>(idx));
            patch.indices.push_back(static_cast<unsigned int>(idxRight));
            patch.indices.push_back(static_cast<unsigned int>(idxDown));

            patch.indices.push_back(static_cast<unsigned int>(idxRight));
            patch.indices.push_back(static_cast<unsigned int>(idxDiag));
            patch.indices.push_back(static_cast<unsigned int>(idxDown));
        }
    }
}

// ==================== OPENGL SETUP ====================

void setupObjectBuffers(GameObject& obj) {
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);

    glBindVertexArray(obj.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(obj.vertices.size() * sizeof(glm::vec3)),
        obj.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(obj.indices.size() * sizeof(unsigned int)),
        obj.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void setupTexturedPatchBuffers(TexturedBezierPatch& patch) {
    glGenVertexArrays(1, &patch.VAO);
    glGenBuffers(1, &patch.VBO);
    glGenBuffers(1, &patch.textureVBO);
    glGenBuffers(1, &patch.EBO);

    glBindVertexArray(patch.VAO);

    // Vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, patch.VBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(patch.vertices.size() * sizeof(glm::vec3)),
        patch.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, patch.textureVBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(patch.texCoords.size() * sizeof(glm::vec2)),
        patch.texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, patch.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(patch.indices.size() * sizeof(unsigned int)),
        patch.indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

GLuint createProceduralTexture() {
    const int width = 512, height = 512;
    std::vector<unsigned char> image(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t index = static_cast<size_t>((y * width + x) * 3);
            float fx = static_cast<float>(x) / static_cast<float>(width);
            float fy = static_cast<float>(y) / static_cast<float>(height);

            image[index + 0] = static_cast<unsigned char>(255.0f * (0.5f + 0.5f * sinf(fx * 10.0f)));
            image[index + 1] = static_cast<unsigned char>(255.0f * (0.5f + 0.5f * cosf(fy * 8.0f)));
            image[index + 2] = static_cast<unsigned char>(255.0f * (0.5f + 0.5f * sinf((fx + fy) * 6.0f)));
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

void setupPickingFramebuffer() {
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glGenTextures(1, &pickingTexture);
    glBindTexture(GL_TEXTURE_2D, pickingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

    GLuint RBO;
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ==================== INPUT HANDLING ====================

void processPicking(GLFWwindow* window, double x, double y) {
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);

    glUseProgram(pickingShader);

    for (const auto& obj : objects) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position);

        int r = (obj.objectID & 0x000000FF) >> 0;
        int g = (obj.objectID & 0x0000FF00) >> 8;
        int b = (obj.objectID & 0x00FF0000) >> 16;
        glm::vec3 idColor = glm::vec3(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f);

        glUniformMatrix4fv(glGetUniformLocation(pickingShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(pickingShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(pickingShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(pickingShader, "objectColor"), 1, glm::value_ptr(idColor));

        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(obj.indices.size()), GL_UNSIGNED_INT, 0);
    }

    glFlush();
    glFinish();

    unsigned char pixel[3];
    glReadPixels(static_cast<GLint>(x), static_cast<GLint>(600 - y), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    int pickedID = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);

    if (pickedID != 0) {
        for (auto& obj : objects) {
            if (obj.objectID == pickedID) {
                obj.color = generateRandomColor();
                std::cout << "Object " << obj.objectID << " color changed!" << std::endl;
                break;
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mousePressed = true;
        processPicking(window, lastX, lastY);

        // Capture mouse for camera control
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // Release mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else {
        lastX = xpos;
        lastY = ypos;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement (only when mouse is captured)
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(0, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(3, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.ProcessKeyboard(4, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.ProcessKeyboard(5, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camera.Reset();
    }

    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        antiAliasingEnabled = !antiAliasingEnabled;
        if (antiAliasingEnabled) {
            glEnable(GL_MULTISAMPLE);
            std::cout << "Anti-aliasing enabled" << std::endl;
        }
        else {
            glDisable(GL_MULTISAMPLE);
            std::cout << "Anti-aliasing disabled" << std::endl;
        }
        spacePressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spacePressed = false;
    }

    static bool tPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tPressed) {
        textureMappingEnabled = !textureMappingEnabled;
        proceduralTexturingEnabled = false;
        std::cout << "Texture mapping: " << (textureMappingEnabled ? "ON" : "OFF") << std::endl;
        tPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tPressed = false;
    }

    static bool pPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
        proceduralTexturingEnabled = !proceduralTexturingEnabled;
        textureMappingEnabled = false;
        std::cout << "Procedural texturing: " << (proceduralTexturingEnabled ? "ON" : "OFF") << std::endl;
        pPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        pPressed = false;
    }

    // Toggle mouse capture with TAB
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
        tabPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        tabPressed = false;
    }
}

// ==================== RENDERING ====================

void renderObjects() {
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);

    GLuint currentShader;
    if (proceduralTexturingEnabled) {
        currentShader = proceduralShader;
    }
    else if (textureMappingEnabled) {
        currentShader = textureShader;
    }
    else {
        currentShader = mainShader;
    }

    glUseProgram(currentShader);

    glUniform3fv(glGetUniformLocation(currentShader, "lightPos"), 1, glm::value_ptr(camera.Position));
    glUniform3fv(glGetUniformLocation(currentShader, "viewPos"), 1, glm::value_ptr(camera.Position));
    glUniform3f(glGetUniformLocation(currentShader, "lightColor"), 1.0f, 1.0f, 1.0f);

    for (const auto& obj : objects) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position);

        glUniformMatrix4fv(glGetUniformLocation(currentShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(currentShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(currentShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        if (!proceduralTexturingEnabled && !textureMappingEnabled) {
            glUniform3fv(glGetUniformLocation(currentShader, "objectColor"), 1, glm::value_ptr(obj.color));
        }

        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(obj.indices.size()), GL_UNSIGNED_INT, 0);
    }
}

void renderTexturedPatch() {
    if (!textureMappingEnabled) return;

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));

    glUseProgram(textureShader);

    glUniformMatrix4fv(glGetUniformLocation(textureShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(textureShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(textureShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(textureShader, "lightPos"), 1, glm::value_ptr(camera.Position));
    glUniform3fv(glGetUniformLocation(textureShader, "viewPos"), 1, glm::value_ptr(camera.Position));
    glUniform3f(glGetUniformLocation(textureShader, "lightColor"), 1.0f, 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturedPatch.texture);
    glUniform1i(glGetUniformLocation(textureShader, "texture1"), 0);

    glBindVertexArray(texturedPatch.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(texturedPatch.indices.size()), GL_UNSIGNED_INT, 0);
}

// ==================== MAIN ====================

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-aliasing

    GLFWwindow* window = glfwCreateWindow(800, 600, "Advanced Graphics Assignment - Professional Camera", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing and anti-aliasing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Create shaders
    mainShader = createShaderProgram(mainVertexShaderSource, mainFragmentShaderSource);
    pickingShader = createShaderProgram(pickingVertexShaderSource, pickingFragmentShaderSource);
    textureShader = createShaderProgram(textureVertexShaderSource, textureFragmentShaderSource);
    proceduralShader = createShaderProgram(proceduralVertexShaderSource, proceduralFragmentShaderSource);

    if (!mainShader || !pickingShader || !textureShader || !proceduralShader) {
        std::cout << "Failed to create shaders. Exiting." << std::endl;
        glfwTerminate();
        return -1;
    }

    // Setup picking framebuffer
    setupPickingFramebuffer();

    // Create objects
    GameObject sphere, cube, cone;
    generateSphere(sphere, 1.0f, 36, 18);
    generateCube(cube, 1.5f);
    generateCone(cone, 1.0f, 2.0f, 36);

    sphere.position = glm::vec3(-3.0f, 0.0f, 0.0f);
    cube.position = glm::vec3(0.0f, 0.0f, 0.0f);
    cone.position = glm::vec3(3.0f, 0.0f, 0.0f);

    sphere.color = glm::vec3(1.0f, 0.0f, 0.0f);
    cube.color = glm::vec3(0.0f, 1.0f, 0.0f);
    cone.color = glm::vec3(0.0f, 0.0f, 1.0f);

    sphere.objectID = 1;
    cube.objectID = 2;
    cone.objectID = 3;

    objects = { sphere, cube, cone };

    // Setup object buffers
    for (auto& obj : objects) {
        setupObjectBuffers(obj);
    }

    // Create textured Bezier patch
    generateTexturedBezierPatch(texturedPatch);
    texturedPatch.texture = createProceduralTexture();
    setupTexturedPatchBuffers(texturedPatch);

    // Print controls
    std::cout << "=== CONTROLS ===" << std::endl;
    std::cout << "CAMERA MOVEMENT (when mouse captured):" << std::endl;
    std::cout << "  W/S - Move forward/backward" << std::endl;
    std::cout << "  A/D - Move left/right" << std::endl;
    std::cout << "  Q/E - Move down/up" << std::endl;
    std::cout << "  Mouse - Look around" << std::endl;
    std::cout << "  Mouse Wheel - Zoom in/out" << std::endl;
    std::cout << "MOUSE CONTROLS:" << std::endl;
    std::cout << "  Left Click - Select object + Enable camera" << std::endl;
    std::cout << "  Right Click - Release camera" << std::endl;
    std::cout << "  TAB - Toggle camera mode" << std::endl;
    std::cout << "FEATURES:" << std::endl;
    std::cout << "  SPACE - Toggle anti-aliasing" << std::endl;
    std::cout << "  T - Toggle texture mapping" << std::endl;
    std::cout << "  P - Toggle procedural texturing" << std::endl;
    std::cout << "  R - Reset camera" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "=================" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderObjects();
        renderTexturedPatch();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &pickingTexture);
    glDeleteTextures(1, &texturedPatch.texture);

    for (auto& obj : objects) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
        glDeleteBuffers(1, &obj.EBO);
    }

    glDeleteVertexArrays(1, &texturedPatch.VAO);
    glDeleteBuffers(1, &texturedPatch.VBO);
    glDeleteBuffers(1, &texturedPatch.textureVBO);
    glDeleteBuffers(1, &texturedPatch.EBO);

    glDeleteProgram(mainShader);
    glDeleteProgram(pickingShader);
    glDeleteProgram(textureShader);
    glDeleteProgram(proceduralShader);

    glfwTerminate();
    return 0;
}