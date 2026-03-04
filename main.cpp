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

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 lightRotation;
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
GLint nightModeLoc;
GLint isSkyLoc;


const unsigned int SHADOW_WIDTH = 8192; 
const unsigned int SHADOW_HEIGHT = 8192;

GLuint shadowMapFBO;
GLuint depthMapTexture;

gps::Shader depthMapShader;

glm::mat4 lightSpaceMatrix;

gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 5.5f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.5f;

bool presentationActive = true;
bool cameraUserControl = false;

struct CameraPoint {
    glm::vec3 position;
    glm::vec3 target;
    float duration;
};

std::vector<CameraPoint> cameraPath = {
     { { 0.0f, 1.5f, 14.0f }, { 0.0f, 0.5f, 0.0f }, 5.0f },
     { { 12.0f, 2.0f, 8.0f }, { 0.0f, 0.5f, 0.0f }, 5.0f },
     { { 0.0f, 2.5f, -14.0f }, { 0.0f, 0.5f, 0.0f }, 5.0f },
     { { -12.0f, 2.0f, 8.0f }, { 0.0f, 0.5f, 0.0f }, 5.0f },
     { { 0.0f, 9.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 6.0f }
};

int currentCamPoint = 0;
float camSegmentTime = 0.0f;
float deltaTime = 0.0f;

float yaw = -90.0f;
float pitch = -25.0f;
bool firstMouse = true;
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
GLboolean pressedKeys[1024];

// models

gps::Model3D farm;
GLfloat angle;

gps::Model3D skiZi;
gps::Model3D skiNoapte;

gps::Shader myBasicShader;

 
GLint fogLoc;
bool fogEnabled = false;

bool nightMode = false;

 
glm::vec3 lightDirWorld;
glm::vec3 lightDirEye;
GLfloat lightAngle;
float angleY = 0.0f;
struct PointLight {
    glm::vec3 position;   
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
};

std::vector<PointLight> pointLights;

gps::Model3D corp;
gps::Model3D alicee;

float bladesAngle = 0.0f;


glm::vec3 windmillPosition = glm::vec3(-36.0f, -1.0f, -20.0f);
glm::vec3 elicePosition = glm::vec3(0.041493f, 13.242f, -1.0308f);


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
	//fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    glViewport(0, 0, fbWidth, fbHeight);

    projection = glm::perspective(
        glm::radians(45.0f),
        (float)fbWidth / (float)fbHeight,
        0.1f,
        1000.0f
    );

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        fogEnabled = !fogEnabled;
    }
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        nightMode = !nightMode;
    }
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        glm::vec3 pos = myCamera.getPosition();
        std::cout << "Camera Position: ("
            << pos.x << ", "
            << pos.y << ", "
            << pos.z << ")" << std::endl;
    }

}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);

}

void processMovement() {
    if (!cameraUserControl)
        return;

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
       
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
       
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angleY -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angleY += 1.0f;
    }

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
    }

    
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
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    
    farm.LoadModel("models/farm/farm.obj");
    corp.LoadModel("models/windmil/corp.obj");
    alicee.LoadModel("models/windmil/alicee.obj");
    skiZi.LoadModel("models/skiZi/skiZi.obj");
    skiNoapte.LoadModel("models/skiNoapte/skiNoapte.obj");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    depthMapShader.loadShader(
        "shaders/depthMapShader.vert",
        "shaders/depthMapShader.frag"
    );
   
}
void initPointLights()
{
    
    PointLight l1;
    l1.position = glm::vec3(18.328f, 0.265f, -10.698f);
    l1.color = glm::vec3(1.0f, 0.6f, 0.2f);    
    l1.constant = 1.0f;
    l1.linear = 0.22f;
    l1.quadratic = 0.20f;

    pointLights.push_back(l1);

    PointLight l2;

    l2.position = glm::vec3(-12.6485f, 0.178165f, 15.758f);
    l2.color = glm::vec3(1.0f, 0.6f, 0.2f);
    l2.constant = 1.0f;
    l2.linear = 0.22f;
    l2.quadratic = 0.20f;
    pointLights.push_back(l2);

    PointLight l3;

    l3.position = glm::vec3(-43.808f, 0.18438f, 34.289f);
    l3.color = glm::vec3(1.0f, 0.6f, 0.2f);
    l3.constant = 1.0f;
    l3.linear = 0.22f;
    l3.quadratic = 0.20f;
    pointLights.push_back(l3);
    
  
    myBasicShader.useShaderProgram();
    glUniform1i(
        glGetUniformLocation(myBasicShader.shaderProgram, "numPointLights"),
        pointLights.size()
    );

}

void initUniforms() {
	myBasicShader.useShaderProgram();
    isSkyLoc = glGetUniformLocation(
        myBasicShader.shaderProgram,
        "isSky"
    );

    fogLoc = glGetUniformLocation(
        myBasicShader.shaderProgram,
        "enableFog"
    );
    glUniform1i(fogLoc, 0);
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
                               0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	
    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
   
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); 
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    
    nightModeLoc = glGetUniformLocation(
        myBasicShader.shaderProgram,
        "nightMode"
    );
    initPointLights();
    
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
}

