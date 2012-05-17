function AssertException(message) { this.message = message; }
AssertException.prototype.toString = function () {
  return 'AssertException: ' + this.message;
}
function assert(condition, message) {
  if (!condition)
    throw new AssertException(message);
}

var explicitToArray = function () {
  var a = [];
  for (var i = 0; i < this.length; ++i) {
    a.push(this[i]);
  }
  return a;
}
Float32Array.prototype.toArray = explicitToArray;
Uint8Array.prototype.toArray = explicitToArray;
Array.prototype.toArray = function () { return this; }

function prettyBytes(b)
{
  var kib = b / 1024;
  var mib = kib / 1024;
  var gib = mib / 1024;
  if (gib > 1)
    return "" + gib.toFixed(1) + " GiB";
  if (mib > 1)
    return "" + mib.toFixed(1) + " MiB";
  if (kib > 1)
    return "" + kib.toFixed(1) + " KiB";
  return "" + b + " bytes";
}

// Asynchronous get of several files
function getMany(urls, successCallback, errorCallback)
{
  var numPending = urls.length;
  var numFailed = 0;
  var results = new Array(urls.length);
  for (var ii = 0; ii < urls.length; ++ii) {
    // Javascript's scoping is a pain
    (function (i) {
      $.get(urls[i], function (data) {
	results[i] = data;
	--numPending;

	if (numPending == 0) {
	  if (numFailed == 0) {
	    successCallback(results);
	  }
	  else {
	    errorCallback(results);
	  }
	}
      }).error(function () {
	++numFailed;
	--numPending;
	if (numPending == 0) {
	  errorCallback(results);
	}
      });
    })(ii);
  }
}



function getCanvasWebGLContext(canvas, ctxAttribs) {
  var contextNames = ["webgl","experimental-webgl", "moz-webgl","webkit-3d"];

  var ctx;
  for(var i = 0; i < contextNames.length; i++){
    try{
      ctx = canvas.getContext(contextNames[i], ctxAttribs);
      if(ctx){
        break;
      }
    }catch(e){}
  }
  if(!ctx){
    console.error("Your browser does not support WebGL.");
    return null;
  }

  //return WebGLDebugUtils.makeDebugContext(ctx);
  return ctx;
}

function MatrixStack() {
  this.stack = [M4x4.I];
}
MatrixStack.prototype.top = function() {
  return this.stack[this.stack.length - 1];
}
MatrixStack.prototype.peek = function () {
  return M4x4.clone(this.stack[this.stack.length - 1]);
}
MatrixStack.prototype.push = function () {
  this.stack.push(this.peek());
}
MatrixStack.prototype.pop = function () {
  this.stack.pop();
}
MatrixStack.prototype.load = function (mat) {
  this.stack[this.stack.length - 1] = mat;
}
MatrixStack.prototype.mult = function(mat) {
  this.load(M4x4.mul(this.peek(), mat));
}






// Constants
var TRIGGER_ON_VIEWPOINT_CHANGE = true;
var RUN_ON_CACHE_ONLY = false;

// Globals
var desired_cell_count = 0;
var custom_clipping = false;
var far_plane_distance = 10000000.0; //10,000 km
var near_plane_distance = 1.0; //1.0 meters
var trees_info = [];
var views_info = [];
var jobs_info = [];
var saved_views = [];
var first_cloud = true;
var box_size_px = 1.0;
var min_box_size_meters = 0.0;
var point_size_multiplier = 1.4142;
var fov = 20.0;
var is_first_cloud;
var canvas;
var ctx;
var selection_cloud;
var projectionMatrix;
var cam = null;

var cloudShader;
var defaultProgram;
var currProgram;
var oldProgram;
var lineShader;

var cloudRequestWidget;
var cloudReceiveWidget;
var cloudRequestCounter;
var cloudReceiveCounter;

var frustumQuerier;
var testBoxCell;
var currentCells;

var cellCache = new LRUCache(10000);

var vertexShaderSource =
  "varying vec4 frontColor;" +
  "attribute vec3 ps_Vertex;" +
  "attribute vec3 ps_Color;" +
  
  "uniform float ps_PointSize;" +
  "uniform float meters_to_pixels;" +
  "uniform vec3 ps_Attenuation;" +
  
  "uniform mat4 ps_ModelViewMatrix;" +
  "uniform mat4 ps_ProjectionMatrix;" +
  
  "void main(void) {" +
  "  if(ps_Vertex.z > 0.0){" +
  "  frontColor = vec4(ps_Color, 1.0);" +
  "  vec4 ecPos4 = ps_ModelViewMatrix * vec4(ps_Vertex, 1.0);" +
  "  float dist = length(ecPos4);" +
  "  float attn = ps_Attenuation[0] + " +
  "              (ps_Attenuation[1] * dist) + " + 
  "              (ps_Attenuation[2] * dist * dist);" +
  
  //"  gl_PointSize = (attn > 0.0) ? ps_PointSize * sqrt(1.0/attn) : 1.0;" +
  //"  gl_PointSize = ps_PointSize;" +
  //"  gl_PointSize = meters_to_pixels * 0.9 / abs(ecPos4.z);" +
  "  gl_Position = ps_ProjectionMatrix * ecPos4;" +
  "  gl_PointSize = 10.0;" +
  "  }" +
  "  else{" +
  "  gl_PointSize = 30.0;" +
  "  gl_Position = vec4(ps_Vertex.xy, -0.5, 1.0);" +
  "  frontColor = vec4(ps_Color, 1.0);" +
  "  }" +
  "}";


var fragmentShaderSource =
  '#ifdef GL_ES                 \n\
  precision highp float;        \n\
  #endif                        \n\
  \n\
  varying vec4 frontColor;      \n\
  void main(void){              \n\
  gl_FragColor = frontColor;  \n\
  }';


function createProgramObject(ctx, vetexShaderSource, fragmentShaderSource) {
  var vertexShaderObject = ctx.createShader(ctx.VERTEX_SHADER);
  ctx.shaderSource(vertexShaderObject, vetexShaderSource);
  ctx.compileShader(vertexShaderObject);
  if (!ctx.getShaderParameter(vertexShaderObject, ctx.COMPILE_STATUS)) {
    throw ctx.getShaderInfoLog(vertexShaderObject);
  }

  var fragmentShaderObject = ctx.createShader(ctx.FRAGMENT_SHADER);
  ctx.shaderSource(fragmentShaderObject, fragmentShaderSource);
  ctx.compileShader(fragmentShaderObject);
  if (!ctx.getShaderParameter(fragmentShaderObject, ctx.COMPILE_STATUS)) {
    throw ctx.getShaderInfoLog(fragmentShaderObject);
  }

  var programObject = ctx.createProgram();
  ctx.attachShader(programObject, vertexShaderObject);
  ctx.attachShader(programObject, fragmentShaderObject);
  ctx.linkProgram(programObject);
  if (!ctx.getProgramParameter(programObject, ctx.LINK_STATUS)) {
    throw "Error linking shaders.";
  }

  return programObject;
};


