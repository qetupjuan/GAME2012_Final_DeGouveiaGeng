﻿
///////////////////////////////////////////////////////////////////////
//
// GAME2012 Final Project by De Gouveia (101203253) and Geng Li (101277255)
//
// Description
// 1. Maze constructed according to design plan, with a place for plumbob and a center room.
// 2. Wall encloses the maze from 4 sides. Gate at the main wall and exit at the back wall.
// 3. Functional plumbob collection and deposite system to unlock exit
// 4. Point lights at the important segments of our dark maze and torches (color changes in the center room if plumbob has been successfully deposited)
//
///////////////////////////////////////////////////////////////////////

using namespace std;

#include <cstdlib>
#include <ctime>
#include "vgl.h"
#include "LoadShaders.h"
#include "Light.h"
#include "Shape.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define MOVESPEED 1.5f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,1,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)


enum keyMasks {
	KEY_FORWARD = 0b00000001,		// 0x01 or 1 or 01
	KEY_BACKWARD = 0b00000010,		// 0x02 or 2 or 02
	KEY_LEFT = 0b00000100,
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000
	// Any other keys you want to add.
};

// IDs.
GLuint vao, ibo, points_vbo, colors_vbo, uv_vbo, normals_vbo, modelID, viewID, projID;
GLuint program;

// Matrices.
glm::mat4 View, Projection;

// Our bitflags. 1 byte for up to 8 keys.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f, b_height = 1.0f, b_dir = 0.05f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;

//Collection and collision stuff

bool exitActivated = false, entryDoorLock = false;

enum bob_condition
{
	UNTOUCHED,
	COLLECTED,
	RETURNED
};
bob_condition currentCond = UNTOUCHED;

// Texture variables.
GLuint alexTx, blankTx, leavesTx, wallTx, wall2Tx, glassTx, woodTx, doorTx, roofTx, door2Tx, door3Tx, windowTx, cylinderTx;
GLint width, height, bitDepth;

// Light variables.
AmbientLight aLight(glm::vec3(1.0f, 1.0f, 1.0f),	// Ambient colour.
	0.15f);							// Ambient strength.

DirectionalLight dLight(glm::vec3(-1.0f, 0.0f, -0.5f), // Direction.
	glm::vec3(1.0f, 1.0f, 0.25f),  // Diffuse colour.
	0.1f);						  // Diffuse strength.

PointLight pLights[7] = { { glm::vec3(29.0f, 6.0f, -4.0f), 50.0f, glm::vec3(1.0f, 0.6f, 0.0f), 1.0f },
						  { glm::vec3(37.5f, 6.0f, -4.0f), 50.0f, glm::vec3(1.0f, 0.6f, 0.0f), 1.0f }, 
						{ glm::vec3(21.0f, 2.0f, -24.0f), 50.0f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f }, 
						{ glm::vec3(34.5f, 1.1f, -43.5f), 50.0f, glm::vec3(1.0f, 0.0f, 0.0f), 1.0f },
						{ glm::vec3(53.5f, 4.0f, -77.0f), 50.0f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f },
						{ glm::vec3(59.0f, 7.0f, -15.5f), 50.0f, glm::vec3(1.0f, 0.6f, 0.0f), 1.0f }, 
						{ glm::vec3(11.0f, 7.0f, -72.5f), 50.0f, glm::vec3(1.0f, 0.6f, 0.0f), 1.0f } };

PointLight* p_lightP = &pLights[3];

//SpotLight sLight(glm::vec3(5.0f, 1.75f, -5.0f),	// Position.
//	glm::vec3(1.0f, 1.0f, 1.0f),	// Diffuse colour.
//	1.0f,							// Diffuse strength.
//	glm::vec3(0.0f, -1.0f, 0.0f),  // Direction.
//	15.0f);

void timer(int);