void initFBO() {
    //2.1.1 
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
   
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightView = glm::lookAt(glm::vec3(lightRotation * glm::vec4(lightDir, 1.0f)), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    const GLfloat near_plane = -50.0f, far_plane = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    return lightSpaceTrMatrix;
}
void updatePointLights()
{
    myBasicShader.useShaderProgram();

    for (int i = 0; i < pointLights.size(); i++) {
        std::string base = "pointLights[" + std::to_string(i) + "]";

        
        glm::vec3 posEye =
            glm::vec3(view * glm::vec4(pointLights[i].position, 1.0f));

        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram,
            (base + ".position").c_str()), 1, glm::value_ptr(posEye));

        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram,
            (base + ".color").c_str()), 1, glm::value_ptr(pointLights[i].color));

        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram,
            (base + ".constant").c_str()), pointLights[i].constant);

        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram,
            (base + ".linear").c_str()), pointLights[i].linear);

        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram,
            (base + ".quadratic").c_str()), pointLights[i].quadratic);
    }
}
void renderSky(gps::Shader shader)
{
    shader.useShaderProgram();

    glDepthMask(GL_FALSE);
    
    model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f));

    glUniformMatrix4fv(
        modelLoc,
        1,
        GL_FALSE,
        glm::value_ptr(model)
    );

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(
        normalMatrixLoc,
        1,
        GL_FALSE,
        glm::value_ptr(normalMatrix)
    );

    if (nightMode)
        skiNoapte.Draw(shader);
    else
        skiZi.Draw(shader);


    glDepthMask(GL_TRUE);
  
}


void renderTeapot(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
  
    model = glm::scale(model, glm::vec3(0.05f));

    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    
    farm.Draw(shader);
}

void renderWindmill(gps::Shader shader)
{
    shader.useShaderProgram();

    
    model = glm::mat4(1.0f);
    model = glm::translate(model, windmillPosition);
    model = glm::scale(model, glm::vec3(0.5f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    corp.Draw(shader);

    
    model = glm::mat4(1.0f);
    model = glm::translate(model, windmillPosition);
    model = glm::scale(model, glm::vec3(0.5f));
    model = glm::translate(model, elicePosition);
    model = glm::rotate(
        model,
        glm::radians(bladesAngle),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    model = glm::translate(model, -elicePosition);
    

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    alicee.Draw(shader);
}

void renderScene() {

    bladesAngle += deltaTime * 120.0f; 
    if (bladesAngle > 360.0f)
        bladesAngle -= 360.0f;

    if (presentationActive) {
        camSegmentTime += deltaTime;

        CameraPoint& start = cameraPath[currentCamPoint];
        CameraPoint& end = cameraPath[(currentCamPoint + 1) % cameraPath.size()];

        float t = camSegmentTime / start.duration;
        t = glm::clamp(t, 0.0f, 1.0f);

        glm::vec3 camPos = glm::mix(start.position, end.position, t);
        glm::vec3 camTarget = glm::mix(start.target, end.target, t);

        
        view = glm::lookAt(
            camPos,
            camTarget,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        if (t >= 1.0f) {
            camSegmentTime = 0.0f;
            currentCamPoint++;

            if (currentCamPoint >= cameraPath.size() - 1) {
                presentationActive = false;
                cameraUserControl = true;
            }
        }
    }

    lightSpaceMatrix = computeLightSpaceTrMatrix();

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix)
    );
    

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f));
   
    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );

    farm.Draw(depthMapShader);

    model = glm::mat4(1.0f);
    model = glm::translate(model, windmillPosition);
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    corp.Draw(depthMapShader);

    
    model = glm::mat4(1.0f);
    model = glm::translate(model, windmillPosition);
    model = glm::scale(model, glm::vec3(0.5f));
    model = glm::translate(model, elicePosition);
    model = glm::rotate(model, glm::radians(bladesAngle), glm::vec3(0, 0, 1));
    model = glm::translate(model, -elicePosition);
    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    alicee.Draw(depthMapShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    
    int fbW, fbH;
    glfwGetFramebufferSize(myWindow.getWindow(), &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);


    
    myBasicShader.useShaderProgram();
    
    glUniform1i(nightModeLoc, nightMode);
    if (nightMode)
    {
        glUniform3f(lightColorLoc, 0.4f, 0.45f, 0.6f);
    }
    else
    {
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); 
    }
    glUniformMatrix4fv(
        glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix)
    );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    glUniform1i(
        glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"),
        3
    );

	
    if (!presentationActive) {
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    updatePointLights();

    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));
    glUniform1i(
        glGetUniformLocation(myBasicShader.shaderProgram, "enableFog"),
        fogEnabled
    );
    
    renderTeapot(myBasicShader);
    renderWindmill(myBasicShader);
    glUniform1i(isSkyLoc, 1);
    renderSky(myBasicShader);
    glUniform1i(isSkyLoc, 0);
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
    initFBO();

	initUniforms();
    
    setWindowCallbacks();

	glCheckError();
    float lastFrameTime = glfwGetTime();

	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrameTime;
        lastFrameTime = currentFrame;

        processMovement();
        processInput(myWindow.getWindow());

	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
