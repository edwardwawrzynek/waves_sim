#ifndef MAIN_H
#define MAIN_H

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
  // Triangle buffer for quad that covers entire screen
  GLuint vao;
  GLuint triangle_buffer;

  // Vertex shader for screen covering quad
  GLuint vertex_shader;
  // Fragment shader that runs simulation step
  GLuint sim_shader;
  // Fragment shader that draws simulation state
  GLuint display_shader;
  // Program that runs simulation step
  GLuint sim_program;
  // Program that draws simulation state
  GLuint display_program;

  // Window and gl context
  SDL_Window *window;
  SDL_Surface *surface;
  SDL_GLContext gl_context;

  // current screen size
  GLsizei width, height;

  // Simulation state storage texture. This is an rgba floating point texture.
  // The red channel is position (u), green is velocity (du/dt), blue is wave speed (c).
  // Because sim_program reads and writes the state, we use two textures. One texture stores the
  // current state and is read from, while the new state is written to the other texture. After each
  // cycle, each texture's role is flipped.
  GLuint sim_textures[2];
  // Framebuffers that sim_texture0 and sim_texture1 are bound to
  GLuint sim_framebuffers[2];
  // Index of sim texture that is to be written to next. The opposite texture contains the last
  // written state.
  int current_sim_texture;

  // uniform location for sim_texture in sim_program
  GLint sim_sim_tex_loc;
  // uniform location for sim_texture in display_program
  GLint display_sim_tex_loc;
  // uniform location for screen_size in display_program
  GLint display_screen_size_loc;

  // uniform locations for simulation params
  GLint delta_x_loc;
  GLint delta_t_loc;
  GLint time_loc;

  // Size of the simulation texture. Powers of two make the most efficient use of vram.
  GLsizei sim_texture_size;
  // Physical size of each texel in the simulation texture (in m).
  float delta_x;
  // Time step size for simulation (in s).
  float delta_t;
  // Default wave speed (in m/s).
  float default_wave_c;

  float time;

  // Read a file into a GLchar[]
  static const GLchar *read_shader_file(const char *path);
  // Load and compile shader from file and return its shader id
  static GLuint load_shader(const char *path, GLenum shader_type);
  // Link a vertex and fragment shader into a program, and return the program id
  static GLuint create_program(GLuint vertex_shader, GLuint frag_shader);

  // Initialize SDL and OpenGL
  int init_sdl_opengl();
  // Initialize SDL window
  int init_sdl_window();
  // Initialize imgui on window
  int init_imgui();
  // Create screen covering triangle
  int init_vao();
  // Create the texture and framebuffer for simulation
  int init_sim_texture();
  // Load programs
  int init_programs();

  // Handle window events, and return non zero if program should quit
  int handle_sdl_events();

  // Render the screen covering quad. This does not set the program or framebuffer, which must be set before hand.
  void draw_quad();

  // Run one step of the simulation program
  void run_simulation();
  // Render the simulated state
  void run_display();

public:
  WavesApp(): width(640), height(640), current_sim_texture(0), sim_texture_size(256), delta_x(0.1), delta_t(0.005), default_wave_c(343.0), time(0.0) {};

  // Initialize the app. Return 0 on success, non zero on failure.
  int init();

  // Run one simulation loop and update the gui
  int draw_frame();

  // Destroy imgui, sdl, and gl context
  void shutdown();
};

#endif