#include "geometry.hpp"
#include <cmath>

#define PI 3.141592653589793

void GeometryManager::init_geometry() {
  GLfloat point_vertex[2] = {0.0, 0.0};
  GLfloat line_vertices[2][2] = {{0.0, 0.0}, {1.0, 0.0}};
  GLfloat square_vertices[4][2] = {{0.0, 1.0}, {1.0, 1.0}, {0.0, 0.0}, {1.0, 0.0}};
  GLfloat square_line_vertices[5][2] = {{0.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}, {0.0, 0.0}};

  for (int i = 0; i < 4; i++) {
    glGenVertexArrays(1, &vao[i]);
    glBindVertexArray(vao[i]);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    switch (i) {
    case 0:
      glBufferData(GL_ARRAY_BUFFER, sizeof(point_vertex), point_vertex, GL_STATIC_DRAW);
      break;
    case 1:
      glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);
      break;
    case 2:
      glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
      break;
    case 3:
      glBufferData(GL_ARRAY_BUFFER, sizeof(square_line_vertices), square_line_vertices,
                   GL_STATIC_DRAW);
      break;
    }

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)nullptr);
    glEnableVertexAttribArray(0);
  }
}

void GeometryManager::draw_geo(GeometryType geo) const {
  glBindVertexArray(vao[static_cast<int>(geo)]);

  switch (geo) {
  case GeometryType::Point:
    glDrawArrays(GL_POINTS, 0, 1);
    break;
  case GeometryType::Line:
    glDrawArrays(GL_LINES, 0, 2);
    break;
  case GeometryType::Square:
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    break;
  case GeometryType::SquareLine:
    glDrawArrays(GL_LINE_STRIP, 0, 5);
    break;
  }
}

