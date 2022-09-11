#include "main.hpp"

#include <cstdio>
#include <fstream>

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
  window = SDL_CreateWindow("Waves", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                            window_flags);
  gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

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

// Create the two simulation textures and bind them to framebuffers
int WavesApp::init_sim_texture() {
  for (int i = 0; i < 2; i++) {
    // create framebuffer for sim program
    glGenFramebuffers(1, &sim_framebuffers[i]);
    glBindFramebuffer(GL_FRAMEBUFFER, sim_framebuffers[i]);
    // create empty texture
    glActiveTexture(GL_TEXTURE0 + i);
    glGenTextures(1, &sim_textures[i]);
    glBindTexture(GL_TEXTURE_2D, sim_textures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texture_width, texture_height, 0, GL_RGBA, GL_FLOAT,
                 nullptr);

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
  open_file_browser.SetTitle("Open File");
  save_file_browser.SetTitle("Save File");
  open_file_browser.SetTypeFilters({".sim"});
  save_file_browser.SetTypeFilters({".sim"});

  return init_sdl_opengl() || init_sdl_window() || init_imgui() || programs.init() ||
         init_sim_texture();
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
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED &&
        event.window.windowID == SDL_GetWindowID(window)) {
      width = event.window.data1;
      height = event.window.data2;
    }
  }

  return 0;
}

glm::vec2 WavesApp::get_scale_factor() const {
  return glm::vec2(2.0 / ((float)(texture_width)*delta_x), 2.0 / ((float)(texture_height)*delta_x));
}

glm::vec2 WavesApp::get_display_scale_factor() const {
  return glm::vec2(2.0 / ((texture_width - 2.0 * damping_area_size) * delta_x),
                   2.0 / ((texture_height - 2.0 * damping_area_size) * delta_x));
}

void WavesApp::clear_sim() {
  glBindFramebuffer(GL_FRAMEBUFFER, sim_framebuffers[current_sim_texture ? 0 : 1]);
  glViewport(0, 0, (GLsizei)texture_width, (GLsizei)texture_height);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

// Draw the environment on the simulation texture
void WavesApp::draw_environment() {
  // Bind the last written (ie next to be read) framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, sim_framebuffers[current_sim_texture ? 0 : 1]);
  glViewport(0, 0, (GLsizei)texture_width, (GLsizei)texture_height);

  environment.draw(programs, get_scale_factor(), time);
}

// Run one step of the simulation
void WavesApp::run_simulation() {
  // draw to target framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, sim_framebuffers[current_sim_texture]);
  glViewport(0, 0, (GLsizei)texture_width, (GLsizei)texture_height);

  glUseProgram(programs.sim_program);
  // set program to read from texture not being written to
  glUniform1i(programs.sim_sim_tex_loc, current_sim_texture ? 0 : 1);

  glUniform1f(programs.sim_delta_x_loc, delta_x);
  glUniform1f(programs.sim_delta_t_loc, delta_t);
  glUniform1f(programs.sim_wave_speed_vacuum_loc, wave_speed_vacuum);
  glUniform1f(programs.sim_damping_area_size_loc, (float)damping_area_size);

  glUniformMatrix4fv(programs.sim_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(GeometryManager::square_screen_cover_transform));
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  programs.geo.draw_geo(GeometryType::Square);
  // swap sim textures
  current_sim_texture = current_sim_texture ? 0 : 1;

  time += delta_t;
}

// Get size (in pixels) of area to draw
glm::vec2 WavesApp::get_display_size() {
  float display_size = std::min(width, height);
  return {display_size, display_size};
}

// Draw simulation state to display
void WavesApp::run_display() {
  glm::vec2 display_size = get_display_size();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, (GLsizei)display_size.x, (GLsizei)display_size.y);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glClearColor(0.2, 0.2, 0.2, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(programs.display_program);
  glUniform1i(programs.display_sim_tex_loc, current_sim_texture ? 0 : 1);
  glUniform2f(programs.display_screen_size_loc, display_size.x, display_size.y);
  glUniform1f(programs.display_damping_area_size_loc, (GLfloat)damping_area_size);

  glUniformMatrix4fv(programs.display_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(GeometryManager::square_screen_cover_transform));
  programs.geo.draw_geo(GeometryType::Square);
}

void WavesApp::draw_env_controls() {
  glm::vec2 display_size = get_display_size();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, (GLsizei)display_size.x, (GLsizei)display_size.y);

  environment.draw_controls(programs, get_display_scale_factor());

  // only handle mouse events if they aren't on imgui windows
  if (!ImGui::GetIO().WantCaptureMouse) {
    environment.handle_events(
        glm::vec2(delta_x * (texture_width - 2.0 * damping_area_size) / display_size.x,
                  delta_x * (texture_height - 2.0 * damping_area_size) / display_size.y),
        display_size);
  }
}

