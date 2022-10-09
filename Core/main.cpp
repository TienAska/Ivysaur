#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14 
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>
#include <cyCodeBase/cyHairFile.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "logger.h"
#include "camera.h"
#include "shader.h"

struct Light
{
	glm::vec3 direction;
	float padding0 = 0.0f;
	glm::vec3 color;
	float padding1 = 0.0f;
};

struct MatrixUBO
{
	glm::mat4 VP;
	glm::mat4 M;
	glm::vec3 CameraPos;
	float padding = 0.0f;
};

struct Vertex_Data
{
	float position[3];
	float normal[3];
	float texcoord[2];
};

struct Meshlet
{
	unsigned int vertex_offset = 0;
	unsigned int vertex_count  = 0;
	unsigned int index_offset  = 0;
	unsigned int index_count   = 0;
};

std::vector<Meshlet> BuildMeshlets(const cyHairFile& hairfile)
{
	std::vector<Meshlet> meshlets;
	
	int pointIndex = 0;
	int hairCount = hairfile.GetHeader().hair_count;
	const unsigned short* segments = hairfile.GetSegmentsArray();
	for(size_t i = 0; i < hairCount; ++i)
	{
		Meshlet meshlet;
		meshlet.vertex_offset = pointIndex;
		meshlet.vertex_count = segments[i] + 1;
		//meshlet.index_offset = i;
		//meshlet.index_count++;
		meshlets.push_back(meshlet);
		pointIndex += meshlet.vertex_count;
	}

	return meshlets;
}

void LoadHairModel(const char* filename, cyHairFile& hairfile, float*& dirs)
{
	// Load the hair model
	int result = hairfile.LoadFromFile(filename);
	// Check for errors
	switch (result) {
	case CY_HAIR_FILE_ERROR_CANT_OPEN_FILE:
		LOG_RUNTIME_WARN("Cannot open hair file!");
		return;
	case CY_HAIR_FILE_ERROR_CANT_READ_HEADER:
		LOG_RUNTIME_WARN("Cannot read hair file header!");
		return;
	case CY_HAIR_FILE_ERROR_WRONG_SIGNATURE:
		LOG_RUNTIME_WARN("File has wrong signature!");
		return;
	case CY_HAIR_FILE_ERROR_READING_SEGMENTS:
		LOG_RUNTIME_WARN("Cannot read hair segments!");
		return;
	case CY_HAIR_FILE_ERROR_READING_POINTS:
		LOG_RUNTIME_WARN("Cannot read hair points!");
		return;
	case CY_HAIR_FILE_ERROR_READING_COLORS:
		LOG_RUNTIME_WARN("Cannot read hair colors!");
		return;
	case CY_HAIR_FILE_ERROR_READING_THICKNESS:
		LOG_RUNTIME_WARN("Cannot read hair thickness!");
		return;
	case CY_HAIR_FILE_ERROR_READING_TRANSPARENCY:
		LOG_RUNTIME_WARN("Cannot read hair transparency!");
		return;
	default:
		LOG_RUNTIME_INFO("Hair file \"{}\" loaded.", filename);
	}
	int hairCount = hairfile.GetHeader().hair_count;
	int pointCount = hairfile.GetHeader().point_count;
	LOG_RUNTIME_INFO("Number of hair strands = {}", hairCount);
	LOG_RUNTIME_INFO("Number of hair points = {}", pointCount);
	// Compute directions
	if (hairfile.FillDirectionArray(dirs) == 0) {
		LOG_RUNTIME_WARN("Cannot compute hair directions!");
	}
}

void APIENTRY gldebugmessage_callback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::string src;
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             src = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   src = "Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: src = "Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     src = "Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     src = "Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           src = "Other"; break;
	}

	std::string ty;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               ty = "Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ty = "Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ty = "Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         ty = "Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         ty = "Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              ty = "Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          ty = "Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           ty = "Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               ty = "Other"; break;
	}

	std::string svr;
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         svr = "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       svr = "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          svr = "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: svr = "Severity: notification"; break;
	}

	LOG_OPENGL_ERROR("Debug Message ({}):{}\nSource: {}\nType: {}\nSeverity: {}\n", id, message, src, ty, svr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
		Camera::Instance().MoveForward(0.1f);
	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
		Camera::Instance().MoveForward(-0.1f);
	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
		Camera::Instance().MoveRight(-0.1f);
	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
		Camera::Instance().MoveRight(0.1f);
	if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT))
		Camera::Instance().MoveUp(0.1f);
	if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT))
		Camera::Instance().MoveUp(-0.1f);
}

