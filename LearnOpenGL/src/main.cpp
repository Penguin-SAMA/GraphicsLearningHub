#include <cstddef>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* windows);

const char* vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos, 1.0);\n"
                                 "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "uniform vec4 ourColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = ourColor;\n"
                                   "}\n\0";

int main() {
    // GLFW初始化
    // glfwWindowHint：
    // 第一个参数：代表选项的名称
    // 第二个参数：int，设置第一个参数的值
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口对象
    // 第一个参数：宽；第二个参数：高
    // 第三个参数：窗口名称
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 视口
    // 前两个参数控制窗口左下角的位置
    // 第三个和第四个参数控制渲染窗口的宽度和高度
    // glViewport(0, 0, 800, 600);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 编译着色器
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // glShaderSource
    // 第一个参数：要编译的着色器对象
    // 第二个参数：指定传递的源码字符串数量
    // 第三个参数：顶点着色器真正的源码
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    // 检查glCompileShader是否编译成功
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 片段着色器
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 着色器程序
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 顶点输入
    // clang-format off
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f
    };
    // clang-format on

    // 顶点缓冲对象
    // 它会在显存中存储大量顶点
    unsigned int VBO;
    glGenBuffers(1, &VBO); // 生成一个VBO对象

    // 顶点数组对象
    // 任何随后的顶点属性
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    // 绑定VAO
    glBindVertexArray(VAO);

    // 把顶点数组复制到缓冲中供OpenGL使用
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // 把新创建的缓冲绑定到GL_ARRAY_BUFFER上
    // 把之前定义的顶点数据复制到缓冲的内存中
    // 第一个参数：目标缓冲的类型
    // 第二个参数：指定传输数据的大小
    // 第三个参数：希望发送的实际数据
    // 第四个参数：指定我们希望显卡如何管理给定的数据
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 链接顶点属性
    // 第一个参数：指定顶点属性
    // 第二个参数：指定顶点属性的大小
    // 第三个参数：指定数据类型
    // 第四个参数：定义是否希望数据被标准化
    // 第五个参数：步长，指的是连续的顶点属性组之间的间隔
    // 第六个参数：表示数据在缓冲中起始位置的偏移量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // 告诉OpenGL该如何解析顶点数据

    glBindVertexArray(VAO);

    // 渲染循环
    // glfwWindowShouldClose在每次循环开始前检查一次GLFW是否被要求退出
    // glfwSwapBuffers交换颜色缓冲
    // glfwPollEvents检查有没有触发什么事件、更新窗口状态，并调用对应的回调函数
    while (!glfwWindowShouldClose(window)) {
        // 输入
        processInput(window);

        // 渲染指令
        // glClearColor清空屏幕所用的颜色
        // glClear用于清空屏幕的颜色缓冲
        // 当调用glClear函数清空颜色缓冲之后，整个颜色缓冲都会被填充为glClearColor里所设置的颜色
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 激活着色器
        glUseProgram(shaderProgram);

        // 更新uniform颜色
        double timeValue           = glfwGetTime();
        float  greenValue          = static_cast<float>(sin(timeValue) / 2.0 + 0.5);
        int    vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

        glBindVertexArray(VAO);
        // glDrawArrays
        // 第一个参数：图元的类型
        // 第二个参数：顶点数组的起始索引
        // 第三个参数：绘制的顶点数量
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 检查并调用事件，交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// 帧缓冲大小回调函数
// 每当窗口大小被调整时调用
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 输入控制
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}