void resetView()
{
	position = glm::vec3(40.0f, 10.0f, 10.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = 0.0f;
	yaw = -90.0f;
	// View will now get set only in transformObject
}

// Shapes. Recommend putting in a map
Cube g_cube;
Cube g_wall;
Cone g_hat(24);
Cone g_plumbob(5);
Prism g_prism(24);
Plane g_plane;
std::vector<glm::vec3> g_wallPositions;
std::vector<glm::vec3> g_merlonsPositions;

void createWallPos()
{
	for (int z = 6; z <= 82; z += 1)
	{
		g_wallPositions.push_back(glm::vec3(8, -0.8, -z));
		g_wallPositions.push_back(glm::vec3(7, -1, -z));
		g_wallPositions.push_back(glm::vec3(6, 0, -z));

		g_wallPositions.push_back(glm::vec3(61, -0.8, -z));
		g_wallPositions.push_back(glm::vec3(62, -1, -z));
		g_wallPositions.push_back(glm::vec3(63, 0, -z));
	}
	for (int x = 0; x <= 8; x += 1)
	{
		g_wallPositions.push_back(glm::vec3(x + 10, -0.8, -8));
		g_wallPositions.push_back(glm::vec3(x + 10, -1, -7));
		g_wallPositions.push_back(glm::vec3(x + 10, 0, -6));
		g_wallPositions.push_back(glm::vec3(x + 18, -0.8, -8));
		g_wallPositions.push_back(glm::vec3(x + 18, -1, -7));
		g_wallPositions.push_back(glm::vec3(x + 18, 0, -6));
		g_wallPositions.push_back(glm::vec3(x + 21, -0.8, -8));
		g_wallPositions.push_back(glm::vec3(x + 21, -1, -7));
		g_wallPositions.push_back(glm::vec3(x + 21, 0, -6));
		g_wallPositions.push_back(glm::vec3(x + 37, -0.8, -8));
		g_wallPositions.push_back(glm::vec3(x + 37, -1, -7));
		g_wallPositions.push_back(glm::vec3(x + 37, 0, -6));
		g_wallPositions.push_back(glm::vec3(x + 40, -0.8, -8));
		g_wallPositions.push_back(glm::vec3(x + 40, -1, -7));
		g_wallPositions.push_back(glm::vec3(x + 40, 0, -6));
		g_wallPositions.push_back(glm::vec3(x + 47, -0.8, -8));
		g_wallPositions.push_back(glm::vec3(x + 47, -1, -7));
		g_wallPositions.push_back(glm::vec3(x + 47, 0, -6));

		g_wallPositions.push_back(glm::vec3(x + 17, -0.8, -80));
		g_wallPositions.push_back(glm::vec3(x + 17, -1, -81));
		g_wallPositions.push_back(glm::vec3(x + 17, 0, -82));
		g_wallPositions.push_back(glm::vec3(x + 25, -0.8, -80));
		g_wallPositions.push_back(glm::vec3(x + 25, -1, -81));
		g_wallPositions.push_back(glm::vec3(x + 25, 0, -82));
		g_wallPositions.push_back(glm::vec3(x + 33, -0.8, -80));
		g_wallPositions.push_back(glm::vec3(x + 33, -1, -81));
		g_wallPositions.push_back(glm::vec3(x + 33, 0, -82));
		g_wallPositions.push_back(glm::vec3(x + 41, -0.8, -80));
		g_wallPositions.push_back(glm::vec3(x + 41, -1, -81));
		g_wallPositions.push_back(glm::vec3(x + 41, 0, -82));
		g_wallPositions.push_back(glm::vec3(x + 43, -0.8, -80));
		g_wallPositions.push_back(glm::vec3(x + 43, -1, -81));
		g_wallPositions.push_back(glm::vec3(x + 43, 0, -82));
		g_wallPositions.push_back(glm::vec3(x + 55, -0.8, -80));
		g_wallPositions.push_back(glm::vec3(x + 55, -1, -81));
		g_wallPositions.push_back(glm::vec3(x + 55, 0, -82));
	}
	for (int z = 6; z <= 82; z += 2)
	{
		g_merlonsPositions.push_back(glm::vec3(6, 0, -z));
		g_merlonsPositions.push_back(glm::vec3(63, 0, -z));

	}
	for (int x = 0; x <= 8; x += 2)
	{
		g_merlonsPositions.push_back(glm::vec3(x + 10, 0, -6));
		g_merlonsPositions.push_back(glm::vec3(x + 18, 0, -6));
		g_merlonsPositions.push_back(glm::vec3(x + 20, 0, -6));
		g_merlonsPositions.push_back(glm::vec3(x + 38, 0, -6));
		g_merlonsPositions.push_back(glm::vec3(x + 40, 0, -6));
		g_merlonsPositions.push_back(glm::vec3(x + 48, 0, -6));

		g_merlonsPositions.push_back(glm::vec3(x + 15, 0, -82));
		g_merlonsPositions.push_back(glm::vec3(x + 23, 0, -82));
		g_merlonsPositions.push_back(glm::vec3(x + 31, 0, -82));
		g_merlonsPositions.push_back(glm::vec3(x + 39, 0, -82));
		g_merlonsPositions.push_back(glm::vec3(x + 43, 0, -82));
		g_merlonsPositions.push_back(glm::vec3(x + 55, 0, -82));
	}
};

void init(void)
{
	srand((unsigned)time(NULL));
	//Specifying the name of vertex and fragment shaders.
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	//Loading and compiling shaders
	program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	modelID = glGetUniformLocation(program, "model");
	projID = glGetUniformLocation(program, "projection");
	viewID = glGetUniformLocation(program, "view");

	// Projection matrix : 45∞ Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 300.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	resetView();

	// Image loading.
	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load("dirt.png", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &alexTx);
	glBindTexture(GL_TEXTURE_2D, alexTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	unsigned char* image2 = stbi_load("blank.jpg", &width, &height, &bitDepth, 0);
	if (!image2) cout << "Unable to load file!" << endl;

	glGenTextures(1, &blankTx);
	glBindTexture(GL_TEXTURE_2D, blankTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image2);

	// leaves.

	unsigned char* image3 = stbi_load("leaves.jpg", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;

	glGenTextures(1, &leavesTx);
	glBindTexture(GL_TEXTURE_2D, leavesTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image3);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image3);

	unsigned char* image4 = stbi_load("brick.jpg", &width, &height, &bitDepth, 0);
	if (!image4) cout << "Unable to load file!" << endl;

	glGenTextures(1, &wallTx);
	glBindTexture(GL_TEXTURE_2D, wallTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image4);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image4);

	unsigned char* image5 = stbi_load("brickk.jpg", &width, &height, &bitDepth, 0);
	if (!image5) cout << "Unable to load file!" << endl;

	glGenTextures(1, &wall2Tx);
	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image5);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image5);

	unsigned char* image6 = stbi_load("glass.png", &width, &height, &bitDepth, 0);
	if (!image6) cout << "Unable to load file!" << endl;

	glGenTextures(1, &glassTx);
	glBindTexture(GL_TEXTURE_2D, glassTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image6);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image6);

	// dfgsggsdfgfds  woodTx, doorTx, roofTx;

	unsigned char* image7 = stbi_load("wood1.jpg", &width, &height, &bitDepth, 0);
	if (!image7) cout << "Unable to load file!" << endl;

	glGenTextures(1, &woodTx);
	glBindTexture(GL_TEXTURE_2D, woodTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image7);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image7);

	unsigned char* image8 = stbi_load("door6.png", &width, &height, &bitDepth, 0);
	if (!image8) cout << "Unable to load file!" << endl;

	glGenTextures(1, &doorTx);
	glBindTexture(GL_TEXTURE_2D, doorTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image8);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image8);

	unsigned char* image9 = stbi_load("roof.png", &width, &height, &bitDepth, 0);
	if (!image9) cout << "Unable to load file!" << endl;

	glGenTextures(1, &roofTx);
	glBindTexture(GL_TEXTURE_2D, roofTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image9);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image9);

	unsigned char* image10 = stbi_load("door5.png", &width, &height, &bitDepth, 0);
	if (!image10) cout << "Unable to load file!" << endl;

	glGenTextures(1, &door2Tx);
	glBindTexture(GL_TEXTURE_2D, door2Tx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image10);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image10);

	unsigned char* image11 = stbi_load("door10.png", &width, &height, &bitDepth, 0);
	if (!image11) cout << "Unable to load file!" << endl;

	glGenTextures(1, &door3Tx);
	glBindTexture(GL_TEXTURE_2D, door3Tx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image11);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image11);

	unsigned char* image12 = stbi_load("window.png", &width, &height, &bitDepth, 0);
	if (!image12) cout << "Unable to load file!" << endl;

	glGenTextures(1, &windowTx);
	glBindTexture(GL_TEXTURE_2D, windowTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image12);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image12);

	unsigned char* image13 = stbi_load("cylinder.png", &width, &height, &bitDepth, 0);
	if (!image13) cout << "Unable to load file!" << endl;

	glGenTextures(1, &cylinderTx);
	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image13);
	// Note: image types with native transparency will need to be GL_RGBA instead of GL_RGB.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image13);

	glUniform1i(glGetUniformLocation(program, "texture0"), 0);

	// Setting ambient Light.
	glUniform3f(glGetUniformLocation(program, "aLight.ambientColour"), aLight.ambientColour.x, aLight.ambientColour.y, aLight.ambientColour.z);
	glUniform1f(glGetUniformLocation(program, "aLight.ambientStrength"), aLight.ambientStrength);

	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColour"), dLight.diffuseColour.x, dLight.diffuseColour.y, dLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

	// Setting point lights.
	glUniform3f(glGetUniformLocation(program, "pLights[0].base.diffuseColour"), pLights[0].diffuseColour.x, pLights[0].diffuseColour.y, pLights[0].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].base.diffuseStrength"), pLights[0].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[0].position"), pLights[0].position.x, pLights[0].position.y, pLights[0].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[0].constant"), pLights[0].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[0].linear"), pLights[0].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[0].exponent"), pLights[0].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[1].base.diffuseColour"), pLights[1].diffuseColour.x, pLights[1].diffuseColour.y, pLights[1].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].base.diffuseStrength"), pLights[1].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[1].position"), pLights[1].position.x, pLights[1].position.y, pLights[1].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[1].constant"), pLights[1].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[1].linear"), pLights[1].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[1].exponent"), pLights[1].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[2].base.diffuseColour"), pLights[2].diffuseColour.x, pLights[2].diffuseColour.y, pLights[2].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[2].base.diffuseStrength"), pLights[2].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[2].position"), pLights[2].position.x, pLights[2].position.y, pLights[2].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[2].constant"), pLights[2].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[2].linear"), pLights[2].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[2].exponent"), pLights[2].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[3].base.diffuseColour"), pLights[3].diffuseColour.x, pLights[3].diffuseColour.y, pLights[3].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[3].base.diffuseStrength"), pLights[3].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[3].position"), pLights[3].position.x, pLights[3].position.y, pLights[3].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[3].constant"), pLights[3].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[3].linear"), pLights[3].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[3].exponent"), pLights[3].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[4].base.diffuseColour"), pLights[4].diffuseColour.x, pLights[4].diffuseColour.y, pLights[4].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[4].base.diffuseStrength"), pLights[4].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[4].position"), pLights[4].position.x, pLights[4].position.y, pLights[4].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[4].constant"), pLights[4].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[4].linear"), pLights[4].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[4].exponent"), pLights[4].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[5].base.diffuseColour"), pLights[5].diffuseColour.x, pLights[5].diffuseColour.y, pLights[5].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[5].base.diffuseStrength"), pLights[5].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[5].position"), pLights[5].position.x, pLights[5].position.y, pLights[5].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[5].constant"), pLights[5].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[5].linear"), pLights[5].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[5].exponent"), pLights[5].exponent);

	glUniform3f(glGetUniformLocation(program, "pLights[6].base.diffuseColour"), pLights[6].diffuseColour.x, pLights[6].diffuseColour.y, pLights[6].diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLights[6].base.diffuseStrength"), pLights[6].diffuseStrength);
	glUniform3f(glGetUniformLocation(program, "pLights[6].position"), pLights[6].position.x, pLights[6].position.y, pLights[6].position.z);
	glUniform1f(glGetUniformLocation(program, "pLights[6].constant"), pLights[6].constant);
	glUniform1f(glGetUniformLocation(program, "pLights[6].linear"), pLights[6].linear);
	glUniform1f(glGetUniformLocation(program, "pLights[6].exponent"), pLights[6].exponent);


	// Setting spot light.
	//glUniform3f(glGetUniformLocation(program, "sLight.base.diffuseColour"), sLight.diffuseColour.x, sLight.diffuseColour.y, sLight.diffuseColour.z);
	//glUniform1f(glGetUniformLocation(program, "sLight.base.diffuseStrength"), sLight.diffuseStrength);

	//glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	//glUniform3f(glGetUniformLocation(program, "sLight.direction"), sLight.direction.x, sLight.direction.y, sLight.direction.z);
	//glUniform1f(glGetUniformLocation(program, "sLight.edge"), sLight.edgeRad);

	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	ibo = 0;
	glGenBuffers(1, &ibo);

	points_vbo = 0;
	glGenBuffers(1, &points_vbo);

	colors_vbo = 0;
	glGenBuffers(1, &colors_vbo);

	uv_vbo = 0;
	glGenBuffers(1, &uv_vbo);

	normals_vbo = 0;
	glGenBuffers(1, &normals_vbo);

	glBindVertexArray(0); // Can optionally unbind the vertex array to avoid modification.

	// Change shape data.
	g_prism.SetMat(0.1, 16);
	g_hat.SetMat(0.1, 16);
	g_plumbob.SetMat(0.25, 35);

	// Enable depth test and blend.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Enable face culling.
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	timer(0);

	createWallPos();
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
	glUniform3f(glGetUniformLocation(program, "eyePosition"), position.x, position.y, position.z);
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);

	calculateView();
	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}

