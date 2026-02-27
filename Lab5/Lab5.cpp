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

// Размеры окна
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

bool flatShading = true; // Включает плоское затенение
float lightPos[3] = { 2.0f, 2.0f, 2.0f };

// Камера
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool cameraUpKeyPressed = false;
bool cameraDownKeyPressed = false;
float verticalSpeed = 1.5f; // Скорость вертикального перемещения

// Мышь
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float sensitivity = 0.1f;

// Время для плавного движения
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Прототипы функций
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
std::string readShaderFile(const std::string& filePath);
unsigned int compileShader(GLenum type, const std::string& source);
unsigned int createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);

int main() {
	// ============ 1. ИНИЦИАЛИЗАЦИЯ GLFW ============
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// ============ 2. СОЗДАНИЕ ОКНА ============
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab 4: Camera with GLM", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// ============ 3. ИНИЦИАЛИЗАЦИЯ GLAD ============
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// ============ 4. ШЕЙДЕРЫ ============
	// Вершинный шейдер
	std::string vertexShaderSource = readShaderFile("vertex_shader.glsl");
	std::string fragmentShaderSource = readShaderFile("fragment_shader.glsl");

	if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
		std::cerr << "ERROR: Failed to load shaders from files!" << std::endl;
		return -1;
	}

	unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
	if (shaderProgram == 0) return -1;

	// ============ 5. СОЗДАНИЕ И ЗАГРУЗКА 3D МОДЕЛИ  ============
	const char* cubeModel = R"(
	# Simple cube
	v -0.5 -0.5 0.5
	v 0.5 -0.5 0.5
	v -0.5 0.5 0.5
	v 0.5 0.5 0.5
	v -0.5 -0.5 -0.5
	v 0.5 -0.5 -0.5
	v -0.5 0.5 -0.5
	v 0.5 0.5 -0.5
	
	f 1 2 3
	f 2 3 4
	f 5 6 7
	f 6 7 8
	f 1 2 5
	f 2 5 6
	f 3 4 7
	f 4 7 8
	f 2 4 6
	f 4 6 8
	f 1 3 5
	f 3 5 7
	)";

	// Создаем папку models, если ее нет
#ifdef _WIN32
	system("mkdir models 2>nul");
#else
	system("mkdir -p models 2>/dev/null");
#endif

	// Сохраняем в файл
	ofstream file("models/cube.obj");
	if (file.is_open()) {
		file << cubeModel;
		file.close();
		cout << "Model file created: models/cube.obj" << endl;
	}
	else {
		std::cerr << "Failed to create model file!" << std::endl;
	}

	// Загружаем модель через ASSIMP
	Model ourModel("models/Laba3.1.obj");

	if (ourModel.meshes.empty()) {
		std::cerr << "The model is not loaded!" << std::endl;
		Model ourModel("models/Cube.obj");
		return -1;
	}

	// Включение теста глубины
	glEnable(GL_DEPTH_TEST);

	// ============ 6. ГЛАВНЫЙ ЦИКЛ ============
	while (!glfwWindowShouldClose(window)) {

		// Передача параметров освещения
		glUniform3f(glGetUniformLocation(shaderProgram, "cameraPos"),
			cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"),
			lightPos[0], lightPos[1], lightPos[2]);
		glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"),
			cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform1f(glGetUniformLocation(shaderProgram, "time"),
			(float)glfwGetTime());
		glUniform1i(glGetUniformLocation(shaderProgram, "flatShading"),
			flatShading ? 1 : 0);

		// Время
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Обработка ввода
		processInput(window);

		// Очистка
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Использование шейдера
		glUseProgram(shaderProgram);

		// Матрица модели
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -0.6f, 0.0f));

		// Матрица вида (LookAt)
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		// Матрица проекции
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			(float)SCR_WIDTH / (float)SCR_HEIGHT,
			0.1f,
			100.0f
		);

		// Передача матриц в шейдер
		unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
		unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
		unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3f(glGetUniformLocation(shaderProgram, "modelColor"),
			0.7f, 0.7f, 0.7f); // Светло-серый

		// Отрисовка
		ourModel.Draw();

		// Обмен буферов
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Очистка
	glDeleteProgram(shaderProgram);
	glfwTerminate();

	return 0;
}

// ============ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ============

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Управление камерой (WASD)
	float cameraSpeed = 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	// Вертикальное управление (C/V)
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		// Подъем камеры вверх
		cameraPos.y += verticalSpeed * deltaTime;
		cameraUpKeyPressed = true;
	}
	else {
		cameraUpKeyPressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		// Опускание камеры вниз
		cameraPos.y -= verticalSpeed * deltaTime;
		cameraDownKeyPressed = true;
	}
	else {
		cameraDownKeyPressed = false;
	}

	// Переключение режима по SPACE
	static bool spacePressed = false;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
		flatShading = !flatShading;
		if (flatShading) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // Каркасный режим
			cout << "Wireframe mode ON" << endl;
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // Обычный режим
			cout << "Wireframe mode OFF" << endl;
		}
		spacePressed = true;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
		spacePressed = false;
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

	// Ограничение pitch
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	// Вычисление вектора направления
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraFront = glm::normalize(direction);
}

// Чтение шейдера из файла
std::string readShaderFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		std::cerr << "ERROR: Failed to open shader file: " << filePath << std::endl;
		return "";
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	return buffer.str();
}

// Компиляция шейдера
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

// Создание шейдерной программы
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
