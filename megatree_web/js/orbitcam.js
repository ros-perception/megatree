/*
 * OrbitCamera
 *
 * Author: Stuart Glaser
 */
function OrbitCam (config, motionCallback) {
  this.motionCallback = motionCallback;

  this.origin =  V3.$(0, 0, 0);
  this.old_origin =  V3.$(0, 0, 0);
  if (config){
    if (config.origin){
      this.origin[0] = config.origin[0];
      this.origin[1] = config.origin[1];
      this.origin[2] = config.origin[2];

      this.old_origin[0] = config.origin[0];
      this.old_origin[1] = config.origin[1];
      this.old_origin[2] = config.origin[2];
    }
    this.distance = config.distance || 10.0;
    this.old_distance = this.distance;
    this._pitch = config.pitch || 0.0;  // Rotation about +y
    this.old_pitch = this._pitch;
    this._yaw = config.yaw || 0.0;  // Rotation about +z
    this.old_yaw = this._yaw;

    this.closestDistance = config.closest || 0.1;
    this.farthestDistance = config.farthest || 100.0;
  }

  this.last_pitch_time = null;

  this.damping = 4.0;
  this.lowpass = 0.1;

  this.camera_motion_id = 0;
  this.profile_executor_id = 0;
}

sgn = function(v){
  if (v > 0)
    return 1.0;
  else
    return -1.0;
}

ang_dist = function(a1, a2){
  var res = a1 - a2;
  if (res > Math.PI)
    res -= Math.PI * 2.0;
  if (res < -Math.PI)
    res += Math.PI * 2.0;
  return res;
}



OrbitCam.prototype.getConfig = function() {
  var config = new Object;
  config.closest = this.closestDistance;
  config.farthest = this.farthestDistance;
  config.origin = V3.$(0, 0, 0);
  config.origin[0] = this.origin[0];
  config.origin[1] = this.origin[1];
  config.origin[2] = this.origin[2];
  config.distance = this.distance;;
  config.yaw = this._yaw;
  config.pitch = this._pitch;
  return config;
}

OrbitCam.prototype.toString = function() {
    var m = this.getMatrix();
    var m_inv = M4x4.inverse(m);

    var res = "";
    res += "'origin_x':"+this.origin[0]+",'origin_y':"+this.origin[1]+",'origin_z':"+this.origin[2];
    res += ",'distance':"+this.distance+",'pitch':"+this._pitch+",'yaw':"+this._yaw;
    res += ",'closest_distance':"+this.closestDistance+",'farthest_distance':"+this.farthestDistance;
    res += ",'transform_world_to_camera':[";
    for (var i = 0; i<16; i++) {
        res += m_inv[i];
        if (i<15)
            res += ",";
    }
    res += "]"
    return res;
}



OrbitCam.prototype.smoothMotion = function()
{
}



mapping1 = function(map1, map2, value, power)
{
  if (map2 > map1){
    var res = Math.pow(value, power)*map2/((map2-map1)*value + map1*Math.pow(value, power-1) + 1.0 - value);
  }
  else{
    value = -value + 1.0;
    var res = 1.0 - Math.pow(value, power)*map1/((map1-map2)*value + map2*Math.pow(value,power-1) + 1.0 - value);
  }
  return res;
}

mapping2 = function(value)
{
  return (1.0-Math.cos(value*Math.PI))/2.0;
}


angle_diff = function(a1, a2)
{
  var d = a1 - a2;
  while (d < -Math.PI)
    d += Math.PI*2.0;
  while (d > Math.PI)
   d -= Math.PI*2.0;
  return d;
}


