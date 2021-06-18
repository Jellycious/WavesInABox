// THIRD-PARTY LIBRARIES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// STANDARD LIBRARIES
#include <math.h>
#include <time.h>
#include <string.h>
#include <iostream>

// MY LIBRARIES
#include "shader.hpp"
#include "util.hpp"
#include "source.hpp"

#define MAX_SOURCES 10 // changing this requires a change in the shader.

int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;
const int SIMULATION_WIDTH = 256;
const int SIMULATION_HEIGHT = 256;
const int PADDING = 2;

const float SPEED = 1;
const float FREQ = 2;
const float AMPLITUDE = 2;
float DAMPING = 0.25;

Shader* waveShader;
Shader* lightShader;
ComputeShader* computeShader;
ComputeShader* copyShader;

// ImGui
bool show_demo_window = true;


// OpenGL objects neccesary for rendering.
struct GlObjects {
    unsigned int tex_w;
    unsigned int tex_h;
    unsigned int* textures;
    unsigned int VAO;
    unsigned int LightVAO;
    unsigned int PLANE_N; // Number of plane segments.
} glObjects;

// Information that should be send to the shader.
struct SimulationData {
    Source* sources[MAX_SOURCES];
    int src_counter;
    float camPos[3];
    float lightPos[3];
} simData;

// Struct to keep track of previous input state.
struct InputState {
    bool prev_right_mouse;
    bool prev_left_mouse;
} inputState;

void render(GLFWwindow* window, double time);

void initializeSimulationData() {
    for (int i = 0; i < MAX_SOURCES; i++) {
        Source* s = new Source(-1, -1, AMPLITUDE, FREQ);
        simData.sources[i] = s;
    }

    simData.src_counter = 0;
    simData.camPos[0] = 0.0;
    simData.camPos[1] = 5.0;
    simData.camPos[2] = 10.0; 

    float lightPos[3] = {0, 5.0f, 0};
    memcpy((void*) simData.lightPos, (void*)lightPos, 3 * sizeof(float));

}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    int left_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int right_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (inputState.prev_left_mouse == GLFW_RELEASE && left_mouse_button == GLFW_PRESS) {
        // Use has pressed the button.
        // create source
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int x = (int) (xpos * ((double) SIMULATION_WIDTH / (double) WINDOW_WIDTH));
        int y = (int) (ypos * ((double) SIMULATION_HEIGHT / (double) WINDOW_HEIGHT));
        simData.sources[simData.src_counter]->setPos(x, SIMULATION_HEIGHT - y);
        simData.sources[simData.src_counter]->setPhase(0.0);
        simData.sources[simData.src_counter]->setActive(AMPLITUDE);
        std::cout << "Added source at index: " << simData.src_counter << std::endl;
        simData.src_counter = (simData.src_counter + 1) % MAX_SOURCES;
    }

    if (inputState.prev_right_mouse == GLFW_RELEASE && right_mouse_button == GLFW_PRESS) {
        // user has pressed right mouse button.
        // remove source
        int r = (simData.src_counter - 1) % MAX_SOURCES;
        r = r < 0 ? r + MAX_SOURCES : r;
        simData.src_counter = r;
        std::cout << "Removed source at index: " << simData.src_counter << std::endl;
        simData.sources[simData.src_counter]->setInactive();
    }


    const float CAMSPEED = 0.10f;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        simData.camPos[2] = simData.camPos[2] - CAMSPEED;
    }
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        simData.camPos[1] = simData.camPos[1] - CAMSPEED;
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        simData.camPos[2] = simData.camPos[2] + CAMSPEED;
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        simData.camPos[1] = simData.camPos[1] + CAMSPEED;
    }

    inputState.prev_right_mouse = right_mouse_button;
    inputState.prev_left_mouse = left_mouse_button;

    // Damping
    if(glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
        DAMPING = fmax(0.0, DAMPING - 0.005);
    }
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        DAMPING = DAMPING + 0.005;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0,0, width, height);
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
}

double difference_in_sec(struct timespec* start, struct timespec* end) {
    double diff_in_seconds = ((double)end->tv_sec + 1.0e-9 * end->tv_nsec) - ((double) start->tv_sec + 1.0e-9 * start->tv_nsec);
    return diff_in_seconds;
}

void update(double time) {
    simData.lightPos[0] = 4 * sin(time * 0.5);
    simData.lightPos[2] = 4 * cos(time * 0.5);
}

void renderImGuiWindow(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    glClearColor(1.0, 1.0, 1.0, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Demo Window");
    ImGui::Button("Hello!");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);

}

