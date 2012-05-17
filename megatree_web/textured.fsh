#ifdef GL_ES
precision highp float;
#endif

varying vec2 vTextureCoord;
uniform sampler2D sampler;

void main(void){
  //gl_FragColor = texture2D(sampler, vec2(vTextureCoord.s, vTextureCoord.t));
  //gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
  //gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
  gl_FragColor.r = 1.0;
  gl_FragColor.g = 1.0;
  if (gl_FragColor.b != 1.0)
    gl_FragColor.b = 1.0;
  gl_FragColor.a = 1.0;

  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);

  gl_FragColor = texture2D(sampler, vTextureCoord.st);
}