OrbitCam.prototype.goto = function(goal, speed_scale)
{
  if (!speed_scale)
    speed_scale = 1.0;
  var start_time = new Date();

  var max_angle_acc = 2.0 * speed_scale;
  var max_angle_vel = 2.0 * speed_scale;
  var max_trans_acc = 2.0 * speed_scale;
  var max_trans_vel = 2.0 * speed_scale;

  // compute velocity profiles
  var pitch_profile = new VelocityProfileTrap(this._pitch, this._pitch + angle_diff(goal._pitch, this._pitch), max_angle_vel, max_angle_acc);
  var yaw_profile = new VelocityProfileTrap(this._yaw, this._yaw + angle_diff(goal._yaw, this._yaw), max_angle_vel, max_angle_acc);

  var origin_distance = V3.length(V3.sub(this.origin, goal.origin));
  if (origin_distance < (this.distance + goal.distance)){
    var map1 = this.distance;
    var map2 = goal.distance;   
    var distance_time = 4.0 * Math.abs(this.distance - goal.distance) / (this.distance + goal.distance);
    var distance_profile = new VelocityProfileMapped(this.distance, goal.distance, 
						     function(value){return mapping1(map1, map2, value, 3);},
						     max_trans_vel / distance_time, max_trans_acc / distance_time);
    var origin_time = 4.0 * origin_distance / (map1 + map2);
    var origin_profile = new VelocityProfileMapped3D(this.origin, goal.origin, 
                                                     function(value){return mapping1(map1, map2, value, 3);}, 
                                                     max_trans_vel / origin_time, max_trans_acc / origin_time);
  }
  else{
    var map1 = this.distance;
    var map2 = 2.0*origin_distance;
    var map3 = goal.distance; 
    var distance_time = 4.0 * (Math.abs(map1-map2) + Math.abs(map2-map3)) / (map1 + map2 + map3);
    var distance_profile = new VelocityProfileMappedDouble(this.distance, 2.0*origin_distance, goal.distance, 
							   function(value){return mapping1(map1, map2, value, 3);},
							   function(value){return mapping1(map2, map3, value, 3);},
							   max_trans_vel * 2.0 / distance_time, max_trans_acc * 2.0 / distance_time);
    var origin_time = 4.0 * origin_distance / (map1 + map2 + map3);
    var origin_profile = new VelocityProfileMapped3D(this.origin, goal.origin, 
                                                     function(value){return mapping2(value);}, 
                                                     max_trans_vel / origin_time, max_trans_acc / origin_time);
  }


  // compute common duration for all profiles
  var date = new Date();
  var time = 0.0;
  var duration = Math.max(pitch_profile.duration, yaw_profile.duration, distance_profile.duration, origin_profile.duration);

  pitch_profile.setProfileDuration(duration);  
  yaw_profile.setProfileDuration(duration);  
  distance_profile.setProfileDuration(duration);  
  origin_profile.setProfileDuration(duration);  

  console.log("Computed trajectories in " + (new Date() - time) / 1000.0 + " seconds");
  this.executeProfile(time, duration, pitch_profile, yaw_profile, distance_profile, origin_profile);
}


OrbitCam.prototype.executeProfile = function(time, duration, pitch_profile, yaw_profile, distance_profile, origin_profile)
{
   this.profile_executor_id++;
   var id = this.profile_executor_id;

   this.executeProfileIteration(id, time, duration, pitch_profile, yaw_profile, distance_profile, origin_profile);
}


OrbitCam.prototype.stopProfile = function()
{
   this.profile_executor_id++;
}


OrbitCam.prototype.executeProfileIteration = function(id, time, duration, pitch_profile, yaw_profile, distance_profile, origin_profile)
{
  if (this.profile_executor_id != id)
    return;

  var rate = 30;

  // move the camera
  this.setPitch(pitch_profile.getPos(time));
  this.setYaw(yaw_profile.getPos(time));
  this.setDistance(distance_profile.getPos(time));
  this.setOrigin(origin_profile.getPos(time));

  // schedule next profile iteration 
  var next_time = time + 1.0 / rate;
  var this_ = this;
  if (next_time < duration)
    setTimeout(function(){this.executeProfileIteration(id, next_time, duration, pitch_profile, yaw_profile, distance_profile, origin_profile);}.bind(this_), 
	       1000.0/rate);  
}



// Camera's current axis
OrbitCam.prototype.getForward = function () {
  return V3.$(Math.cos(this._yaw) * Math.cos(this._pitch),
	      Math.sin(this._yaw) * Math.cos(this._pitch),
	      Math.sin(this._pitch));
}
OrbitCam.prototype.getLeft = function () {
  return V3.$(-Math.sin(this._yaw), Math.cos(this._yaw), 0);
}
OrbitCam.prototype.getUp = function () {
  return V3.$(Math.cos(this._yaw) * Math.sin(-this._pitch),
	      Math.sin(this._yaw) * Math.sin(-this._pitch),
	      Math.cos(this._pitch));
}

