// THIRD-PARTY LIBRARIES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

// STANDARD LIBRARIES
#include <math.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <thread> 

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

const float SPEED = 0.1;
const float FREQ = 1.5;
const float AMPLITUDE = 2;
float DAMPING = 0.02;

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
    float perspectives[2][3] = {
        {0.0, 5.0, 8.8},
        {0.0, 6.5, 0.1}
    };
    size_t cur_perspective_idx = 0;
    float lightPos[3];
} simData;

// Struct to keep track of previous input state.
struct InputState {
    bool prev_right_mouse;
    bool prev_left_mouse;
    int prev_space;
} inputState;

void render(double time);

void initializeSimulationData() {
    for (int i = 0; i < MAX_SOURCES; i++) {
        Source* s = new Source(-1, -1, AMPLITUDE, FREQ);
        s->setInactive();
        simData.sources[i] = s;
    }

    simData.src_counter = 0;
    simData.camPos[0] = simData.perspectives[simData.cur_perspective_idx][0];
    simData.camPos[1] = simData.perspectives[simData.cur_perspective_idx][1];
    simData.camPos[2] = simData.perspectives[simData.cur_perspective_idx][2];

    float lightPos[3] = {0, 5.0f, 4.0};
    memcpy((void*) simData.lightPos, (void*)lightPos, 3 * sizeof(float));

}

void setNewCamPos(float* pos) {
    simData.camPos[0] = pos[0];
    simData.camPos[1] = pos[1];
    simData.camPos[2] = pos[2];
}

void saveImage(int width, int height, GLsizei nrChannels, unsigned char* data, GLsizei stride) {
    stbi_write_png("screenshot.png", width, height, nrChannels, data, stride);
    free(data);
}

void mixVec(float* v1, float* v2, float* res, float alpha, int size) {
    for (int i = 0; i < size; i++) {
        res[i] = v1[i] * (1 - alpha) + v2[i] * alpha;
    }
}

void processInput(GLFWwindow* window) {
    
    int left_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int right_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    int spacebar = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;


    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);


    if (inputState.prev_left_mouse == GLFW_RELEASE && left_mouse_button == GLFW_PRESS) {
        // Use has pressed the button.
        // create source
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int x = (int) (xpos * ((double) SIMULATION_WIDTH / (double) WINDOW_WIDTH));
        int y = (int) (ypos * ((double) SIMULATION_HEIGHT / (double) WINDOW_HEIGHT));
        simData.sources[simData.src_counter]->setPos(x, SIMULATION_HEIGHT - y);
        simData.sources[simData.src_counter]->setPhase(0.0);
        simData.sources[simData.src_counter]->setActive();
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

    if (inputState.prev_space == GLFW_RELEASE && spacebar == GLFW_PRESS) {
        // Try to save image.
        printf("Saving Image\n");
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        GLsizei nrChannels = 3;
        GLsizei stride = nrChannels * width;
        stride += (stride % 4) ? (4 - stride % 4) : 0;
        GLsizei bufferSize = stride * height;
        unsigned char* buffer = (unsigned char*) malloc(bufferSize);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadBuffer(GL_FRONT);
        glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE, buffer);
        stbi_flip_vertically_on_write(true);

        // Lambda for thread
        std::thread t(saveImage, width, height, nrChannels, buffer, stride);
        t.detach();
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


    // Damping
    if(glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
        DAMPING = fmax(0.0, DAMPING - 0.005);
        printf("Damping: %f\n", DAMPING);
    }
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        DAMPING = DAMPING + 0.005;
        printf("Damping: %f\n", DAMPING);
    }

    inputState.prev_right_mouse = right_mouse_button;
    inputState.prev_left_mouse = left_mouse_button;
    inputState.prev_space = spacebar;
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

void resetSources() {
    for (int i = 0; i < MAX_SOURCES; i++) {
        simData.sources[i]->setInactive();
        simData.sources[i]->setPos(-1,-1);
        simData.sources[i]->setFreq(FREQ);
    }
}