// Read a file into a GLchar[] and return
const GLchar *Programs::read_shader_file(const char *path) {
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
GLuint Programs::load_shader(const char *path, GLenum shader_type) {
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
GLuint Programs::create_program(GLuint vertex_shader, GLuint frag_shader) {
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

// Setup the programs
int Programs::init() {
  // initialize geometry
  geo.init_geometry();

  vertex_shader =
      load_shader("/home/edward/Documents/waves_sim/shaders/vertex.vert", GL_VERTEX_SHADER);
  sim_shader =
      load_shader("/home/edward/Documents/waves_sim/shaders/wave_sim.frag", GL_FRAGMENT_SHADER);
  display_shader =
      load_shader("/home/edward/Documents/waves_sim/shaders/display.frag", GL_FRAGMENT_SHADER);
  object_shader =
      load_shader("/home/edward/Documents/waves_sim/shaders/object.frag", GL_FRAGMENT_SHADER);
  handle_shader =
      load_shader("/home/edward/Documents/waves_sim/shaders/handle.frag", GL_FRAGMENT_SHADER);

  if (!vertex_shader || !sim_shader || !display_shader || !object_shader || !handle_shader) {
    return -1;
  }

  sim_program = create_program(vertex_shader, sim_shader);
  display_program = create_program(vertex_shader, display_shader);
  object_program = create_program(vertex_shader, object_shader);
  handle_program = create_program(vertex_shader, handle_shader);
  if (!sim_program || !display_program || !object_shader || !handle_program) {
    return -1;
  }

  // get uniform locations
  sim_sim_tex_loc = glGetUniformLocation(sim_program, "sim_texture");
  display_sim_tex_loc = glGetUniformLocation(display_program, "sim_texture");
  display_screen_size_loc = glGetUniformLocation(display_program, "screen_size");
  display_damping_area_size_loc = glGetUniformLocation(display_program, "damping_area_size");

  sim_delta_x_loc = glGetUniformLocation(sim_program, "delta_x");
  sim_delta_t_loc = glGetUniformLocation(sim_program, "delta_t");
  sim_wave_speed_vacuum_loc = glGetUniformLocation(sim_program, "wave_speed_vacuum");
  sim_damping_area_size_loc = glGetUniformLocation(sim_program, "damping_area_size");

  sim_transform_loc = glGetUniformLocation(sim_program, "transform");
  display_transform_loc = glGetUniformLocation(display_program, "transform");
  object_transform_loc = glGetUniformLocation(object_program, "transform");

  object_object_props_loc = glGetUniformLocation(object_program, "object_props");

  handle_hole_loc = glGetUniformLocation(handle_program, "hole");
  handle_selected_loc = glGetUniformLocation(handle_program, "selected");

  return 0;
}

void MediumType::set_gl_color_mask() const {
  if (is_boundary) {
    glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
  } else {
    glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);
  }
}

void MediumType::set_object_uniforms(const Programs &programs) const {
  glUniform4f(programs.object_object_props_loc, 0.0, 0.0, 1.0 / ior, 1.0);
}

void MediumType::set_gl_program(const Programs &programs) const {
  glUseProgram(programs.object_program);
  set_gl_color_mask();
  set_object_uniforms(programs);
}

MediumType MediumType::Medium(float index_of_refraction) {
  return MediumType(false, index_of_refraction);
}

MediumType MediumType::Boundary() { return MediumType(true, 1.0); }

void MediumType::draw_imgui_controls() {
  ImGui::Text("Medium Type:");
  const char *type_options[2] = {"Boundary", "Medium"};

  int type_index = is_boundary ? 0 : 1;
  ImGui::Combo("Type", &type_index, type_options, IM_ARRAYSIZE(type_options));
  is_boundary = type_index == 0;

  if (!is_boundary) {
    ImGui::SliderFloat("Refractive Index", &ior, 1.0, 4.0);
  }
}

std::string MediumType::serialize() const {
  if (is_boundary) {
    return "(Boundary)";
  } else {
    return "(Medium " + std::to_string(ior) + ")";
  }
}

// read a token from an istream until a space or ) appears
static std::string read_token(std::istream &in) {
  std::string token = "";
  char c;
  // read any whitespace
  do {
    c = in.get();
    if (in.eof() || in.bad()) {
      return "";
    }
  } while (c == ' ' || c == '(' || c == '\n' || c == '\t');
  in.putback(c);

  while (true) {
    c = in.get();
    if (in.eof() || in.bad()) {
      return token;
    }

    if (c == ' ' || c == ')' || c == '\n' || c == '\t') {
      in.putback(c);
      return token;
    }
    token += c;
  }
}

std::optional<MediumType> MediumType::deserialize(std::istream &in) {
  auto type = read_token(in);
  if (type == "Boundary") {
    if (in.get() != ')')
      return {};

    return MediumType::Boundary();
  } else if (type == "Medium") {
    auto ior_s = read_token(in);
    if (in.get() != ')')
      return {};

    return MediumType::Medium(std::stof(ior_s));
  } else {
    // unknown medium type
    return {};
  }
}

void SimObject::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                              bool active) const {
  // by default, don't draw any controls
}

bool SimObject::handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) {
  // by default, don't use any events
  return false;
}

bool SimObject::draw_imgui_controls() {
  ImGui::Text("Selected object has no properties.");
  ImGui::Separator();
  return ImGui::Button("Delete Object");
}

static std::tuple<float, float> read_coord(std::istream &in) {
  float x = std::stof(read_token(in));
  float y = std::stof(read_token(in));

  return {x, y};
}

static std::tuple<float, float, float, float> read_coord_pair(std::istream &in) {
  auto [x0, y0] = read_coord(in);
  auto [x1, y1] = read_coord(in);

  return {x0, y0, x1, y1};
}

