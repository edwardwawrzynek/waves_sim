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

  // Simulation state framebuffer and texture
  GLuint sim_framebuffer;
  GLuint sim_texture;

  GLint sim_tex_loc_sim;
  GLint sim_tex_loc_display;

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
  WavesApp(): width(640), height(480) {};

  // Initialize the app. Return 0 on success, non zero on failure.
  int init();

  // Run one simulation loop and update the gui
  int draw_frame();

  // Destroy imgui, sdl, and gl context
  void shutdown();
};