glm::quat rotateY(1.0f, 0.0f, 0.0f, 0.0f);
bool cameraRotate = false;
bool modelRotate = false;
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	static float lastX = 720, lastY = 480;

	float xPos = static_cast<float>(xpos);
	float yPos = static_cast<float>(ypos);
	float deltaX = xPos - lastX;
	float deltaY = lastY - yPos;
	lastX = xPos;
	lastY = yPos;

	if (cameraRotate)
	{
		Camera::Instance().Rotate(deltaY, deltaX);
	}

	if (modelRotate)
	{
		rotateY = glm::rotate(rotateY, glm::radians(deltaX), glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
		cameraRotate = action != GLFW_RELEASE;
	if (button == GLFW_MOUSE_BUTTON_LEFT && !(mods ^ GLFW_MOD_SHIFT))
		modelRotate = action != GLFW_RELEASE;
}

GLuint CreateCubeMap(std::filesystem::path path)
{
	GLuint textureID;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);

	static const char* const filenames[] = {
		"px.png",
		"nx.png",
		"py.png",
		"ny.png",
		"pz.png",
		"nz.png"
	};

	glTextureStorage2D(textureID, 1, GL_RGB8, 2048, 2048);
	int width, height, nrChannels;
	for (int i = 0; i < 6; ++i)
	{
		unsigned char* data = stbi_load((path / filenames[i]).string().c_str(), &width, &height, &nrChannels, 0);
		if (data)
			glTextureSubImage3D(textureID, 0, 0, 0, i, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
		else
			LOG_RUNTIME_WARN("Cubemap tex failed to load at path: {}", path.string());
		stbi_image_free(data);
	}
	glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

int main(int argc, char* argv[])
{
	Logger::Init();
	
	// Init GLFW3.
	if (!glfwInit()) 
	{
		LOG_RUNTIME_ERROR("failed to initialize GLFW.");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 8);

	auto window = glfwCreateWindow(1920, 1200, "Khuon", nullptr, nullptr);
	if (!window) 
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwMakeContextCurrent(window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	// Init gl loader.
	if (gl3wInit()) 
	{
		LOG_RUNTIME_ERROR("failed to initialize OpenGL");
		return -1;
	}

	LOG_RUNTIME_INFO("OpenGL {0}, GLSL {1}", reinterpret_cast<const char*>(glGetString(GL_VERSION)), reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

	GLint max_vertices, max_primitives;
	glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &max_vertices);
	glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &max_primitives);
	LOG_RUNTIME_INFO("Max mesh output vertices: {0}, primitives {1}", max_vertices, max_primitives);

	// Setup debug logger.
	int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(gldebugmessage_callback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		//glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
	}

	Shader::GenerateCaches();

	cyHairFile hair = cyHairFile();
	float* dirs = nullptr;
	LoadHairModel("Assets/Models/wWavyThin.hair", hair, dirs);
	std::vector<Meshlet> meshlets = BuildMeshlets(hair);

	// Shader compilation.
	GLuint skybox_program = Shader::CreateProgram("skybox.mesh", "skybox.frag");
	GLuint hair_program = Shader::CreateProgram("hair.mesh", "hair.frag");

	Shader cube_mesh("cube.mesh"), base_frag("base.frag");
	GLuint cube_program = Shader::CreateProgram(cube_mesh, base_frag);


	GLuint UBOs[2];	glCreateBuffers(2, UBOs);
	glBindBuffersBase(GL_UNIFORM_BUFFER, 0, 2, UBOs);
	glNamedBufferData(UBOs[0], sizeof(MatrixUBO), nullptr, GL_STATIC_DRAW);
	glNamedBufferData(UBOs[1], sizeof(Light), nullptr, GL_STATIC_DRAW);


	glProgramUniform4f(hair_program, 0, hair.GetHeader().d_color[0], hair.GetHeader().d_color[1], hair.GetHeader().d_color[2], hair.GetHeader().d_transparency);

	GLuint SSBOs[2]; glCreateBuffers(2, SSBOs);
	glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 2, SSBOs);
	// vertices
	glNamedBufferStorage(SSBOs[0], sizeof(float) * 3 * hair.GetHeader().point_count, hair.GetPointsArray(), GL_NONE);
	// meshlets
	glNamedBufferStorage(SSBOs[1], sizeof(Meshlet) * meshlets.size(), meshlets.data(), GL_NONE);

	GLuint skyboxTexture = CreateCubeMap("Assets/Textures/Clarens Night 02/");
	glBindTextureUnit(0, skyboxTexture);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// Set to line mode.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glLineWidth(2);

	// Set modern original coordinate and fit NDC z to [0, 1].
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	
	GLfloat clearColor[] = { 0.32f, 0.51f, 0.39f, 1.0f };
	GLfloat* clearDepth = new GLfloat(1.0f);
	
	MatrixUBO ubo;
	Light sun;

	// Render loop.
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearNamedFramebufferfv(0, GL_COLOR, 0, clearColor);
		glClearNamedFramebufferfv(0, GL_DEPTH, 0, clearDepth);

		// Send cubemap matrix.
		ubo = { Camera::Instance().GetProjectionMatrix(), Camera::Instance().GetRotationMatrix() };
		glNamedBufferSubData(UBOs[0], 0, sizeof(MatrixUBO), &ubo);

		// Draw skybox.
		glDepthFunc(GL_LEQUAL);
		glUseProgram(skybox_program);
		glDrawMeshTasksNV(0, 1);
		glDepthFunc(GL_LESS);

		// Uniform Buffer
		ubo.VP = Camera::Instance().GetViewProjection();
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		ubo.M = model;
		ubo.CameraPos = Camera::Instance().GetPosition();
		glNamedBufferSubData(UBOs[0], 0, sizeof(MatrixUBO), &ubo);
		
		glUseProgram(cube_program);
		glDrawMeshTasksNV(0, 1);


		// Send hair matrix.
		model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = toMat4(rotateY) * model;
		ubo.M = model;
		glNamedBufferSubData(UBOs[0], 0, sizeof(MatrixUBO), &ubo);

		sun.direction = glm::vec3(1.0f, 1.0f, 1.0f);
		sun.color = glm::vec3(0.1f, 0.3f, 2.0f) * 3.0f;
		glNamedBufferSubData(UBOs[1], 0, sizeof(Light), &sun);

		// Draw Hair.
		glUseProgram(hair_program);
		glDrawMeshTasksNV(0, hair.GetHeader().hair_count);


		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Hello, world!");
		if (ImGui::Button("Refresh Shader"))
		{
			Shader::UpdateProgram(cube_program, cube_mesh, base_frag);
		}
		ImGui::End();

		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Clean up.
	glDeleteBuffers(2, UBOs);
	glDeleteBuffers(2, SSBOs);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}