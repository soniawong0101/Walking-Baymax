#include "../Include/Common.h"

//For GLUT to handle 
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_HELLO 4
#define MENU_WORLD 5
#define MENU_ROTATE 6
#define MENU_WALK 7
#define MENU_STOPROTATE 8
#define MENU_STOPWALK 9

//GLUT timer variable
GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

bool Rotate = false;
bool Start_walking = false;
double walking_speed = 300.0;

using namespace glm;
using namespace std;

mat4 view;					// V of MVP, viewing matrix
mat4 projection;			// P of MVP, projection matrix
mat4 model;					// M of MVP, model matrix
vec3 temp = vec3();			// a 3 dimension vector which represents how far did the ladybug should move

GLint um4p;	
GLint um4mv;

GLuint program;			// shader program id

typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object
	GLuint vboTex;		// vertex buffer object of texture
	GLuint ebo;			

	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
    
    mat4 translation_matrix;
    vec3 rotate_axis;
    mat4 rotation_matrix;
    mat4 scale_matrix;
    mat4 model;
    mat4 pivot;
    mat4 r_pivot;
    GLfloat angle;
    
} Shape;

Shape m_shpae;
Shape Head;
Shape Body;
Shape Arm_r;
Shape Arm_l;
Shape Leg_r;
Shape Leg_l;

// Load shader file to program
char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

// Free shader file
void freeShaderSource(char** srcp)
{
    delete srcp[0];
    delete srcp;
}

// Load .obj model
Shape My_LoadModels(const char* model, const char* texture)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;

	bool ret = tinyobj::LoadObj(shapes, materials, err, model);
    
	if(err.size()>0)
	{
		printf("Load Models Fail! Please check the solution path");
		return m_shpae;
	}
	
	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());

	for(int i = 0; i < shapes.size(); i++)
	{
		glGenVertexArrays(1, &m_shpae.vao);
		glBindVertexArray(m_shpae.vao);

		glGenBuffers(3, &m_shpae.vbo);
		glGenBuffers(1,&m_shpae.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, m_shpae.vbo);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float)+shapes[i].mesh.normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, 0, shapes[i].mesh.positions.size() * sizeof(float), &shapes[i].mesh.positions[0]);
		glBufferSubData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.normals.size() * sizeof(float), &shapes[i].mesh.normals[0]);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *) (shapes[i].mesh.positions.size() * sizeof(float)));

		glBindBuffer(GL_ARRAY_BUFFER, m_shpae.p_normal);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.normals.size() * sizeof(float), shapes[i].mesh.normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_shpae.vboTex);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(float), shapes[i].mesh.texcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_shpae.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);
		m_shpae.materialId = shapes[i].mesh.material_ids[0];
		m_shpae.indexCount = shapes[i].mesh.indices.size();


		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
	}

	texture_data tdata = load_png(texture);

	glGenTextures( 1, &m_shpae.m_texture );
	glBindTexture( GL_TEXTURE_2D, m_shpae.m_texture);


	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    delete[] tdata.data;
    return m_shpae;
}

