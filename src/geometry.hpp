#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <SDL.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <SDL_opengles.h>
#include <emscripten.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class GeometryType {
  // A VAO with a point at (0, 0)
  Point = 0,
  // A VAO with a line from (0, 0) to (1, 0)
  Line = 1,
  // A VAO with a rectangle with corners (0, 0) and (1, 1)
  Square = 2,
  // A VAO with lines through (0, 0), (0, 1), (1, 1), (1, 0), to (0, 0)
  SquareLine = 3,
};

// GeometryManager contains the VAOs needed to draw the primitive shapes used in simulation.
class GeometryManager {
  GLuint vao[4]{0, 0, 0, 0};

public:
  // Create the VAOs with appropriate geometry. Requires the gl context to be up.
  void init_geometry();

  // Draw the vao for the given geometry type
  void draw_geo(GeometryType geo) const;

  // A transform matrix that transforms the Square geometry into a screen covering quad
  static constexpr glm::mat4 square_screen_cover_transform =
      glm::mat4(2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, -1.0, 0.0, 1.0);

  GeometryManager() = default;
};

// Programs contains the programs used and their uniform locations
class Programs {
  // vertex shader for all geometry
  GLuint vertex_shader{};
  // fragment shader that runs simulation step
  GLuint sim_shader{};
  // fragment shader that draws simulation state
  GLuint display_shader{};
  // fragment shader that draws objects
  GLuint object_shader{};
  // fragment shader that draws editing handles
  GLuint handle_shader{};

public:
  // program that runs simulation step
  GLuint sim_program{};
  // program that draws simulation state
  GLuint display_program{};
  // program that draw sim objects
  GLuint object_program{};
  // program that draws editing handles
  GLuint handle_program{};

  // uniform location for sim_texture in sim_program
  GLint sim_sim_tex_loc{};
  // uniform locations in display_program
  GLint display_sim_tex_loc{};
  GLint display_screen_size_loc{};
  GLint display_damping_area_size_loc{};

  // uniform locations for sim_program physical parameters
  GLint sim_delta_x_loc{};
  GLint sim_delta_t_loc{};
  GLint sim_wave_speed_vacuum_loc{};
  GLint sim_damping_area_size_loc{};

  // transform matrix location
  GLint sim_transform_loc{};
  GLint display_transform_loc{};
  GLint object_transform_loc{};

  // object parameter locations
  GLint object_object_props_loc{};

  // handle uniform locations
  GLint handle_hole_loc{};
  GLint handle_selected_loc{};

  // geometry primitives
  GeometryManager geo{};

  // Read a file into a GLchar[]
  static const GLchar *read_shader_file(const char *path);
  // Load and compile shader from file and return its shader id
  static GLuint load_shader(const char *path, GLenum shader_type);
  // Link a vertex and fragment shader into a program, and return the program id
  static GLuint create_program(GLuint vertex_shader, GLuint frag_shader);

  // Load and compile/link all programs. Return -1 on failure, 0 on success.
  int init();

  Programs() = default;
};

// The physical type of a SimObject: either a boundary or a medium with a wave speed.
class MediumType {
public:
  bool is_boundary;
  float ior;

  static MediumType Medium(float index_of_refraction);
  static MediumType Boundary();

  // setup the appropriate glColorMask for this medium
  void set_gl_color_mask() const;
  // Set the object_program uniforms for this medium
  void set_object_uniforms(const Programs &programs) const;

  // Setup gl program, uniforms, and glColorMask to render this medium
  void set_gl_program(const Programs &programs) const;

  // Draw imgui editing controls for this medium type
  void draw_imgui_controls();
  // convert the medium type to a text representation
  std::string serialize() const;
  // convert a text representation of the type into a MediumType
  static std::optional<MediumType> deserialize(std::istream &in);

private:
  MediumType(bool is_boundary, float index_of_refraction)
      : is_boundary(is_boundary), ior(index_of_refraction){};
};