OrbitCam.prototype.getPosition = function () {
  return V3.add(this.origin, V3.scale(this.getForward(), -this.distance));
}

// Gives C, where
//
// p_camera = C * p_world
OrbitCam.prototype.getMatrix = function () {
  // Explicitly computes the up vector so we don't have discontinuities
  // when (position - origin) ==~ up

    /*
    console.log("makeLookAt(" + this.getPosition().toArray() + ",\n" +
                "           " + this.origin.toArray() + ",\n" +
                "           " + this.getUp().toArray() + ")\n" +
                " = " + M4x4.makeLookAt(this.getPosition(), this.origin, this.getUp()).toArray() + "\n" +
                " = " + M4x4.inverse(M4x4.makeLookAt(this.getPosition(), this.origin, this.getUp())).toArray());
    */
  return M4x4.makeLookAt(this.getPosition(), this.origin, this.getUp());
}


OrbitCam.prototype.setOrigin = function (origin) {
  this.origin = origin;

  var diff_origin = V3.length(V3.sub(this.old_origin, this.origin));
  if (diff_origin > this.distance*0.05){
      this.old_origin[0] = this.origin[0];
      this.old_origin[1] = this.origin[1];
      this.old_origin[2] = this.origin[2];
      this.motionCallback();
  }
}

// Distance modifiers
OrbitCam.prototype.setDistance = function (distance) {
  this.distance = Math.max(this.closestDistance, Math.min(distance, this.farthestDistance));

  var diff_distance = Math.abs(this.old_distance - this.distance);
  if (diff_distance > this.distance * 0.05){
    this.old_distance = this.distance;
    this.motionCallback();
  }
}

OrbitCam.prototype.goCloser = function (step) {
  var prev_distance = this.distance;
  this.setDistance(this.distance - step);
}

// Pitch modifiers
OrbitCam.prototype.setPitch = function (pitch) {
  this._pitch = Math.max(-Math.PI/2, Math.min(pitch, Math.PI/2));

  var diff_pitch = Math.abs(this.old_pitch - this._pitch);
  if (diff_pitch > 5.0 * Math.PI / 180.0){
      this.old_pitch = this._pitch;
      this.motionCallback();
  }
}
OrbitCam.prototype.pitch = function (step) {
  this.setPitch(this._pitch + step);
}

// Yaw modifiers
OrbitCam.prototype.setYaw = function (yaw) {
  this._yaw = yaw;

  var diff_yaw = Math.abs(this.old_yaw - this._yaw);
  if (diff_yaw > 5.0 * Math.PI / 180.0){
      this.old_yaw = this._yaw;
      this.motionCallback();
  }
}
OrbitCam.prototype.yaw = function (step) {
  this.setYaw(this._yaw + step);
}

// Movement modifiers
OrbitCam.prototype.goForward = function (step) {
  this.addOrigin(V3.scale(this.getForward(), step));
}
OrbitCam.prototype.goLeft = function (step) {
  this.addOrigin(V3.scale(this.getLeft(), step));
}
OrbitCam.prototype.goUp = function (step) {
  this.addOrigin(V3.scale(this.getUp(), step));
}

OrbitCam.prototype.addOrigin = function(step){
  this.setOrigin(V3.add(this.origin, step));
}

OrbitCam.prototype.dumpBasis = function () {
  var m = this.getMatrix();
  console.log("Camera:  \n"
	      + "  Pitch = " + this._pitch + ", yaw = " + this._yaw
	      + ", distance = " + this.distance + "\n"
	      + "  [Forward = " + this.getForward().toArray() + "]\n"
	      + "  [Left = " + this.getLeft().toArray() + "]\n"
	      + "  [Up = " + this.getUp().toArray() + "]\n"
	      + "  Transform: [" + m[0] + ", " + m[1] + ", " + m[2] + ", " + m[3] + "\n"
	      + "              " + m[4] + ", " + m[5] + ", " + m[6] + ", " + m[7] + "\n"
	      + "              " + m[8] + ", " + m[9] + ", " + m[10] + ", " + m[11] + "\n"
	      + "              " + m[12] + ", " + m[13] + ", " + m[14] + ", " + m[15] + "]"
	     );
}
