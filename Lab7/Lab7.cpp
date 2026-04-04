#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "model.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

float lightPos[3] = { 8.0f, 2.0f, 0.0f };

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool cameraUpKeyPressed = false;
bool cameraDownKeyPressed = false;
float verticalSpeed = 1.5f;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float sensitivity = 0.1f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ManipulatorPart {
    int startMeshIndex;
    int endMeshIndex;
    glm::vec3 rotationAxis;
    glm::vec3 pivotPoint;
    float rotationAngle;
    float minAngle;
    float maxAngle;
    glm::vec3 translationAxis;
    float translationOffset;
    float minOffset;
    float maxOffset;
    bool isTranslational;
};

std::vector<ManipulatorPart> manipulatorParts;

float rotationSpeed = 50.0f;
float translationSpeed = 0.2f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
std::string readShaderFile(const std::string& filePath);
unsigned int compileShader(GLenum type, const std::string& source);
unsigned int createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
void initializeManipulatorParts();
glm::mat4 calculateTransformForPart(const ManipulatorPart& part);

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab 7: Manipulator Control", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::string vertexShaderSource = readShaderFile("vertex_shader.glsl");
    std::string fragmentShaderSource = readShaderFile("fragment_shader.glsl");
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) return -1;

#ifdef _WIN32
    system("mkdir models 2>nul");
#else
    system("mkdir -p models 2>/dev/null");