std::optional<std::unique_ptr<SimObject>> SimObject::deserialize(std::istream &in) {
  auto object = read_token(in);
  if (object == "AreaClear") {
    if (in.get() != ')')
      return {};
    return std::make_unique<AreaClear>();
  } else if (object == "Rectangle") {
    auto [x0, y0, x1, y1] = read_coord_pair(in);

    auto medium = MediumType::deserialize(in);
    if (!medium || in.get() != ')')
      return {};

    return std::make_unique<Rectangle>(x0, y0, x1, y1, *medium);
  } else if (object == "Line") {
    auto [x0, y0, x1, y1] = read_coord_pair(in);
    auto width = std::stof(read_token(in));

    auto medium = MediumType::deserialize(in);
    if (!medium || in.get() != ')')
      return {};

    return std::make_unique<Line>(x0, y0, x1, y1, width, *medium);
  } else if (object == "PointSource") {
    auto [x, y] = read_coord(in);
    auto waveform = Waveform::deserialze(in);
    auto phase = std::stof(read_token(in));
    if (!waveform || in.get() != ')')
      return {};

    return std::make_unique<PointSource>(x, y, std::move(waveform.value()), phase);
  } else if (object == "LineSource") {
    auto [x0, y0, x1, y1] = read_coord_pair(in);
    auto width = std::stof(read_token(in));
    auto waveform = Waveform::deserialze(in);
    auto phase = std::stof(read_token(in));
    if (!waveform || in.get() != ')')
      return {};

    return std::make_unique<LineSource>(x0, y0, x1, y1, width, std::move(waveform.value()), phase);
  } else {
    std::cout << "Object: " << object << ":\n";
    // unrecognized object
    return {};
  }
}

void Environment::draw(const Programs &programs, glm::vec2 physical_scale_factor,
                       float time) const {
  for (const auto &obj : objects) {
    obj->draw(programs, physical_scale_factor, time);
  }
}

void Environment::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor) const {
  for (size_t i = 0; i < objects.size(); i++) {
    objects[i]->draw_controls(programs, physical_scale_factor, (long int)i == active_object);
  }
}

void Environment::handle_events(glm::vec2 delta_x, glm::vec2 screen_size) {
  // allow active object to capture events first
  if (active_object >= 0 && active_object < (long int)objects.size()) {
    // check if the events cause deactivation
    if (!objects[active_object]->handle_events(delta_x, true, screen_size)) {
      active_object = -1;
    }
    // active object stayed active, so no need to pass events to other objects
    else {
      return;
    }
  }

  // check events on each object, stopping if the events make an object active
  for (size_t i = objects.size(); i-- > 0;) {
    if (objects[i]->handle_events(delta_x, false, screen_size)) {
      active_object = i;
      break;
    }
  }
}

void Environment::draw_imgui_controls() {
  if (active_object >= 0 && active_object < (long int)objects.size()) {
    if (objects[active_object]->draw_imgui_controls()) {
      objects.erase(objects.begin() + active_object);
      active_object = -1;
    }
  } else {
    ImGui::Text("No object selected.");
  }
}

bool Environment::has_active_object() const {
  return active_object >= 0 && active_object < (long int)objects.size();
}

std::string Environment::serialize() const {
  std::string res = "";
  for (const auto &obj : objects) {
    res += obj->serialize() + "\n";
  }
  return res;
}

std::optional<Environment> Environment::deserialize(std::istream &in) {
  Environment res;

  while (true) {
    // handle eof or whitespace
    while (true) {
      char c = in.get();
      // return if we reached end of file
      if (in.eof() || in.bad()) {
        return res;
      }
      // stop at anything that is not whitespace
      else if (c != ' ' && c != '\n' && c != '\t') {
        in.putback(c);
        break;
      }
    }
    // try to read object
    auto obj = SimObject::deserialize(in);
    if (!obj)
      return {};

    res.objects.push_back(std::move(obj.value()));
  }
}

// transform a square at (0, 0) with length 1 to a rectangle with corners (x0, y0), (x1, y1)
static glm::mat4 transform_rect(float x0, float y0, float x1, float y1,
                                glm::vec2 physical_scale_factor) {
  return glm::scale(glm::translate(glm::mat4(1.0f),
                                   glm::vec3(x0, y0, 0.0) * glm::vec3(physical_scale_factor, 1.0)),
                    glm::vec3(x1 - x0, y1 - y0, 1.0) * glm::vec3(physical_scale_factor, 1.0));
}

