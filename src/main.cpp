#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tinyobjloader/tiny_obj_loader.h>
#include <glm/gtx/quaternion.hpp>

using namespace std;
using namespace glm;
using namespace tinyobj;

// Window dimensions
const GLuint WIDTH = 1280, HEIGHT = 720;
const GLfloat half_WIDTH = WIDTH / 2, half_HEIGHT = HEIGHT / 2;


// light0 = position, light1 = direction, default = on
int light0 = 1;
int light1 = 1;

// model obj loading
string inputfile = "../../model/bunny.obj";
attrib_t attrib;
vector<shape_t> shapes;
vector<material_t> materials;
string warn;
string err;

bool ret = LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str());

void BunnyDraw()
{
    if (!warn.empty()) {
        cout << warn << endl;
    }

    if (!err.empty()) {
        cerr << err << endl;
    }

    if (!ret) {
        exit(1);
    }

    vector<real_t> vx(3), vy(3), vz(3);
    vector<real_t> nx(3), ny(3), nz(3);
    size_t index_offset = 0;
    vector faces = shapes[0].mesh.num_face_vertices;
    int n = 0;

    // loop over faces.size()
    for (size_t f = 0; f < faces.size(); f++) {
        size_t fv = size_t(faces[f]);

        // Loop over vertices in the face.
        for (size_t v = 0; v < fv; v++) {
            // access to vertex
            index_t idx = shapes[0].mesh.indices[index_offset + v];

            vx[v] = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
            vy[v] = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
            vz[v] = attrib.vertices[3 * size_t(idx.vertex_index) + 2];  

            if (idx.normal_index >= 0) {
                nx[v] = attrib.normals[3 * size_t(idx.normal_index) + 0];
                ny[v] = attrib.normals[3 * size_t(idx.normal_index) + 1];
                nz[v] = attrib.normals[3 * size_t(idx.normal_index) + 2];
            }
        }

        float _nx=0, _ny=0, _nz=0;
        for (int i = 0; i < nx.size(); i++)
        {
            _nx += nx[i];
            _ny += ny[i];
            _nz += nz[i];
        }
        _nx /= nx.size();
        _ny /= ny.size();
        _nz /= nz.size();

        glBegin(GL_POLYGON);
        glNormal3f(_nx, _ny, _nz);
        for (int i = 0; i < vx.size(); i++)        
            glVertex3d(vx[i], vy[i], vz[i]);        
        glEnd();
        n++;

        index_offset += fv;
    }
    //cout << "faces" << faces.size() << endl;
    //cout << "draw" << n << endl;

}

void TestDraw()
{
    glBegin(GL_TRIANGLES);

    //{ Front }

    glNormal3f(0, 0.44721, 0.89443);
    glVertex3f(0.0f, 1.0f, 0.0f); // Top Of Triangle (Front)
    glVertex3f(-1.0f, -1.0f, 1.0f); // Left Of Triangle (Front)
    glVertex3f(1.0f, -1.0f, 1.0f); // Right Of Triangle (Front)

    //{ Right }
    glNormal3f(0.89443, 0.44721, 0);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    //{ Back }
    glNormal3f(0, 0.44721, -0.89443);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);

    //{ Left }
    glNormal3f(-0.89443, 0.44721, 0);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glEnd();

}


float radius = HEIGHT * 0.5f;

float clampX(float x)
{
    if (x <= -half_WIDTH)
        x = -half_WIDTH + 1;
    else if (x >= half_WIDTH)
        x = half_WIDTH - 1;
    return x;
}

float clampY(float y)
{
    if (y <= -half_HEIGHT)
        y = -half_HEIGHT + 1;
    else if (y >= half_HEIGHT)
        y = half_HEIGHT - 1;
    return y;
}

// projection on shpere
vec3 getVectorWithProject(float x, float y)
{
    /*
    Vector3 vec = Vector3(x, y, 0);
    float d = x*x + y*y;
    float rr = radius * radius;
    if(d <= rr)
    {
        vec.z = sqrtf(rr - d);
    }
    else
    {
        vec.z = 0;
        // put (x,y) on the sphere radius
        vec.normalize();
        vec *= radius;
    }

    return vec;
    */

    vec3 vec = vec3(x, y, 0);
    float d = x * x + y * y;
    float rr = radius * radius;

    // use sphere if d<=0.5*r^2:  z = sqrt(r^2 - (x^2 + y^2))
    if (d <= (0.5f * rr))
        vec.z = sqrtf(rr - d);

    // use hyperbolic sheet if d>0.5*r^2:  z = (r^2 / 2) / sqrt(x^2 + y^2)
    // referenced from trackball.c by Gavin Bell at SGI
    else
    {
        // compute z first using hyperbola
        vec.z = 0.5f * rr / sqrtf(d);

        // scale x and y down, so the vector can be on the sphere
        // y = ax => x^2 + (ax)^2 + z^2 = r^2 => (1 + a^2)*x^2 = r^2 - z^2
        // => x = sqrt((r^2 - z^2) / (1 - a^2)
        float x2, y2, a;
        if (x == 0.0f) // avoid dividing by 0
        {
            x2 = 0.0f;
            y2 = sqrtf(rr - vec.z * vec.z);
            if (y < 0)       // correct sign
                y2 = -y2;
        }
        else
        {
            a = y / x;
            x2 = sqrtf((rr - vec.z * vec.z) / (1 + a * a));
            if (x < 0)   // correct sign
                x2 = -x2;
            y2 = a * x2;
        }

        vec.x = x2;
        vec.y = y2;
    }

    return vec;
}

