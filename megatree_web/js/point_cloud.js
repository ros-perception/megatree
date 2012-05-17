/*
 * Copyright (c) 2011 Willow Garage
 */

/*
 *  Class Point
 */
function Point(point, radius)
{
  this.point = point;
  this.radius = radius;
};

Point.prototype.intersect = function(origin, ray) {
  return Point.intersect(origin, ray, this.point, this.radius);
};

//return the intersection point between the point, with some radius, and a ray
Point.intersect = function(origin, ray, point, radius)
{
  var toPoint = origin.subtract(point);
  var a = V3.dot(ray, ray);
  var b = 2 * V3.dot(toPoint, ray);
  var c = V3.dot(toPoint, toPoint) - radius * radius;
  var discriminant = b * b - 4 * a * c;
  if(discriminant > 0)
  {
    var t = (-b - Math.sqrt(discriminant)) / (2 * a);
    if(t > 0)
    {
      return t;
    }
  }
  return Number.MAX_VALUE;
};

/*
 * Class Point Cloud
 */
function PointCloud()
{
  // The points and colors.  Each is an mjs V3
  this.vertices = [];
  this.colors = [];

  // WebGL stuff
  this.ctx = null;
  this.vertex_buffer = null;
  this.color_buffer = null;
};

function flattenV3Array(v3_array) {
  a = new Float32Array(3 * v3_array.length);
  for (var i in v3_array) {
    a[i*3+0] = v3_array[i][0];
    a[i*3+1] = v3_array[i][1];
    a[i*3+2] = v3_array[i][2];
  }
  return a;
}

PointCloud.prototype.setupGL = function(ctx)
{
  this.ctx = ctx;
  
  // Vertices
  this.vertex_buffer = ctx.createBuffer();
  ctx.bindBuffer(ctx.ARRAY_BUFFER, this.vertex_buffer);
  ctx.bufferData(ctx.ARRAY_BUFFER, flattenV3Array(this.vertices), ctx.STATIC_DRAW);
  this.vertex_buffer.itemSize = 3;
  this.vertex_buffer.numItems = this.vertices.length;

  // Colors
  this.color_buffer = ctx.createBuffer();
  ctx.bindBuffer(ctx.ARRAY_BUFFER, this.color_buffer);
  ctx.bufferData(ctx.ARRAY_BUFFER, flattenV3Array(this.colors), ctx.STATIC_DRAW);
  this.color_buffer.itemSize = 3;  // TODO: is this correct?
  this.color_buffer.numItems = this.color_buffer.length;
}

PointCloud.prototype.render = function(program)
{
  //console.log("Rendering point cloud");
  if (!this.ctx) {
    console.error("No WebGL context");
  }

  // Binds the vertex data
  this.ctx.bindBuffer(this.ctx.ARRAY_BUFFER, this.vertex_buffer);
  this.ctx.vertexAttribPointer(program.vertexPositionAttribute, this.vertex_buffer.itemSize, this.ctx.FLOAT, false, 0, 0);

  // Binds the color data
  this.ctx.bindBuffer(this.ctx.ARRAY_BUFFER, this.color_buffer);
  this.ctx.vertexAttribPointer(program.vertexColorAttribute, this.color_buffer.itemSize, this.ctx.FLOAT, false, 0, 0);

  this.ctx.drawArrays(this.ctx.POINTS, 0, this.vertex_buffer.numItems);
}