void animate(int frame) {
    int ANIMATION_PERIOD = 100 * 400;
    int f = frame % ANIMATION_PERIOD;
    // 1 Centered source
    if (f == 0) {
        DAMPING = 0.02;
        resetSources();
    }
    if (f < 100 * 16) {
        int cycle_period = 100 * 4;
        if (f % cycle_period == 1) {
            simData.sources[0]->setActive();
            simData.sources[0]->setPos((int) SIMULATION_WIDTH / 2.0, (int) SIMULATION_HEIGHT / 2.0);
            simData.sources[0]->setAmplitude(2.0);
        }
        if (f % cycle_period == (int) cycle_period / 2.0) {
            simData.sources[0]->setInactive();
        }
    }
    // Play and damp.
    if (f == 100 * 16) simData.sources[0]->setInactive();
    if (f < 100 * 24) return;
    if (f == 100 * 36) DAMPING = 0.5;
    if (f < 100 * 44) return;

    // 2 Sources
    if (f == 100 * 44){
        DAMPING = 0.025;
        int offset = 50;
        simData.sources[0]->setActive();
        simData.sources[0]->setAmplitude(2.0);
        simData.sources[0]->setPos((int) SIMULATION_WIDTH / 2.0 - offset, (int) SIMULATION_HEIGHT / 2.0);

        simData.sources[1]->setActive();
        simData.sources[1]->setAmplitude(2.0);
        simData.sources[1]->setPos((int) SIMULATION_WIDTH / 2.0 + offset, (int) SIMULATION_HEIGHT / 2.0);
        return;
    }
    if (f == 100 * 94) {
        simData.sources[0]->setInactive();
        simData.sources[1]->setInactive();
        DAMPING = 0.25;
        return;
    }

    if ( f < 100 * 134) return;
    // Four sources in the corner.
    if ( f == 100 * 134) {
        simData.sources[0]->setActive();
        simData.sources[0]->setAmplitude(3.0);
        simData.sources[0]->setPos(2, 2);

        simData.sources[1]->setActive();
        simData.sources[1]->setAmplitude(3.0);
        simData.sources[1]->setPos(2, SIMULATION_HEIGHT - 3);

        simData.sources[2]->setActive();
        simData.sources[2]->setAmplitude(3.0);
        simData.sources[2]->setPos(SIMULATION_WIDTH - 3,2);

        simData.sources[3]->setActive();
        simData.sources[3]->setAmplitude(3.0);
        simData.sources[3]->setPos(SIMULATION_WIDTH - 3,SIMULATION_HEIGHT - 3);
        DAMPING = 0.02;
    }
    if ( f == 100 * 146) {
        resetSources();
    }

    if (f < 100 * 200) return; 
    // One slow source in center.
    if ( f == 100 * 200) {
        simData.sources[0]->setActive();
        simData.sources[0]->setAmplitude(1.0);
        simData.sources[0]->setFreq(0.5);
        simData.sources[0]->setPos((int) SIMULATION_WIDTH / 2.0, (int) SIMULATION_HEIGHT / 2.0);
    }

    if ( f < 100 * 220) return;
    if ( f == 100 * 220) {
        simData.sources[0]->setPhase(0.0);
        simData.sources[0]->setFreq(4.0);
        DAMPING = 0.1;
    }
    if ( f < 100 * 240) return;
    if ( f == 100 * 240) {
        simData.sources[0]->setAmplitude(3.0);
    }
    if ( f < 100 * 260) return;
    if ( f == 100 * 260) {
        resetSources();
        DAMPING = 0.15;
    }

    if ( f < 100 * 285) return; 
    if ( f == 100 * 285) {
        simData.sources[0]->setFreq(0.5);
        simData.sources[0]->setPos((int) SIMULATION_WIDTH / 2.0, (int) SIMULATION_HEIGHT / 2.0);
        simData.sources[0]->setAmplitude(3.0);
        simData.sources[0]->setActive();
    }

    if (f < 100 * 305) return;
    if (f == 100 * 305) {
        resetSources();
        DAMPING = 0.2;
    }
    if (f < 100 * 325) {
        return;
    }
    if (f == 100 * 325) {
        DAMPING = 0.0;
        simData.sources[0]->setPos(((int) SIMULATION_WIDTH / 2.0) + 50 * cos(0.666*M_PI),((int) SIMULATION_HEIGHT / 2.0) + 50 * sin(0.666*M_PI));
        simData.sources[0]->setActive();
        simData.sources[0]->setAmplitude(2.0);
        simData.sources[1]->setPos(((int) SIMULATION_WIDTH / 2.0) + 50 * cos(1.333*M_PI),((int) SIMULATION_HEIGHT / 2.0) + 50 * sin(1.333*M_PI));
        simData.sources[1]->setActive();
        simData.sources[1]->setAmplitude(2.0);
        simData.sources[2]->setPos(((int) SIMULATION_WIDTH / 2.0) + 50,((int) SIMULATION_HEIGHT / 2.0));
        simData.sources[2]->setActive();
        simData.sources[2]->setFreq(4.0);
        simData.sources[2]->setAmplitude(2.0);
    }
    if (f < 100 * 345) {
    }
    if (f == 100 * 345) {
        DAMPING = 0.4;
        resetSources();
    }
    if (f < 100 * 355) return;
    if ( f == 100 * 355) {
        DAMPING = 0.05;
        simData.sources[0]->setPos(2, (int) SIMULATION_HEIGHT / 2.0);
        simData.sources[0]->setActive();
        simData.sources[0]->setAmplitude(2.0);
        simData.sources[1]->setPos(SIMULATION_WIDTH - 2, (int) SIMULATION_HEIGHT / 2.0);
        simData.sources[1]->setActive();
        simData.sources[1]->setAmplitude(2.0);
    }
    if ( f < 100 * 380) return;
    if ( f == 100 * 380) {
        resetSources();
        return;
    }
    // change perspective
    if (f < 100 * 390) {
        int arr_length = (int) sizeof(simData.perspectives) / sizeof(simData.perspectives[0]);
        size_t new_perspective_idx = (simData.cur_perspective_idx + 1) % arr_length;
        float alpha = (float) ((float) f - 100 * 380) / (100 * 10);
        
        float* vec1 = simData.perspectives[simData.cur_perspective_idx];
        float* vec2 = simData.perspectives[new_perspective_idx];
        float newVec[3];
        mixVec(vec1, vec2, newVec, alpha, 3);
        setNewCamPos(newVec);
        return;
    }

    if (f == 100 * 390) {
        int arr_length = (int) sizeof(simData.perspectives) / sizeof(simData.perspectives[0]);
        size_t newIdx = (simData.cur_perspective_idx + 1) % arr_length;
        simData.cur_perspective_idx = newIdx;
        return;
    } 
    return;

}

