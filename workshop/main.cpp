#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#define GLM_FORCE_COLUMN_MAJOR
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <OpenImageIO/imageio.h>

#include <filesystem>
#include <print>
#include <ranges>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
// Load shader programs compiled to SPIR-V binary format from given files into a new shader program
// Returns 0 on error
GLuint loadShaderSpirV(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord1;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

struct MeshGpu {
  GLuint vertexArray;
  GLuint vertexBuffer;
  GLuint indexBuffer;
  size_t numVertices;
  size_t numIndices;
};

GLuint createVertexBuffer(uint32_t numVertices, GLuint vao) {
  GLuint vbo;
  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, sizeof(Vertex) * numVertices, nullptr, GL_DYNAMIC_STORAGE_BIT);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
  return vbo;
};

GLuint createIndexBuffer(uint32_t numIndices, GLuint vao) {
  GLuint ibo;
  glCreateBuffers(1, &ibo);
  glNamedBufferStorage(ibo, sizeof(uint32_t) * numIndices, nullptr, GL_DYNAMIC_STORAGE_BIT);
  glVertexArrayElementBuffer(vao, ibo);
  return ibo;
}

MeshGpu createMeshGpu(const Mesh& mesh) {
  MeshGpu m{};
  glCreateVertexArrays(1, &m.vertexArray);
  glBindVertexArray(m.vertexArray);

  // See Vertex. {position, normal, texCoord1}
  static const std::vector<int32_t> sizes = {3, 3, 2};
  uint32_t offset = 0;
  for (uint32_t ix = 0; ix < sizes.size(); ++ix) {
    glEnableVertexArrayAttrib(m.vertexArray, ix);
    glVertexArrayAttribFormat(m.vertexArray, ix, sizes[ix], GL_FLOAT, GL_FALSE, offset);
    glVertexArrayAttribBinding(m.vertexArray, ix, 0);
    offset += sizes[ix] * sizeof(float);
  }
  
  m.numVertices = mesh.vertices.size();
  m.vertexBuffer = createVertexBuffer(static_cast<uint32_t>(m.numVertices), m.vertexArray);

  m.numIndices = mesh.indices.size(); 
  m.indexBuffer = createIndexBuffer(static_cast<uint32_t>(m.numIndices), m.vertexArray);

  glNamedBufferSubData(m.vertexBuffer, 0, sizeof(Vertex) * m.numVertices, mesh.vertices.data());
  glNamedBufferSubData(m.indexBuffer, 0, sizeof(uint32_t) * m.numIndices, mesh.indices.data());

  return m;
}

void loadMeshesFromAiNode(aiNode *node, const aiScene *scene, std::vector<Mesh>& outMeshes);

template<typename T>
struct UniformBuffer {
  GLuint ubo;
  T* data;
  uint32_t count;
};

template<typename T>
UniformBuffer<T> createPersistentUniformBuffer(uint32_t binding, uint32_t count = 1) {
  const size_t sizeBytes = sizeof(T) * count;
  GLuint ubo;
  glCreateBuffers(1, &ubo);
  const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
  glNamedBufferStorage(ubo, sizeBytes, nullptr, flags);
  glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);

  void* mappedPtr = static_cast<T*>(glMapNamedBufferRange(ubo, 0, sizeBytes, flags));
  if (mappedPtr == nullptr) {
    std::println("Failed to map uniform buffer {} of size {} persistently!", ubo, sizeBytes);
    glDeleteBuffers(1, &ubo);
    return UniformBuffer<T>{};
  }

  UniformBuffer<T> ub{ubo, static_cast<T*>(mappedPtr), count};

  return ub;
}

struct PerFrameData {
  glm::mat4 viewFromWorld;
  glm::mat4 projectionFromView;
  float fishEyeStrength{0.25f};
};

struct PerObjectData {
  glm::mat4 worldFromModel;
};

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
  std::vector<MeshGpu> meshGpus;
  for (const auto& mesh : meshes)
    meshGpus.push_back(createMeshGpu(mesh));
  
  std::println("loading a texture");
  const std::filesystem::path texFile{"C:/Users/veliu/repos/graphics-workshop/assets/textures/openimageio-acronym-gradient.png"};
  auto inp = OIIO::ImageInput::open(texFile.string());
  if(!inp) {
    std::println("Error loading texture file: {}", OIIO::geterror());
    return 1;
  }
  const OIIO::ImageSpec &spec = inp->spec();
  std::println("Image: width {}, height {}, depth {}, channels {}", spec.width, spec.height, spec.depth, spec.nchannels);

  const std::filesystem::path vertFile{"C:/Users/veliu/repos/graphics-workshop/assets/shaders/solid_color_vert.spv"};
  const std::filesystem::path fragFile{"C:/Users/veliu/repos/graphics-workshop/assets/shaders/solid_color_frag.spv"};
  GLuint program = loadShaderSpirV(vertFile, fragFile);
  if (program == 0) {
    std::println("Error loading shaders.");
    return 1;
  }
  
  const uint32_t cellCnt = 9;
  const uint32_t objectCnt = cellCnt * cellCnt;

  PerFrameData& frameData = *createPersistentUniformBuffer<PerFrameData>(0).data;
  UniformBuffer<PerObjectData> perObjectData = createPersistentUniformBuffer<PerObjectData>(1, objectCnt);

  std::vector<glm::mat4> transforms;
  for (uint32_t i = 0; i < cellCnt; ++i) {
    for (uint32_t j = 0; j < cellCnt; ++j) {
      const float x = static_cast<float>(i) - static_cast<float>(cellCnt) / 2.f;
      const float y = static_cast<float>(j) - static_cast<float>(cellCnt) / 2.f;
      const glm::vec3 pos = {x, 0.f, y};
      const glm::mat4 transform = glm::translate(glm::mat4(1), 2.f * pos);
      transforms.push_back(transform);
      const uint32_t objectIndex = i * cellCnt + j;
      perObjectData.data[objectIndex].worldFromModel = transform;
    }
  }

  glViewport(0, 0, kWidth, kHeight);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float t = static_cast<float>(glfwGetTime());
    const glm::vec3 eye = 6.f * glm::vec3{glm::cos(t), 0.75f, glm::sin(t)};
    frameData.viewFromWorld = glm::lookAt(eye, glm::vec3{0, 0.5f, 0}, glm::vec3{0, 1, 0});
    static float fovDegrees = 45.0f;
    frameData.projectionFromView = glm::perspective(glm::radians(fovDegrees), static_cast<float>(kWidth) / kHeight, 0.1f, 100.0f);
    ImGui::Begin("Props");
    ImGui::SliderFloat("Fish eye strength", &frameData.fishEyeStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("FOV", &fovDegrees, 0.0f, 180.0f);
    ImGui::End();

    glUseProgram(program);
    for (auto const& [ix, transform] : std::views::enumerate(transforms)) {
      glBindBufferRange(GL_UNIFORM_BUFFER, 1, perObjectData.ubo, sizeof(PerObjectData) * ix, sizeof(PerObjectData));
      for (const MeshGpu& mg : meshGpus) {
        glBindVertexArray(mg.vertexArray);   
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mg.numIndices), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
      }
    }
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
      vertex.texCoord1 = {mesh->mTextureCoords[0][vertIx].x, mesh->mTextureCoords[0][vertIx].y};
    } else {
      vertex.texCoord1 = {};
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