//---------------------------------------------------------------------
//
// display
// //////////////////////

void drawWalls()
{
	for (int i = 0; i < g_wallPositions.size(); i++)
	{
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 6.0f, 1.0f), X_AXIS, 0.0f, g_wallPositions[i]);
		glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int i = 0; i < g_merlonsPositions.size(); i++)
	{
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_wall.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 7.0f, 1.0f), X_AXIS, 0.0f, g_merlonsPositions[i]);
		glDrawElements(GL_TRIANGLES, g_wall.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
}

void checkCollision()
{
	if ((abs(position.x - 22.0f) < 2.0f) && (abs(position.z + 25.0f) < 2.0f) && (abs(position.y) < 5.0f))
	{
		if (currentCond == UNTOUCHED)
		{
			std::cout << "PLUMBOB COLLECTED!" << std::endl;
			currentCond = COLLECTED;
		}
	}
	else if ((abs(position.x - 35.0f) < 2.0f) && (abs(position.z + 45.0f) < 2.0f) && (abs(position.y) < 5.0f))
	{
		if (currentCond == COLLECTED)
		{
			std::cout << "PLUMBOB DETECTED..." << std::endl;
			std::cout << "EXIT NOW UNLOCKED!" << std::endl;
			p_lightP->diffuseColour = glm::vec3(0.0f, 1.0f, 0.0f);
			exitActivated = true;
			currentCond = RETURNED;
		}
	}

	if ((abs(position.x - 54.0f) < 2.0f) && (abs(position.z + 78.0f) < 2.0f) && (abs(position.y) < 5.0f))
	{
		if (exitActivated)
		{
			std::cout << "THANK YOU FOR PLAYING, WELCOME BACK ANY TIME!" << std::endl;
		}
		else
		{
			std::cout << "WARNING! MAZE EXIT CURRENTLY LOCKED, PLEASE COLLECT THE PLUMBOB AND DEPLOY AT THE CENTER ROOM TO UNLOCK" << std::endl;
		}
	}

	//Initial aka entry check, will lock after passing, so you can't go back
	if ((abs(position.x - 33.0f) < 3.0f) && (abs(position.z + 8.0f) < 3.0f) && (abs(position.y) < 5.0f)) //entry door check
	{
		if (entryDoorLock)
		{
			std::cout << "ENTRY LOCKED, YOU ONLY HAVE ONE WAY TO EXIT!" << std::endl;
		}
		else
		{
			std::cout << "WELCOME TO THE MAZE CASTLE!" << std::endl;
			if ((abs(position.x - 33.0f) < 3.0f) && (abs(position.z + 12.0f) < 3.0f) && (abs(position.y) < 5.0f)) //will lock the door
			{
				std::cout << "ENTRY DOOR IS NOW LOCKED!" << std::endl;
				entryDoorLock = true;
			}
		}
	}
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.15f, 0.0f, 0.3f, 1.0f);

	glBindVertexArray(vao);
	// Draw all shapes.

	/*glBindTexture(GL_TEXTURE_2D, alexTx);
	g_plane.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 10.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_plane.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	/*glBindTexture(GL_TEXTURE_2D, alexTx);
	g_grid.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(100.0f, 100.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_grid.NumIndices(), GL_UNSIGNED_SHORT, 0);*/

	glBindTexture(GL_TEXTURE_2D, alexTx);
	g_plane.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(70.0f, 100.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_plane.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glUniform3f(glGetUniformLocation(program, "pLights[3].base.diffuseColour"), pLights[3].diffuseColour.x, pLights[3].diffuseColour.y, pLights[3].diffuseColour.z);

	//glUniform3f(glGetUniformLocation(program, "sLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);

	drawWalls();
	// first sector
	// borders and left side

	glBindTexture(GL_TEXTURE_2D, leavesTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 40.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -50.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(13.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -20.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(19.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -10.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -12.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -12.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -20.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -14.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(19.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(19.0f, 0.0f, -18.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(26.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(21.0f, 0.0f, -14.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -14.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -18.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(13.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -18.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -14.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(28.0f, 0.0f, -13.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(16.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -10.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 35.0f), X_AXIS, 0.0f, glm::vec3(59.0f, 0.0f, -50.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -25.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(22.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(22.0f, 0.0f, -30.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// middle

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(32.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -20.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -13.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(26.0f, 0.0f, -15.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(28.0f, 0.0f, -20.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(28.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// right
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -15.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -12.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -12.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -12.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(12.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(40.0f, 0.0f, -14.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
;
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(40.0f, 0.0f, -24.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -24.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(42.0f, 0.0f, -16.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(14.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(44.0f, 0.0f, -18.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(44.0f, 0.0f, -23.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(44.0f, 0.0f, -21.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(51.0f, 0.0f, -20.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(11.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -24.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -24.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -22.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(42.0f, 0.0f, -24.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(42.0f, 0.0f, -24.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// second sector
	// left

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(9.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -30.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -34.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -34.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -40.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -40.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -36.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -46.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(14.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -46.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -42.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -40.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(22.0f, 0.0f, -40.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -38.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -36.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -34.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(25.0f, 0.0f, -42.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -48.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -34.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -29.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -29.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -48.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -48.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -56.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -52.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);


	// middle

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -35.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -31.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -31.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -38.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(18.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(33.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -35.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -33.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(40.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(43.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(41.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(42.0f, 0.0f, -35.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(41.0f, 0.0f, -33.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);



	// right

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -26.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -40.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -30.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(54.0f, 0.0f, -30.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -28.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -32.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -34.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -37.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);


	// Center of the maze

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 4.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -41.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 4.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -41.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 4.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -43.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 4.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 4.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 4.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 4.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 4.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -43.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(9.0f, 0.1f, 5.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -46.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// pedestals 

	glBindTexture(GL_TEXTURE_2D, wall2Tx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -44.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// third sector
	// left

	glBindTexture(GL_TEXTURE_2D, leavesTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -58.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -48.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -48.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(22.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -48.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -50.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -50.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 11.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -64.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(14.0f, 0.0f, -62.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -62.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -62.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -58.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -64.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(22.0f, 0.0f, -64.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -62.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(16.0f, 0.0f, -56.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -54.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -51.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(20.0f, 0.0f, -52.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// middle

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(32.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -51.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(15.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(18.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(11.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(46.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -51.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// right

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(41.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(43.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
	
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(41.0f, 0.0f, -40.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -49.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(13.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -41.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(50.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -39.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(46.0f, 0.0f, -45.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(9.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(46.0f, 0.0f, -45.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(52.0f, 0.0f, -45.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -45.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -47.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -45.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -43.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// fourth sector

	// left

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -70.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -70.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(9.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -64.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(19.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(11.0f, 0.0f, -62.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(12.0f, 0.0f, -68.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(15.0f, 0.0f, -68.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(17.0f, 0.0f, -70.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(19.0f, 0.0f, -68.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(21.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(21.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(25.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -71.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -69.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -67.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 12.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -77.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);


	// middle and rifht

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(26.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(32.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(23.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -73.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 9.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -65.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(24.0f, 0.0f, -65.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(12.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -65.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(26.0f, 0.0f, -63.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(26.0f, 0.0f, -63.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(28.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(32.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(34.0f, 0.0f, -63.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -65.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -63.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(44.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 12.0f), X_AXIS, 0.0f, glm::vec3(44.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(42.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(40.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(40.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -51.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(46.0f, 0.0f, -51.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -51.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(51.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -53.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(46.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(50.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(52.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(54.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(56.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(58.0f, 0.0f, -55.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(51.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -57.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(50.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(52.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(54.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(56.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(58.0f, 0.0f, -59.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// right

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(53.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 14.0f), X_AXIS, 0.0f, glm::vec3(51.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(56.0f, 0.0f, -61.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 0.0f, -63.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(56.0f, 0.0f, -65.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(46.0f, 0.0f, -63.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(51.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(48.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(54.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(54.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 9.0f), X_AXIS, 0.0f, glm::vec3(49.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// middle top

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -71.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 8.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(29.0f, 0.0f, -67.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -67.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, -67.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -75.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(33.0f, 0.0f, -73.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(33.0f, 0.0f, -77.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, -77.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -77.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -75.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 5.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, -77.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -69.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(33.0f, 0.0f, -69.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, -70.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(8.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(41.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(41.0f, 0.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(43.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(43.0f, 0.0f, -74.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -72.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(45.0f, 0.0f, -72.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, -69.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 3.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(47.0f, 0.0f, -70.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// more borders

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 28.0f), X_AXIS, 0.0f, glm::vec3(59.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 22.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -72.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(15.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(17.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(16.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(32.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 7.0f), X_AXIS, 0.0f, glm::vec3(17.0f, 0.0f, -78.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 0.0f, -72.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// TOWERSSSSSSSSSS

	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 8.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(5.0f, 0.0f, -9.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(12.0f, 8.0f, 12.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -15.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(12.0f, 8.0f, 12.0f), X_AXIS, 0.0f, glm::vec3(5.0f, 0.0f, -85.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 8.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(59.0f, 0.0f, -83.0f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 8.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, cylinderTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 8.0f, 4.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 0.0f, -8.5f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// tower doors

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(10.0f, 5.0f, -8.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(7.0f, 5.0f, -75.5f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(7.0f, 5.0f, -9.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(15.0f, 5.0f, -81.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(59.0f, 5.0f, -81.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(61.0f, 5.0f, -79.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 5.0f, -8.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(61.0f, 5.0f, -15.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(27.0f, 5.0f, -8.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, door3Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 5.0f, -8.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// windows

	glBindTexture(GL_TEXTURE_2D, windowTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(60.0f, 5.0f, -5.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, windowTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(62.0f, 5.0f, -5.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, windowTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(62.0f, 5.0f, -83.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, windowTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 3.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(7.0f, 5.0f, -5.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, windowTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 3.0f, 1.8f), Y_AXIS, 45.0f, glm::vec3(14.0f, 5.0f, -76.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, windowTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 3.0f, 2.0f), Y_AXIS, 15.0f, glm::vec3(12.0f, 5.0f, -75.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);


	// steps

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -9.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 0.60f, 1.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -9.8f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(7.0f, 0.3f, 1.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -10.6f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(0.5f, 2.0f, 3.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 0.0f, -10.7f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(0.5f, 2.0f, 3.0f), Y_AXIS, 10.0f, glm::vec3(36.5f, 0.0f, -10.7f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// Door

	glBindTexture(GL_TEXTURE_2D, doorTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 6.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 2.0f, -7.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 2.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -7.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(5.0f, 1.6f, 1.0f), X_AXIS, 0.0f, glm::vec3(31.0f, 0.0f, -8.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, woodTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.0f, 0.5f, 6.0f), X_AXIS, 20.0f, glm::vec3(31.5f, 1.8f, -6.5f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// torches 

	glBindTexture(GL_TEXTURE_2D, woodTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(0.2f, 1.4f, 0.4f), X_AXIS, 10.0f, glm::vec3(29.0f, 5.0f, -4.5f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, woodTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(0.2f, 1.4f, 0.4f), X_AXIS, 10.0f, glm::vec3(37.5f, 5.0f, -4.5f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, woodTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(0.2f, 1.4f, 0.4f), X_AXIS, 10.0f, glm::vec3(11.0f, 5.0f, -73.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, woodTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(0.2f, 1.4f, 0.4f), X_AXIS, -10.0f, glm::vec3(59.0f, 5.0f, -15.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	// back door

	glBindTexture(GL_TEXTURE_2D, door2Tx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 4.0f, 2.0f), X_AXIS, 0.0f, glm::vec3(52.0f, 0.0f, -82.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, wallTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(3.0f, 1.2f, 3.0f), X_AXIS, 0.0f, glm::vec3(52.0f, 4.0f, -82.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, leavesTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(51.0f, 0.0f, -79.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, leavesTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(55.0f, 0.0f, -79.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	//Corner hats
	glBindTexture(GL_TEXTURE_2D, roofTx);
	g_hat.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 12.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(3.0f, 8.0f, -11.0f));
	glDrawElements(GL_TRIANGLES, g_hat.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, roofTx);
	g_hat.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(15.0f, 16.0f, 15.0f), X_AXIS, 0.0f, glm::vec3(53.5f, 8.0f, -16.3f));
	glDrawElements(GL_TRIANGLES, g_hat.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, roofTx);
	g_hat.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(15.0f, 15.0f, 15.0f), X_AXIS, 0.0f, glm::vec3(3.5f, 8.0f, -86.5f));
	glDrawElements(GL_TRIANGLES, g_hat.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, roofTx);
	g_hat.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(10.0f, 12.0f, 10.0f), X_AXIS, 0.0f, glm::vec3(57.0f, 8.0f, -85.0f));
	glDrawElements(GL_TRIANGLES, g_hat.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, roofTx);
	g_hat.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 7.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 8.0f, -9.5f));
	glDrawElements(GL_TRIANGLES, g_hat.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, roofTx);
	g_hat.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(6.0f, 7.0f, 6.0f), X_AXIS, 0.0f, glm::vec3(26.0f, 8.0f, -9.5f));
	glDrawElements(GL_TRIANGLES, g_hat.NumIndices(), GL_UNSIGNED_SHORT, 0);

	checkCollision();

	//plumbob

	if (currentCond != COLLECTED)
	{
		if ((b_height < 1.0f) || (b_height >= 2.0f))
		{
			b_dir *= -1.0f;
		}
		b_height += b_dir;
		g_plumbob.ColorShape(0.0f, 1.0f, 0.0f);
		glBindTexture(GL_TEXTURE_2D, blankTx);
	}
	
	switch (currentCond)
	{
	case UNTOUCHED:
		g_plumbob.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 0.5f), X_AXIS, 0.0f, glm::vec3(21.2f, b_height, -24.8f));
		glDrawElements(GL_TRIANGLES, g_plumbob.NumIndices(), GL_UNSIGNED_SHORT, 0);

		g_plumbob.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 0.5f), X_AXIS, 180.0f, glm::vec3(21.2f, b_height, -24.3f));
		glDrawElements(GL_TRIANGLES, g_plumbob.NumIndices(), GL_UNSIGNED_SHORT, 0);
		break;
	case COLLECTED:
		g_plumbob.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 0.5f), X_AXIS, 0.0f, glm::vec3(21.2f, -100.0f, -24.8f));
		glDrawElements(GL_TRIANGLES, g_plumbob.NumIndices(), GL_UNSIGNED_SHORT, 0);

		g_plumbob.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 0.5f), X_AXIS, 180.0f, glm::vec3(21.2f, -100.0f, -24.3f));
		glDrawElements(GL_TRIANGLES, g_plumbob.NumIndices(), GL_UNSIGNED_SHORT, 0);
		break;
	case RETURNED:
		g_plumbob.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 0.5f), X_AXIS, 0.0f, glm::vec3(34.25f, b_height + 1.0f, -43.75f));
		glDrawElements(GL_TRIANGLES, g_plumbob.NumIndices(), GL_UNSIGNED_SHORT, 0);

		g_plumbob.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(0.5f, 1.0f, 0.5f), X_AXIS, 180.0f, glm::vec3(34.25f, b_height + 1.0f, -43.25f));
		glDrawElements(GL_TRIANGLES, g_plumbob.NumIndices(), GL_UNSIGNED_SHORT, 0);
		break;
	}

	//Glass
	glBindTexture(GL_TEXTURE_2D, glassTx);
	g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(9.0f, 0.3f, 5.0f), X_AXIS, 0.0f, glm::vec3(30.0f, 3.7f, -46.0f));
	glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0); // Done writing.
	glutSwapBuffers(); // Now for a potentially smoother render.
}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	else if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	else if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position.y += MOVESPEED;
	else if (keys & KEY_DOWN)
		position.y -= MOVESPEED;
}

void timer(int) { // essentially our update()
	parseKeys();
	glutPostRedisplay();
	glutTimerFunc(1000 / FPS, timer, 0); // 60 FPS or 16.67ms.
}

//---------------------------------------------------------------------
//
// keyDown
//
void keyDown(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD; break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT; break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT; break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP; break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN; break;
	//case 'i':
	//	sLight.position.z -= 0.1; break;
	//case 'j':
	//	sLight.position.x -= 0.1; break;
	//case 'k':
	//	sLight.position.z += 0.1; break;
	//case 'l':
	//	sLight.position.x += 0.1; break;
	//case 'p':
	//	sLight.position.y += 0.1; break;
	//case ';':
	//	sLight.position.y -= 0.1; break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; break;
	case 's':
		keys &= ~KEY_BACKWARD; break;
	case 'a':
		keys &= ~KEY_LEFT; break;
	case 'd':
		keys &= ~KEY_RIGHT; break;
	case 'r':
		keys &= ~KEY_UP; break;
	case 'f':
		keys &= ~KEY_DOWN; break;
	case ' ':
		resetView();
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		keys &= ~KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		keys &= ~KEY_BACKWARD;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch += (GLfloat)((y - lastY) * TURNSPEED);
		yaw -= (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	glDeleteTextures(1, &alexTx);
	glDeleteTextures(1, &wallTx);
	glDeleteTextures(1, &wall2Tx);
	glDeleteTextures(1, &windowTx);
	glDeleteTextures(1, &woodTx);
	glDeleteTextures(1, &leavesTx);
	glDeleteTextures(1, &glassTx);
	glDeleteTextures(1, &doorTx);
	glDeleteTextures(1, &roofTx);
	glDeleteTextures(1, &door2Tx);
	glDeleteTextures(1, &door3Tx);
	glDeleteTextures(1, &cylinderTx);

}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitWindowSize(1000, 1000);
	glutCreateWindow("GAME2012 Final Project De Gouveia Geng");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp); // New function for third example.
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.

	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	glutMainLoop();

	return 0;
}
