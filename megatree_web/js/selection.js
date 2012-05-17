
// Picked point in a frame that is alligned with the world, and with its origin in the camera frame
Selection = function(picked_point, R_camera_world, p_world_camera){
  this.R_selection_world = R_camera_world;
  this.p_world_selection = p_world_camera;

  var picked_point_camera = V3.mul3x3(this.R_selection_world, picked_point);
  this.depth = picked_point_camera[2];

  // store begin point in selection frame (= camera frame at this timestamp)
  this.picked_point_world = V3.add(p_world_camera, picked_point);
  this.begin_selection = V3.add(picked_point_camera, V3.$(0.0, 0.0, this.depth*0.5));
  this.end_selection = null;
}



// Normalized ray from camera to picked point, in a frame that is alligned with the world, and with its origin in the camera frame
Selection.prototype.setEndPoint = function(ray)
{
  var ray_camera = V3.mul3x3(this.R_selection_world, ray);
  var picked_point_camera = V3.scale(ray_camera, this.depth/ray_camera[2]);

  // store begin and end point in selection frame (= camera frame at creation timestamp)
  this.end_selection = V3.add(picked_point_camera, V3.$(0.0, 0.0, -this.depth*0.5));
}


Selection.prototype.isPoint = function()
{
  return !this.end_selection;
}


Selection.prototype.getPoint = function()
{
  assert(!this.end_selection);
  return this.picked_point_world;
}

Selection.prototype.getBox = function()
{
  assert(this.end_selection);
  var res = [];
  res.push(V3.add( V3.mul3x3(M3x3.transpose(this.R_selection_world), this.begin_selection), this.p_world_selection));
  res.push(V3.add( V3.mul3x3(M3x3.transpose(this.R_selection_world), this.end_selection), this.p_world_selection));
  return res;
}


Selection.prototype.lookAt = function(current_cam)
{
    // compute center in world coordinates
    if (this.end_selection)
      var center = V3.scale(V3.add(this.begin_selection, this.end_selection), 1.0/2.0);
    else
      var center = this.end_selection;

    // in camera frame, compute distance between corners in x-y plane
    var plane_begin = V3.clone(this.begin_selection);
    var plane_end = V3.clone(this.end_selection);    
    plane_begin[2] = 0.0;
    plane_end[2] = 0.0;
    var dist = V3.length(V3.sub(plane_begin, plane_end));

    // camera 
    var config = new Object;
    config.origin = V3.add( V3.mul3x3(M3x3.transpose(this.R_selection_world), center), this.p_world_selection);
    config.distance = dist * 2.0;
    console.log("setting distance to " + config.distance);
    config.yaw = current_cam._yaw;
    config.pitch = current_cam._pitch;

    return config;
}