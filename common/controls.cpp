/*
* Author: Liam Long
* Class: ECE4122 (Lab 3)
* Last Date Modified: 2025-10-18
*
* Description / Purpose of this file:
* Orbit-style camera controls and render-toggles used by the Lab 3 scene.
* The camera orbits the world origin with Z as the up-axis. The user can
* zoom (W/S), orbit left/right (A/D), and change elevation (Up/Down). The
* 'L' key toggles a boolean used by the shader to enable/disable
* diffuse/specular lighting. This module exposes the current View and
* Projection matrices to the renderer.
*/

#include <GLFW/glfw3.h>
extern GLFWwindow* window;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "controls.hpp"

// Matrices exposed to renderer
static glm::mat4 ViewMatrix;        // camera -> world transform inverse
static glm::mat4 ProjectionMatrix;  // perspective projection

/*
* getViewMatrix
* ------------------------------------------------------------------
* Purpose: Return the current view matrix (camera transform).
* Inputs: none
* Returns: glm::mat4 view matrix used by the renderer this frame.
*/
glm::mat4 getViewMatrix()
{ 
    return ViewMatrix; 
}

/*
* getProjectionMatrix
* ------------------------------------------------------------------
* Purpose: Return the current projection matrix.
* Inputs: none
* Returns: glm::mat4 perspective projection matrix.
*/
glm::mat4 getProjectionMatrix() 
{ 
    return ProjectionMatrix; 
}

// ---- Camera state (orbit around origin, Z up) ----
static float g_radius    = 12.0f;                 // distance from origin
static float g_azimuth   = 0.0f;                  // radians around Z
static float g_elevation = glm::radians(25.0f);   // radians up/down
static float g_fovDeg    = 60.0f;                 // field of view
static bool  g_enableDiffuseSpec = true;          // 'L' toggles this

// Speeds
static const float kAngSpeed  = 1.8f;  // rad/sec
static const float kZoomSpeed = 6.0f;  // units/sec

/*
* getEnableDiffuseSpec
* ------------------------------------------------------------------
* Purpose: Expose the current diffuse/specular enable flag for the
* shader program in the renderer.
* Inputs: none
* Returns: bool flag; true => enable diffuse/spec highlights in shader.
*/
bool getEnableDiffuseSpec() 
{ 
    return g_enableDiffuseSpec; 
}

/*
* computeMatricesFromInputs
* ------------------------------------------------------------------
* Purpose:
* Poll the keyboard, integrate camera state over time (orbit radius,
* azimuth, elevation), apply clamps, then build the View and Projection
* matrices for use by the renderer. Also performs an edge-triggered 'L'
* toggle for the diffuse/specular lighting flag.
*
* Inputs:
* (reads) GLFW key states from the global window.
*
* Key Bindings:
* W/S : zoom in/out (decrease/increase radius)
* A/D : orbit left/right (increase/decrease azimuth)
* Up/Down arrows: raise/lower elevation (pitch)
* L : toggle diffuse+specular lighting on/off (edge-triggered)
*
* Returns: void. Updates module‑global matrices and flags.
*/
void computeMatricesFromInputs()
{
    // Frame delta-time, independent of frame rate
    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    float dt = float(now - lastTime);
    lastTime = now;

    // Keyboard — spec requires WASD + arrows
    // W to zoom in 
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        g_radius  -= kZoomSpeed * dt;
    }
    
    // S to zoom out
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        g_radius  += kZoomSpeed * dt;
    }
    
    // A to move camera radially left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        g_azimuth -= kAngSpeed  * dt; // right
    }
    
    // D to move camera radially right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        g_azimuth += kAngSpeed  * dt; // left
    }
    
    // Up arrow to move camera radially up
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        g_elevation += kAngSpeed * dt;
    }
    
    // Down arrow to move camera radially down
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        g_elevation -= kAngSpeed * dt;
    }
    
    // Edge-triggered toggle for 'L' to switch diffuse and specular settings
    static int lastL = GLFW_RELEASE;
    int lNow = glfwGetKey(window, GLFW_KEY_L);
    if (lNow == GLFW_PRESS && lastL == GLFW_RELEASE)
    {
        g_enableDiffuseSpec = !g_enableDiffuseSpec;
    }
    lastL = lNow;
    
    // Prevent passing through the origin and flipping models at the poles
    g_radius = glm::max(g_radius, 2.0f);
    const float maxEl = glm::radians(89.9f); // avoid pole flip
    if (g_elevation >  maxEl)
    {
        g_elevation =  maxEl;
    }
    
    if (g_elevation < -maxEl)
    {
        g_elevation = -maxEl;
    }

    // Spherical -> Cartesian (Z up)
    float x = g_radius * cosf(g_elevation) * cosf(g_azimuth);
    float y = g_radius * cosf(g_elevation) * sinf(g_azimuth);
    float z = g_radius * sinf(g_elevation);
    glm::vec3 cam(x, y, z);

    // Look from cam toward origin with world up = +Z
    ViewMatrix = glm::lookAt(cam, glm::vec3(0.0f), glm::vec3(0,0,1));
    
    ProjectionMatrix = glm::perspective(glm::radians(g_fovDeg), 4.0f/3.0f, 0.1f, 100.0f);
}