void Rectangle::draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const {
  medium.set_gl_program(programs);

  glUniformMatrix4fv(programs.object_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(transform_rect(x0, y0, x1, y1, physical_scale_factor)));
  programs.geo.draw_geo(GeometryType::Square);
}

// create the transformation matrix for a translation from the origin to the specified x and y
static glm::mat4 translate_to_point(float x, float y, glm::vec2 physical_scale_factor) {
  return glm::translate(glm::mat4(1.0f),
                        glm::vec3(x, y, 0.0) * glm::vec3(physical_scale_factor, 1.0));
}

// draw a gl point at the given physical coordinates
static void draw_point(const Programs &programs, float x, float y,
                       glm::vec2 physical_scale_factor) {
  glUniformMatrix4fv(programs.object_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(translate_to_point(x, y, physical_scale_factor)));
  programs.geo.draw_geo(GeometryType::Point);
}

const int rectangle_handle_size = 12;

void Rectangle::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                              bool active) const {
  glUseProgram(programs.handle_program);
  glPointSize(rectangle_handle_size);
  glUniform1i(programs.handle_hole_loc, 0);
  glUniform1i(programs.handle_selected_loc, active);
  // draw handles at corners of rectangle
  draw_point(programs, x0, y0, physical_scale_factor);
  draw_point(programs, x0, y1, physical_scale_factor);
  draw_point(programs, x1, y0, physical_scale_factor);
  draw_point(programs, x1, y1, physical_scale_factor);
  // draw outline of rectangle
  glUniformMatrix4fv(programs.object_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(transform_rect(x0, y0, x1, y1, physical_scale_factor)));
  programs.geo.draw_geo(GeometryType::SquareLine);
}

// check if a mouse position is within the given pixel rectangle
static bool pixel_pos_in_rect(ImVec2 pos, glm::vec2 c0, glm::vec2 c1) {
  return pos.x >= c0.x && pos.x < c1.x && pos.y >= c0.y && pos.y < c1.y;
}

// check if a mouse position is within a handle
static bool pixel_pos_in_handle(ImVec2 pos, glm::vec2 pixel_handle_center, int handle_size) {
  return pixel_pos_in_rect(pos,
                           pixel_handle_center - glm::vec2(handle_size / 2.0, handle_size / 2.0),
                           pixel_handle_center + glm::vec2(handle_size / 2.0, handle_size / 2.0));
}

static glm::vec2 physical_pos_to_pixel(float x, float y, glm::vec2 delta_x, glm::vec2 screen_size) {
  return glm::vec2(x / delta_x.x + screen_size.x / 2.0, -y / delta_x.y + screen_size.y / 2.0);
}

static glm::vec2 pixel_pos_to_physical(const ImVec2 &mouse_pos, glm::vec2 delta_x,
                                       glm::vec2 screen_size) {
  return glm::vec2(
      (glm::clamp(mouse_pos.x, 0.f, (float)screen_size.x) - screen_size.x / 2.0) * delta_x.x,
      -(glm::clamp(mouse_pos.y, 0.f, (float)screen_size.y) - screen_size.y / 2.0) * delta_x.y);
}

// handle events for a handle located at physical coordinates (x, y)
static bool handle_handle_events(glm::vec2 delta_x, bool active, float &x, float &y,
                                 int handle_size, glm::vec2 screen_size) {
  if (!ImGui::IsMousePosValid()) {
    return active;
  }

  const auto &mouse_pos = ImGui::GetMousePos();
  // if mouse was clicked this frame
  const bool clicked = ImGui::IsMouseClicked(0);
  // if mouse is being dragged this frame
  const bool dragged = ImGui::IsMouseDragging(0);

  bool mouse_in_handle = pixel_pos_in_handle(
      mouse_pos, physical_pos_to_pixel(x, y, delta_x, screen_size), handle_size);

  if (clicked && mouse_in_handle) {
    active = true;
  }

  if (active) {
    if (dragged) {
      // handle is being dragged, so adjust positioning
      const auto &new_pos = pixel_pos_to_physical(mouse_pos, delta_x, screen_size);
      x = new_pos.x;
      y = new_pos.y;
    } else {
      // if a click happened outside this handle, deactivate
      if (clicked && !mouse_in_handle) {
        active = false;
      }
    }
  }

  return active;
}

