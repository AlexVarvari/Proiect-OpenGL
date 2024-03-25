#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"


#include <iostream>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

// window
gps::Window myWindow;
int glWindowWidth = 800;
int glWindowHeight = 600;

// matrices
glm::mat4 model;
glm::mat4 modelBarca;
glm::mat4 modelWindmillBlades;
glm::mat4 modelWindmillBody;
glm::mat4 modelCub;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint timeLoc;

bool rainOn = false;
bool pKeyPressed = false;
bool rKeyPressed = false;
bool cameraAnimation = false;
enum SequenceState { IDLE, DELAY, PRESS_Q, PRESS_S, PRESS_D, PRESS_A, DONE };

SequenceState currentSequenceState = IDLE;
GLfloat sequenceStartTime = 0.0f;
const GLfloat durationQ = 0.5f; // Duration for Q
const GLfloat durationS = 1.5f; // Duration for S
const GLfloat durationD = 1.8f; // Duration for D
const GLfloat durationA = 4.5f; // Duration for A
const GLfloat initialDelayDuration = 2.5f; // 1 second delay
GLfloat barcaMovementSpeed = 0.25f; // Speed of the movement
GLfloat barcaMovementDistance = 45.0f; // Distance of the movement

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.5f;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat animationStartTime = 0.0f;
GLfloat animationDuration = 2.0f; // 2 seconds

GLboolean pressedKeys[1024];

// models
gps::Model3D bloc1;
gps::Model3D barca;
gps::Model3D windmillBlades;
gps::Model3D cub;
gps::Model3D windmillBody;

GLfloat angle;

GLfloat lastX = 400;
GLfloat lastY = 300;
GLfloat pitch = 0.0f;
GLfloat yaw = -90.0f;
GLboolean first_Mouse = true;
GLfloat x_offset;
GLfloat y_offset;

// shaders
gps::Shader myBasicShader;

glm::vec3 rain[500];

glm::vec3 cameraStartPosition = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraEndPosition = glm::vec3(0.0f, 0.0f, 5.0f);


GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);

    // Update the viewport to match the new window dimensions
    glViewport(0, 0, width, height);

    // Recalculate the projection matrix to maintain the aspect ratio
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 500.0f);

    // Update the projection matrix uniform in the shader
    myBasicShader.useShaderProgram();
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS && !cameraAnimation) {
        cameraAnimation = true;
        animationStartTime = glfwGetTime();
        pKeyPressed = true;
    }
    else if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
        pKeyPressed = false; // Reset the flag when P key is released
    }
    
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        rKeyPressed = true; // Set the flag when P key is pressed
    }
    else if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
        rKeyPressed = false; // Reset the flag when P key is released
    }
    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        PlaySound(TEXT("sounds\\metin.wav"), NULL, SND_ASYNC | SND_FILENAME);
        currentSequenceState = DELAY;
        sequenceStartTime = glfwGetTime();
    }

    else if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
  /*  if (first_Mouse) {
        lastX = xpos;
        lastY = ypos;
        first_Mouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // Change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain the pitch
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
    */
}




void processMovement() {
    GLfloat currentTime = glfwGetTime();

    switch (currentSequenceState) {
    case DELAY:
        if (currentTime - sequenceStartTime < initialDelayDuration) {
            // Waiting during the delay
        }
        else {
            currentSequenceState = PRESS_Q; // Start the sequence
            sequenceStartTime = currentTime;
        }
        break;
    case PRESS_Q:
        if (currentTime - sequenceStartTime < durationQ) {
            myCamera.move(gps::MOVE_UP, cameraSpeed);
        }
        else {
            currentSequenceState = PRESS_S;
            sequenceStartTime = currentTime;
        }
        break;
    case PRESS_S:
        if (currentTime - sequenceStartTime < durationS) {
            myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        }
        else {
            currentSequenceState = PRESS_D;
            sequenceStartTime = currentTime;
        }
        break;
    case PRESS_D:
        if (currentTime - sequenceStartTime < durationD) {
            myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        }
        else {
            currentSequenceState = PRESS_A;
            sequenceStartTime = currentTime;
        }
        break;
    case PRESS_A:
        if (currentTime - sequenceStartTime < durationA) {
            myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        }
        else {
            currentSequenceState = DONE;
        }
        break;

    case DONE:
        // Sequence is complete, do nothing or reset if needed
        break;
    case IDLE:
        // No sequence running, do nothing
        break;
    }
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
       
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
       
    }
    if (pressedKeys[GLFW_KEY_Q]) { // Assuming Q is for moving up
        myCamera.move(gps::MOVE_UP, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_E]) { // Assuming E is for moving down
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_Z]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode
    }

    if (pressedKeys[GLFW_KEY_X]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // solid mode
    }

    if (pressedKeys[GLFW_KEY_C]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); // mod poligonal cu puncte
    }

    if (pKeyPressed) {
        PlaySound(TEXT("sounds\\metin.wav"), NULL, SND_ASYNC | SND_FILENAME);
        pKeyPressed = false; // Reset the flag so the sound plays only once per key press
    }
    if (rKeyPressed) {
        PlaySound(NULL, NULL, SND_ASYNC); // Stop any currently playing sound
        rKeyPressed = false; // Reset the flag
    }
  

    // Update view matrix after moving the camera
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    bloc1.LoadModel("models/bloc1/bloc1.obj");
    barca.LoadModel("models/bloc1/barca.obj");
    windmillBody.LoadModel("models/bloc1/moara.obj"); // Adjust path as needed
    windmillBlades.LoadModel("models/bloc1/roatamorii.obj");
    cub.LoadModel("models/bloc1/cub.obj");

    
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 200.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    // Set up the time uniform
    timeLoc = glGetUniformLocation(myBasicShader.shaderProgram, "time");

    modelBarca = glm::mat4(1.0f); // Identity matrix
    modelWindmillBody = glm::mat4(1.0f); // Identity matrix for windmill body
    modelWindmillBlades = glm::mat4(1.0f); // Identity matrix for windmill blades
    modelCub = glm::mat4(1.0f);
}

void renderBloc1(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    // Render bloc1
    // send bloc1 model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    bloc1.Draw(shader);

    // Render barca
    // Calculate the position for barca
    GLfloat timeValue = glfwGetTime();
    GLfloat barcaMovement = abs(sin(timeValue * barcaMovementSpeed)) * barcaMovementDistance;
    modelBarca = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, barcaMovement));

    // send barca model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelBarca));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * modelBarca));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    barca.Draw(shader);

 
}

void renderWindmill(gps::Shader shader) {
    shader.useShaderProgram();

    // Render the windmill body
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelWindmillBody));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * modelWindmillBody));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    windmillBody.Draw(shader);

   
    windmillBlades.Draw(shader);
}



void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLfloat currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    glUniform1f(timeLoc, currentFrame);
	//render the scene

	// render the teapot
	renderBloc1(myBasicShader);
    renderWindmill(myBasicShader);

    myBasicShader.useShaderProgram();
    modelCub = glm::mat4(1.0f); // Adjust this transformation as needed for the cube's position and scale
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCub));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * modelCub));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    cub.Draw(myBasicShader);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