// OpenGL initialization
void My_Init()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Create Shader Program
    program = glCreateProgram();

	// Create customize shader by tell openGL specify shader type 
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");

	// Assign content of these shader files to those shaders we created before
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

	// Logging
	shaderLog(vertexShader);
    shaderLog(fragmentShader);

	// Assign the program we created before with these shaders
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

	// Get the id of inner variable 'um4p' and 'um4mv' in shader programs
	um4p = glGetUniformLocation(program, "um4p");
    um4mv = glGetUniformLocation(program, "um4mv");

	// Tell OpenGL to use this shader program now
    glUseProgram(program);
   
	Body = My_LoadModels("Sphere.obj", "White.png");
    Head = My_LoadModels("Sphere.obj", "EyesWhite.png");
    Arm_r = My_LoadModels("Capsule.obj", "White.png");
    Arm_l = My_LoadModels("Capsule.obj", "White.png");
    Leg_l = My_LoadModels("Capsule.obj", "White.png");
    Leg_r = My_LoadModels("Capsule.obj", "White.png");
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	// Clear display buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind a vertex array for OpenGL (OpenGL will apply operation only to the vertex array objects it bind)
	//glBindVertexArray(m_shpae.vao);

	// Tell openGL to use the shader program we created before
	glUseProgram(program);
	// Todo
	// Build 2 matrix (translation matrix and rotation matrix)
	// then multiply these matrix with proper order
	// final result should restore in the variable 'model'

    //Body
    glBindVertexArray(Body.vao);
    Body.translation_matrix = translate(mat4(), temp);
    Body.rotate_axis = vec3(0.0, 1.0, 0.0);
    Body.rotation_matrix = rotate(mat4(), Body.angle, Body.rotate_axis);
    Body.scale_matrix = scale(mat4(), vec3(2.5, 2.8, 2.5));
    Body.model = Body.translation_matrix * Body.rotation_matrix * Body.scale_matrix;
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Body.model));
    glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
    glBindTexture(GL_TEXTURE_2D, Body.m_texture);
    glDrawElements(GL_TRIANGLES, Body.indexCount, GL_UNSIGNED_INT, 0);

    //Head
    glBindVertexArray(Head.vao);
    Head.translation_matrix = translate(mat4(), vec3(0, 0.6, 0));
    Head.rotate_axis = vec3(0.0, 0.0, 1.0);
    Head.rotation_matrix = rotate(mat4(), Head.angle, Head.rotate_axis);
    Head.scale_matrix = scale(mat4(), vec3(0.5, 0.38, 0.5));
    Head.model = Body.model * Head.translation_matrix * Head.rotation_matrix * Head.scale_matrix;
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Head.model));
    glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
    glBindTexture(GL_TEXTURE_2D, Head.m_texture);
    glDrawElements(GL_TRIANGLES, Head.indexCount, GL_UNSIGNED_INT, 0);

    //R_arm
    glBindVertexArray(Arm_r.vao);
    Arm_r.pivot = translate(mat4(), vec3(-0.2, 0.3, 0.68));
    Arm_r.r_pivot = translate(mat4(), vec3(0.2, -0.3, -0.68));
    Arm_r.translation_matrix = translate(mat4(), vec3(-0.0, 0.0, 0.58));
    Arm_r.rotate_axis = vec3(0.0, 0.0, -1.0);
    Arm_r.rotation_matrix = rotate(mat4(), Arm_r.angle, Arm_r.rotate_axis);
    Arm_r.scale_matrix = scale(mat4(), vec3(0.5, 0.3, 0.3));
    //Arm_r.model = Body.model * Arm_r.pivot * Arm_r.rotation_matrix * Arm_r.r_pivot * Arm_r.translation_matrix * Arm_r.scale_matrix;
    Arm_r.model = Body.model * Arm_r.translation_matrix * Arm_r.pivot * Arm_r.rotation_matrix * Arm_r.r_pivot * Arm_r.scale_matrix;
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Arm_r.model));
    glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
    glBindTexture(GL_TEXTURE_2D, Arm_r.m_texture);
    glDrawElements(GL_TRIANGLES, Arm_r.indexCount, GL_UNSIGNED_INT, 0);
    
    //L_arm
    glBindVertexArray(Arm_l.vao);
    Arm_l.pivot = translate(mat4(), vec3(-0.2, 0.3, -0.68));
    Arm_l.r_pivot = translate(mat4(), vec3(0.2, -0.3, 0.68));
    Arm_l.translation_matrix = translate(mat4(), vec3(-0.0, 0.0, -0.58));
    Arm_l.rotate_axis = vec3(0.0, 0.0, -1.0);
    Arm_l.rotation_matrix = rotate(mat4(), Arm_l.angle, Arm_l.rotate_axis);
    Arm_l.scale_matrix = scale(mat4(), vec3(0.5, 0.3, 0.3));
    //Arm_l.model = Body.model * Arm_l.pivot * Arm_l.rotation_matrix * Arm_l.r_pivot * Arm_l.translation_matrix * Arm_l.scale_matrix;
    Arm_l.model = Body.model * Arm_l.translation_matrix * Arm_l.pivot * Arm_l.rotation_matrix * Arm_l.r_pivot * Arm_l.scale_matrix;
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Arm_l.model));
    glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
    glBindTexture(GL_TEXTURE_2D, Arm_l.m_texture);
    glDrawElements(GL_TRIANGLES, Arm_l.indexCount, GL_UNSIGNED_INT, 0);

    //L_leg
    glBindVertexArray(Leg_l.vao);
    Leg_l.pivot = translate(mat4(), vec3(-0.0, -0.3, -0.3));
    Leg_l.r_pivot = translate(mat4(), vec3(0.0, 0.3, 0.3));
    Leg_l.translation_matrix = translate(mat4(), vec3(-0.0, -0.46, -0.2));
    Leg_l.rotate_axis = vec3(0.0, 0.0, -1.0);
    Leg_l.rotation_matrix = rotate(mat4(), Leg_l.angle, Leg_l.rotate_axis);
    Leg_l.scale_matrix = scale(mat4(), vec3(0.5, 0.2, 0.3));
    Leg_l.model = Body.model * Leg_l.pivot * Leg_l.rotation_matrix * Leg_l.r_pivot * Leg_l.translation_matrix * Leg_l.scale_matrix;
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Leg_l.model));
    glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
    glBindTexture(GL_TEXTURE_2D, Leg_l.m_texture);
    glDrawElements(GL_TRIANGLES, Leg_l.indexCount, GL_UNSIGNED_INT, 0);
    
    //R_leg
    glBindVertexArray(Leg_r.vao);
    Leg_r.pivot = translate(mat4(), vec3(-0.0, -0.3, 0.3));
    Leg_r.r_pivot = translate(mat4(), vec3(0.0, 0.3, -0.3));
    Leg_r.translation_matrix = translate(mat4(), vec3(-0.0, -0.46, 0.2));
    Leg_r.rotate_axis = vec3(0.0, 0.0, -1.0);
    Leg_r.rotation_matrix = rotate(mat4(), Leg_r.angle, Leg_r.rotate_axis);
    Leg_r.scale_matrix = scale(mat4(), vec3(0.5, 0.2, 0.3));
    Leg_r.model = Body.model * Leg_r.pivot * Leg_r.rotation_matrix * Leg_r.r_pivot * Leg_r.translation_matrix * Leg_r.scale_matrix;
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Leg_r.model));
    glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
    glBindTexture(GL_TEXTURE_2D, Leg_r.m_texture);
    glDrawElements(GL_TRIANGLES, Leg_r.indexCount, GL_UNSIGNED_INT, 0);
    
	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
    //glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	//glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	// Tell openGL to draw the vertex array we had binded before
	//glDrawElements(GL_TRIANGLES, m_shpae.indexCount, GL_UNSIGNED_INT, 0);
  
    if (Start_walking) {
        Leg_r.angle = (-sin(glutGet(GLUT_ELAPSED_TIME) / walking_speed) + 0.5)/2;
        Leg_l.angle = (sin(glutGet(GLUT_ELAPSED_TIME) / walking_speed) + 0.5)/2;
        Arm_l.angle = (-sin((glutGet(GLUT_ELAPSED_TIME)) / walking_speed));
        Arm_r.angle = (sin(glutGet(GLUT_ELAPSED_TIME) / walking_speed));
        Head.angle = (sin(glutGet(GLUT_ELAPSED_TIME) / walking_speed)) / 30;
    }
    if (Rotate) {
        Body.angle = glutGet(GLUT_ELAPSED_TIME) / 500.0;
    }
    if (!Start_walking && !Rotate) {
        Arm_l.angle = 0.0;
        Arm_r.angle = 0.0;
        Leg_r.angle = 0.0;
        Leg_l.angle = 0.0;
        Head.angle = 0.0;
    }
    
	// Change current display buffer to another one (refresh frame) 
    glutSwapBuffers();
}