void WavesApp::draw_error_popups() {
  if (ImGui::BeginPopupModal("Cannot Read File")) {
    ImGui::Text("Cannot read the selected file.");
    ImGui::Separator();
    if (ImGui::Button("Ok")) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  if (ImGui::BeginPopupModal("Invalid Environment File")) {
    ImGui::Text("The selected file is not a valid environment description.");
    ImGui::Separator();
    if (ImGui::Button("Ok")) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void WavesApp::load_from_file(const std::string &path) {
  std::fstream file{path, std::ios_base::in};
  if (!file.is_open()) {
    fprintf(stderr, "Cannot open file: %s\n", path.c_str());
    ImGui::OpenPopup("Cannot Read File");
    return;
  }

  if (!deserialize_settings(file)) {
    fprintf(stderr, "Environment file is missing simulation settings\n");
    ImGui::OpenPopup("Invalid Environment File");
    return;
  }

  auto new_env = Environment::deserialize(file);
  if (!new_env) {
    fprintf(stderr, "Environment file is invalid\n");
    ImGui::OpenPopup("Invalid Environment File");
    return;
  }

  environment = std::move(*new_env);
}

void WavesApp::draw_add_menu() {
  bool added = false;

  if (ImGui::MenuItem("Point Source")) {
    add_object(std::make_unique<PointSource>(0, 0, std::make_unique<SineWaveform>(5.0, 1.0), 0.0));
    added = true;
  }
  if (ImGui::MenuItem("Line Source")) {
    add_object(std::make_unique<LineSource>(-5, -5, 5, 5, 1.0,
                                            std::make_unique<SineWaveform>(1.0, 1.0), 0.0));
    added = true;
  }
  if (ImGui::MenuItem("Box")) {
    add_object(std::make_unique<Rectangle>(-3, -3, 3, 3, MediumType::Boundary()));
    added = true;
  }
  if (ImGui::MenuItem("Wall")) {
    add_object(std::make_unique<Line>(-5, 5, 5, -5, 1, MediumType::Boundary()));
    added = true;
  }

  if (added) {
    environment.active_object = (int)environment.objects.size() - 1;
  }
}

std::string WavesApp::serialize() { return serialize_settings() + "\n" + environment.serialize(); }

void WavesApp::save_to_file() {
  if (!open_file_path) {
    save_file_browser.Open();
  } else {
    std::fstream file{*open_file_path, std::ios_base::out};
    file << serialize();
    file.close();
  }
}

void WavesApp::draw_file_menu() {
  if (ImGui::MenuItem("Open")) {
    open_file_browser.Open();
  }
  if (ImGui::MenuItem("Save")) {
    save_to_file();
  }
  if (ImGui::MenuItem("Save As")) {
    open_file_path = {};
    save_to_file();
  }
}

void WavesApp::draw_menu_bar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      draw_file_menu();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add")) {
      draw_add_menu();
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Settings")) {
      show_settings = true;
    }

    ImGui::EndMainMenuBar();
  }
}