bool Rectangle::handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) {
  float *corners[4][2] = {{&x0, &y0}, {&x1, &y0}, {&x0, &y1}, {&x1, &y1}};
  // check if event effected handle
  for (int i = 0; i < 4; i++) {
    if (handle_handle_events(delta_x, active_handle == i, *corners[i][0], *corners[i][1],
                             rectangle_handle_size, screen_size)) {
      active_handle = i;
      return true;
    }
  }

  active_handle = -1;

  // check if event effected body of rectangle
  const bool mouse_clicked = ImGui::IsMouseClicked(0);

  const auto &mouse_pos = ImGui::GetMousePos();
  const bool mouse_in_body = pixel_pos_in_rect(
      mouse_pos, physical_pos_to_pixel(glm::min(x0, x1), glm::max(y0, y1), delta_x, screen_size),
      physical_pos_to_pixel(glm::max(x0, x1), glm::min(y0, y1), delta_x, screen_size));

  // handle body select / deselect
  if (mouse_clicked) {
    active = mouse_in_body;
  }

  if (active && ImGui::IsMouseDragging(0)) {
    // handle dragging of body
    const auto &drag = ImGui::GetMouseDragDelta(0);
    ImGui::ResetMouseDragDelta(0);

    float dx = drag.x * delta_x.x;
    float dy = -drag.y * delta_x.y;

    x0 += dx;
    x1 += dx;
    y0 += dy;
    y1 += dy;
  }

  return active;
}

static void imgui_point_input(const char *label, float &x, float &y) {
  float p0[2] = {x, y};
  ImGui::DragFloat2(label, p0, 0.5);
  x = p0[0];
  y = p0[1];
}

bool Rectangle::draw_imgui_controls() {
  medium.draw_imgui_controls();

  imgui_point_input("Corner 0 (m)", x0, y0);
  imgui_point_input("Corner 1 (m)", x1, y1);

  ImGui::Separator();
  return ImGui::Button("Delete Object");
}

std::string Rectangle::serialize() const {
  return "(Rectangle " + std::to_string(x0) + " " + std::to_string(y0) + " " + std::to_string(x1) +
         " " + std::to_string(y1) + " " + medium.serialize() + ")";
}

// get the transformation matrix for drawing a line from (x0, y) to (x1, y1)
static glm::mat4 transform_line(float x0, float y0, float x1, float y1,
                                glm::vec2 physical_scale_factor) {
  // line geometry has points (0, 0) and (1, 0)
  // we transform these to (x0, y0) and (x1, y1)
  glm::vec2 p0 = glm::vec2(x0, y0) * physical_scale_factor;
  glm::vec2 p1p0 = glm::vec2(x1 - x0, y1 - y0) * physical_scale_factor;
  return glm::mat4(p1p0.x, p1p0.y, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, p0.x, p0.y,
                   0.0, 1.0);
}

static void draw_line(const Programs &programs, float x0, float y0, float x1, float y1,
                      glm::vec2 physical_scale_factor) {
  glUniformMatrix4fv(programs.object_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(transform_line(x0, y0, x1, y1, physical_scale_factor)));
  programs.geo.draw_geo(GeometryType::Line);
}

void LineBase::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor, bool active,
                             bool draw_holes) const {
  glUseProgram(programs.handle_program);
  glPointSize(rectangle_handle_size);
  glUniform1i(programs.handle_hole_loc, draw_holes);
  glUniform1i(programs.handle_selected_loc, active);

  // draw handles at points of line
  draw_point(programs, x0, y0, physical_scale_factor);
  draw_point(programs, x1, y1, physical_scale_factor);

  // draw line
  glLineWidth(width + 1);
  draw_line(programs, x0, y0, x1, y1, physical_scale_factor);
}

