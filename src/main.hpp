#ifndef MAIN_H
#define MAIN_H

#include "geometry.hpp"

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
  SDL_Surface *surface;
  SDL_GLContext gl_context;

  // Display window size
  GLsizei width{800}, height{800};

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

  // Get the factor by which physical coordinates are scaled to screen coordinates
  glm::vec2 get_scale_factor() const;

  // Draw the environment onto the last written sim texture
  void draw_environment();
  // Run one step of the simulation program, rendering the new state onto the current texture
  void run_simulation();
  // Render the state of the last written sim texture
  void run_display();
  // Render simulation environment controls
  void draw_env_controls();

public:
  WavesApp() = default;

  // Initialize the app. Return 0 on success, non zero on failure.
  int init();

  // Run one simulation loop and update the gui
  int draw_frame();

  // Destroy imgui, sdl, and gl context
  void shutdown();

  void add_object(std::unique_ptr<SimObject> object);
};

#endif