function populateShader(ctx, shader, attributes, uniforms)
{
  shader.a = new Object();  // Attributes
  shader.u = new Object();  // Uniforms

  // Assigns the attributes
  for (var i = 0; i < attributes.length; ++i)
  {
    shader.a[attributes[i]] = ctx.getAttribLocation(shader, attributes[i]);
    ctx.enableVertexAttribArray(shader.a[attributes[i]]);
    if (shader.a[attributes[i]] < 0) {
      console.warn("Shader could not get attribute " + attributes[i]);
      ctx.getError();
    }
  }

  // Assigns the uniforms
  for (var i = 0; i < uniforms.length; ++i)
  {
    shader.u[uniforms[i]] = ctx.getUniformLocation(shader, uniforms[i]);
    if (shader.u[uniforms[i]] < 0) {
      ctx.getError();
      throw new AssertionException("Shader could not get uniform " + uniforms[i]);
    }
  }
  glCheck();
}

window.requestAnimationFrame = (function(){
				  return window.requestAnimationFrame ||
				    window.webkitRequestAnimationFrame ||
				    window.mozRequestAnimationFrame ||
				    window.oRequestAnimationFrame ||
				    window.msRequestAnimationFrame ||
				    function(callback, cvs){
				      window.setTimeout(callback, 1000.0/60.0);
				    };
				})();
function animationLoop() {
  renderLoop();
  requestAnimationFrame(animationLoop, canvas);
}


// Assumes that ray is normalized.
function intersectCell(origin, ray, cell_center, cell_radius, radius) 
{
  // Optimized for speed.  Uses in-place operations and global
  // temporaries to avoid allocating new V3 objects.
  var toPoint = V3.sub(cell_center, origin, V3._temp1);
  var t = V3.dot(ray, toPoint);
  var projection = V3.scale(ray, t, V3._temp2);
  var rayToPoint = V3.sub(toPoint, projection, V3._temp3);
  var distanceToRay = V3.length(rayToPoint) - cell_radius;

  if (t > 0) {
    if ((distanceToRay / t) < radius) {
      //console.log("T = " + t + ", dist = " + distanceToRay + ", gives " + (distanceToRay/t));
      return t;
    }
  }
  return Number.MAX_VALUE;
}


// Assumes that ray is normalized.
function intersectPoint(origin, ray, point, radius) 
{
  // Optimized for speed.  Uses in-place operations and global
  // temporaries to avoid allocating new V3 objects.
  var toPoint = V3.sub(point, origin, V3._temp1);
  var t = V3.dot(ray, toPoint);
  var projection = V3.scale(ray, t, V3._temp2);
  var rayToPoint = V3.sub(toPoint, projection, V3._temp3);
  var distanceToRay = V3.length(rayToPoint);

  if (t > 0) {
    if ((distanceToRay / t) < radius) {
      //console.log("T = " + t + ", dist = " + distanceToRay + ", gives " + (distanceToRay/t));
      return t;
    }
  }
  return Number.MAX_VALUE;
}

/*
 * Uses globals (cam, projectionMatrix, canvas) to compute the pick ray.
 */
// returns a ray in a frame aligned with the world, and in the camera origin
function computePickRay(mouseX, mouseY) {
  //get the selected point in the world
  var image_to_camera = M4x4.inverse(projectionMatrix);

  var canvas = $("#canvas");
  var image_point = V3.$((mouseX / canvas.width()) * 2 - 1, 1 - (mouseY / canvas.height()) * 2, -0.5);

  //we want to make sure to compute the ray in a frame that is aligned with the world
  var camera_to_world_rotation = M4x4.topLeft3x3(M4x4.inverseOrthonormal(cam.getMatrix()));
  var cloud_pt = V3.mul3x3(camera_to_world_rotation, V3.mul4x4(image_to_camera, image_point));

  //TODO IT SEEMS LIKE THERE ARE PRECISION PROBLEMS IN GOING FROM IMAGE TO WORLD ON TAHOE
  //var image_point_2 = V3.mul4x4(world_to_image, cloud_pt);
  //var identity_we_hope = M4x4.mul(cam.getMatrix(), M4x4.inverseOrthonormal(cam.getMatrix()));
  
  //var pick_origin = cam.getPosition();
  var pick_origin = V3.$(0.0, 0.0, 0.0);
  var ray = V3.sub(cloud_pt, pick_origin);
  ray = V3.normalize(ray);

  return {point: pick_origin, ray: ray};
}



// returns a ray in a frame aligned with the world, and in the camera origin
function computePickFrustum(mouseX, mouseY) {
  //get the selected point in the world
  var image_to_camera = M4x4.inverse(projectionMatrix);

  var canvas = $("#canvas");
  var image_point = V3.$((mouseX / canvas.width()) * 2 - 1, 1 - (mouseY / canvas.height()) * 2, -0.5);

  //we want to make sure to compute the ray in a frame that is aligned with the world
  var camera_to_world_rotation = M4x4.topLeft3x3(M4x4.inverseOrthonormal(cam.getMatrix()));
  var cloud_pt = V3.mul3x3(camera_to_world_rotation, V3.mul4x4(image_to_camera, image_point));

  //var pick_origin = cam.getPosition();
  var pick_origin = V3.$(0.0, 0.0, 0.0);
  var ray = V3.sub(cloud_pt, pick_origin);
  ray = V3.normalize(ray);

  return {point: pick_origin, ray: ray};
}


/*
 * pickRay: {point, ray}
 */
// returns a point in a frame aligned with the world, and in the camera origin
function pickPointFromCells(min_dist, pickRay, cells) {
  //console.log("clipping plane dist " + min_dist);

  var start_time = new Date();
  var min_t = Number.MAX_VALUE;
  var hit_pt = undefined;

  var temp_pt = V3.$(0,0,0);
  var magic_ratio = Math.tan(Math.PI / 6.0 / ($("#canvas").height() / 4.0));
  var cam_position = cam.getPosition();
  for (var i=cells.length-1; i>=0; i--) {
    var cell = cells[i];
    var cell_center_aligned_camera = V3.sub(cell.getCenter(), cam_position);
    if (intersectCell(pickRay.point, pickRay.ray, cell_center_aligned_camera, cell.radius, magic_ratio) < Number.MAX_VALUE &&
	V3.length(cell_center_aligned_camera)-cell.radius < min_t){
      for (var j = 0; j < cell.numPoints(); ++j) {
        // TODO: Very inefficient.
        if(cell.pickValid(j))
        {
          //var pt = cell.getPoint(j, temp_pt);
          var pt = cell.getPointInAlignedCamera(j, cam_position, temp_pt);
          var t = intersectPoint(pickRay.point, pickRay.ray, pt, magic_ratio);
          if(t > min_dist * 2.0 && t < min_t){
            hit_pt = V3.clone(pt);
            min_t = t;
          }
        }
      }
    }
  }
  

  console.log("Picked a point in " + (new Date() - start_time) / 1000.0 + " seconds");
  return hit_pt;
}


function createTextureFromCanvas(gl, canvas)
{
  var texture = gl.createTexture();
  gl.pixelStorei(ctx.UNPACK_FLIP_Y_WEBGL, true);
  gl.bindTexture(ctx.TEXTURE_2D, texture);
  gl.texImage2D(ctx.TEXTURE_2D, 0, ctx.RGBA, ctx.RGBA, ctx.UNSIGNED_BYTE, canvas);
  gl.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_MAG_FILTER, ctx.LINEAR);
  gl.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_MIN_FILTER, ctx.LINEAR_MIPMAP_NEAREST);
  gl.generateMipmap(ctx.TEXTURE_2D);
  gl.bindTexture(ctx.TEXTURE_2D, null);
  return texture;
}