int WavesApp::draw_frame() {
  if (handle_sdl_events()) {
    return 1;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  // Draw simulation controls
  if (show_settings) {
    if (ImGui::Begin("Simulation Settings", &show_settings)) {
      if (ImGui::Button(run_sim ? "Stop Simulation" : "Start Simulation")) {
        run_sim = !run_sim;
      }
      if (ImGui::Button(show_edit ? "Hide Controls" : "Show Controls")) {
        show_edit = !show_edit;
      }
      if (ImGui::Button("Reset Simulation State")) {
        time = 0.0;
        clear_sim();
      }

      ImGui::NewLine();
      ImGui::Text("Time: %f s", time);
      ImGui::DragFloat("Wave Speed", &wave_speed_vacuum, 1e25, 0.0, 1e29, "%.3f m/s",
                       ImGuiSliderFlags_Logarithmic);

      if (ImGui::CollapsingHeader("PDE Solver Settings")) {
        ImGui::DragFloat("Delta t", &delta_t, 1e25, 0.0, 1e29, "%.3f s",
                         ImGuiSliderFlags_Logarithmic);
        ImGui::DragFloat("Delta x", &delta_x, 1e25, 0.0, 1e29, "%.3f m",
                         ImGuiSliderFlags_Logarithmic);

        ImGui::SliderInt("Absorbing layer width", &damping_area_size, 0,
                         std::min(texture_width, texture_height) / 2 - 1, "%i tx");
        ImGui::SliderInt("Iterations per display cycle", &sim_cycles, 1, 100);
      }
    }
    ImGui::End();
  }

  open_file_browser.Display();
  save_file_browser.Display();

  if (save_file_browser.HasSelected()) {
    open_file_path = save_file_browser.GetSelected().string();
    save_to_file();
    save_file_browser.ClearSelected();
  }

  if (open_file_browser.HasSelected()) {
    open_file_path = open_file_browser.GetSelected().string();
    load_from_file(*open_file_path);
    time = 0;
    clear_sim();
    open_file_browser.ClearSelected();
  }

  // run simulation step
  if (run_sim) {
    for (int i = 0; i < sim_cycles; i++) {
      draw_environment();
      run_simulation();
    }
  } else {
    draw_environment();
  }

  // render state
  run_display();
  // handle environment controls
  if (show_edit) {
    draw_env_controls();

    if (environment.has_active_object()) {
      if (ImGui::Begin("Edit Object")) {
        environment.draw_imgui_controls();
      }
      ImGui::End();
    }
  }

  draw_error_popups();
  draw_menu_bar();

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

void WavesApp::add_object(std::unique_ptr<SimObject> object) {
  environment.objects.push_back(std::move(object));
}

std::string WavesApp::serialize_settings() const {
  return "(Settings " + std::to_string(delta_t) + " " + std::to_string(delta_x) + " " +
         std::to_string(wave_speed_vacuum) + " " + std::to_string(damping_area_size) + " " +
         std::to_string(texture_width) + " " + std::to_string(texture_height) + ")";
}

bool WavesApp::deserialize_settings(std::istream &in) {
  auto type = SimObject::read_token(in);
  if (type == "Settings") {
    delta_t = std::stof(SimObject::read_token(in));
    delta_x = std::stof(SimObject::read_token(in));
    wave_speed_vacuum = std::stof(SimObject::read_token(in));
    damping_area_size = std::stoi(SimObject::read_token(in));
    texture_width = std::stoul(SimObject::read_token(in));
    texture_height = std::stoul(SimObject::read_token(in));
    // read closing paren
    return in.get() == ')';
  }

  return false;
}

WavesApp app{};

void webDrawFrame() { app.draw_frame(); }

int main() {
  if (app.init()) {
    return -1;
  }

  app.add_object(std::make_unique<AreaClear>());

#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop(webDrawFrame, 0, true);
#else
  while (!app.draw_frame())
    ;
#endif
  app.shutdown();

  return 0;
}
