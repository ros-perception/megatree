var ROOT3 = Math.sqrt(3);

function Cell(id, geometry, buffer)
{
  this.id = id;
  this.parent_id = id.substr(0, id.length-1);
  this.which_child = parseInt(id.charAt(id.length-1));
  this.children = new Uint8Array(buffer, 0, 1)[0];
  this.data = new Uint8Array(buffer, 1);

  this.geometry = geometry;
  this.scale = geometry.hi[0] - geometry.lo[0];
  this.node_size = this.scale / (1<<6);
  this.offset = V3.clone(geometry.lo);
  this.radius = this.scale / 2.0 * ROOT3;  // Bounding sphere radius

  this.override_color = [-1.0, 0.0, 0.0];

  this.ctx = null;
  this.vbo = null;
  this.parent_cell = null;
  this.inside_frustum = -1;
  this.show_children = 0;

  // for debugging, add a box to this cell
  // this.toBox();
}

Cell.prototype.shallowCopy = function() {
  c = new Object();
  c.__proto__ = Cell.prototype;
  c.id = this.id;
  c.parent_id = this.parent_id;
  c.which_child = this.which_child;
  c.children = this.children;
  c.data = this.data;
  c.geometry = this.geometry;
  c.scale = this.scale;
  c.node_size = this.node_size;
  c.offset = this.offset;
  c.radius = this.radius;
  c.override_color = this.override_color;
  c.ctx = this.ctx;
  c.vbo = this.vbo;
  c.parent_cell = this.parent_cell;

  // The ones we really care about
  c.inside_frustum = this.inside_frustum;
  c.show_children = this.show_children;
  return c;
}

Cell.prototype.toBox = function() {
  var new_data = new Uint8Array(this.data.length + 6*8);
  new_data.set(this.data);

  var count = this.data.length;
  //var n = [10, 245];
  var n = [120, 136];
  for (var child=0; child<8; child++){
    new_data[count+0] = child & (1<<X_BIT) ? n[1]:n[0];
    new_data[count+1] = child & (1<<Y_BIT) ? n[1]:n[0];
    new_data[count+2] = child & (1<<Z_BIT) ? n[1]:n[0];
    new_data[count+3] = this.hasChild(child) ? 0:255;
    new_data[count+4] = this.hasChild(child) ? 255:0;
    new_data[count+5] = 0;
    count += 6;
  }
  this.data = new_data;
}

Cell.prototype.getNumChildren = function() {
    var count = 0;
    for (var i=0; i<8; i++){
	if (this.hasChild(i)){
	    count++;
	}
    }
    return count;
}

Cell.prototype.getChildren = function() {
    return this.children;
}

Cell.prototype.hasChild = function(child) {
  return ((1 << child) & this.children) > 0;
}

Cell.prototype.isLeaf = function() {
  return this.children == 0;
}

Cell.prototype.getCenter = function() {
  return V3.$(this.offset[0] + this.scale / 2.0,
	      this.offset[1] + this.scale / 2.0,
	      this.offset[2] + this.scale / 2.0);
}

Cell.prototype.numPoints = function() {
  return this.data.length / 6;
}

// The point gets placed into r (if supplied) and returned.
Cell.prototype.getPoint = function(i, r) {
  if (r == undefined)
    r = V3.$(0, 0, 0);
  r[0] = this.data[6*i+0] * this.scale / 255.0 + this.offset[0];
  r[1] = this.data[6*i+1] * this.scale / 255.0 + this.offset[1];
  r[2] = this.data[6*i+2] * this.scale / 255.0 + this.offset[2];
  return r;
}

Cell.prototype.getPointInAlignedCamera = function(i, cam_position, r) {
  if (r == undefined)
    r = V3.$(0, 0, 0);
  //TODO The subtraction here might need to be done in double precision for
  //offset and cam_position
  r[0] = this.data[6*i+0] * this.scale / 255.0 + (this.offset[0] - cam_position[0]);
  r[1] = this.data[6*i+1] * this.scale / 255.0 + (this.offset[1] - cam_position[1]);
  r[2] = this.data[6*i+2] * this.scale / 255.0 + (this.offset[2] - cam_position[2]);
  return r;
}

Cell.prototype.pickValid = function(i){
  //check if the point is a leaf
  if((this.data[6*i + 5] & 1) == 1)
    return true;

  //otherwise, we have to check what octant the point is in
  //and see if we're showing children for that octant
  var child = ((this.data[6*i+0]>>7)<<2) & ((this.data[6*i+1]>>7)<<1) & (this.data[6*i+2]>>7);
  if((this.show_children & (1<<child)) == 0)
    return true;

  //if we're showing this child octanct, the summary point is not pickable
  return false;
}

Cell.prototype.loadGL = function(ctx) {
  this.ctx = ctx;

  this.vbo = ctx.createBuffer();
  ctx.bindBuffer(ctx.ARRAY_BUFFER, this.vbo);
  ctx.bufferData(ctx.ARRAY_BUFFER, this.data, ctx.STATIC_DRAW);
}

var MIN_POINT_SIZE = 0.001;  // In meters
Cell.prototype.render = function(program) {
  assert(this.vbo != null, "VBO is null");

  // Sets the uniforms
  var children = new Array();
  for (var i=0; i<8; i++){
    if ((this.show_children & (1<<i)) == 0){
      ctx.uniform1i(program.u.show_children[i], 0);
    }
    else{
      ctx.uniform1i(program.u.show_children[i], 1);
    }
  }
  for (var i=0; i<3; i++){
    ctx.uniform1f(program.u.override_color[i], this.override_color[i]);
  }      
  ctx.uniform1f(program.u.scale, this.scale);
  ctx.uniform3fv(program.u.cameraToCellInWorld, V3.sub(this.offset, cam.getPosition()));

  var pointSizeInMeters = Math.max(MIN_POINT_SIZE, this.scale / (1<<6));
  ctx.uniform1f(program.u.point_size_meters, pointSizeInMeters);

  // Binds the vertex and color data
  this.ctx.bindBuffer(this.ctx.ARRAY_BUFFER, this.vbo);
  this.ctx.vertexAttribPointer(program.a.vertex, 3, this.ctx.UNSIGNED_BYTE, true, 6, 0);
  this.ctx.vertexAttribPointer(program.a.color,  3, this.ctx.UNSIGNED_BYTE, true, 6, 3);

  this.ctx.drawArrays(this.ctx.POINTS, 0, this.data.length / 6);
}

Cell.prototype.dump = function() {
  console.log("CELL    " + this.scale + "  (" + this.offset[0] + ", " + this.offset[1] + ", " + this.offset[2] + ")");
  for (var i = 0; i < this.data.length; i += 6) {
    console.log("    (" +
		((this.data[i+0] * this.scale / 255.0) + this.offset[0]) + ", " +
		((this.data[i+1] * this.scale / 255.0) + this.offset[1]) + ", " +
		((this.data[i+2] * this.scale / 255.0) + this.offset[2]) + ")" + "      (" +
	       this.data[i+3] + ", " + this.data[i+4] + ", " + this.data[i+5] + ")");
  }
}