function renderAxis(T)
{
  var gl = ctx;
  if (!renderAxis.vbo)
  {
    var L = 10.0;
    var data = new Float32Array(
      [ 0, 0, 0,  1, 0, 0,
	L, 0, 0,  1, 0, 0,
	0, 0, 0,  0, 1, 0,
	0, L, 0,  0, 1, 0,
	0, 0, 0,  0, 0, 1,
	0, 0, L,  0, 0, 1,
      ]);
    renderAxis.num = data.length / 6;
    renderAxis.vbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, renderAxis.vbo);
    gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);
    glCheck();
  }
  
  gl.useProgram(lineShader);
  gl.uniformMatrix4fv(lineShader.u.T_camera_world, false, T);
  gl.uniform1f(lineShader.u.lift, 0);
  gl.bindBuffer(gl.ARRAY_BUFFER, renderAxis.vbo);
  gl.vertexAttribPointer(lineShader.a.vertex, 3, gl.FLOAT, false, 6*4, 0);
  gl.vertexAttribPointer(lineShader.a.color,  3, gl.FLOAT, false, 6*4, 3*4);
  gl.drawArrays(gl.LINES, 0, renderAxis.num);
  gl.useProgram(null);
  glCheck();
}

// Returns a transform T (pure rotation) for spherical billboards
//
// ModelView = T_cam_world * position * T
function makeBillboardSpher(T_cam_world)
{
  var billboardSpherR = M4x4.inverse(T_cam_world);
  billboardSpherR[12] = billboardSpherR[13] = billboardSpherR[14] = 0.0;
  return billboardSpherR;
}

var existing_frustum_trigger = null;
function triggerFrustumUpdate()
{
  if (cam != null){
    var box_size_meters = (2 * Math.tan((fov / 2.0) * Math.PI / 180) * box_size_px) / $("#canvas").height();
    //console.log("Box size meters " + box_size_meters);
    if(!existing_frustum_trigger)
    { 
      existing_frustum_trigger = 
        setTimeout(function () {
          frustumQuerier.getFrustum(trees_info, 
            cam.getMatrix(),
            function (cells, errors) {
            currentCells = cells;
            },
            function (cells, errors) {
            currentCells = cells;
            }, 
            box_size_meters);
          existing_frustum_trigger = null;
          }, 15);
    }
  }
}


function toggleTreeMode(evt, tree)
{
  if (trees_info.length >= (tree+1)){
      if (evt.shiftKey){
	  console.log("pressed key: shift-" + (tree+1));
	  trees_info[tree].enabled = true;
	  trees_info[tree].override_color = !trees_info[tree].override_color;
      }
      else{
	  console.log("pressed key: " + (tree+1)); 
	  trees_info[tree].enabled = !trees_info[tree].enabled;
      }
      triggerFrustumUpdate();
  }
}

function setProjectionMatrix()
{
  var canvas = $("#canvas");
  projectionMatrix = M4x4.makePerspective(fov, $("#canvas").width() / $("#canvas").height(), near_plane_distance, far_plane_distance);
  //projectionMatrix = M4x4.makePerspectiveToZero(fov, canvas.width(), canvas.height(), Math.max(1.0, near_plane_distance));
  ctx.useProgram(currProgram);
  ctx.uniformMatrix4fv(currProgram.u.projectionMatrix, false, projectionMatrix);

  ctx.useProgram(oldProgram);
  ctx.uniformMatrix4fv(oldProgram.projectionMatrixUniform, false, projectionMatrix);

  ctx.useProgram(lineShader);
  ctx.uniformMatrix4fv(lineShader.u.ps_ProjectionMatrix, false, projectionMatrix);

  ctx.useProgram(texturedShader);
  ctx.uniformMatrix4fv(texturedShader.u.ps_ProjectionMatrix, false, projectionMatrix);

  ctx.useProgram(currProgram);

  // recompute frustum planes
  frustumQuerier.setProjectionMatrix(M4x4.makePerspective(fov + 2, $("#canvas").width() / $("#canvas").height(), near_plane_distance, far_plane_distance));
  triggerFrustumUpdate();
}

// ---------- Keyboard stuff


var WALK_TO = 13;
function onKeyDown(evt) {
  //console.log("Key pressed");

  switch (evt.keyCode) {
  case 49:  // 1
    toggleTreeMode(evt, 0);
    break; 
  case 50:  // 2
    toggleTreeMode(evt, 1);
    break; 
  case 51:  // 3
    toggleTreeMode(evt, 2);
    break; 
  case 52:  // 4
    toggleTreeMode(evt, 3);
    break; 
  case 53:  // 5
    toggleTreeMode(evt, 4);
    break; 
  case 54:  // 6
    toggleTreeMode(evt, 5);
    break; 
  case 55:  // 7
    toggleTreeMode(evt, 6);
    break; 
  case 56:  // 8
    toggleTreeMode(evt, 7);
    break; 
  case 57:  // 9
    toggleTreeMode(evt, 8);
    break; 
  case 67: // c
    RUN_ON_CACHE_ONLY = !RUN_ON_CACHE_ONLY;
    console.log("Run on cache only set to " + RUN_ON_CACHE_ONLY);
    break;
  case 70: // f
    triggerFrustumUpdate();
    break;
  case 76: // l
    var url = document.location.href.split("?")[0] + "?tree=";
    for (var t=0; t<trees_info.length; t++){
      if (trees_info[t]){
        console.log("Adding " + trees_info[t].name);
        url += trees_info[t].name + ",";
      }
    }
    url = url.substr(0, url.length-1);
    url += "&c_x="+cam.origin[0]+"&c_y="+cam.origin[1]+"&c_z="+cam.origin[2]
          +"&c_d="+cam.distance+"&c_pi="+cam._pitch+"&c_ya="+cam._yaw;
    $.prompt(url);
    break;
  case 78: // n
    walkN(WALK_TO, function(cells) {
      console.log("Received a tree with " + cells.length + " cells");
      for (var i = 0; i < cells.length; ++i) {
	cells[i].loadGL(ctx);
      }
      currentCells = cells;
    });
    break;
  case 73: // i
    custom_clipping = false;
    near_plane_distance = 1.0;
    setProjectionMatrix();
    break;
  case 79: // o
    near_plane_distance -= 0.5;
    near_plane_distance = Math.max(1.0, near_plane_distance);
    setProjectionMatrix();
    if(near_plane_distance > 1.0)
      custom_clipping = true;
    else
      custom_clipping = false;
    break;
  case 80: // p
    near_plane_distance += 0.5;
    setProjectionMatrix();
    if(near_plane_distance > 1.0)
      custom_clipping = true;
    else
      custom_clipping = false;
    break;
  case 27: // esc
    clearSelection();
    break;
  }

}

// ---------- Mouse stuff
function getCursorPosition(e){
  var x;
  var y;
  if (e.pageX || e.pageY) { 
    x = e.pageX;
    y = e.pageY;
  }
  else { 
    x = e.clientX + document.body.scrollLeft + document.documentElement.scrollLeft; 
    y = e.clientY + document.body.scrollTop + document.documentElement.scrollTop; 
  } 
  x -= canvas.offsetLeft;
  y -= canvas.offsetTop;

  return [x, y];
}

