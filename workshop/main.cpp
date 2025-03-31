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
// Load shader programs compiled to SPIR-V binary format from given files into a new shader program
// Returns 0 on error
GLuint loadShaderSpirV(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord0;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

void loadMeshesFromAiNode(aiNode *node, const aiScene *scene, std::vector<Mesh>& outMeshes);

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
  aiProcess_Triangulate            |
  aiProcess_GenNormals             |
  aiProcess_CalcTangentSpace       |
  aiProcess_JoinIdenticalVertices  |
  aiProcess_SortByPType);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    std::println("Error loading model file: {}", importer.GetErrorString());
    return 1;
  }
  for (uint32_t meshIx = 0; meshIx < scene->mNumMeshes; ++meshIx) {
    const aiMesh* mesh = scene->mMeshes[meshIx];
    std::println("Mesh {}: {} vertices, {} faces.", meshIx, mesh->mNumVertices, mesh->mNumFaces);
  }
  std::vector<Mesh> meshes;
  loadMeshesFromAiNode(scene->mRootNode, scene, meshes);
  
  std::println("loading a texture");
  const std::filesystem::path texFile{"C:/Users/veliu/repos/graphics-workshop/assets/textures/openimageio-acronym-gradient.png"};
  auto inp = OIIO::ImageInput::open(texFile.string());
  if(!inp) {
    std::println("Error loading texture file: {}", OIIO::geterror());
    return 1;
  }
  const OIIO::ImageSpec &spec = inp->spec();
  std::println("Image: width {}, height {}, depth {}, channels {}", spec.width, spec.height, spec.depth, spec.nchannels);

   
  const std::filesystem::path vertFile{"C:/Users/veliu/repos/graphics-workshop/assets/shaders/triangle_without_vbo_vert.spv"};
  const std::filesystem::path fragFile{"C:/Users/veliu/repos/graphics-workshop/assets/shaders/triangle_without_vbo_frag.spv"};
  GLuint program = loadShaderSpirV(vertFile, fragFile);
  if (program == 0) {
    std::println("Error loading shaders.");
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

bool readBinaryFile(const std::filesystem::path& path, std::vector<std::byte>& outBuffer) {
  if (!std::filesystem::exists(path)) {
    std::println("File does not exist: {}", path.string());
    return false;
  }    
  const auto fileSize = std::filesystem::file_size(path);
  if (fileSize == 0) {
    std::println("File is empty: {}", path.string());
    return false;
  }    
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::println("Error opening shader file: {}", path.string());
    return false;
  }

  file.seekg(0, std::ios::beg);
  outBuffer.resize(fileSize);
  file.read(reinterpret_cast<char*>(outBuffer.data()), fileSize);
  file.close(); 
  
  return true;
}

GLuint loadShaderSpirV(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
  int32_t success{};
  const uint32_t infoLogSize = 512;
  char infoLog[512];

  std::vector<std::byte> vertBuffer;
  if(!readBinaryFile(vertPath, vertBuffer)) {
    return 0;
  }
  GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderBinary(1, &vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertBuffer.data(), static_cast<GLsizei>(vertBuffer.size()));
  glSpecializeShader(vertShader, "main", 0, nullptr, nullptr);
  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertShader, infoLogSize, NULL, infoLog);
    std::println(std::cerr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n{}", infoLog);
    glDeleteShader(vertShader);
    return 0;
  }

  std::vector<std::byte> fragBuffer;
  if(!readBinaryFile(fragPath, fragBuffer)) {
    glDeleteShader(vertShader);
    return 0;
  }
  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderBinary(1, &fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragBuffer.data(), static_cast<GLsizei>(fragBuffer.size()));
  glSpecializeShader(fragShader, "main", 0, nullptr, nullptr);    
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragShader, infoLogSize, NULL, infoLog);
    std::println(std::cerr, "ERROR::SHADER::FRAG::COMPILATION_FAILED\n{}", infoLog);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return 0;
  }

  const GLuint program = glCreateProgram();
  glAttachShader(program, vertShader);
  glAttachShader(program, fragShader);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, infoLogSize, NULL, infoLog);
    std::println(std::cerr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n{}", infoLog);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteProgram(program);
    return 0; 
  }

  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
  return program;
}

Mesh processMesh(const aiMesh *mesh, const aiScene *scene) {
  Mesh outMesh;
  outMesh.vertices.reserve(mesh->mNumVertices);
  for (uint32_t vertIx = 0; vertIx < mesh->mNumVertices; ++vertIx) {
    Vertex& vertex = outMesh.vertices.emplace_back();
    if (mesh->HasPositions()) {
      vertex.position = {mesh->mVertices[vertIx].x, mesh->mVertices[vertIx].y, mesh->mVertices[vertIx].z};
    } else {
      vertex.position = {};
    }
    if (mesh->HasNormals()) {
      vertex.normal = {mesh->mNormals[vertIx].x, mesh->mNormals[vertIx].y, mesh->mNormals[vertIx].z};
    }
    else {
      vertex.normal = {};
    }
    if (mesh->mTextureCoords[0]) {
      vertex.texCoord0 = {mesh->mTextureCoords[0][vertIx].x, mesh->mTextureCoords[0][vertIx].y};
    } else {
      vertex.texCoord0 = {};
    }
  }
  outMesh.indices.reserve(mesh->mNumFaces * 3);
  for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
    const aiFace& face = mesh->mFaces[i];
    assert(face.mNumIndices == 3);
    for (uint32_t j = 0; j < face.mNumIndices; ++j) {
      outMesh.indices.push_back(face.mIndices[j]);
    }
  }

  aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
  std::println("Processing mesh '{}'. has position? {}, has normals? {}, has TexCoord0? {}, uv channels {}. Material '{}'", mesh->mName.C_Str(), mesh->HasPositions(), mesh->HasNormals(), mesh->HasTextureCoords(0), mesh->GetNumUVChannels(), material->GetName().C_Str());
  return outMesh;
}

void loadMeshesFromAiNode(aiNode *node, const aiScene *scene, std::vector<Mesh>& outMeshes) {
  for(unsigned int i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
      outMeshes.push_back(processMesh(mesh, scene));			
  }
  for(unsigned int i = 0; i < node->mNumChildren; i++) {
    loadMeshesFromAiNode(node->mChildren[i], scene, outMeshes);
  }
}