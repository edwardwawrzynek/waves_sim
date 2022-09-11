#ifndef MAIN_H
#define MAIN_H

#include "geometry.hpp"
#include <imfilebrowser.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

#include <SDL.h>
#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <SDL_opengles.h>
#include <emscripten.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#endif

class WavesApp {
  // Window and gl context
  SDL_Window *window;
  SDL_GLContext gl_context;

  // Display window size
  GLsizei width{1400}, height{800};

  ImGui::FileBrowser open_file_browser{};
  ImGui::FileBrowser save_file_browser{ImGuiFileBrowserFlags_EnterNewFilename};

  // Simulation state storage texture. This is an rgba floating point texture.
  // The red channel is position (u), green is velocity (du/dt), blue is wave speed (c), alpha is
  // boundary (0 = normal, 1 = boundary).
  // Because sim_program reads and writes the state, we use two textures. One texture stores the
  // current state and is read from, while the new state is written to the other texture. After each
  // cycle, each texture's role is flipped.
  GLuint sim_textures[2];
  // Framebuffers that sim_texture0 and sim_texture1 are bound to
  GLuint sim_framebuffers[2];
  // Index of sim texture that is to be written to next. The opposite texture contains the last
  // written state.
  int current_sim_texture{0};
  // Width and height (in texels) of texture
  size_t texture_width{1024}, texture_height{1024};

  // Time step size for simulation (in s).
  float delta_t{0.01};
  // Current time (in s)
  float time{0.0};
  // Physical size of each texel (in m/texel)
  float delta_x{0.04};
  // Wave speed in free space (in m/s)
  float wave_speed_vacuum{2.0};
  // Size (in texels) of absorbing boundary layer
  int damping_area_size{128};

  // if editing controls should be shown
  bool show_edit{true};
  // if program simulation should be run (or is paused)
  bool run_sim{true};
  // number of simulation iterations to run each display cycle
  int sim_cycles{1};
  // if simulation settings should be shown
  bool show_settings{true};

  // the current open path
  std::optional<std::string> open_file_path{};

  // gl programs and geometry
  Programs programs{};

  // Simulation objects
  Environment environment{};

  // Initialize SDL and OpenGL
  int init_sdl_opengl();
  // Initialize SDL window
  int init_sdl_window();
  // Initialize imgui on window
  int init_imgui();
  // Create the texture and framebuffer for simulation
  int init_sim_texture();

  // Handle window events, and return non zero if program should quit
  int handle_sdl_events();

  // Get the factor by which physical coordinates are scaled to texture coordinates
  glm::vec2 get_scale_factor() const;
  // Get the factor by which physical coordinates are scaled to display coordinates
  // (same as texture coordinates, but excludes absorbing layer area)
  glm::vec2 get_display_scale_factor() const;

  // Draw the environment onto the last written sim texture
  void draw_environment();
  // Clear current wave state
  void clear_sim();
  // Run one step of the simulation program, rendering the new state onto the current texture
  void run_simulation();
  // Get the size (in pixels) to display the simulation state at
  glm::vec2 get_display_size();
  // Render the state of the last written sim texture
  void run_display();
  // Render simulation environment controls
  void draw_env_controls();
  // Draw error popup modals (if needed)
  void draw_error_popups();
  // Draw and handle main menu options
  void draw_menu_bar();
  void draw_add_menu();
  void draw_file_menu();
  // Save the current environment
  void save_to_file();

  // Serialize the simulation settings
  std::string serialize_settings() const;
  bool deserialize_settings(std::istream &in);

  // Write the current environment to stream
  std::string serialize();

public:
  WavesApp() = default;

  // Initialize the app. Return 0 on success, non zero on failure.
  int init();

  // Run one simulation loop and update the gui
  int draw_frame();

  // Destroy imgui, sdl, and gl context
  void shutdown();

  // Add an object to the environment
  void add_object(std::unique_ptr<SimObject> object);

  // Load the environment from a file
  void load_from_file(const std::string &path);
};

#endif