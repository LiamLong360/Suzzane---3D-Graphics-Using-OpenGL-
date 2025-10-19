/*
* Author: Liam Long
* Class: ECE4122 (Lab 3)
* Last Date Modified: 2025-10-18
*
* Description / Purpose of this file:
* OpenGL/GLFW application that renders a textured floor quad and eight
* indexed “Suzanne” monkey heads arranged uniformly on a ring. The heads are
* rotated/uprighted so the chins sit on the z = 0 plane, and each head faces
* radially outward. Camera/view/projection are driven by the provided
* controls helper. The program demonstrates VBO/IBO usage, VAO setup, basic
* lighting uniforms, texturing, polygon offset to avoid z‑fighting, and
* back‑face culling control.
*/

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <limits>               // for std::numeric_limits
#include <glm/gtc/constants.hpp> // for glm::half_pi<float>()
#include <cmath>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

/*
* main()
* ------------------------------------------------------------------
* Purpose:
* Entry point. Initializes GLFW/GLEW, compiles shaders, loads geometry and
* textures, sets up buffers/VAOs, and drives the render loop until the user
* exits (ESC or window close).
*
* Inputs:
* (none)
*
* Outputs / Return:
* int process exit code (0 on success; negative on initialization failure).
*/
int main()
{
	// Initialize GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "ECE 4122 - Lab 3", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it is closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("uvmap.DDS");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("suzanne.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);
	
	// Rotate model so suzannes rest chin‑down on z = 0 after lift
	glm::mat4 Rfix = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1, 0, 0)); // base matrix, angle to turn, axis of which to turn about
	
	// Compute vertical lift so the lowest transformed vertex sits at z = 0
	float minZ = std::numeric_limits<float>::infinity();
    for (const auto& v : indexed_vertices)
    { // find lowest z offset to place chin on z = 0 plane
        glm::vec3 vt = glm::vec3(Rfix * glm::vec4(v, 1.0f));
        minZ = std::min(minZ, vt.z);
    }
    // add to model z to place chin on the floor
    float liftToGround = -minZ;

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	
	// Geometry (two triangles) centered at origin on z = 0
    float s = 4.5f; // Length of 1 side of rectangle
    
    // Position of vertices for triangles
    glm::vec3 floorPos[4] =
    {
        {-s, -s, 0.0f},
        { s, -s, 0.0f},
        { s,  s, 0.0f},
        {-s,  s, 0.0f}
    };
    
    // Mapping vertices to uv-plane
    glm::vec2 floorUV[4] = 
    {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };

    // Normal vectors to triangles
    glm::vec3 floorNorm[4] = 
    {
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    };
    
    unsigned short floorIdx[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    
    GLuint floorVAO, floorVBO, floorUVBO, floorNBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glBindVertexArray(floorVAO);
    
    glGenBuffers(1, &floorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorPos), floorPos, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // matches your shader layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glGenBuffers(1, &floorUVBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorUVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorUV), floorUV, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glGenBuffers(1, &floorNBO);
    glBindBuffer(GL_ARRAY_BUFFER, floorNBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorNorm), floorNorm, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glGenBuffers(1, &floorEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIdx), floorIdx, GL_STATIC_DRAW);
    
    glBindVertexArray(0);

	// For speed computation - frame time counter
	double lastTime = glfwGetTime();
	int nbFrames = 0;

    // Main loop
	do
	{
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0/double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		
		// Use our shader
		glUseProgram(programID);
	
		glm::vec3 lightPos = glm::vec3(4,4,4);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]); // This one doesn't change between objects, so this can be done once for all objects that use "programID"
		
		// Draw floor rectangle
		
		// Make sure we see it from both sides (or set proper winding)
        GLboolean wasCulled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);
        
        // Avoid z-fighting with heads touching z=0
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        
        glm::mat4 M_floor(1.0f); // stays on z = 0
        glm::mat4 MVP_floor = ProjectionMatrix * ViewMatrix * M_floor;
        
        // uniforms the shader already expects
        glUniformMatrix4fv(MatrixID,      1, GL_FALSE, &MVP_floor[0][0]);
        glUniformMatrix4fv(ViewMatrixID,  1, GL_FALSE, &ViewMatrix[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &M_floor[0][0]);
        
        // bind SAME texture + sampler uniform
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glUniform1i(TextureID, 0);
        
        // get uniform locations once (after program link)
        GLint uUseTintLoc = glGetUniformLocation(programID, "uUseTint");
        GLint uTintLoc    = glGetUniformLocation(programID, "uTint");
        
        GLint uEnableLoc = glGetUniformLocation(programID, "uEnableDiffuseSpec");
        glUniform1i(uEnableLoc, getEnableDiffuseSpec() ? 1 : 0);
        
        // Use a solid green tint for the floor (shader multiplies it)
        glUniform1i(uUseTintLoc, 0); // 0 = use texture (still)
        glUniform3f(uTintLoc, 0.0f, 1.0f, 0.0f); // tint color if enabled
        
        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0); // Draws the floor
        glBindVertexArray(0);
        
        glDisable(GL_POLYGON_OFFSET_FILL);
        if (wasCulled) glEnable(GL_CULL_FACE);
		
		// Draw all suzanne heads
		// Place 8 heads on a circuit of radius r in the x-y plane
		const float r = sqrt(14.0f);    // radial distance from origin
        const float start = -M_PI / 2;  // start at -90 degrees
        const float step = M_PI / 4;    // 45 degree offsets
        
        for (int i = 0; i < 8; i++)
        {
            // Polar placement
            float theta = start + i * step;
            float x = r * std::cos(theta);
            float y = r * std::sin(theta);
            float yaw = theta + M_PI / 2;
            
            // Model transformation:
            // 1. Move onto ring
            // 2. Yaw the head to face radially away from origin
            // 3. Apply x-rotation and vertical lift to place chins on z = 0 plane
            glm::mat4 M(1.0f);
            M = glm::translate(M, glm::vec3(x, y, liftToGround));  // place on ring
            M = glm::rotate(M, yaw, glm::vec3(0,0,1));             // face radially
            M = M * Rfix;                                          // chin fix
            glm::mat4 MVP = ProjectionMatrix * ViewMatrix * M;
            
            // Send our transformation to the currently bound shader, 
    		// in the "MVP" uniform
    		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &M[0][0]);
    
    		glBindVertexArray(VertexArrayID);  // re-bind a VAO we can attach attributes to
    		glUniform1i(uUseTintLoc, 0); // back to textured heads
    		
    		// 1rst attribute buffer : vertices
    		glEnableVertexAttribArray(0);
    		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    		// 2nd attribute buffer : UVs
    		glEnableVertexAttribArray(1);
    		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    		// 3rd attribute buffer : normals
    		glEnableVertexAttribArray(2);
    		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    		// Index buffer
    		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    
    		// Draw the triangles !
    		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);
    	
        }

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

