#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "scout.h"

#define error(...) {fprintf(stderr, __VA_ARGS__); abort();}

#define UNUSED(x) (void)(x)

#define VERTSHDSRC_COMMON "" \
	"uniform float screenRatio;" \
	"vec2 nodePosNDC(vec2 p)" \
	"{" \
	"	return (p * 2 - vec2(1.0)) * vec2(1.0, -1.0);" \
	"}" \
	"vec2 scalePercent(vec2 p, float perc)" \
	"{" \
	"	return p * vec2(1.0, screenRatio) / 100 * perc;" \
	"}"

#define VERTSHDSRC_NODES "#version 330 core\n" \
	"layout(location = 0) in vec2 vertexCoord;" \
	"uniform vec2 nodePos;" \
	"uniform bool nodeSelected;" \
	VERTSHDSRC_COMMON \
	"void main()" \
	"{" \
	"	vec2 pos = nodePosNDC(nodePos) + scalePercent(vertexCoord, nodeSelected ? 7.5 : 5.0);" \
	"	gl_Position = vec4(pos, 0.0, 1.0);" \
	"}"


#define FRAGSHDSRC_NODES "#version 330 core\n" \
	"uniform bool nodeSelected;" \
	"void main()" \
	"{" \
	"	gl_FragColor = vec4(nodeSelected ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0), 1.0);" \
	"}"
	
#define VERTSHDSRC_WAYS "#version 330 core\n" \
	"layout(location = 0) in vec2 vertexCoord;" \
	"out float progress;" \
	"uniform vec2 wayPos;" \
	"uniform float wayLength;" \
	"uniform vec2 way;" \
	VERTSHDSRC_COMMON \
	"void main()" \
	"{" \
	"	progress = (vertexCoord.x + 0.5) * wayLength;" \
	"	vec2 w = normalize(way);" \
	"	vec2 pos = nodePosNDC(wayPos) + mat2(w.x, -w.y, w.y, w.x) * ((vertexCoord * vec2(wayLength * 2, 0.0125)) * vec2(1.0, screenRatio));" \
	"	gl_Position = vec4(pos, 0.0, 1.0);" \
	"}"
	
#define FRAGSHDSRC_WAYS "#version 330 core\n" \
	"in float progress;" \
	"uniform float wayProgress;" \
	"uniform float wayLength;" \
	"void main()" \
	"{" \
	"	vec3 color = vec3(1.0, 0.0, 0.0);" \
	"	if (wayProgress > 0.0 && wayProgress >= progress || wayProgress < 0.0 && (wayLength + wayProgress) <= progress)" \
	"		color = vec3(0.0, 0.0, 1.0);" \
	"	gl_FragColor = vec4(color, 1.0);" \
	"}"

struct {
	float width, height;
} screenBounds, screenBoundsNormalized;

struct {
	float x, y;
} cursorPos;

struct Node {
	float x, y;
	scnode *scnod;
	bool selected;
	struct Node *next;
};

struct Way {
	float posx, posy;
	float vecx, vecy;
	bool done;
	bool active;
	scway *scway_1;
	scway *scway_2;
	struct Way *next;
};

typedef struct Node Node;
typedef struct Way Way;

Node *nodelist = NULL;
Way *waylist = NULL;

Node *selected_node = NULL;

scwaypoint *currentwp = NULL;
float currentprog = 0.0;
bool unselect = false;

GLuint screenRatio_nodes_loc, nodePos_loc, nodeSelected_loc, screenRatio_ways_loc, wayPos_loc, wayLength_loc, way_loc, wayProgress_loc;
GLuint nodes_shaders, ways_shaders;

Node *createNode(float x, float y)
{
	Node *node = malloc(sizeof(Node));
	node->x = x;
	node->y = y;
	node->selected = false;
	node->scnod = scnodalloc(node);
	node->next = NULL;
	for (Node *nptr = nodelist; nptr != NULL; nptr = nptr->next) {
		if (nptr->next == NULL) {
			return nptr->next = node;
		}
	}
	return nodelist = node;
}

Way *connectNodes(Node *from, Node *to)
{
	if (scisconnected(from->scnod, to->scnod))
		return NULL;
	Way *way = malloc(sizeof(Way));
	way->posx = (from->x + to->x) / 2;
	way->posy = (from->y + to->y) / 2;
	way->vecx = to->x - from->x;
	way->vecy = to->y - from->y;
	way->active = false;
	way->done = false;
	float len = sqrt(way->vecx * way->vecx + way->vecy * way->vecy);
	way->scway_1 = scaddway(from->scnod, to->scnod, len, way);
	way->scway_2 = scaddway(to->scnod, from->scnod, len, way);
	way->next = NULL;
	for (Way *wptr = waylist; wptr != NULL; wptr = wptr->next) {
		if (wptr->next == NULL) {
			return wptr->next = way;
		}
	}
	return waylist = way;
}

