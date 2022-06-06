#ifndef SPACE_H
#define SPACE_H

#include <SDL.h>
#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <SDL_opengles.h>
#include <emscripten.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#endif

// The simulation texture memory, along with its width and height
struct SimState {
  // The current time (in s)
  float time;
  // Physical size of each texel (in m)
  float delta_x;
  // Width and height (in texels) of texture
  GLsizei width, height;
  // The simulation texture memory. It is indexed as texture[y][x][channel].
  float *texture;
};

// An object that changes the state of the simulation on the cpu. This includes boundary objects and
// objects that set their value manually, such as wave sources or fixed points.
class SimObject {
public:
  virtual void draw(SimState &sim) = 0;

};

#endif