void mainloop(GLFWwindow* window) {
    // The main render loop
    struct timespec clock;
    clock_gettime(CLOCK_MONOTONIC, &clock);

    clock_gettime(CLOCK_MONOTONIC, &clock);
    double time = glfwGetTime();
    double prevTime = time;
    double deltaTime;
    double timeSinceStart = 0.0;
    int frames =  0 * 100;

    while(!glfwWindowShouldClose(window)) {

        time = glfwGetTime();

        if (time - prevTime >= (1.0 / 100.0)) {

            glfwPollEvents();
            processInput(window);

            deltaTime = time - prevTime;
            prevTime = time;
            timeSinceStart += deltaTime;

            glfwMakeContextCurrent(window);

            glClearColor(1.0, 1.0, 1.0, 1.0f);
            //glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            for (int i = 0; i < 1; i++){
                // Compute Shader

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
            animate(frames);

            // Render
            render(time);
            glfwSwapBuffers(window);
            frames++;
        }
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

void render(double time) {
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

        //lightShader->use();
        //lightShader->setMat4("view", view);
        //renderLightSource();
}


int main() {

    initializeSimulationData();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_REFRESH_RATE, 60);

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


    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);



    // Set up vertex data and configure attributes
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
    s.setVec3("col1", glm::vec3(120,197,220));
    s.setVec3("col2", glm::vec3(183,236,234));

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
    s.setVec3("lightColor", 242.0 / 255.0, 218.0 / 255.0, 200.0 / 255.0);

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
    mainloop(window);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double diff_in_seconds = ((double)end.tv_sec + 1.0e-9 * end.tv_nsec) - ((double) start.tv_sec + 1.0e-9 * start.tv_nsec);
    printf("time elapsed in s: %lf\n", diff_in_seconds);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();

    return 0;
}
