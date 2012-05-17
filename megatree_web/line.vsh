varying vec4 frontColor;

attribute vec3 vertex;
attribute vec3 color;

// camera and projection matrix
uniform mat4 T_camera_world;
uniform mat4 ps_ProjectionMatrix;

//for logarithmic depth buffer
uniform float far_plane_distance;
uniform float C;

uniform float lift;  // Lifts the vertex slightly in the z buffer

void main(void) {

  // VERTEX

  // Transforms the vertex into clipping space
  vec4 p_camera = T_camera_world * vec4(vertex, 1.0);
  gl_Position = ps_ProjectionMatrix * p_camera;

  //we're going to use a logarithmic depth buffer to combat z-fighting
  //from http://www.gamasutra.com/blogs/BranoKemen/20090812/2725/Logarithmic_Depth_Buffer.php
  gl_Position.z = log(C * gl_Position.z + 1.0) / log(C * far_plane_distance + 1.0) * gl_Position.w + lift;


  // COLOR
  frontColor = vec4(color, 1.0);

  gl_PointSize = 20.0;
}