var allMeasurements = [];
var distanceMeasurementStart = null;
var selection = null;
var mousePosition = [0, 0];
var mouseButtonDown = 0;
var mev;
function onMouseDown (evt) {
  // set focus back to the canvas
  $('#canvas').focus();

  // stop any automated camera motion
  cam.stopProfile();

  // get mouse coordinates
  var cursor_coords = getCursorPosition(evt);
  mousePosition = getCursorPosition(evt);

  // Pick a point
  if ((evt.which == 1 && evt.shiftKey) || evt.which == 3){  
    var pickRay = computePickRay(cursor_coords[0], cursor_coords[1]);
    var pickedPoint = pickPointFromCells(Math.max(1.0, near_plane_distance), pickRay, currentCells);
    var world_pick_point = V3.add(pickedPoint, cam.getPosition());
  }
  else{
    console.log("Failed to select a point");
  }

  // pan//tilt/strafe
  if ((evt.which == 1 || evt.which == 2) && !evt.shiftKey){  
    mouseButtonDown = evt.which;
  }

  // Pick a point to use for selection
  if (evt.which == 1 && evt.shiftKey && pickedPoint){  
    mouseButtonDown = 4;
    var R_camera_world = M4x4.topLeft3x3(cam.getMatrix());
    var p_world_camera = cam.getPosition();
    selection = new Selection(pickedPoint, R_camera_world, p_world_camera);

    if (distanceMeasurementStart)
    {
      // Completes the distance measurement
      var distanceMeasurementEnd = selection.getPoint();
      var m = new DistanceMeasurement(distanceMeasurementStart, distanceMeasurementEnd);
      allMeasurements.push(m);
      console.log("Distance: " + m.distance + " meters");
      distanceMeasurementStart = null;
    }
    else
    {
      distanceMeasurementStart = selection.getPoint();
    }
    //draw the point for debugging
    /*
    dummy_cloud.vertices[0] = world_pick_point;
    dummy_cloud.colors[0] = [1.0, 0.0, 0.0];
    dummy_cloud.vertices[1] = V3.add(cam.getPosition(), V3.scale(pickRay.ray, 10.0));
    dummy_cloud.colors[1] = [0.0, 1.0, 1.0];
    dummy_cloud.setupGL(ctx);
    */    
  }

  // Get google maps link
  if (evt.which == 3 && evt.shiftKey && pickedPoint){
    lat_lon = utm_to_lat_long(world_pick_point);
    $.prompt("See this place in <a href='http://maps.google.com/maps?q=" + lat_lon[0] + " " + lat_lon[1] + "' target='_blank'>Google Maps</a>", {opacity: 0.8});
  } 

  // Re-center camera on point
  if (evt.which == 3 && !evt.shiftKey && pickedPoint){
    var goal_cam = new OrbitCam({distance: V3.length(pickedPoint), origin: world_pick_point, yaw: cam._yaw, pitch: cam._pitch});
    cam.goto(goal_cam, 4.0);  // quad speed
  }
}



function onMouseUp (evt) {
  switch (mouseButtonDown){
    case 2:
      cam.smoothMotion();
      break;
    case 1:
      cam.smoothMotion();
      break;

    case 4:  // selection
      assert(selection);
      // box selection
      if (!selection.isPoint()){  
	console.log("selection of a box");
        var cursor_coords = getCursorPosition(evt);
        selection.setEndPoint(computePickRay(cursor_coords[0], cursor_coords[1]).ray);
      }
      else{
	console.log("selection of a single point " + selection.getPoint()[0] + " " + selection.getPoint()[1] + " " + selection.getPoint()[2]);
        selection_cloud.vertices[0] = selection.getPoint();
        selection_cloud.colors[0] = [1.0, 0.0, 0.0];
        selection_cloud.setupGL(ctx);
      }        
      break;
  }

  mouseDown = false;
  mouseButtonDown = 0;

}

function onMouseMove (evt) {
  var cursor_coords = getCursorPosition(evt);
  var deltaX = (cursor_coords[0] - mousePosition[0]) / $("#canvas").width();;
  var deltaY = (cursor_coords[1] - mousePosition[1]) / $("#canvas").height();;

  switch (mouseButtonDown) {
  case 1:  // left click
    // Yaws/pitches the camera
    cam.yaw(-deltaX * 5.0);
    cam.pitch(-deltaY * 5.0);
    mousePosition = cursor_coords;
    break;
    
  case 2:  // center click
    // Strafes the camera
    cam.goLeft(deltaX * 0.5 * cam.distance);
    cam.goUp(deltaY * 0.5 * cam.distance);
    mousePosition = cursor_coords;
    break;

  case 4:  // left click + shift for box selection
    if (selection.isPoint()){  // first time we move the mouse
      selection.setEndPoint(computePickRay(cursor_coords[0], cursor_coords[1]).ray);
      ctx.uniform3fv(currProgram.u.selection_begin, selection.begin_selection);
      ctx.uniform3fv(currProgram.u.selection_end, selection.end_selection);
      ctx.uniform3fv(currProgram.u.p_selection_camera, V3.$(0.0, 0.0, 0.0));
      ctx.uniformMatrix3fv(currProgram.u.R_selection_world, false, selection.R_selection_world);
    }
    else{
      selection.setEndPoint(computePickRay(cursor_coords[0], cursor_coords[1]).ray);
      ctx.uniform3fv(currProgram.u.selection_end, selection.end_selection);
    }
    break;
 }
}

function mouseScroll (wheel_delta) {
  //console.log(wheel_delta);
  // stop any automated camera motion
  cam.stopProfile();

  var move_by = wheel_delta * 0.01 * cam.distance / 10.0;

  //make sure to change the near plane as well
  if(custom_clipping)
  {
    console.log("Here with " + near_plane_distance + " " + move_by);
    near_plane_distance = Math.max(1.0, near_plane_distance - move_by / 2.0);
    console.log("New dist " + near_plane_distance);
    setProjectionMatrix();
  }

  cam.goCloser(move_by);
  cam.smoothMotion();
}

function onFFMouseWheel (evt)
{
  mouseScroll(-40 * evt.detail);
  evt.preventDefault();
}


function onMouseWheel (evt) 
{
  mouseScroll(evt.wheelDelta);
  evt.preventDefault();
}


function onMouseOver (evt) {
}



function initCam (config){
    // set some defaults
    if (!config.closest)
      config.closest = 0.1;
    if (!config.farthest)
      config.farthest = 1000000.0;

    // create camera
    cam = new OrbitCam(config, function (){
        if (TRIGGER_ON_VIEWPOINT_CHANGE){
            triggerFrustumUpdate();
        }
      });
    setupEventListeners();
    triggerFrustumUpdate();
}


function onContextMenu (evt) {
  console.log("Context menu handler")  
  return false
}

// Sets up event listeners, for both mouse and keyboard
function setupEventListeners () {
  console.log("setting up event listeners");

  document.getElementById("canvas");
 
  canvas.addEventListener('mousedown', onMouseDown, false);
  canvas.addEventListener('mouseup', onMouseUp, false);
  canvas.addEventListener('mousemove', onMouseMove, false);
  canvas.addEventListener('mousewheel', onMouseWheel, false);
  canvas.addEventListener('DOMMouseScroll', onFFMouseWheel, false);
  canvas.addEventListener('mouseover', onMouseOver, false);
  canvas.addEventListener('keydown', onKeyDown, false);
}