bool LineBase::handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) {
  float *corners[2][2] = {{&x0, &y0}, {&x1, &y1}};
  // check if event effected handle
  for (int i = 0; i < 2; i++) {
    if (handle_handle_events(delta_x, active_handle == i, *corners[i][0], *corners[i][1],
                             rectangle_handle_size, screen_size)) {
      active_handle = i;
      return true;
    }
  }

  active_handle = -1;
  return active && !ImGui::IsMouseClicked(0);
}

void LineBase::imgui_line_controls() {
  imgui_point_input("Point 0 (m)", x0, y0);
  imgui_point_input("Point 1 (m)", x1, y1);
  ImGui::SliderFloat("Width (px)", &width, 1, 5);
}

std::string LineBase::serialize_coordinates() const {
  return std::to_string(x0) + " " + std::to_string(y0) + " " + std::to_string(x1) + " " +
         std::to_string(y1) + " " + std::to_string(width);
}

void Line::draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const {
  medium.set_gl_program(programs);
  glLineWidth(width);
  draw_line(programs, x0, y0, x1, y1, physical_scale_factor);
}

void Line::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                         bool active) const {
  LineBase::draw_controls(programs, physical_scale_factor, active, false);
}

bool Line::draw_imgui_controls() {
  medium.draw_imgui_controls();
  imgui_line_controls();
  ImGui::Separator();
  return ImGui::Button("Delete Object");
}

std::string Line::serialize() const {
  return "(Line " + serialize_coordinates() + " " + medium.serialize() + ")";
}

void AreaClear::draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const {
  glUseProgram(programs.object_program);
  glUniform4f(programs.object_object_props_loc, 0.0, 0.0, 1.0, 0.0);

  glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);

  glUniformMatrix4fv(programs.object_transform_loc, 1, GL_FALSE,
                     glm::value_ptr(GeometryManager::square_screen_cover_transform));
  programs.geo.draw_geo(GeometryType::Square);
}

std::string AreaClear::serialize() const { return "(AreaClear)"; }

void Waveform::set_gl_program(const Programs &programs, float time, float phase) const {
  glUseProgram(programs.object_program);
  glUniform4f(programs.object_object_props_loc, sample(time, phase), sample_diff(time, phase), 0.0,
              0.0);

  glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE);
}

void Waveform::draw_imgui_controls(std::unique_ptr<Waveform> &waveform, const char *label,
                                   const char *type_label) {
  ImGui::Text("%s", label);
  const char *waveform_type_names[4] = {"Sine", "Triangle", "Square", "Pulse"};
  int waveform_type = waveform->waveform_type_index();
  if (ImGui::Combo(type_label, &waveform_type, waveform_type_names,
                   IM_ARRAYSIZE(waveform_type_names))) {
    if (waveform_type != waveform->waveform_type_index()) {
      const auto &prev_freq_amp = waveform->get_freq_amp();
      switch (waveform_type) {
      case 0:
        waveform = std::make_unique<SineWaveform>(prev_freq_amp.second, prev_freq_amp.first);
        break;
      case 1:
        waveform = std::make_unique<TriangleWaveform>(prev_freq_amp.second, prev_freq_amp.first);
        break;
      case 2:
        waveform = std::make_unique<SquareWaveform>(prev_freq_amp.second, prev_freq_amp.first);
        break;
      case 3:
        waveform = std::make_unique<GaussianEnvelope>(std::move(waveform), 1.0, 0.0);
      }
    }
  }

  waveform->draw_imgui_prop_controls();
}

std::pair<float, float> Waveform::get_freq_amp() const { return {1.0, 1.0}; }