void mainloop(GLFWwindow* window, GLFWwindow* window2) {
    // The main render loop
    struct timespec clock;
    clock_gettime(CLOCK_MONOTONIC, &clock);

    clock_gettime(CLOCK_MONOTONIC, &clock);
    double prevTime = (double) clock.tv_sec + 1.0e-9 * clock.tv_nsec;
    double deltaTime, time;

    while(!glfwWindowShouldClose(window) || !glfwWindowShouldClose(window2)) {
        glfwPollEvents();
        processInput(window);
        renderImGuiWindow(window2);

        glfwMakeContextCurrent(window);

        glClearColor(1.0, 1.0, 1.0, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        for (int i = 0; i < 2; i++){
            // Compute Shader
            clock_gettime(CLOCK_MONOTONIC, &clock);
            time = (double) clock.tv_sec + 1.0e-9 * clock.tv_nsec;
            deltaTime = time - prevTime;
            prevTime = time;

            // Initialize data for compute shader.
            int sourcePos[MAX_SOURCES][2];
            float phase[MAX_SOURCES];
            float amps[MAX_SOURCES];
            float freqs[MAX_SOURCES];

            for (int s = 0; s < MAX_SOURCES; s++) {
                simData.sources[s]->update(deltaTime);
                SourcePos pos = simData.sources[s]->getPos();
                sourcePos[s][0] = pos.x;
                sourcePos[s][1] = pos.y;
                phase[s] = simData.sources[s]->getPhase();
                amps[s] = simData.sources[s]->getAmplitude();
                freqs[s] = simData.sources[s]->getFreq();
            }

            {
                computeShader->use();
                // Setup uniform variables, which change every iteration.
                computeShader->setFloat("time", time);
                computeShader->setFloat("delta", deltaTime);
                computeShader->setFloat("damping", DAMPING);
                // Sources uniform
                computeShader->setVec2iArray("sources", MAX_SOURCES, sourcePos);

                //int loc = glGetUniformLocation(computeProgram.ID, "sources");
                //const GLint* ptr = (const GLint*) simData.sources;
                //glUniform2iv(loc, MAX_SOURCES, ptr);
                //loc = glGetUniformLocation(computeProgram.ID, "source_phases");
                //if (loc < 0) {
                //    printf("Could not find uniform source_phases\n");
                //}
                computeShader->setFloatArray("source_phases", MAX_SOURCES, phase);
                computeShader->setFloatArray("source_amplitude", MAX_SOURCES, amps);
                computeShader->setFloatArray("source_freq", MAX_SOURCES, freqs);

                // Dispatch the shader.
                glDispatchCompute(SIMULATION_WIDTH, SIMULATION_HEIGHT, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                prevTime = time;
            }

            // Copy Compute Shader
            {
                // This switches around some values in order to have everything ready for the next compute shader pass.
                copyShader->use();
                glDispatchCompute(SIMULATION_WIDTH, SIMULATION_HEIGHT, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
        }

        // Update
        update(time);

        // Render
        render(window, time);
        glfwSwapBuffers(window);
    }
}

void renderLightSource() {
    lightShader->use();
    glBindVertexArray(glObjects.LightVAO);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(simData.lightPos[0], simData.lightPos[1], simData.lightPos[2]));
    model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
    lightShader->setMat4("model", model);

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void render(GLFWwindow* window, double time) {
        waveShader->use();

        glm::vec3 eye = glm::vec3(simData.camPos[0], simData.camPos[1], simData.camPos[2]);
        glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        waveShader->setMat4("view", view);
        waveShader->setFloat("time", (float) time);
        waveShader->setVec3("lightPos", simData.lightPos[0], simData.lightPos[1], simData.lightPos[2]);

        glBindVertexArray(glObjects.VAO);

        // TODO: make neat.
        int N = glObjects.PLANE_N;
        glDrawElements(GL_TRIANGLES, 3*2*(N-1)*(N-1), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        lightShader->use();
        lightShader->setMat4("view", view);
        renderLightSource();
}


int main() {

    initializeSimulationData();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    //glfwWindowHint(GLFW_REFRESH_RATE, 60);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LearnOpenGl", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window");
        return -1;
    }
    glfwMakeContextCurrent(window);
    //
    // Setup Dear ImGui

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("Failed to initialize GLAD");
        return -1;
    }

    GLFWwindow* window2 = glfwCreateWindow(600, 600, "Controls", NULL, NULL);
    glfwMakeContextCurrent(window2);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.WantCaptureMouse = true;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window2, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glfwMakeContextCurrent(window);
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);



    // Set up vertex data and configure attributes
    float vertices[] = {
        1, 1, 0.0f, 1, 0, // top right
        1, -1, 0.0f, 1, 1, // bottom right
        -1, -1, 0.0f, 0, 1, // bottom left
        -1, 1, 0.0f, 0, 0 // top left
    };

    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
    
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
    
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
    
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
    
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
    
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    int N = 200;
    glObjects.PLANE_N = N;
    float meshv[5*N*N];
    unsigned int meshi[3*2*(N-1)*(N-1)];
    generatePlane(N, 10, meshv, meshi);

    // Setup Vertex Array, Vertex Buffer and Element Buffer Objects
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(meshv), meshv, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(meshi), meshi, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Setup light source info.
    unsigned int LightVBO, LightVAO;
    glGenVertexArrays(1, &LightVAO);
    glGenBuffers(1, &LightVBO);

    glBindVertexArray(LightVAO);
    glObjects.LightVAO = LightVAO;

    glBindBuffer(GL_ARRAY_BUFFER, LightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    // unbind the array and buffer
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    Shader s("./src/shaders/compute/vertex.glsl", "./src/shaders/compute/fragment.glsl");
    waveShader = &s;

    // setup texture uniforms
    s.use();
    s.setInt("texture1", 0);
    s.setInt("texture2", 1);
    s.setInt("normals", 2);
    s.setInt("colorPalette", 3);

    // Transformation matrices for the plane
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(45.0f), (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, 0.1f, 100.0f);
    view = glm::translate(view, glm::vec3(0.0f, 5.0f, 5.0f));

    s.setMat4("model", model);
    s.setMat4("view", view);
    s.setMat4("projection", projection);

    s.setVec2i("SIM_SIZE", SIMULATION_WIDTH, SIMULATION_HEIGHT);
    s.setVec3("lightColor", 1.0f, 1.0f, 1.0f);

    printf("Created RenderProgram\n");
    // create compute shader
    ComputeShader computeS = ComputeShader("./src/shaders/compute/compute.glsl");
    computeShader = &computeS;

    // setup uniform variables
    computeS.use();
    computeS.setFloat("csqrd", 1.0);
    computeS.setFloat("freq", FREQ);
    computeS.setFloat("amplitude", AMPLITUDE);
    computeS.setFloat("padding", PADDING);
    computeS.setFloat("damping", DAMPING);
    computeS.setVec2i("SIM_SIZE", SIMULATION_WIDTH, SIMULATION_HEIGHT);

    printf("Created ComputeProgram\n");


    Shader l("./src/shaders/compute/light/light_cube.vs", "./src/shaders/compute/light/light_cube.fs");
    lightShader = &l;

    l.use();
    l.setMat4("model", model);
    l.setMat4("view", view);
    l.setMat4("projection", projection);

    printf("Created Light shader\n");

    ComputeShader copyS = ComputeShader("./src/shaders/compute/copy.glsl");
    copyShader = &copyS;

    copyS.use();
    copyS.setFloat("padding", PADDING);
    printf("Created copy compute shader\n");

    // Generate textures for use by compute shader
    // Creates a texture which contains a border of 1 pixel.
    // It is instrumental that the compute shader keeps this in mind as the padding
    // serves as the boundary condition for the wave simulation.

    int tex_w = SIMULATION_WIDTH + PADDING * 2, tex_h = SIMULATION_HEIGHT + PADDING * 2;
    unsigned int tex_output[3];
    glGenTextures(3, tex_output);


    // Initialize some zero data for the textures.
    float* data = (float *) malloc((tex_w * 4 * sizeof(float)) * (tex_h * 4 *sizeof(float)));
    for (int i = 0; i< tex_w * 4; i++) {
        for (int j = 0; j< tex_h * 4; j++) {
            data[j + i * tex_w] = 0;
            data[j + i * tex_w + 1] = 0.5;
            data[j + i * tex_w + 2] = 0.5;
            data[j + i * tex_w + 3] = 0.5;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, (float *) data);
    glBindImageTexture(0, tex_output[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_output[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, (float *) data);
    glBindImageTexture(1, tex_output[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex_output[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, (float *) data);
    glBindImageTexture(2, tex_output[2], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    //Create color pallette texture
    // load image
    int width, height, nrChannels;
    unsigned char* paletteData = stbi_load("res/palette2d1.png", &width, &height, &nrChannels, 0);
    unsigned int colorPaletteTexture;
    glActiveTexture(GL_TEXTURE3);
    glGenTextures(1, &colorPaletteTexture); 
    glBindTexture(GL_TEXTURE_2D, colorPaletteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, paletteData); 
    stbi_image_free(paletteData);



    // Check work group variables of local device
    int work_grp_cnt[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    printf("max global (total) work group counts: x:%i, y:%i, z:%i\n",
            work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    int work_grp_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    printf("max global (total) work group sizes: x:%i, y:%i, z:%i\n",
            work_grp_size[0], work_grp_size[1], work_grp_size[2]);

    // Initialize data struct
    glObjects.VAO = VAO;
    glObjects.tex_w = tex_w;
    glObjects.tex_h = tex_h;
    glObjects.textures = tex_output;

    struct timespec start={0,0}, end={0,0};
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Start the mainloop
    mainloop(window, window2);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double diff_in_seconds = ((double)end.tv_sec + 1.0e-9 * end.tv_nsec) - ((double) start.tv_sec + 1.0e-9 * start.tv_nsec);
    printf("time elapsed in s: %lf\n", diff_in_seconds);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();

    return 0;
}