var logged = false;
var lastTime = new Date();
var frames = 0;
function renderLoop()
{
  ctx.getError();
  if (cam == null){
      return;
  }

  var now = new Date();
  
  //ctx.clearColor(0., 0., 0., 1.);
  ctx.clearColor(0., 0.5, 1., 1.);
  ctx.clear(ctx.COLOR_BUFFER_BIT | ctx.DEPTH_BUFFER_BIT);

  ctx.useProgram(currProgram);
  var box_size_meters = (2 * Math.tan((fov / 2.0) * Math.PI / 180) * box_size_px) / $("#canvas").height();
  ctx.uniform1f(currProgram.u.m, box_size_meters);
  ctx.uniform1f(currProgram.u.point_size_multiplier, point_size_multiplier);
  var m_to_px = $("#canvas").height() / (2.0 * Math.tan(((fov / 2.0) * Math.PI / 180)));
  ctx.uniform1f(currProgram.u.meters_to_pixels, m_to_px);
  //console.log("meters_to_px: " + m_to_px);

  // Draws the real point cloud
  var totalPoints = 0;
  if (currentCells) {
    ctx.useProgram(currProgram);
    ctx.uniformMatrix3fv(currProgram.u.R_camera_world, false, M4x4.topLeft3x3(cam.getMatrix()));

    // update selection transform
    if (selection)
      ctx.uniform3fv(currProgram.u.p_selection_camera, V3.sub(cam.getPosition(), selection.p_world_selection));

    // re-compute selection
    if (selection != null){
    }

    // render points
    for (var i=0; i<currentCells.length; i++) {
      var cell = currentCells[i];
      totalPoints += cell.numPoints();
      ctx.uniform1f(currProgram.u.pointSize, 20.0);

      if (!cell.vbo)
        cell.loadGL(ctx);
      cell.render(currProgram);
    }
  }
  var err = ctx.getError();
  if (err != 0) {
    console.error("Error in rendering cloud: " + err);
  }

  // Renders lines for distance measurements
  ctx.useProgram(lineShader);
  ctx.uniformMatrix4fv(lineShader.u.T_camera_world, false, cam.getMatrix());
  ctx.uniform1f(lineShader.u.lift, -10.0);
  glCheck();
  for (var i = 0; i < allMeasurements.length; ++i)
  {
    if (!allMeasurements[i].vbo)
      allMeasurements[i].loadGL(ctx, $("#textureCanvas")[0]);
    allMeasurements[i].render(lineShader);
  }
  glCheck();
  
  // Renders text for distance measurements
  ctx.useProgram(texturedShader);
  ctx.uniform1f(texturedShader.u.lift, -1.0);
  // Computes transforms for billboards
  var billboardSpherR = makeBillboardSpher(cam.getMatrix());
  for (var i = 0; i < allMeasurements.length; ++i)
  {
    var trans = M4x4.makeTranslate(allMeasurements[i].mid);
    var T_billboard = M4x4.mul(cam.getMatrix(), M4x4.mul(trans, billboardSpherR));
    ctx.uniformMatrix4fv(texturedShader.u.T_camera_world, false, T_billboard);
    allMeasurements[i].renderText(texturedShader);
  }
  glCheck();

  // Draws the selection
  if (selection_cloud){
    ctx.clear(ctx.DEPTH_BUFFER_BIT);
    ctx.useProgram(oldProgram);
    ctx.uniformMatrix4fv(oldProgram.modelViewMatrixUniform, false, cam.getMatrix());
    ctx.uniform1f(oldProgram.meters_to_pixels, m_to_px);
    selection_cloud.render(oldProgram);
    ctx.useProgram(currProgram);
  }
  logged = true;

  // Recomputes FPSevery second
  ++frames;
  if (now - lastTime > 250) {
    // display
    var display = "Display:";
    var fps = frames / (now - lastTime) * 1000;
    lastTime = now;
    frames = 0;
    display += "<br>-  " + Math.floor(fps) + " fps";
    display += "<br>-  ";
    if (currentCells){
      if (totalPoints > 1000000)
	  display += (totalPoints/1000000).toFixed(1) + " M points ";
      else
	  display += (totalPoints/1000).toFixed(0) + " k points ";
      display += "(" + currentCells.length + " cells)<br>";
    }
    $("#display").html(display);

    // cache usage
    var cache = "Cache usage:";
    cache += "<br>-  " + cellCache.size + "/" + cellCache.max_size + " = " +
      (Math.floor(100.0 * cellCache.size / cellCache.max_size)) + "%";
    if (totalCellBytesDownloaded)
      cache += "<br>-  " + prettyBytes(totalCellBytesDownloaded);
    if (frustumQuerier.inFlight)
      cache += "<br>-  loading " + frustumQuerier.inFlight + " cells";
    $("#cache").html(cache);
  }
}

function onCanvasResize()
{
  var canvas = $("#canvas");
  console.log("onCanvasReisze: " + canvas.width() + " x " + canvas.height());
  min_box_size_meters = (2 * Math.tan((fov / 2.0) * Math.PI / 180)) / $("#canvas").height();

  ctx.viewport(0, 0, canvas.width(), canvas.height());
  setProjectionMatrix();

  //we actually need to set the canvas element to reflect this new width and height,
  //you'd really think this would happen automatically on resize
  var canvasElement = document.getElementById('canvas');
  canvasElement.width = canvas.width();
  canvasElement.height = canvas.height();
}

function onWindowResize()
{
    $("#canvas").height(window.innerHeight-20);
    $("#canvas").width(window.innerWidth-20);      
    onCanvasResize();
}

function onPointSliderMove(value)
{
  $("#point_scale").html("Point Scale: " + value);
  point_size_multiplier = value;
}

function onPointCountSliderMove(value)
{
  $("#cell_count").html("Cell Count (M): " + value);
  desired_cell_count = value;
  triggerFrustumUpdate();
}


function onSliderMove(value)
{
  box_size_px = value;

  $("#resolution").html("Resolution: " + value);
  triggerFrustumUpdate();
}


function getVariableFromQuery(query, variable) 
{ 
    var vars = query.split("&"); 
    for (var i=0;i<vars.length;i++) { 
	var pair = vars[i].split("="); 
	if (pair[0] == variable) { 
	    return pair[1]; 
	} 
    } 
    return null;
}


function getViewpointFromQuery(query)
{
  var config = new Object;
  config.origin = V3.$(0, 0, 0);
  config.origin[0] = parseFloat(getVariableFromQuery(query, "c_x"));
  config.origin[1] = parseFloat(getVariableFromQuery(query, "c_y"));
  config.origin[2] = parseFloat(getVariableFromQuery(query, "c_z"));
  config.distance = parseFloat(getVariableFromQuery(query, "c_d"));
  config.pitch = parseFloat(getVariableFromQuery(query, "c_pi"));
  config.yaw = parseFloat(getVariableFromQuery(query, "c_ya"));
  config.name = getVariableFromQuery(query, "name");
  if (!isNaN(config.origin[0]) && !isNaN(config.origin[1]) && !isNaN(config.origin[2]) &&
      !isNaN(config.distance) && !isNaN(config.pitch) && !isNaN(config.yaw)){
    return config;
  }
  else{
    return null;
  }
}