// An object that is draw to the simulation texture, either a source, medium, or boundary.
class SimObject {
public:
  // draw the object to the simulation texture
  virtual void draw(const Programs &programs, glm::vec2 physical_scale_factor,
                    float time) const = 0;
  // draw the object's editing controls to the display
  virtual void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                             bool active) const;
  // handle any imgui io events affecting the object. active is true if the object is selected. If
  // the object is selected or the event caused the object to be selected, return true. If the
  // object isn't selected, return false.
  virtual bool handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size);
  // draw the imgui controls for this object. This is called within an imgui window (ie, within
  // ImGui::Begin and ImGui::End). Return true if this object should be deleted.
  virtual bool draw_imgui_controls();
  // convert the object to its textual representation
  virtual std::string serialize() const = 0;
  // get the object from its textual representation
  static std::optional<std::unique_ptr<SimObject>> deserialize(std::istream &in);

  virtual ~SimObject() = default;
};

class Environment {
public:
  std::vector<std::unique_ptr<SimObject>> objects{};
  long int active_object{-1};

  void draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const;
  void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor) const;
  void handle_events(glm::vec2 delta_x, glm::vec2 screen_size);
  void draw_imgui_controls();
  bool has_active_object() const;
  // generate a textual representation of the environment
  std::string serialize() const;
  // convert a textual representation to an Environment
  static std::optional<Environment> deserialize(std::istream &in);

  Environment() = default;
};

// An object that clears all media and boundaries in the simulation area
class AreaClear : public SimObject {
public:
  void draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const override;
  std::string serialize() const override;

  AreaClear() = default;
};

// A rectangular area defined by two corners
class Rectangle : public SimObject {
public:
  // corner locations
  float x0, y0, x1, y1;
  MediumType medium;

  // which corner handle is active (or -1 if none)
  int active_handle{-1};

  void draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const override;
  void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                     bool active) const override;
  bool handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) override;

  bool draw_imgui_controls() override;
  std::string serialize() const override;

  // (x0, y0) define the bottom left corner, (x1, y1) defines the top right corner
  Rectangle(float x0, float y0, float x1, float y1, MediumType medium)
      : x0(x0), y0(y0), x1(x1), y1(y1), medium(medium){};
};

// A line segment (either medium or source) between two points
class LineBase : public SimObject {
public:
  float x0, y0, x1, y1;
  float width;
  // which corner handle is active
  int active_handle{-1};

  void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor, bool active,
                     bool draw_holes) const;
  bool handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) override;
  void imgui_line_controls();
  std::string serialize_coordinates() const;

  LineBase(float x0, float y0, float x1, float y1, float width)
      : x0(x0), y0(y0), x1(x1), y1(y1), width(width){};
};

// A line segment between two points
class Line : public LineBase {
public:
  MediumType medium;

  void draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const override;
  void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                     bool active) const override;
  bool draw_imgui_controls() override;
  std::string serialize() const override;

  Line(float x0, float y0, float x1, float y1, float width, MediumType medium)
      : LineBase(x0, y0, x1, y1, width), medium(medium){};
};

// A waveform describes the intensity of a source over time.
class Waveform {
public:
  // Waveforms are sampled over both time (s) and phase (rad). For a constant amplitude wave,
  // these correspond to the same thing. For a wave with time dependent envelope, phase should
  // change the phase of the carrier but not of the envelope. For example, an implementation of an
  // am signal might be: signal(time) * cos(carrier_freq * time + phase
  virtual float sample(float time, float phase) const = 0;
  // Return the time derivative (df/dt) of sample
  virtual float sample_diff(float time, float phase) const = 0;

  // Setup the gl program to draw this waveform on a source
  void set_gl_program(const Programs &programs, float time, float phase) const;

