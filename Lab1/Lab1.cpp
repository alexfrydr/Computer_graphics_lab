#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main() {
    // Инициализация библиотеки GLFW. Если инициализация не удалась, выводим ошибку и завершаем программу
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(512, 512, "Прямоугольник - Вариант 28", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Установка текущего контекста OpenGL для этого окна
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;

    GLenum ret = glewInit();
    if (GLEW_OK != ret) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(ret));
        return 1;
    }

    // Получение и вывод информации о версии GLEW, OpenGL и устройстве рендеринга
    const GLubyte* version_str = glGetString(GL_VERSION);
    const GLubyte* device_str = glGetString(GL_RENDERER);
    printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    printf("This version OpenGL running is %s\n", version_str);
    printf("This device OpenGL running is %s\n", device_str);

    while (!glfwWindowShouldClose(window)) {
        // Установка цвета очистки буфера (чёрный цвет с полной непрозрачностью)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // Очистка буфера цвета
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_QUADS);

        // Установка цвета для первой и второй вершин (фиолетовый оттенок)
        glColor3f(0.6f, 0.2f, 0.9f);
        glVertex2f(-0.5f, 0.5f); // Верхняя левая вершина

        glColor3f(0.6f, 0.2f, 0.9f);
        glVertex2f(0.5f, 0.5f); // Верхняя правая вершина

        // Установка цвета для третьей и четвёртой вершин (серый оттенок)
        glColor3f(0.3f, 0.3f, 0.3f);
        glVertex2f(0.5f, -0.5f); // Нижняя правая вершина

        glColor3f(0.3f, 0.3f, 0.3f);
        glVertex2f(-0.5f, -0.5f); // Нижняя левая вершина

        // Завершение рисования
        glEnd();

        // Обмен буферами (double buffering) для отображения нарисованного кадра
        glfwSwapBuffers(window);

        // Обработка событий (ввод с клавиатуры, мыши и т.д.)
        glfwPollEvents();
    }
    // Завершение работы GLFW
    glfwTerminate();

}