std::optional<std::unique_ptr<Waveform>> Waveform::deserialze(std::istream &in) {
  auto type = read_token(in);

  auto amp_s = read_token(in);
  auto freq_s = read_token(in);

  float amp = std::stof(amp_s);
  float freq = std::stof(freq_s);

  if (type == "Sine" || type == "Square" || type == "Triangle") {
    if (in.get() != ')') {
      return {};
    }

    if (type == "Sine") {
      return std::make_unique<SineWaveform>(amp, freq);
    } else if (type == "Square") {
      return std::make_unique<SquareWaveform>(amp, freq);
    } else if (type == "Triangle") {
      return std::make_unique<TriangleWaveform>(amp, freq);
    }
  } else if (type == "GaussianEnvelope") {
    auto waveform = Waveform::deserialze(in);
    if (!waveform || in.get() != ')') {
      return {};
    }

    return std::make_unique<GaussianEnvelope>(std::move(waveform.value()), amp, freq);
  }

  // unrecognized waveform type
  return {};
}

float SineWaveform::sample(float time, float phase) const {
  return amp * sin(2.0 * PI * (freq * time + phase));
}

float SineWaveform::sample_diff(float time, float phase) const {
  return amp * 2.0 * PI * freq * cos(2.0 * PI * (freq * time + phase));
}

void SineWaveform::draw_imgui_prop_controls() {
  ImGui::InputFloat("Frequency (Hz)", &freq, 0.0f, 0.0f, "%e");
  ImGui::InputFloat("Amplitude", &amp, 0.0f, 0.0f, "%e");
}

int SineWaveform::waveform_type_index() { return 0; }

std::pair<float, float> SineWaveform::get_freq_amp() const { return {freq, amp}; }

std::string SineWaveform::serialize() const {
  return "(Sine " + std::to_string(amp) + " " + std::to_string(freq) + ")";
}

float TriangleWaveform::sample(float time, float phase) const {
  time = std::fmod(freq * time + phase, 1.0);
  if (time <= 0.25) {
    return amp * (time * 4.0);
  } else if (time <= 0.75) {
    return amp * (1.0 - 4.0 * (time - 0.25));
  } else {
    return amp * (-1.0 + 4.0 * (time - 0.75));
  }
}

float TriangleWaveform::sample_diff(float time, float phase) const {
  time = std::fmod(freq * time + phase, 1.0);
  if (time < 0.25 || time > 0.75) {
    return 4.0 * freq;
  } else if (time > 0.25 && time < 0.75) {
    return 4.0 * freq;
  } else {
    // triangle wave isn't differentiable at peaks
    // 0 is used because it keeps the peak stable if waveform stops driving u
    return 0.0;
  }
}

void TriangleWaveform::draw_imgui_prop_controls() {
  ImGui::InputFloat("Frequency (Hz)", &freq, 0.0f, 0.0f, "%e");
  ImGui::InputFloat("Amplitude", &amp, 0.0f, 0.0f, "%e");
}

int TriangleWaveform::waveform_type_index() { return 1; }

std::pair<float, float> TriangleWaveform::get_freq_amp() const { return {freq, amp}; }

std::string TriangleWaveform::serialize() const {
  return "(Triangle " + std::to_string(amp) + " " + std::to_string(freq) + ")";
}

float SquareWaveform::sample(float time, float phase) const {
  time = std::fmod(freq * time + phase, 1.0);
  if (time < 0.5) {
    return amp;
  } else {
    return -amp;
  }
}

float SquareWaveform::sample_diff(float time, float phase) const {
  // waveform isn't differentiable at 0, 0.5, 1, ...
  // derivative is 0 everywhere else
  return 0.0;
}

void SquareWaveform::draw_imgui_prop_controls() {
  ImGui::InputFloat("Frequency (Hz)", &freq, 0.0f, 0.0f, "%e");
  ImGui::InputFloat("Amplitude", &amp, 0.0f, 0.0f, "%e");
}

int SquareWaveform::waveform_type_index() { return 2; }

std::pair<float, float> SquareWaveform::get_freq_amp() const { return {freq, amp}; }