// Setting up viewing matrix
void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);

	// lookAt(camera_position, camera_viewing_vector, up_vector)
	// up_vector represent the vector which define the direction of 'up'
	view = lookAt(vec3(-10.0f, 5.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void My_Timer(int val)
{
	timer_cnt++;
	glutPostRedisplay();
	if(timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

// TODO: try to translate by keyboard
void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	if (key == 'd')
	{
		temp += vec3(0, 0, 0.2);
	}
	else if (key == 'a')
	{
		temp -= vec3(0, 0, 0.2);
	}
	else if (key == 'w')
	{
		temp += vec3(0.2, 0, 0);
	}
	else if (key == 's')
	{
		temp -= vec3(0.2, 0, 0);
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case MENU_HELLO:
		// do something
		break;
	case MENU_WORLD:
		// do something
		break;
    case MENU_ROTATE:
        if (!Rotate) Rotate = true;
        break;
    case MENU_WALK:
        if (!Start_walking) Start_walking = true;
        break;
    case MENU_STOPROTATE:
        if (Rotate) Rotate = false;
            break;
    case MENU_STOPWALK:
        if (Start_walking) Start_walking = false;
        break;
	default:
		break;
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
		}
		else if (state == GLUT_UP)
		{
			printf("Mouse %d is released at (%d, %d)\n", button, x, y);
		}
	}
}

void My_newMenu(int id)
{

}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    // TODO :
	// Creating windows here (remeber to set both size & position for it )
	
	// glutInitWindowPostion( ... , ... );
	// glutInitWindowSize( ... , ... );
	// glutCreateWindow( ... ); // You cannot use OpenGL functions before this line; 
    // The OpenGL context must be created first by glutCreateWindow()!
    glutInitWindowPosition(100,100);
    glutInitWindowSize(500,600);
    glutCreateWindow("Baymax");

#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();

    // TODO: Add sub menu as bellow structure
    // New
    // |--Hello
    // |--World

	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
    int menu_animation = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
    glutAddSubMenu("Animation", menu_animation);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);
    
    glutSetMenu(menu_animation);
    glutAddMenuEntry("Rotate", MENU_ROTATE);
    glutAddMenuEntry("Walk", MENU_WALK);
    glutAddMenuEntry("Stop Rotate", MENU_STOPROTATE);
    glutAddMenuEntry("Stop Walking", MENU_STOPWALK);

	// Tell GLUT to design the menu which id==menu_new now
	// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)
	// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);

	// Todo
	// Practice 1 : Register new GLUT event listner here
	// ex. glutXXXXX(my_Func);
	// Remind : you have to implement my_Func (My_Mouse)
    glutMouseFunc(My_Mouse);
	// Enter main event loop.
	glutMainLoop();

	return 0;
}
