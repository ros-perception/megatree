function glCheck()
{
  var err = ctx.getError();
  if (err != 0) {
    throw ("OpenGL error: " + err);
  }
}


var g_measurement_number = 1;
function DistanceMeasurement(begin, end, name)
{
  this.begin = begin;
  this.end = end;
  this.distance = V3.length(V3.sub(this.end, this.begin));
  this.mid = V3.scale(V3.add(this.begin, this.end), 0.5);

  if (name) {
    this.name = name;
  }
  else {
    this.name = "measurement-" + g_measurement_number;
    ++g_measurement_number;
  }

  this.gl = null;
  this.vbo = null;
  this.textTexture = null;
}

DistanceMeasurement.prototype.loadGL = function(gl, textureCanvas)
{
  this.gl = gl;

  // Creates the texture with the measurement text
  var c = textureCanvas.getContext('2d');
  c.fillStyle = "#ffff00";
  c.fillRect(5, 5, 100, 100);
  c.fillStyle = "#990000";
  c.textAlign = "center";
  c.textBaseline = "middle";
  c.font = "14px monospace bold";
  c.fillText("" + this.distance.toFixed(2) + " m", textureCanvas.width/2, textureCanvas.height/2);
  this.textTexture = createTextureFromCanvas(this.gl, textureCanvas);

  // Computes the 3d geometry of the measurement text.
  var mid = V3.scale(V3.add(this.begin, this.end), 0.5);
  

  // Prepares the vbo
  var data = new Float32Array(
    [ // Measurement line
      this.begin[0], this.begin[1], this.begin[2],  1.0, 0.0, 0.0,
      this.end[0], this.end[1], this.end[2],        1.0, 1.0, 0.0,
      // Quad with text
      -0.5, -0.5, 0.0,   0.0, 0.0,
      +0.5, -0.5, 0.0,   1.0, 0.0,
      -0.5, +0.5, 0.0,   0.0, 1.0,
      +0.5, +0.5, 0.0,   1.0, 1.0
    ]);

  this.vbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, this.vbo);
  gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);
  glCheck();
}

DistanceMeasurement.prototype.render = function(program)
{
  assert(this.vbo != null, "VBO is null");

  // Draws the line
  this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.vbo);
  this.gl.vertexAttribPointer(program.a.vertex, 3, this.gl.FLOAT, false, 6*4, 0);
  this.gl.vertexAttribPointer(program.a.color,  3, this.gl.FLOAT, false, 6*4, 3*4);
  this.gl.drawArrays(this.gl.LINES, 0, 2);
  glCheck();
}

DistanceMeasurement.prototype.renderText = function(program)
{
  assert(this.vbo != null, "VBO is null");

  var quadOffset = 12*4;

  // Draws the quad
  ctx.bindBuffer(ctx.ARRAY_BUFFER, this.vbo);
  ctx.vertexAttribPointer(program.a.vertex, 3, ctx.FLOAT, false, 5*4, quadOffset);
  ctx.vertexAttribPointer(program.a.textureCoord,  2, ctx.FLOAT, false, 5*4, quadOffset+3*4);
  ctx.activeTexture(ctx.TEXTURE0);
  ctx.bindTexture(ctx.TEXTURE_2D, this.textTexture);
  ctx.uniform1i(program.u.sampler, 0); 
  ctx.drawArrays(ctx.TRIANGLE_STRIP, 0, 4);
  glCheck();
}