std::string SquareWaveform::serialize() const {
  return "(Square " + std::to_string(amp) + " " + std::to_string(freq) + ")";
}

float GaussianEnvelope::gaussian(float x) const {
  float mu = duration_95 / 2.0 + start_t;
  float sigma = duration_95 / 4.0;

  return std::exp(-1.0 / 2 * std::pow((x - mu) / sigma, 2.0));
}

float GaussianEnvelope::gaussian_diff(float x) const {
  float mu = duration_95 / 2.0 + start_t;
  float sigma = duration_95 / 4.0;

  return (mu - x) / sigma * std::exp(-1.0 / 2 * std::pow((x - mu) / sigma, 2.0));
}

float GaussianEnvelope::sample(float time, float phase) const {
  return gaussian(time) * waveform->sample(time, phase);
}

float GaussianEnvelope::sample_diff(float time, float phase) const {
  return gaussian(time) * waveform->sample_diff(time, phase) +
         gaussian_diff(time) * waveform->sample(time, phase);
}

void GaussianEnvelope::draw_imgui_prop_controls() {
  ImGui::DragFloat("Duration (s)", &duration_95);
  ImGui::DragFloat("Start offset (s)", &start_t);
  ImGui::NewLine();

  Waveform::draw_imgui_controls(waveform, "Component waveform:", "Component Type");
}

int GaussianEnvelope::waveform_type_index() { return 3; }

std::pair<float, float> GaussianEnvelope::get_freq_amp() const { return waveform->get_freq_amp(); }

std::string GaussianEnvelope::serialize() const {
  return "(GaussianEnvelope " + std::to_string(duration_95) + " " + std::to_string(start_t) + " " +
         waveform->serialize() + ")";
}

void PointSource::draw(const Programs &programs, glm::vec2 physical_scale_factor,
                       float time) const {
  waveform->set_gl_program(programs, time, phase);
  glPointSize(1);
  draw_point(programs, x, y, physical_scale_factor);
}

const int point_handle_size = 16;

void PointSource::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                                bool active) const {
  glUseProgram(programs.handle_program);
  glPointSize(point_handle_size);
  glUniform1i(programs.handle_hole_loc, 1);
  glUniform1i(programs.handle_selected_loc, active);

  draw_point(programs, x, y, physical_scale_factor);
}

bool PointSource::handle_events(glm::vec2 delta_x, bool active, glm::vec2 screen_size) {
  return handle_handle_events(delta_x, active, x, y, point_handle_size, screen_size);
}

bool PointSource::draw_imgui_controls() {
  Waveform::draw_imgui_controls(waveform);
  ImGui::SliderFloat("Phase Shift", &phase, 0.0, 1.0);
  ImGui::NewLine();
  imgui_point_input("Position (m)", x, y);
  ImGui::NewLine();
  return ImGui::Button("Delete Object");
}

std::string PointSource::serialize() const {
  return "(PointSource " + std::to_string(x) + " " + std::to_string(y) + " " +
         waveform->serialize() + " " + std::to_string(phase) + ")";
}

void LineSource::draw(const Programs &programs, glm::vec2 physical_scale_factor, float time) const {
  waveform->set_gl_program(programs, time, phase);
  glLineWidth(width);
  draw_line(programs, x0, y0, x1, y1, physical_scale_factor);
}

void LineSource::draw_controls(const Programs &programs, glm::vec2 physical_scale_factor,
                               bool active) const {
  LineBase::draw_controls(programs, physical_scale_factor, active, true);
}

bool LineSource::draw_imgui_controls() {
  Waveform::draw_imgui_controls(waveform);
  ImGui::SliderFloat("Phase Shift", &phase, 0.0, 1.0);
  ImGui::NewLine();
  imgui_line_controls();
  ImGui::NewLine();
  return ImGui::Button("Delete Object");
}

std::string LineSource::serialize() const {
  return "(LineSource " + serialize_coordinates() + " " + waveform->serialize() + " " +
         std::to_string(phase) + ")";
}