function addTree(tree, metadata)
{
    console.log("Received metadata of tree " + tree + ":");
    console.log(metadata.toString());

    var color = [1.0, 0.0, 0.0];
    trees_info.push(new TreeInfo(tree, metadata, color));

    // create camera if it does not exist yet
    if (cam == null || (metadata.with_viewpoint && !cam.from_url)){
	// find viewpoint in url first
	initCam({origin: metadata.camera_origin, distance: metadata.camera_distance, yaw: metadata.camera_yaw, pitch: metadata.camera_pitch});
	console.log("Creating camera viewpoint from metadata of tree " + tree);
    }

    triggerFrustumUpdate();

    // get views of this tree
    getViews(tree, 
	     function(tree_name, view_config_list){
		 for (var j=0; j<view_config_list.length; j++){
		     addView(view_config_list[j].name, view_config_list[j], true);
		 }
	     });


    // retrun new tree_info object
    return trees_info[trees_info.length-1];
}


// Checks that the url is reachable
function ServerConnectionChecker(url, name, status_obj)
{
  this.url = url;
  this.name = name;
  this.status_obj = status_obj;

  this.backoff = 1000;  // ms
  this.check();
}
ServerConnectionChecker.prototype.check = function()
{
  clearTimeout(this.timeout);
  var this_scc = this;
  // Tests the the server is available
  var ping_server = $.get(this.url);
  ping_server.success(function () {
    this_scc.status_obj.html("Connected to " + this_scc.name);
    this_scc.backoff = 1000;
    this_scc.timeout = setTimeout(function() { this_scc.check(); }, this_scc.backoff);
  });
  ping_server.error(function () {
    var retry_link = $('<a href="#">(retry)</a>').click(function() { this_scc.check(); });
    this_scc.status_obj.html("<font color='red'>Connection to " + this_scc.name + " lost!</font> ");
    this_scc.status_obj.append(retry_link);
    this_scc.backoff = Math.min(2 * this_scc.backoff, 5*60*1000);
    this_scc.timeout = setTimeout(function() { this_scc.check(); }, this_scc.backoff);
  });
}


function clearSelection()
{
  console.log("Clear selection");

  // remove selection
  selection = null;
  ctx.uniform3fv(currProgram.u.selection_begin, V3.$(0.0, 0.0, 0.0)); 
  ctx.uniform3fv(currProgram.u.selection_end, V3.$(0.0, 0.0, 0.0));
}



function onRoadButtonClicked(job_info)
{
  // Start detecting roads on first tree
  detectRoads(trees_info[0].name, selection.getBox(), 
              // progress
              function(progress){
		job_info.progress_bar.progressbar({width: "20%", value:progress*100});
	      },
              // success
	      function(result_tree){
                  console.log("Setting tree name " + result_tree);
                  job_info.tree_name = result_tree;
                  if (!job_info.removed)  // only serve tree if job was not removed
                    serveNewTree(result_tree, 
                               // success
			       function(tree_object){
				   if (!job_info.removed){
				     console.log("Road detection succeeded");
                                     job_info.finished = true;
				     tree_object.override_color = true;
				     clearSelection();
				   }
			       }, 
                               // failure
			       function(){
				   job_info.failed = true;
				   console.log("Road detection succeeded, but failed to serve the resuling tree called");
				   $.prompt("Road detection succeeded, but failed to serve the results.");
			       }
			      );
	      }, 
              // failure
	      function(){
                  job_info.failed = true;
		  console.log("Road detection failed");
		  $.prompt("Road detection failed");
	      });
}



// Tells the file server to serve a new tree,
// gets the metadata of the new tree, and 
// adds the tree to the trees_info list
function serveNewTree(tree_name, successcallback, failurecallback)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/files/serve_new_tree?new_tree_name=" + tree_name, true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      console.log("New tree " + tree_name + " is now being served");
      getMetaData(tree_name, 
		  function(tree_name, metadata) {
		      console.log("New tree " + tree_name + " is now one of the active trees");
		      var tree_object = addTree(tree_name, metadata); 
		      successcallback(tree_object);
		  },
		  failurecallback);
    }
    else {
      console.log("Failed to serve new tree");
      failurecallback();
    }
  };
  xhr.send();
  
  return xhr;
}


addView = function(name, viewpoint, saved)
{
  $("#view_list").append("<li class='ui-state-default' align='left'><div id='close_view_id_" + view_id + "'/><div id='view_id_" + view_id + "'/></li>")

  var viewpoint_camera = new OrbitCam(viewpoint);
  var view = $("#view_id_"+view_id);
  view.button({text: true, label: name});
  view.click(function(){cam.goto(viewpoint_camera, 0.9);});

  var close = $("#close_view_id_"+view_id);
  if (saved)
    close.button({text: true, label: "&nbsp;", icons: {primary: "ui-icon-locked"}});
  else{
    close.button({text: true, label: "&nbsp;", icons: {primary: "ui-icon-trash"}});
    close.click(function(){
        $(this).parent().remove();
    });
  }
  close.width(28);

  view_id++;
}


addJob = function(name, viewpoint)
{
  $("#job_list").append("<li id='li_" + job_id + "' class='ui-state-default' align='left'><div id='progressbar_" + job_id + "'/><div id='close_job_id_" + job_id + "'/><div id='job_id_" + job_id + "'/></li>")

  var viewpoint_camera = new OrbitCam(viewpoint);
  var job_info = new JobInfo(name);

  job_info.button = $("#job_id_"+job_id);
  job_info.button.button({text: true, label: name});
  job_info.button.click(function(){cam.goto(viewpoint_camera, 0.9);});
  job_info.progress_bar = $("#progressbar_" + job_id);
  job_info.progress_bar.progressbar({value:0});
  jobs_info.push(job_info);

  var close = $("#close_job_id_"+job_id);
  close.button({text: true, label: "&nbsp;", icons: {primary: "ui-icon-trash"}});
  close.click(function(){
     job_info.remove();
     $(this).parent().remove();
  });
  close.width(28);

  job_id++;

  return job_info;
}





