#include "framework.h"
#include "scene.h"

// Global variables
bool mouseDown = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
Scene scene;

// Forward declarations
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double x, double y);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-verbose") {
            setVerboseLogging(true);
        }
    }

    debugLog("[debug] Starting Marching Cubes Terrain");

    // Initialize GLFW
    if (!glfwInit()) {
        const char* glfwErrorDescription = nullptr;
        glfwGetError(&glfwErrorDescription);
        fprintf(stderr, "[error] Failed to initialize GLFW%s%s\n",
            glfwErrorDescription ? ": " : "",
            glfwErrorDescription ? glfwErrorDescription : "");
        return -1;
    }
    debugLog("[debug] GLFW initialized");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    debugLog("[debug] Creating %ux%u window", windowWidth, windowHeight);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Marching Cubes", nullptr, nullptr);
    if (!window) {
        const char* glfwErrorDescription = nullptr;
        glfwGetError(&glfwErrorDescription);
        fprintf(stderr, "[error] Failed to create GLFW window%s%s\n",
            glfwErrorDescription ? ": " : "",
            glfwErrorDescription ? glfwErrorDescription : "");
        glfwTerminate();
        return -1;
    }

    glfwSetWindowPos(window, 50, 50);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // VSync: 1 = ON, 0 = OFF
    debugLog("[debug] OpenGL context initialized");

    // Set GLFW callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        const GLubyte* glewErrorString = glewGetErrorString(glewStatus);
        const char* glfwErrorDescription = nullptr;
        int glfwErrorCode = glfwGetError(&glfwErrorDescription);
        GLFWwindow* currentContext = glfwGetCurrentContext();
        const GLubyte* vendor = currentContext ? glGetString(GL_VENDOR) : nullptr;
        const GLubyte* renderer = currentContext ? glGetString(GL_RENDERER) : nullptr;
        const GLubyte* version = currentContext ? glGetString(GL_VERSION) : nullptr;

        fprintf(stderr,
            "[error] Failed to initialize GLEW (status=0x%x, message=%s)\n",
            static_cast<unsigned int>(glewStatus),
            glewErrorString ? reinterpret_cast<const char*>(glewErrorString) : "unknown GLEW error");

        if (glfwErrorCode != GLFW_NO_ERROR && glfwErrorDescription) {
            fprintf(stderr, "[error] GLFW error after GLEW init failure (code=%d, message=%s)\n",
                glfwErrorCode,
                glfwErrorDescription);
        }

        if (currentContext) {
            fprintf(stderr,
                "[error] OpenGL context info: vendor=%s, renderer=%s, version=%s\n",
                vendor ? reinterpret_cast<const char*>(vendor) : "unavailable",
                renderer ? reinterpret_cast<const char*>(renderer) : "unavailable",
                version ? reinterpret_cast<const char*>(version) : "unavailable");
        } else {
            fprintf(stderr, "[error] OpenGL context info unavailable (no current context)\n");
        }

        glfwTerminate();
        return -1;
    }
    debugLog("[debug] GLEW initialized");

    // Set viewport and OpenGL options
    int display_width, display_height;
    glfwGetFramebufferSize(window, &display_width, &display_height);
    glViewport(0, 0, display_width, display_height);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");
    debugLog("[debug] ImGui initialized");

    // Build the scene
    debugLog("[debug] Building scene");
    scene.Build();
    debugLog("[debug] Scene build complete");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene.Render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {                                     
        scene.moveCamera(key);
}

void cursor_position_callback(GLFWwindow* window, double x, double y) {
    if (x > windowWidth - gui_width) return;
    if (mouseDown) scene.rotateCamera(x, y);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        scene.setCameraFirstMouse();
        mouseDown = true;
    } else {
        mouseDown = false;
    }
}
