#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include "main.hpp"

#include <cstdio>
#include <iostream>

// opengl version
#if defined(__EMSCRIPTEN__)
#define OPENGL_SDL_PROFILE SDL_GL_CONTEXT_PROFILE_ES
#define OPENGL_MAJOR 3
#define OPENGL_MINOR 0
#define GLSL_VERSION_STRING "#version 300 es"
#else
#define OPENGL_SDL_PROFILE SDL_GL_CONTEXT_PROFILE_CORE
#define OPENGL_MAJOR 3
#define OPENGL_MINOR 3
#define GLSL_VERSION_STRING "#version 330 core"
#endif

// Initialize SDL and OpenGL
int WavesApp::init_sdl_opengl() {
  // setup sdl
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    fprintf(stderr, "Failed to init SDL: %s", SDL_GetError());
    return -1;
  }
  // init opengl
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, OPENGL_SDL_PROFILE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR);

  return 0;
}

int WavesApp::init_sdl_window() {
  // setup graphics context
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow("Waves", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                            height, window_flags);
  gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);
  surface = SDL_GetWindowSurface(window);

  return 0;
}

// Initialize imgui
int WavesApp::init_imgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(GLSL_VERSION_STRING);

  return 0;
}

// Setup screen covering triangles
int WavesApp::init_vao() {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // triangle strip defining quad covering whole screen
  GLfloat vertices[4][2] = {{-1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0}};

  glGenBuffers(1, &triangle_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangle_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)nullptr);
  glEnableVertexAttribArray(0);

  return 0;
}

// Read a file into a GLchar[] and return
const GLchar *WavesApp::read_shader_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (file == nullptr) {
    fprintf(stderr, "Can't open shader: %s\n", path);
    return nullptr;
  }
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  fseek(file, 0, SEEK_SET);

  GLchar *contents = new GLchar[len + 1];
  fread(contents, sizeof(GLchar), len, file);
  contents[len] = '\0';

  fclose(file);

  return contents;
}

// Load and compile a shader from file
GLuint WavesApp::load_shader(const char *path, GLenum shader_type) {
  const GLchar *contents = read_shader_file(path);
  if (contents == nullptr) {
    return 0;
  }

  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &contents, nullptr);
  delete[] contents;

  glCompileShader(shader);
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLsizei info_len;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    GLchar *info_log = new GLchar[info_len + 1];
    glGetShaderInfoLog(shader, info_len, &info_len, info_log);
    fprintf(stderr, "Error compiling shader %s: %s\n", path, info_log);
    delete[] info_log;
    return 0;
  }

  return shader;
}

// Load vertex and fragment shaders into a program
GLuint WavesApp::create_program(GLuint vertex_shader, GLuint frag_shader) {
  GLuint program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, frag_shader);

  glLinkProgram(program);

  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLsizei info_len;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);
    GLchar *info_log = new GLchar[info_len + 1];
    glGetProgramInfoLog(program, info_len, &info_len, info_log);
    fprintf(stderr, "Error linking shaders: %s\n", info_log);
    delete[] info_log;
    return 0;
  }

  return program;
}

// Setup the simulation and display programs
int WavesApp::init_programs() {
  vertex_shader = load_shader("/home/edward/Documents/waves_sim/shaders/screen_cover.vert", GL_VERTEX_SHADER);
  sim_shader = load_shader("/home/edward/Documents/waves_sim/shaders/wave_sim.frag", GL_FRAGMENT_SHADER);
  display_shader = load_shader("/home/edward/Documents/waves_sim/shaders/display.frag", GL_FRAGMENT_SHADER);

  if(!vertex_shader || !sim_shader || !display_shader) {
    return -1;
  }

  sim_program = create_program(vertex_shader, sim_shader);
  display_program = create_program(vertex_shader, display_shader);
  if(!sim_program || !display_program) {
    return -1;
  }

  // get uniform locations
  sim_sim_tex_loc = glGetUniformLocation(sim_program, "sim_texture");
  display_sim_tex_loc = glGetUniformLocation(display_program, "sim_texture");
  display_screen_size_loc = glGetUniformLocation(display_program, "screen_size");

  delta_x_loc = glGetUniformLocation(sim_program, "delta_x");
  delta_t_loc = glGetUniformLocation(sim_program, "delta_t");
  time_loc = glGetUniformLocation(sim_program, "time");

  return 0;
}

// Create the two simulation textures and bind them to framebuffers
int WavesApp::init_sim_texture() {
  for(int i = 0; i < 2; i++) {
    // create framebuffer for sim program
    glGenFramebuffers(1, &sim_framebuffers[i]);
    glBindFramebuffer(GL_FRAMEBUFFER, sim_framebuffers[i]);
    // create empty texture
    glActiveTexture(GL_TEXTURE0 + i);
    glGenTextures(1, &sim_textures[i]);
    glBindTexture(GL_TEXTURE_2D, sim_textures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sim_texture_size, sim_texture_size, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sim_textures[i], 0);

    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      return -1;
    }
  }

  return 0;
}

int WavesApp::init() {
  return init_sdl_opengl() || init_sdl_window() || init_imgui() || init_vao() || init_sim_texture() || init_programs();
}

int WavesApp::handle_sdl_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      return 1;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(window))
      return 1;
    if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && event.window.windowID == SDL_GetWindowID(window)) {
      width = event.window.data1;
      height = event.window.data2;
    }
  }

  return 0;
}

// draw the screen covering triangles
void WavesApp::draw_quad() {
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Run one step of the simulation
void WavesApp::run_simulation() {
  // draw to target framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, sim_framebuffers[current_sim_texture]);
  glViewport(0, 0, sim_texture_size, sim_texture_size);

  glUseProgram(sim_program);
  // set program to read from texture not being written to
  glUniform1i(sim_sim_tex_loc, current_sim_texture ? 0 : 1);

  glUniform1f(delta_x_loc, delta_x);
  glUniform1f(delta_t_loc, delta_t);
  glUniform1f(time_loc, time);

  draw_quad();
  // swap sim textures
  current_sim_texture = current_sim_texture ? 0 : 1;

  time += delta_t;
}

// Draw simulation state to display
void WavesApp::run_display() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, width, height);

  glUseProgram(display_program);
  glUniform1i(display_sim_tex_loc, current_sim_texture);
  glUniform2f(display_screen_size_loc, (GLfloat)width, (GLfloat)height);
  draw_quad();
}

int WavesApp::draw_frame() {
  if(handle_sdl_events()) {
    return 1;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  // run simulation step
  run_simulation();
  // render state
  run_display();

  ImGui::Begin("Simulation Settings");
  ImGui::SliderFloat("Delta t", &delta_t, 0.0, 0.1);
  ImGui::SliderFloat("Delta x", &delta_x, 0.0, 0.1);
  ImGui::End();

  // render imgui
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_GL_SwapWindow(window);

  return 0;
}

void WavesApp::shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void webDrawFrame() {
  // TODO
}

int main() {
  WavesApp app{};
  if(app.init()) {
    return -1;
  }

#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop(webDrawFrame, 0, true);
#else
  while (!app.draw_frame())
    ;
#endif
  app.shutdown();

  return 0;
}
