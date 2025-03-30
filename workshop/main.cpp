#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <OpenImageIO/imageio.h>

#include <filesystem>
#include <print>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);

int main() {
  std::println("Hi!");

  if (!glfwInit()) {
    std::println("Failed to initialize GLFW");
    return -1;
  }

  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 8);

  const GLuint kWidth = 1280, kHeight = 768;
  GLFWwindow* window =
      glfwCreateWindow(kWidth, kHeight, "LearnOpenGL", NULL, NULL);
  if (!window) {
    std::println("Failed to create GLFW window");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, keyCallback);

  int version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    std::println("Failed to initialize OpenGL context via glad.");
    return -1;
  }
  std::println("Loaded OpenGL version {}.{}", GLAD_VERSION_MAJOR(version),
               GLAD_VERSION_MINOR(version));

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  ImGui::StyleColorsDark();
  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  const char* kGlslVersion = "#version 460";
  ImGui_ImplOpenGL3_Init(kGlslVersion);

  glm::vec3 origin{};
  std::println("Origin: ({}, {}, {})", origin.x, origin.y, origin.z);

  const std::filesystem::path modelFile{"C:/Users/veliu/repos/graphics-workshop/assets/models/teapot/teapot.obj"};
  std::println("Loading model file: {}...", modelFile.string());
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(modelFile.string(),
  aiProcess_CalcTangentSpace       |
  aiProcess_Triangulate            |
  aiProcess_JoinIdenticalVertices  |
  aiProcess_SortByPType);
  if (!scene) {
    std::println("Error loading model file: {}", importer.GetErrorString());
    return 1;
  }
  for (uint32_t meshIx = 0; meshIx < scene->mNumMeshes; ++meshIx) {
    const aiMesh* mesh = scene->mMeshes[meshIx];
    std::println("Mesh {}: {} vertices, {} faces.", meshIx, mesh->mNumVertices, mesh->mNumFaces);
  }
  
  std::println("loading a texture");
  const std::filesystem::path texFile{"C:/Users/veliu/repos/graphics-workshop/assets/textures/openimageio-acronym-gradient.png"};
  auto inp = OIIO::ImageInput::open(texFile.string());
  if(!inp) {
    std::println("Error loading texture file: {}", OIIO::geterror());
    return 1;
  }
  const OIIO::ImageSpec &spec = inp->spec();
  std::println("Image: width {}, height {}, depth {}, channels {}", spec.width, spec.height, spec.depth, spec.nchannels);

  GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
  {
    const std::filesystem::path vertFile{"C:/Users/veliu/repos/graphics-workshop/assets/shaders/triangle_without_vbo_vert.spv"};
    if (!std::filesystem::exists(vertFile)) {
      std::println("File does not exist: {}", vertFile.string());
      return 1;
    }    
    const auto fileSize = std::filesystem::file_size(vertFile);
    if (fileSize == 0) {
      std::println("File is empty: {}", vertFile.string());
      return 1;
    }    
    std::ifstream file(vertFile, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::println("Error opening shader file: {}", vertFile.string());
      return 1;
    }
    file.seekg(0, std::ios::beg);
    std::vector<std::byte> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();  

    glShaderBinary(1, &vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, buffer.data(), static_cast<GLsizei>(buffer.size()));
    glSpecializeShader(vertShader, "main", 0, nullptr, nullptr);
    int32_t success{};
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[512];
      glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
      std::println(std::cerr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n{}", infoLog);
      return 1;
    }    
  }
  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  {
    const std::filesystem::path fragFile{"C:/Users/veliu/repos/graphics-workshop/assets/shaders/triangle_without_vbo_frag.spv"};
    std::ifstream file(fragFile, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::println("Error opening shader file: {}", fragFile.string());
      return 1;
    }
    const auto fileSize = std::filesystem::file_size(fragFile);
    std::vector<std::byte> buffer(fileSize);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();  

    glShaderBinary(1, &fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, buffer.data(), static_cast<GLsizei>(buffer.size()));
    glSpecializeShader(fragShader, "main", 0, nullptr, nullptr);    
  }
  const GLuint program = glCreateProgram();
  glAttachShader(program, vertShader);
  glAttachShader(program, fragShader);
  glLinkProgram(program);
  int32_t success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    if (!success) {
      char infoLog[512];
      glGetProgramInfoLog(program, 512, NULL, infoLog);
      std::println(std::cerr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n{}", infoLog);
      return success;
    }

    return 1;
  }

  uint32_t vertexArray;
  glCreateVertexArrays(1, &vertexArray);

  glViewport(0, 0, kWidth, kHeight);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    ImGui::ShowDemoWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow* backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();

  std::println("Bye!");
  return 0;
}

void keyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mode) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}