  // Draw imgui controls for the waveform properties
  virtual void draw_imgui_prop_controls() = 0;
  // draw imgui controls for the waveform properties and type
  static void draw_imgui_controls(std::unique_ptr<Waveform> &waveform,
                                  const char *label = "Waveform:", const char *type_label = "Type");

  virtual int waveform_type_index() = 0;

  // return the frequency and amplitude of the wave (if periodic), 1.0 otherwise
  virtual std::pair<float, float> get_freq_amp() const;

  // convert the waveform to its stored textual representation
  virtual std::string serialize() const = 0;
  // convert a textual representation to a Waveform
  static std::optional<std::unique_ptr<Waveform>> deserialze(std::istream &in);

  virtual ~Waveform() = default;
};

// A sine wave
class SineWaveform : public Waveform {
public:
  // Amplitude and frequency (Hz) of wave
  float amp, freq;

  float sample(float time, float phase) const override;
  float sample_diff(float time, float phase) const override;

  void draw_imgui_prop_controls() override;
  int waveform_type_index() override;
  std::pair<float, float> get_freq_amp() const override;
  std::string serialize() const override;

  SineWaveform(float amplitude, float frequency) : amp(amplitude), freq(frequency){};
};

// A triangle wave
class TriangleWaveform : public Waveform {
public:
  float amp, freq;

  float sample(float time, float phase) const override;
  float sample_diff(float time, float phase) const override;

  void draw_imgui_prop_controls() override;
  int waveform_type_index() override;
  std::pair<float, float> get_freq_amp() const override;
  std::string serialize() const override;

  TriangleWaveform(float amp, float freq) : amp(amp), freq(freq){};
};

// A square wave
class SquareWaveform : public Waveform {
public:
  float amp, freq;

  float sample(float time, float phase) const override;
  float sample_diff(float time, float phase) const override;

  void draw_imgui_prop_controls() override;
  int waveform_type_index() override;
  std::pair<float, float> get_freq_amp() const override;
  std::string serialize() const override;

  SquareWaveform(float amp, float freq) : amp(amp), freq(freq){};
};

// A pulse (a guassian envelope) of some other waveform
class GaussianEnvelope : public Waveform {
public:
  // the underlying waveform
  std::unique_ptr<Waveform> waveform;

  // length of <the pulse (in s). This is really the 95% interval of the gaussian.
  float duration_95;
  // the time at which the pulse begins (in s).
  float start_t;

  // evaluate gaussian distribution at x
  // the return value is normalized to hit 1 at mean (ie, not a proper pdf)
  float gaussian(float x) const;
  float gaussian_diff(float x) const;

  float sample(float time, float phase) const override;
  float sample_diff(float time, float phase) const override;

  void draw_imgui_prop_controls() override;
  int waveform_type_index() override;
  std::pair<float, float> get_freq_amp() const override;
  std::string serialize() const override;

  GaussianEnvelope(std::unique_ptr<Waveform> waveform, float duration, float start_t)
      : waveform(std::move(waveform)), duration_95(duration), start_t(start_t){};
};

// A point wave source
class PointSource : public SimObject {
public:
  // location
  float x, y;
  std::unique_ptr<Waveform> waveform;
  float phase;

  void draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const override;
  void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                     bool active) const override;
  bool handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) override;
  bool draw_imgui_controls() override;
  std::string serialize() const override;

  PointSource(float x, float y, std::unique_ptr<Waveform> waveform, float phase)
      : x(x), y(y), waveform(std::move(waveform)), phase(phase){};
};

// A line wave source (which leads to a plane wave)
class LineSource : public LineBase {
public:
  std::unique_ptr<Waveform> waveform;
  float phase;

  void draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const override;
  void draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                     bool active) const override;
  bool draw_imgui_controls() override;
  std::string serialize() const override;

  LineSource(float x0, float y0, float x1, float y1, float width,
             std::unique_ptr<Waveform> waveform, float phase)
      : LineBase(x0, y0, x1, y1, width), waveform(std::move(waveform)), phase(phase){};
};

#endif