// get vector on sphere
vec3 getVector(double x, double y)
{
    if (radius == 0 || WIDTH == 0 || HEIGHT == 0)
        return vec3(0, 0, 0);

    // compute mouse position from the centre of screen (-half ~ +half)
    //float mx = x - half_WIDTH;
    //float my = half_HEIGHT - y;    // OpenGL uses bottom to up orientation
    float mx = clampX(x - half_WIDTH);
    float my = clampY(half_HEIGHT - y);    // OpenGL uses bottom to up orientation


    return normalize(getVectorWithProject(mx, my)); // default mode
}


double xpos, ypos;
double prevX, prevY;
vec3 s_cord = {};
bool pressed = false;
vec3 v1(0) , v2(0) ; // v1 = rotation start, v2 = rotation end

// rotation calculation
quat RotationBetweenVectors(vec3 start, vec3 dest) {
    start = normalize(start);
    dest = normalize(dest);
    float cosTheta = dot(start, dest);
    vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions:
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
        if (length2(rotationAxis) <
            0.01) // bad luck, they were parallel, try again!
            rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);
        rotationAxis = normalize(rotationAxis);
        return angleAxis(radians(180.0f), rotationAxis);
    }
    rotationAxis = cross(start, dest);
    float s = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;
    return quat(s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs);
}


// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


mat4 matModel = identity<mat4>(); 
mat4 modelView = identity<mat4>();
mat4 matProj = identity<mat4>();
mat4 matView = identity<mat4>();
mat4 R = identity<mat4>();
mat4 prevR = identity<mat4>();
vec3 eye = vec3(3, 7, 4);
vec3 target = vec3(0, 1, 0);
vec3 forwardv = eye - target;
GLfloat fov = 60.0f;


// The MAIN function, from here we start the application and run the game loop
int main()
{
    cout << "Starting GLFW context, OpenGL 3.1" << endl;

    // Init GLFW
    glfwInit();

    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "glskeleton", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize OpenGL context" << endl;
        return -1;
    }

    // Lighting & material
    {
        // 조명의 성질 설정
        GLfloat ambientLight[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
        GLfloat specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        GLfloat diffuseLight[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat lightPosition0[4] = { 1.0f, 2.0f, 2.0f, 1.0f }; // positional light
        GLfloat lightPosition1[4] = { 0.0f, -1.0f, 0.0f, 0.0f }; // directional light

        // material 설정
        GLfloat ks[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat shine = 80.0;

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glMaterialfv(GL_FRONT, GL_SPECULAR, ks);
        glMaterialf(GL_FRONT, GL_SHININESS, shine);
        
        glShadeModel(GL_SMOOTH); //phong shading
        glEnable(GL_LIGHTING); //조명 켜기

        glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight); //설정
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight); //설정
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular); //설정
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition0); //설정

        glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight); //설정
        glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight); //설정
        glLightfv(GL_LIGHT1, GL_SPECULAR, specular); //설정
        glLightfv(GL_LIGHT1, GL_POSITION, lightPosition1); //설정
    }


    // Set the required callback functions
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);


    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);


    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        glfwWaitEvents();
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // 밝은 회색을 배경색으로 설정 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        if (light0 == 1) glEnable(GL_LIGHT0); // 0(pos)번 조명 사용           
        else glDisable(GL_LIGHT0);
        if (light1 == 1) glEnable(GL_LIGHT1); // 1(dir)번 조명 사용           
        else glDisable(GL_LIGHT1);

        glColor3f(0.7f, 0.5f, 0.7f);
        BunnyDraw();

        matProj = perspective(radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        glMatrixMode(GL_PROJECTION);     // switch to projection matrix
        glLoadMatrixf(value_ptr(matProj));

        matView = lookAt(eye, target, vec3(0, 1, 0));
        modelView = matView * matModel;

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(value_ptr(modelView));
        glMultMatrixf(value_ptr(R));

        glfwSwapBuffers(window);
    }

    // Terminates GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();

    return 0;
}



// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        light0 = 1; //0 on
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        light0 = 0; //0 off
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        light1 = 1; //1 on
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
        light1 = 0; //1 off
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        fov -= 5;
        if (fov < 1.0f)
            fov = 1.0f;
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        fov += 5;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (pressed)
    {
        v1 = getVector(prevX, prevY);
        v2 = getVector(xpos, ypos);
        R = toMat4(RotationBetweenVectors(v1, v2)) * prevR;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    glfwGetCursorPos(window, &xpos, &ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            pressed = true;
            prevX = xpos;
            prevY = ypos;     
            prevR = R;
        }

        else if (action == GLFW_RELEASE)
        {
            pressed = false;            
            v2 = v1;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset > 0) 
        eye = eye - forwardv * 0.1f;
    else 
        eye = eye + forwardv * 0.1f;   
}