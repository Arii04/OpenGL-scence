#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// 3D Models
C3dglSkyBox skybox;
C3dglTerrain terrain, road;
C3dglModel lamp;
C3dglModel guy1;
C3dglModel guy2;

// texture ids
GLuint idTexGrass;		// grass texture
GLuint idTexRoad;		// road texture
GLuint idTexNone;
GLuint idTexShadowMap;
GLuint idFBO;


// night (0) or day (1)
GLuint dayLight = 1;
// light attenuation
GLuint lightAtt = 0;

// GLSL Objects (Shader Program)
C3dglProgram program;

// The View Matrix
mat4 matrixView;

// Camera & navigation
float maxspeed = 4.f;	// camera max speed
float accel = 4.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)

void initLights(GLuint dayLight)
{
	program.sendUniform("lightAmbient.color", vec3(dayLight * 0.1, dayLight * 0.1, dayLight * 0.1));
	program.sendUniform("lightEmissive.color", vec3(0.0, 0.0, 0.0));
	program.sendUniform("lightDir.direction", vec3(1.0, 0.5, 1.0));
	program.sendUniform("lightDir.diffuse", vec3(dayLight * 1.0, dayLight * 1.0, dayLight * 1.0));

	GLuint nightLight = 1 - dayLight;
	program.sendUniform("lightPoint1.diffuse", vec3(nightLight * 0.5, nightLight * 0.5, nightLight * 0.5));
	program.sendUniform("lightPoint1.specular", vec3(nightLight * 1.0, nightLight * 1.0, nightLight * 1.0));
	program.sendUniform("lightPoint2.diffuse", vec3(nightLight * 0.5, nightLight * 0.5, nightLight * 0.5));
	program.sendUniform("lightPoint2.specular", vec3(nightLight * 1.0, nightLight * 1.0, nightLight * 1.0));
	program.sendUniform("lightPoint3.diffuse", vec3(nightLight * 0.5, nightLight * 0.5, nightLight * 0.5));
	program.sendUniform("lightPoint3.specular", vec3(nightLight * 1.0, nightLight * 1.0, nightLight * 1.0));
	program.sendUniform("lightPoint4.diffuse", vec3(nightLight * 0.5, nightLight * 0.5, nightLight * 0.5));
	program.sendUniform("lightPoint4.specular", vec3(nightLight * 1.0, nightLight * 1.0, nightLight * 1.0));
	program.sendUniform("lightPoint5.diffuse", vec3(nightLight * 0.5, nightLight * 0.5, nightLight * 0.5));
	program.sendUniform("lightPoint5.specular", vec3(nightLight * 1.0, nightLight * 1.0, nightLight * 1.0));
}

bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// Initialise Shaders
	C3dglShader vertexShader;
	C3dglShader fragmentShader;

	if (!vertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!vertexShader.loadFromFile("shaders/basic.vert")) return false;
	if (!vertexShader.compile()) return false;

	if (!fragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!fragmentShader.loadFromFile("shaders/basic.frag")) return false;
	if (!fragmentShader.compile()) return false;

	if (!program.create()) return false;
	if (!program.attach(vertexShader)) return false;
	if (!program.attach(fragmentShader)) return false;
	if (!program.link()) return false;
	if (!program.use(true)) return false;

	// glut additional setup
	glutSetVertexAttribCoord3(program.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(program.getAttribLocation("aNormal"));

	// load your 3D models here!
	if (!terrain.load("models\\heightmap.png", 10)) return false;
	if (!road.load("models\\roadmap.png", 10)) return false;

	// load Sky Box
	if (!skybox.load("models\\TropicalSunnyDay\\TropicalSunnyDayFront1024.jpg", 
		             "models\\TropicalSunnyDay\\TropicalSunnyDayLeft1024.jpg", 
					 "models\\TropicalSunnyDay\\TropicalSunnyDayBack1024.jpg", 
					 "models\\TropicalSunnyDay\\TropicalSunnyDayRight1024.jpg", 
					 "models\\TropicalSunnyDay\\TropicalSunnyDayUp1024.jpg", 
					 "models\\TropicalSunnyDay\\TropicalSunnyDayDown1024.jpg")) return false;

	if (!lamp.load("models\\street lamp - fancy.obj")) return false;

	if (!guy1.load("models\\guy1.fbx")) return false;
	guy1.loadMaterials("models\\");
	if (!guy2.load("models\\guy2.fbx")) return false;
	guy2.loadMaterials("models\\");


	// create & load textures
	C3dglBitmap bm;
	glActiveTexture(GL_TEXTURE0);
	
	// grass texture
	bm.load("models/grass.png", GL_RGBA);
	glGenTextures(1, &idTexGrass);
	glBindTexture(GL_TEXTURE_2D, idTexGrass);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// Road texture
	bm.load("models/road.png", GL_RGBA);
	glGenTextures(1, &idTexRoad);
	glBindTexture(GL_TEXTURE_2D, idTexRoad);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	BYTE bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_BGR, GL_UNSIGNED_BYTE, &bytes);

	// Send the texture info to the shaders
	program.sendUniform("texture0", 0);

	// setup lights:
	initLights(dayLight);
	program.sendUniform("lightAttOn", lightAtt);

	// setup materials
	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));		// full power (note: ambient light is extremely dim)
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialSpecular", vec3(0.0, 0.0, 0.0));

	// Create shadow map texture

	glActiveTexture(GL_TEXTURE7);

	glGenTextures(1, &idTexShadowMap);

	glBindTexture(GL_TEXTURE_2D, idTexShadowMap);


	// Texture parameters - to get nice filtering & avoid artefact on the edges of the shadowmap

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);


	// This will associate the texture with the depth component in the Z-buffer

	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);

	int w = viewport[2], h = viewport[3];

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w * 2, h * 2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);


	// Send the texture info to the shaders

	program.sendUniform("shadowMap", 7);


	// revert to texture unit 0

	glActiveTexture(GL_TEXTURE0);

	// Create a framebuffer object (FBO)

	glGenFramebuffers(1, &idFBO);

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, idFBO);


	// Instruct openGL that we won't bind a color texture with the currently binded FBO

	glDrawBuffer(GL_NONE);

	glReadBuffer(GL_NONE);


	// attach the texture to FBO depth attachment point

	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, idTexShadowMap, 0);


	// switch back to window-system-provided framebuffer

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glEnable(GL_CULL_FACE);
	
	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1), radians(12.f), vec3(1, 0, 0));
	matrixView *= lookAt(
		vec3(4.0, 1.5, 30.0),
		vec3(4.0, 1.5, 0.0),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.0f, 0.0f, 0.2f, 1.0f);   // night sky background (not needed in the day - there is skybox)

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;
	cout << "  N to switch between day & night" << endl;
	cout << "  T to switch attenuatiuon" << endl;
	cout << endl;

	return true;
}

void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;

	program.sendUniform("lightAttOn", lightAtt);

	if (dayLight)
	{
		// prepare ambient light for the skybox
		program.sendUniform("lightAmbient.color", vec3(1.0, 1.0, 1.0));
		program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
		program.sendUniform("materialDiffuse", vec3(0.0, 0.0, 0.0));

		

		// render the skybox
		m = matrixView;
		skybox.render(m);

		// revert normal light after skybox
		program.sendUniform("lightAmbient.color", vec3(0.4, 0.4, 0.4));
	}
	glCullFace(GL_FRONT);
	// setup the grass texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, idTexGrass);

	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));

	// render the terrain
	m = translate(matrixView, vec3(0, 0, 0));
	terrain.render(m);

	// setup the road texture
	glBindTexture(GL_TEXTURE_2D, idTexRoad);

	// render the road
	m = translate(matrixView, vec3(0, 0, 0));
	m = translate(m, vec3(6.0f, 0.01f, 0.0f));
	road.render(m);

	program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
	program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));

	// lamps
	vec3 lampPos[] = {
		vec3(6.2, 3.0, 24.0f),
		vec3(4.7, 4.2, 12.0f),
		vec3(6.2, 3.1, 0.0f),
		vec3(4.7, 4.5, -12.0f),
		vec3(6.2, 2.1, -24.0f),
	};
	string uniName = "lightPoint1.position";
	for (vec3 pos : lampPos)
	{
		m = translate(matrixView, pos);
		program.sendUniform("materialAmbient", vec3(0.2, 0.2, 0.2));
		program.sendUniform("materialDiffuse", vec3(0.2, 0.2, 0.2));
		lamp.render(scale(m, vec3(0.01f, 0.01f, 0.01f)));	// render scaled lamp
		m = translate(m, vec3(0.f, 1.22f, 0.f));
		program.sendUniform("matrixModelView", m);
		GLuint nightLight = 1 - dayLight;
		program.sendUniform("lightEmissive.color", vec3(1.0 * nightLight, 1.0 * nightLight, 1.0 * nightLight));
		program.sendUniform("materialAmbient", vec3(1.0, 1.0, 1.0));
		program.sendUniform("materialDiffuse", vec3(1.0, 1.0, 1.0));
		glutSolidSphere(0.125, 32, 32);
		program.sendUniform("lightEmissive.color", vec3(0, 0, 0));
		program.sendUniform(uniName, vec3(pos.x, pos.y + 1.22f, pos.z));
		uniName[10]++;
	}

	//guy1
	std::vector<mat4> transforms;
	guy1.getAnimData(0, time, transforms);
	program.sendUniform("bones", &transforms[0], transforms.size());
	m = matrixView;
	m = translate(m, vec3(6.2, 3.1, 1.0f));
	m = scale(m, vec3(0.005f, 0.005f, 0.005f));
	m = rotate(m, radians(270.0f) ,vec3(0.0f, 1.0f, 0.0f));
	guy1.render(m);
	guy1.loadAnimations();

	//guy2
	guy2.getAnimData(0, time, transforms);
	program.sendUniform("bones", &transforms[0], transforms.size());
	m = matrixView;
	m = translate(m, vec3(5.2, 2.52, 1.0f));
	m = scale(m, vec3(0.005f, 0.005f, 0.005f));
	m = rotate(m, radians(90.0f), vec3(0.0f, 1.0f, 0.0f));
	guy2.render(m);
	guy2.loadAnimations();

	glCullFace(GL_BACK);
}



// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	program.sendUniform("matrixProjection", perspective(radians(_fov), ratio, 0.02f, 1000.f));
}

// Creates a shadow map and stores in idFBO

// lightTransform - lookAt transform corresponding to the light position predominant direction

void createShadowMap(mat4 lightTransform, float time, float deltaTime)

{

	glEnable(GL_CULL_FACE);

	glCullFace(GL_FRONT);


	// Store the current viewport in a safe place

	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);

	int w = viewport[2], h = viewport[3];


	// setup the viewport to 2x2 the original and wide (120 degrees) FoV (Field of View)

	glViewport(0, 0, w * 2, h * 2);

	mat4 matrixProjection = perspective(radians(160.f), (float)w / (float)h, 0.5f, 50.0f);

	program.sendUniform("matrixProjection", matrixProjection);


	// prepare the camera

	mat4 matrixView = lightTransform;


	// send the View Matrix

	program.sendUniform("matrixView", matrixView);


	// Bind the Framebuffer

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, idFBO);

	// OFF-SCREEN RENDERING FROM NOW!


	// Clear previous frame values - depth buffer only!

	glClear(GL_DEPTH_BUFFER_BIT);


	// Disable color rendering, we only want to write to the Z-Buffer (this is to speed-up)

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);


	// Prepare and send the Shadow Matrix - this is matrix transform every coordinate x,y,z

	// x = x* 0.5 + 0.5

	// y = y* 0.5 + 0.5

	// z = z* 0.5 + 0.5

	// Moving from unit cube [-1,1] to [0,1]

	const mat4 bias = {

	{ 0.5, 0.0, 0.0, 0.0 },

	{ 0.0, 0.5, 0.0, 0.0 },

	{ 0.0, 0.0, 0.5, 0.0 },

	{ 0.5, 0.5, 0.5, 1.0 }

	};

	program.sendUniform("matrixShadow", bias * matrixProjection * matrixView);


	// Render all objects in the scene

	renderScene(matrixView, time, deltaTime);


	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glDisable(GL_CULL_FACE);

	onReshape(w, h);

}

void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in seconds
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime

	createShadowMap(lookAt(

		vec3(6.2, 5.1, 0.0f), // coordinates of the source of the light

		vec3(0.0f, 5.0f, 0.0f), // coordinates of a point within or behind the scene

		vec3(0.0f, 1.0f, 0.0f)), // a reasonable "Up" vector

		time, deltaTime);


	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// setup the View Matrix (camera)
	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;

	// move the camera up following the profile of terrain (Y coordinate of the terrain)
	float terrainY = -terrain.getInterpolatedHeight(inverse(matrixView)[3][0], inverse(matrixView)[3][2]);
	matrixView = translate(matrixView, vec3(0, terrainY, 0));

	// setup View Matrix
	program.sendUniform("matrixView", matrixView);

	// render the scene objects
	renderScene(matrixView, time, deltaTime);

	// the camera must be moved down by terrainY to avoid unwanted effects
	matrixView = translate(matrixView, vec3(0, -terrainY, 0));

	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;

	case 'n': 
		dayLight = 1 - dayLight; 
		initLights(dayLight);
		break;
	case 't': lightAtt = 1 - lightAtt; break;
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = glm::clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("3DGL Scene: Skyboxed Terrain");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	C3dglLogger::log("Vendor: {}", (const char *)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char *)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}