#endif

    Model ourModel("models/Lab3.4.obj");
    if (ourModel.meshes.empty()) {
        std::cerr << "The model is not loaded!" << std::endl;
        return -1;
    }

    initializeManipulatorParts();
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glUseProgram(shaderProgram);

        glUniform3f(glGetUniformLocation(shaderProgram, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(glGetUniformLocation(shaderProgram, "time"), (float)glfwGetTime());

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3f(glGetUniformLocation(shaderProgram, "material.ambient"), 0.19125f, 0.0735f, 0.0225f);
        glUniform3f(glGetUniformLocation(shaderProgram, "material.diffuse"), 0.7038f, 0.27048f, 0.0828f);
        glUniform3f(glGetUniformLocation(shaderProgram, "material.specular"), 0.6f, 0.4f, 0.3f);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), 32.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.position"), lightPos[0], lightPos[1], lightPos[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.ambient"), 0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.diffuse"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.specular"), 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < ourModel.meshes.size(); i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -0.6f, 0.0f));

            if (!manipulatorParts.empty()) {
                model = model * calculateTransformForPart(manipulatorParts[0]);
            }

            if (i > 0) {
                int partIndex = -1;
                for (int j = 1; j < manipulatorParts.size(); j++) {
                    if (i >= manipulatorParts[j].startMeshIndex && i <= manipulatorParts[j].endMeshIndex) {
                        partIndex = j;
                        break;
                    }
                }
                if (partIndex != -1) {
                    model = model * calculateTransformForPart(manipulatorParts[partIndex]);
                }
            }

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            ourModel.meshes[i].Draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float lightSpeed = 2.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) lightPos[1] += lightSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) lightPos[1] -= lightSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) lightPos[0] += lightSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) lightPos[0] -= lightSpeed;
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) lightPos[2] += lightSpeed;
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) lightPos[2] -= lightSpeed;

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) cameraPos.y += verticalSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) cameraPos.y -= verticalSpeed * deltaTime;

    float rotSpeed = rotationSpeed * deltaTime;
    float moveSpeed = translationSpeed * deltaTime;

    // Base lift
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && manipulatorParts.size() > 0) {
        manipulatorParts[0].translationOffset += moveSpeed;
        if (manipulatorParts[0].translationOffset > manipulatorParts[0].maxOffset)
            manipulatorParts[0].translationOffset = manipulatorParts[0].maxOffset;
    }
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS && manipulatorParts.size() > 0) {
        manipulatorParts[0].translationOffset -= moveSpeed;
        if (manipulatorParts[0].translationOffset < manipulatorParts[0].minOffset)
            manipulatorParts[0].translationOffset = manipulatorParts[0].minOffset;
    }

    // Link1
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && manipulatorParts.size() > 1) {
        manipulatorParts[1].rotationAngle += rotSpeed;
        if (manipulatorParts[1].rotationAngle > manipulatorParts[1].maxAngle)
            manipulatorParts[1].rotationAngle = manipulatorParts[1].maxAngle;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && manipulatorParts.size() > 1) {
        manipulatorParts[1].rotationAngle -= rotSpeed;
        if (manipulatorParts[1].rotationAngle < manipulatorParts[1].minAngle)
            manipulatorParts[1].rotationAngle = manipulatorParts[1].minAngle;
    }

    // Link2
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && manipulatorParts.size() > 2) {
        manipulatorParts[2].rotationAngle += rotSpeed;
        if (manipulatorParts[2].rotationAngle > manipulatorParts[2].maxAngle)
            manipulatorParts[2].rotationAngle = manipulatorParts[2].maxAngle;
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && manipulatorParts.size() > 2) {
        manipulatorParts[2].rotationAngle -= rotSpeed;
        if (manipulatorParts[2].rotationAngle < manipulatorParts[2].minAngle)
            manipulatorParts[2].rotationAngle = manipulatorParts[2].minAngle;
    }

    // Link3
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && manipulatorParts.size() > 3) {
        manipulatorParts[3].rotationAngle += rotSpeed;
        if (manipulatorParts[3].rotationAngle > manipulatorParts[3].maxAngle)
            manipulatorParts[3].rotationAngle = manipulatorParts[3].maxAngle;
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && manipulatorParts.size() > 3) {
        manipulatorParts[3].rotationAngle -= rotSpeed;
        if (manipulatorParts[3].rotationAngle < manipulatorParts[3].minAngle)
            manipulatorParts[3].rotationAngle = manipulatorParts[3].minAngle;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        for (auto& part : manipulatorParts) {
            part.rotationAngle = 0.0f;
            part.translationOffset = 0.0f;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    float xposf = static_cast<float>(xpos);
    float yposf = static_cast<float>(ypos);
    if (firstMouse) {
        lastX = xposf;
        lastY = yposf;
        firstMouse = false;
        return;
    }
    float xoffset = (xposf - lastX) * sensitivity;
    float yoffset = (lastY - yposf) * sensitivity;
    lastX = xposf;
    lastY = yposf;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

unsigned int compileShader(GLenum type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

unsigned int createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertexShader == 0 || fragmentShader == 0) return 0;
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void initializeManipulatorParts() {
    manipulatorParts.clear();

    ManipulatorPart baseLift;
    baseLift.startMeshIndex = 0;
    baseLift.endMeshIndex = 0;
    baseLift.rotationAxis = glm::vec3(0.0f, 0.0f, 0.0f);
    baseLift.pivotPoint = glm::vec3(0.0f, 0.0f, 0.0f);
    baseLift.rotationAngle = 0.0f;
    baseLift.minAngle = 0.0f;
    baseLift.maxAngle = 0.0f;
    baseLift.translationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    baseLift.translationOffset = 0.0f;
    baseLift.minOffset = -2.0f;
    baseLift.maxOffset = 2.0f;
    baseLift.isTranslational = true;
    manipulatorParts.push_back(baseLift);

    ManipulatorPart link1;
    link1.startMeshIndex = 1;
    link1.endMeshIndex = 1;
    link1.rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    link1.pivotPoint = glm::vec3(0.305f, 1.06195f, 0.0f);
    link1.rotationAngle = 0.0f;
    link1.minAngle = -45.0f;
    link1.maxAngle = 45.0f;
    link1.translationAxis = glm::vec3(0.0f, 0.0f, 0.0f);
    link1.translationOffset = 0.0f;
    link1.minOffset = 0.0f;
    link1.maxOffset = 0.0f;
    link1.isTranslational = false;
    manipulatorParts.push_back(link1);

    ManipulatorPart link2;
    link2.startMeshIndex = 2;
    link2.endMeshIndex = 2;
    link2.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    link2.pivotPoint = glm::vec3(0.306555f, 1.06195f, 0.0f);
    link2.rotationAngle = 0.0f;
    link2.minAngle = -45.0f;
    link2.maxAngle = 45.0f;
    link2.translationAxis = glm::vec3(0.0f, 0.0f, 0.0f);
    link2.translationOffset = 0.0f;
    link2.minOffset = 0.0f;
    link2.maxOffset = 0.0f;
    link2.isTranslational = false;
    manipulatorParts.push_back(link2);

    ManipulatorPart link3;
    link3.startMeshIndex = 3;
    link3.endMeshIndex = 3;
    link3.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    link3.pivotPoint = glm::vec3(-0.1335f, 1.06195f, -0.277f);
    link3.rotationAngle = 0.0f;
    link3.minAngle = -45.0f;
    link3.maxAngle = 45.0f;
    link3.translationAxis = glm::vec3(0.0f, 0.0f, 0.0f);
    link3.translationOffset = 0.0f;
    link3.minOffset = 0.0f;
    link3.maxOffset = 0.0f;
    link3.isTranslational = false;
    manipulatorParts.push_back(link3);
}

glm::mat4 calculateTransformForPart(const ManipulatorPart& part) {
    glm::mat4 transform = glm::mat4(1.0f);
    if (part.isTranslational) {
        glm::vec3 offset = part.translationAxis * part.translationOffset;
        transform = glm::translate(transform, offset);
    }
    else {
        if (glm::length(part.rotationAxis) > 0.0001f) {
            glm::vec3 normalizedAxis = glm::normalize(part.rotationAxis);
            if (glm::length(part.pivotPoint) > 0.0001f) {
                transform = glm::translate(transform, part.pivotPoint);
                transform = glm::rotate(transform, glm::radians(part.rotationAngle), normalizedAxis);
                transform = glm::translate(transform, -part.pivotPoint);
            }
            else {
                transform = glm::rotate(transform, glm::radians(part.rotationAngle), normalizedAxis);
            }
        }
    }
    return transform;
}