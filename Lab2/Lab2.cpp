#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>

// ====================================================================
// КОНФИГУРАЦИЯ — выбираем режим работы
// ====================================================================
#define MODE 1   // 1 = Задание 1 (встроенные шейдеры),  2 = Задание 2 (шейдеры из файлов + класс Shader)

// ====================================================================
// Задание 1 — встроенные шейдеры
// ====================================================================
const char* vertex_shader_embedded = R"(
#version 410 core
layout (location = 0) in vec3 vp;
void main() {
    gl_Position = vec4(vp, 1.0);
}
)";

const char* fragment_shader_embedded = R"(
#version 410 core
out vec4 FragColor;
uniform vec4 ourColor;
void main() {
    FragColor = ourColor;
}
)";

GLuint createEmbeddedProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader_embedded, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader_embedded, nullptr);
    glCompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

// ====================================================================
// Задание 2 — класс Shader (шейдеры из файлов)
// ====================================================================
class Shader {
public:
    GLuint Program = 0;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vCode = readFile(vertexPath);
        std::string fCode = readFile(fragmentPath);

        const char* vSource = vCode.c_str();
        const char* fSource = fCode.c_str();

        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vSource, nullptr);
        glCompileShader(vertex);

        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fSource, nullptr);
        glCompileShader(fragment);

        Program = glCreateProgram();
        glAttachShader(Program, vertex);
        glAttachShader(Program, fragment);
        glLinkProgram(Program);

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void Use() const {
        glUseProgram(Program);
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(Program, name.c_str()), x, y, z, w);
    }

private:
    static std::string readFile(const char* path) {
        std::ifstream file(path, std::ios::in);
        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл шейдера: " << path << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

// ====================================================================
// Главная программа
// ====================================================================
int main() {
    if (!glfwInit()) {
        std::cerr << "Ошибка инициализации GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 640, "Rectangle - Variant 28", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Ошибка инициализации GLEW\n";
        return -1;
    }

    // Вершины прямоугольника
    float vertices[] = {
        -0.6f,  0.6f, 0.0f,    // левый верх
         0.6f,  0.6f, 0.0f,    // правый верх
         0.6f, -0.6f, 0.0f,    // правый низ
        -0.6f, -0.6f, 0.0f     // левый низ
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Выбор шейдерной программы в зависимости от режима
    GLuint shaderProgram = 0;
    Shader* customShader = nullptr;

    if (MODE == 1) {
        std::cout << "Режим 1: встроенные шейдеры\n";
        shaderProgram = createEmbeddedProgram();
    }
    else if (MODE == 2) {
        std::cout << "Режим 2: шейдеры из файлов + класс Shader\n";
        customShader = new Shader("vertex.glsl", "fragment.glsl");
        shaderProgram = customShader->Program;
    }

    if (shaderProgram == 0) {
        std::cerr << "Не удалось создать шейдерную программу\n";
        glfwTerminate();
        return -1;
    }

    // ────────────────────────────────────────────────────────────────
    // Основной цикл
    // ────────────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.08f, 0.12f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float t = static_cast<float>(glfwGetTime());
        float r = 0.5f + 0.5f * std::sin(t * 1.3f);
        float g = 0.5f + 0.5f * std::cos(t * 0.9f);
        float b = 0.6f + 0.4f * std::sin(t * 1.7f);

        if (MODE == 1) {
            glUseProgram(shaderProgram);
            GLint loc = glGetUniformLocation(shaderProgram, "ourColor");
            glUniform4f(loc, r, g, b, 1.0f);
        }
        else if (MODE == 2 && customShader) {
            customShader->Use();
            customShader->setVec4("ourColor", r, g, b, 1.0f);
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Очистка
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    delete customShader;

    glfwTerminate();
    return 0;
}