var view_id = 0;
var job_id = 0;
function start() {
  console.log("start()");

  // add slider setting
  $("#settings_list").append("<li class='ui-state-default'><h3><div id='resolution'/></h3><div id='slider'/></li>");
  $("#slider")
    .slider({min: 1.0, max: 10, step: 0.01, value: 5.0, width: "50%", slide: function(evt, ui){onSliderMove(ui.value);}});
  onSliderMove(5.0);

  //add point scale setting
  $("#settings_list").append("<lis class='ui-state-default'><h3><div id='point_scale'/></h3><div id='point_scale_slider'/></li>");
  $("#point_scale_slider")
    .slider({min: 0, max: 10, value: 1.41, step: 0.01, width: "50%", slide: function(evt, ui){onPointSliderMove(ui.value);}});
  onPointSliderMove(1.41);

  // add desired number of points slider
  $("#settings_list").append("<lis class='ui-state-default'><h3><div id='cell_count'/></h3><div id='cell_count_slider'/></li>");
  $("#cell_count_slider")
    .slider({min: 100, max: 2000.0, value: 400.0, step: 1, width: "50%", slide: function(evt, ui){onPointCountSliderMove(ui.value);}});
  onPointCountSliderMove(400);


  // add status
  $("#status_list").append("<li class='ui-state-default'><h3><div id='ping_files'/></h3></li>");
  $("#status_list").append("<li class='ui-state-default'><h3><div id='ping_algorithms'/></h3></li>");
  $("#status_list").append("<li class='ui-state-default'><h3><div id='cache'/></h3></li>");
  $("#status_list").append("<li class='ui-state-default'><h3><div id='display'/></h3></li>");
  $("#ping_files").html("Connecting to file server...");
  $("#ping_algorithms").html("Connecting to algorithm server...");
  new ServerConnectionChecker("/files/ping", "file server", $("#ping_files"));
  new ServerConnectionChecker("/algorithm/ping", "algorithm server", $("#ping_algorithms"));

  // add help
  $("#help_list").append("<li class='ui-state-default'>Drag left mouse to rotate</li></list></li>");
  $("#help_list").append("<li class='ui-state-default'>Drag middle mouse to pan</li></list></li>");
  $("#help_list").append("<li class='ui-state-default'>Mouse wheel to zoom</li></list></li>");
  $("#help_list").append("<li class='ui-state-default'>Click right mouse to focus</li></list></li>");


  $( "#tabs" ).tabs();
  $( ".column" )
    .sortable({connectWith: ".column", opacity: 0.5, zIndex: 999});
  $( ".portlet" ).addClass( "ui-widget ui-widget-content ui-helper-clearfix ui-corner-all" )
  .find( ".portlet-header" )
  .addClass( "ui-widget-header ui-corner-all" )
  .prepend( "<span class='ui-icon ui-icon-minusthick'></span>")
  .end()
  .find( ".portlet-content" );
  $( ".portlet-header .ui-icon" )
   .click(function() {
     $( this ).toggleClass( "ui-icon-minusthick" ).toggleClass( "ui-icon-plusthick" );
     $( this ).parents( ".portlet:first" ).find( ".portlet-content" ).toggle();
   })

  $( ".column" ).disableSelection();
  $( "li" ).disableSelection();

  $("#view_tab").click(function(){$("#view_name").focus();});

  $("#view_name")
   .keypress(function(evt){
     if (evt.which == 13){
       $("#add_view").click();
     }         
   });

  $( "#add_view" )
   .button({text: true, label: "Add view"})
   .click(function(){
      var view_name = $("#view_name").val();
      if (view_name == "")
        $.prompt("First specify a name for the view");
      else{
        var viewpoint = cam.getConfig();
        addView(view_name, viewpoint, false);
        $("#view_name").val("");
      }
   });


  $("#job_tab").click(function(){$("#job_name").focus();});

  $("#job_name")
   .keypress(function(evt){
     if (evt.which == 13){
       $("#start_job").click();
     }         
   });

  $( "#start_job" )
   .button({text: true, label: "Start job" })
   .click(function(){
      var job_name = $("#job_name").val();
      $("#job_name").val("");
      if (job_name == "")
        $.prompt("First specify a name for the job");
      else if (jobs_info.length > 0 && !(jobs_info[jobs_info.length-1].finished || jobs_info[jobs_info.length-1].failed || jobs_info[jobs_info.length-1].removed)){
        $.prompt("Previous job is still running.");        
      }
      else if (!selection){
        $.prompt("Select a region to run a job on.");        
      }
      else{
        var job_info = addJob(job_name, selection.lookAt(cam));
        onRoadButtonClicked(job_info);
        $("#view_name").val("");
      }
   });



  fpsWidget = document.getElementById('fps');
  cloudRequestCounter = 0;
  cloudReceiveCounter = 0;
  cloudRequestWidget = document.getElementById('num_clouds_received');
  cloudReceiveWidget = document.getElementById('num_clouds_requested');

  window.onresize = onWindowResize;

  // Sets up the canvas viewport
  canvas = document.getElementById('canvas');
  canvas.focus();  // Autofocus
  document.oncontextmenu = function() { return false; } // disable context menu for entire document
  canvas.onmousedown = function() {   
      canvas = document.getElementById('canvas'); 
      canvas.style.cursor = "url('images/drag_hand.png')"; 
      return false; } // disable ugly text cursor
  canvas.onmouseup = function() {   
      canvas = document.getElementById('canvas'); 
      //canvas.style.cursor = "url('images/open_hand.png')"; 
      canvas.style.cursor = "default"; 
      return false; } // disable ugly text cursor

  ctx = getCanvasWebGLContext(canvas);
  ctx.viewport(0, 0, $("#canvas").width(), $("#canvas").height());
  ctx.enable(ctx.DEPTH_TEST);
  //ctx.blendFunc(ctx.SRC_ALPHA, ctx.ONE_MINUS_SRC_ALPHA);
  //ctx.enable(ctx.BLEND);



  // Kicks off loading the shaders.  On the async return, we continue
  // creating the opengl program.
  getMany(["cell.vsh", "cell.fsh", "line.vsh", "line.fsh", "textured.vsh", "textured.fsh"],
	  function (results) {
	    console.log("Shaders loaded");
	    var vertexShaderSource = results[0];
	    var fragmentShaderSource = results[1];
	    var lineVertexShaderSource = results[2];
	    var lineFragmentShaderSource = results[3];
	    var texturedVertexShaderSource = results[4];
	    var texturedFragmentShaderSource = results[5];
	    
	    var shader = createProgramObject(ctx, vertexShaderSource, fragmentShaderSource);
	    glCheck();
	    
	    // Initializes the line shader
	    lineShader = createProgramObject(ctx, lineVertexShaderSource, lineFragmentShaderSource);
	    ctx.useProgram(lineShader);
	    populateShader(ctx, lineShader,
			   ["vertex", "color"],
			   ["T_camera_world", "ps_ProjectionMatrix", "far_plane_distance", "C", "lift"]);
	    ctx.uniform1f(lineShader.u.far_plane_distance, far_plane_distance);
	    ctx.uniform1f(lineShader.u.C, 1.0);
	    ctx.uniform1f(lineShader.u.lift, 0.0);
	    glCheck();

	    // Initializes the textured shader
	    texturedShader = createProgramObject(ctx, texturedVertexShaderSource, texturedFragmentShaderSource);
	    ctx.useProgram(texturedShader);
	    populateShader(ctx, texturedShader,
			   ["vertex", "textureCoord"],
			   ["T_camera_world", "ps_ProjectionMatrix", "far_plane_distance", "C", "lift",
			    "sampler"]);
	    ctx.uniform1f(texturedShader.u.far_plane_distance, far_plane_distance);
	    ctx.uniform1f(texturedShader.u.C, 1.0);
	    ctx.uniform1f(texturedShader.u.lift, 0.0);
	    glCheck();
	    
	    
	    // Continues setting up the webgl context
	    ctx.useProgram(null);
	    onProgramLoaded(ctx, shader);
	  },
	  function (results) {
	    console.log("Shaders failed to load");
	    $("#status").html("ERROR: Shaders failed to load");
	  });

  // Creates a cloud to visualize selections
  selection_cloud = new PointCloud();
  selection_cloud.vertices = [];
  selection_cloud.colors = []
  selection_cloud.vertices.push([0, 0, 1]);
  selection_cloud.colors.push([1, 0, 0]);
  selection_cloud.setupGL(ctx);


  // Constructs the point cloud shader
  oldProgram = createProgramObject(ctx, vertexShaderSource, fragmentShaderSource);
  ctx.useProgram(oldProgram);

  // Stores attribute and uniform locations into the shader.  The
  // shader program object becomes an interface to the shader.

  oldProgram.vertexPositionAttribute = ctx.getAttribLocation(oldProgram, "ps_Vertex");
  ctx.enableVertexAttribArray(oldProgram.vertexPositionAttribute);
  assert(oldProgram.vertexPositionAttribute >= 0, "Vertex position attribute not found");
  
  oldProgram.vertexColorAttribute = ctx.getAttribLocation(oldProgram, "ps_Color");
  ctx.enableVertexAttribArray(oldProgram.vertexColorAttribute);
  assert(oldProgram.vertexPositionAttribute >= 0, "Vertex color attribute not found");

  oldProgram.modelViewMatrixUniform = ctx.getUniformLocation(oldProgram, "ps_ModelViewMatrix");
  oldProgram.projectionMatrixUniform = ctx.getUniformLocation(oldProgram, "ps_ProjectionMatrix");
  oldProgram.meters_to_pixels = ctx.getUniformLocation(oldProgram, "meters_to_pixels");

  // Sets some default uniforms
  var loc = ctx.getUniformLocation(oldProgram, "ps_PointSize");
  ctx.uniform1f(loc, 30.0);
  loc = ctx.getUniformLocation(oldProgram, "ps_Attenuation");
  ctx.uniform3fv(loc, [0, 1, 0.003]);

  // Sets the projection matrix
  projectionMatrix = M4x4.makePerspective(fov, $("#canvas").width() / $("#canvas").height(), near_plane_distance, far_plane_distance);
  ctx.uniformMatrix4fv(oldProgram.projectionMatrixUniform, false, projectionMatrix);
  

  // Constructs the frustum querier.
  frustumQuerier = new FrustumQuerier(projectionMatrix);

  // try to find viewpoint in url 
  var query = window.location.search.substring(1); 
  var config = getViewpointFromQuery(query);
  if (config){
    console.log("Create camera from url");
    initCam(config);
    cam.from_url = true;
  }    

  // get tree name
  var tree = getVariableFromQuery(query, "tree");
  if (tree == null || tree == ""){
    // Get supported trees
    var url = document.location.href.split("?")[0] + "?tree=";
    getSupportedTrees(function(trees) {
	var tree_list = "<h3>Which MegaTree would you like to see?</h3><h2><br>";
        for (var i=0; i<trees.length; i++){
	    if (trees[i].substring(0,5) != "proc_")
		tree_list += "<li><a href='" + url + trees[i] + "'>" + trees[i] + "</a></li>";
	}
	tree_list += "</list></h2>";
        $.prompt(tree_list);
    },
                      function() {$.prompt("Failed to get supported trees");});
  }
  else{
    tree = tree.toString();
    tree_list = tree.split(",");
    console.log("got tree list " + tree_list);
  
    // generate metadata and root geometry
    for (var i=0; i<tree_list.length; i++){
        getMetaData(tree_list[i], 
  		  addTree,
  		  function() {});
    }
  }  
}


