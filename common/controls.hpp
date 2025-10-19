/*
* Author: Liam Long
* Class: ECE4122 (Lab 3)
* Last Date Modified: 2025-10-18
*
* Description / Purpose of this file:
* Public interface for the orbit‑camera controls module. The renderer calls
* `computeMatricesFromInputs()` once per frame to update camera state from
* keyboard input, then retrieves the current view and projection matrices via
* accessors. A lighting toggle flag is also exposed so the shader can enable
* or disable diffuse/specular terms.
*/

#ifndef CONTROLS_HPP
#define CONTROLS_HPP

/*
* computeMatricesFromInputs()
* ------------------------------------------------------------------
* Purpose:
* Poll GLFW key states and integrate the orbit‑camera parameters
* (radius/azimuth/elevation). Rebuild and cache the View and Projection
* matrices for this frame. Also handles an edge‑triggered 'L' key toggle for
* enabling/disabling diffuse+specular lighting.
* Inputs:
* None (reads from the GLFW window created in main.cpp).
* Returns:
* void (updates internal module state; call once per frame before rendering).
*/
void computeMatricesFromInputs();

/*
* getViewMatrix()
* ------------------------------------------------------------------
* Purpose: Provide the current view matrix (camera transform).
* Returns: glm::mat4 view matrix built during the most recent call to
* computeMatricesFromInputs().
*/
glm::mat4 getViewMatrix();

/*
* getProjectionMatrix()
* ------------------------------------------------------------------
* Purpose: Provide the current perspective projection matrix.
* Returns: glm::mat4 projection matrix (uses a fixed FOV and aspect).
*/
glm::mat4 getProjectionMatrix();

/*
* getEnableDiffuseSpec()
* ------------------------------------------------------------------
* Purpose: Expose the current state of the lighting toggle so the renderer
* can send it to the shader as a uniform.
* Returns: true => diffuse + specular enabled
* false => disabled
*/
bool getEnableDiffuseSpec();

#endif