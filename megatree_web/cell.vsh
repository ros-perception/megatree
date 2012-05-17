varying vec4 frontColor;

attribute vec3 ps_Vertex;
attribute vec3 ps_Color;

uniform float scale;
uniform vec3 camera_to_cell_in_world;
uniform float point_size_multiplier;
uniform float point_size_meters;
uniform float meters_to_pixels;
uniform float m;

// manually set color
uniform float override_color[3];

// show only part of this box
uniform int show_children[8];

// show selection
uniform vec3 selection_begin;
uniform vec3 selection_end;
uniform vec3 p_selection_camera;
uniform mat3 R_selection_world;

// camera and projection matrix
uniform mat3 R_camera_world;
uniform mat4 ps_ProjectionMatrix;

//for logarithmic depth buffer
uniform float far_plane_distance;
uniform float C;


void main(void) {

  // VERTEX
  // get the vertex in world (or cell, that's the same) coordinates
  vec3 cell_to_vertex_in_world = ps_Vertex * scale;

  // compute the offset between camera and vertex in world coordinates
  vec3 camera_to_vertex_in_world = camera_to_cell_in_world + cell_to_vertex_in_world;

  // compute the offset between camera and vertex in camera coordinates
  // we use a 0.0 at the end to just apply the rotation, we skip the large
  // numbers in the translation
  vec3 camera_to_vertex_in_camera = R_camera_world * camera_to_vertex_in_world;
  gl_Position = ps_ProjectionMatrix * vec4(camera_to_vertex_in_camera, 1.0);

  //we're going to use a logarithmic depth buffer to combat z-fighting
  //from http://www.gamasutra.com/blogs/BranoKemen/20090812/2725/Logarithmic_Depth_Buffer.php
  gl_Position.z = log(C * gl_Position.z + 1.0) / log(C * far_plane_distance + 1.0) * gl_Position.w;


  // COLOR
  if (override_color[0] == -1.0){
    frontColor = vec4(ps_Color, 1.0);
  }
  else{
    frontColor = vec4(override_color[0], override_color[1], override_color[2], 1.0);
  }

  vec3 vertex_rel = (R_selection_world * (camera_to_vertex_in_world + p_selection_camera)) - selection_begin;
  vec3 selection_end_rel = selection_end - selection_begin;
  if (vertex_rel[0] * selection_end_rel[0] > 0.0 && abs(vertex_rel[0]) < abs(selection_end_rel[0]) &&
      vertex_rel[1] * selection_end_rel[1] > 0.0 && abs(vertex_rel[1]) < abs(selection_end_rel[1]) &&
      vertex_rel[2] * selection_end_rel[2] > 0.0 && abs(vertex_rel[2]) < abs(selection_end_rel[2])){
    frontColor[1] = (frontColor[1] + 0.5) / 1.5;  // green selection
  }

  // distance between camera and vertex
  float dist = length(camera_to_vertex_in_camera);

  float point_size_meters_with_leaves = point_size_meters;

  // check if we need to show this vertex
  if (mod(255.0*ps_Color[2], 2.0) == 1.0)  // If the point has children (so it can be hidden)
  {
    int child = int(ps_Vertex.x > 0.5) * 4 + int(ps_Vertex.y > 0.5) * 2 + int(ps_Vertex.z > 0.5);
    if (show_children[child] == 0){
      gl_Position.z = 100.0;
      gl_Position.w = 1.0;
      //gl_PointSize = 3.0;
    }
  }
  else {  // It's a leaf (so it can't be hidden)
    point_size_meters_with_leaves = min(m * dist, point_size_meters);
  }

  gl_PointSize = point_size_multiplier * meters_to_pixels * point_size_meters_with_leaves / abs(camera_to_vertex_in_camera.z);
  //gl_PointSize = point_size_multiplier * meters_to_pixels * point_size_meters_with_leaves / dist;
  if (override_color[0] != -1.0){
    gl_PointSize *= 1.5;
  }
}