Node *getNodeAtPos(float x, float y)
{
	Node *node = NULL;
	float distance;
	for (Node *nptr = nodelist; nptr != NULL; nptr = nptr->next) {
		float dx, dy;
		dx = (nptr->x - x) * screenBoundsNormalized.width;
		dy = (nptr->y - y) * screenBoundsNormalized.height;
		float d = sqrt(dx * dx + dy * dy);
		if (d > (nptr->selected ? 7.5f : 5.0f) / 100.0f / 4.0f)
			continue;
		if (node == NULL || d < distance) {
			node = nptr;
			distance = d;
		}
	}
	return node;
}

Way *getActiveWay()
{
	return currentwp != NULL && currentwp->way != NULL ? (Way *)currentwp->way->dat : NULL;
}

Node *getActiveNode()
{
	return currentwp != NULL ? (Node *)currentwp->nod->dat : NULL;
}

void findPathEnd()
{
	unselect = true;
	scdestroypath(currentwp);
	currentwp = NULL;
}

void findPathStep(double dtime)
{
	if (currentwp == NULL)
		return;
	Node *active_node = getActiveNode();
	active_node->selected = true;
	Way *active_way = getActiveWay();
	if (! active_way) {
		findPathEnd();
		return;
	}
	Node *to = (Node *)active_way->scway_1->lto->dat;
	float len = active_way->scway_1->len;
	float fac = 1.0;
	if (active_node == to)
		fac = -1.0;
	currentprog += dtime * fac * 0.25;
	if (currentprog * fac > len) {
		active_way->done = true;
		currentwp = currentwp->nxt;			
		currentprog = 0.0;
	}
}

void findPathStart(Node *from, Node *to)
{
	currentwp = scout(from->scnod, to->scnod, NULL);
	if (currentwp == NULL)
		findPathEnd();
	currentprog = 0.0;
}

GLuint createShaderProgram(const char *vsrc, const char *fsrc)
{
	GLuint id, vsh, fsh;
	int success;
	char buffer[1024] = {0};
	
	id = glCreateProgram();
	
	{
		vsh = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vsh, 1, &vsrc, NULL);
		glCompileShader(vsh);
		glGetShaderiv(vsh, GL_COMPILE_STATUS, &success);
		if (! success) {
			glGetShaderInfoLog(vsh, 1024, NULL, buffer);
			error("Failed to compile vertex shader: %s\n", buffer);
		}
		glAttachShader(id, vsh);
	}
	
	{
		fsh = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fsh, 1, &fsrc, NULL);
		glCompileShader(fsh);
		glGetShaderiv(fsh, GL_COMPILE_STATUS, &success);
		if (! success) {
			glGetShaderInfoLog(fsh, 1024, NULL, buffer);
			error("Failed to compile fragment shader: %s\n", buffer);
		}
		glAttachShader(id, fsh);
	}
	
	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (! success) {
		glGetProgramInfoLog(id, 1024, NULL, buffer);
		error("Failed to link shader program: %s\n", buffer);
	}
	
	glDeleteShader(vsh);
	glDeleteShader(fsh);
	
	return id;
}

GLuint makeMesh(const GLfloat *vertices, GLsizei vertices_count, const GLuint *indices, GLsizei indices_count)
{
	GLuint VAO, VBO, EBO;
	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	
	glBindVertexArray(VAO);	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	
	glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(GLuint), indices, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	return VAO;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	UNUSED(window);
	glViewport(0, 0, width, height);
	screenBounds.width = width;
	screenBounds.height = height;
	float len = sqrt(width * width + height * height);
	screenBoundsNormalized.width = (float)width / len;
	screenBoundsNormalized.height = (float)height / len;
	glProgramUniform1f(nodes_shaders, screenRatio_nodes_loc, (float)width / (float)height);
	glProgramUniform1f(ways_shaders, screenRatio_ways_loc, (float)width / (float)height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	UNUSED(window);
	UNUSED(mods);
	if (action != GLFW_RELEASE || currentwp != NULL)
		return;
	if (unselect) {
		for (Way *wptr = waylist; wptr != NULL; wptr = wptr->next)
			wptr->done = false;
		for (Node *nptr = nodelist; nptr != NULL; nptr = nptr->next)
			nptr->selected = false;
		unselect = false;
	}
	float x = cursorPos.x / screenBounds.width;
	float y = cursorPos.y / screenBounds.height;
	Node *nod = getNodeAtPos(x, y);
	if (nod) {
		if (selected_node) {
			switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				connectNodes(selected_node, nod);
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				findPathStart(selected_node, nod);
				break;
			}
			selected_node->selected = false;
			selected_node = NULL;
		}
		else {
			selected_node = nod;
			selected_node->selected = true;
		}
	} else {
		createNode(x, y);
	}
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	UNUSED(window);
	cursorPos.x = x;
	cursorPos.y = y;
}