// Called once the shaders are loaded and linked into the program object
function onProgramLoaded(ctx, shader)
{
  console.log("Program loaded");

  ctx.useProgram(shader);
  shader.a = new Object();  // Attributes
  shader.u = new Object();  // Uniforms

  shader.a.vertex = ctx.getAttribLocation(shader, "ps_Vertex");
  ctx.enableVertexAttribArray(shader.a.vertex);
  assert(shader.a.vertex >= 0, "Vertex position attribute not found");
  
  shader.a.color = ctx.getAttribLocation(shader, "ps_Color");
  ctx.enableVertexAttribArray(shader.a.color);
  assert(shader.a.color >= 0, "Vertex color attribute not found");

  shader.u.R_camera_world = ctx.getUniformLocation(shader, "R_camera_world");
  shader.u.projectionMatrix = ctx.getUniformLocation(shader, "ps_ProjectionMatrix");
  shader.u.scale = ctx.getUniformLocation(shader, "scale");
  shader.u.cameraToCellInWorld = ctx.getUniformLocation(shader, "camera_to_cell_in_world");
  shader.u.point_size_multiplier = ctx.getUniformLocation(shader, "point_size_multiplier");
  shader.u.point_size_meters = ctx.getUniformLocation(shader, "point_size_meters");
  shader.u.meters_to_pixels = ctx.getUniformLocation(shader, "meters_to_pixels");
  shader.u.m = ctx.getUniformLocation(shader, "m");
  shader.u.show_children = new Array();
  shader.u.show_children[0] = ctx.getUniformLocation(shader, "show_children[0]");
  shader.u.show_children[1] = ctx.getUniformLocation(shader, "show_children[1]");
  shader.u.show_children[2] = ctx.getUniformLocation(shader, "show_children[2]");
  shader.u.show_children[3] = ctx.getUniformLocation(shader, "show_children[3]");
  shader.u.show_children[4] = ctx.getUniformLocation(shader, "show_children[4]");
  shader.u.show_children[5] = ctx.getUniformLocation(shader, "show_children[5]");
  shader.u.show_children[6] = ctx.getUniformLocation(shader, "show_children[6]");
  shader.u.show_children[7] = ctx.getUniformLocation(shader, "show_children[7]");
  shader.u.override_color = new Array();
  shader.u.override_color[0] = ctx.getUniformLocation(shader, "override_color[0]");
  shader.u.override_color[1] = ctx.getUniformLocation(shader, "override_color[1]");
  shader.u.override_color[2] = ctx.getUniformLocation(shader, "override_color[2]");
  shader.u.selection_begin = ctx.getUniformLocation(shader, "selection_begin");
  shader.u.selection_end = ctx.getUniformLocation(shader, "selection_end");
  shader.u.p_selection_camera = ctx.getUniformLocation(shader, "p_selection_camera");
  shader.u.R_selection_world = ctx.getUniformLocation(shader, "R_selection_world");
  shader.u.far_plane_distance = ctx.getUniformLocation(shader, "far_plane_distance");
  shader.u.C = ctx.getUniformLocation(shader, "C");
  ctx.uniform1f(shader.u.far_plane_distance, far_plane_distance);
  ctx.uniform1f(shader.u.C, 1.0);
  ctx.uniform1f(shader.u.point_size_multiplier, point_size_multiplier);
  defaultProgram = currProgram = shader;

  // Sets the projection matrix
  projectionMatrix = M4x4.makePerspective(fov, $("#canvas").width() / $("#canvas").height(), near_plane_distance, far_plane_distance);
  //projectionMatrix = M4x4.makePerspectiveToZero(fov, $("#canvas").width(), $("#canvas").height(), 0.1);
  ctx.useProgram(shader);
  ctx.uniformMatrix4fv(shader.u.projectionMatrix, false, projectionMatrix);
  ctx.useProgram(lineShader);
  ctx.uniformMatrix4fv(lineShader.u.ps_ProjectionMatrix, false, projectionMatrix);
  ctx.useProgram(shader);

  
  // Kicks off the test cloud load

  // Unfortunately, jquery doesn't yet support arraybuffer responses,
  // so we use xhr manually.
  //
  // See: http://blog.vjeux.com/2011/javascript/jquery-binary-ajax.html
  (function () {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/files/box", true);
    xhr.responseType = "arraybuffer";
    xhr.onload = function (evt) {
      shorts = new Uint8Array(xhr.response);
      console.log("S: " + shorts[0] + ", " + shorts[1] + ", " + shorts[2] + ", " + shorts[3] + ", " + shorts[4] + ", " + shorts[5]);

      testBoxCell = new Cell("box", new NodeGeometry(V3.$(-1.0, -1.0, -1.0), V3.$(1.0, 1.0, 1.0)), xhr.response);
      testBoxCell.loadGL(ctx);
    };
    xhr.send();
  })();

  console.log("Entering animation loop");
  onWindowResize();
  animationLoop();
  triggerFrustumUpdate();
}