int main()
{
	if (! glfwInit())
		error("Failed to initialize GLFW\n");
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	GLFWwindow *window = glfwCreateWindow(10, 10, "libscout Example", NULL, NULL);
	if (! window) {
		glfwTerminate();
		error("Failed to create GLFW window\n");
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);
	glfwSetCursorPosCallback(window, &cursor_pos_callback);
	glfwSetMouseButtonCallback(window, &mouse_button_callback);
	glfwSetWindowSize(window, 750, 500);
	
	GLenum glew_init_err = glewInit();
	if (glew_init_err != GLEW_OK)
		error("Failed to initalize GLEW\n");
	
	float degtorad = M_PI / 180.0f;
	
	GLfloat circle_vertices[361][2];
	GLuint circle_indices[360][3];
	
	circle_vertices[360][0] = 0;
	circle_vertices[360][1] = 0;
	
	for (int deg = 0; deg < 360; deg++) {
		float rad = (float)deg * degtorad;
		circle_vertices[deg][0] = cos(rad) * 0.5;
		circle_vertices[deg][1] = sin(rad) * 0.5;
		
		int nextdeg = deg + 1;
		
		circle_indices[deg][0] = 360;
		circle_indices[deg][1] = deg;
		circle_indices[deg][2] = nextdeg < 360 ? nextdeg : 0;
	}
	
	GLsizei circle_vertices_count = 361 * 2;
	
	GLsizei circle_indices_count = 360 * 3;
	
	GLfloat square_vertices[4][2] = {
		{+0.5, +0.5},
		{+0.5, -0.5},
		{-0.5, -0.5},
		{-0.5, +0.5},
	};
	
	GLsizei square_vertices_count = 4 * 2;
	
	GLuint square_indices[2][3] = {
		{0, 1, 3},
		{1, 2, 3}, 
	};
	
	GLsizei square_indices_count = 2 * 3;
	
	GLuint nodes_VAO, ways_VAO;
	
	nodes_VAO = makeMesh(circle_vertices[0], circle_vertices_count, circle_indices[0], circle_indices_count);
	ways_VAO = makeMesh(square_vertices[0], square_vertices_count, square_indices[0], square_indices_count);
	
	nodes_shaders = createShaderProgram(VERTSHDSRC_NODES, FRAGSHDSRC_NODES);
	ways_shaders = createShaderProgram(VERTSHDSRC_WAYS, FRAGSHDSRC_WAYS);
	
	screenRatio_nodes_loc = glGetUniformLocation(nodes_shaders, "screenRatio");
	nodePos_loc = glGetUniformLocation(nodes_shaders, "nodePos");
	nodeSelected_loc = glGetUniformLocation(nodes_shaders, "nodeSelected");
	
	screenRatio_ways_loc = glGetUniformLocation(ways_shaders, "screenRatio");
	wayPos_loc = glGetUniformLocation(ways_shaders, "wayPos");
	wayLength_loc = glGetUniformLocation(ways_shaders, "wayLength");
	way_loc = glGetUniformLocation(ways_shaders, "way");
	wayProgress_loc = glGetUniformLocation(ways_shaders, "wayProgress");
	
	double last_time = glfwGetTime();
	
	while (! glfwWindowShouldClose(window)) {
		double dtime = glfwGetTime() - last_time;
		last_time = glfwGetTime();
		
		findPathStep(dtime);

		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT); 

		glUseProgram(ways_shaders);
		glBindVertexArray(ways_VAO);
		Way *active_way = getActiveWay();
		for (Way *way = waylist; way != NULL; way = way->next) {
			float progress = 0.0;
			float len = way->scway_1->len;
			if (way == active_way)
				progress = currentprog;
			else if (way->done)
				progress = len;
			glUniform1f(wayProgress_loc, progress);
			glUniform1f(wayLength_loc, len);
			glUniform2f(wayPos_loc, way->posx, way->posy);
			glUniform2f(way_loc, way->vecx, way->vecy);
			glDrawElements(GL_TRIANGLES, square_indices_count, GL_UNSIGNED_INT, 0);
		}
		
		glUseProgram(nodes_shaders);
		glBindVertexArray(nodes_VAO);
		for (Node *node = nodelist; node != NULL; node = node->next) {
			glUniform2f(nodePos_loc, node->x, node->y);
			glUniform1i(nodeSelected_loc, node->selected);
			glDrawElements(GL_TRIANGLES, circle_indices_count, GL_UNSIGNED_INT, 0);
		}
		
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
