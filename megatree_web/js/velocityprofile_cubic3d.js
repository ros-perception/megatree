

VelocityProfileCubic3D = function(pos1, pos2, duration)
{
  this.profile0 = new VelocityProfileCubic(pos1[0], pos2[0], duration);
  this.profile1 = new VelocityProfileCubic(pos1[1], pos2[1], duration);
  this.profile2 = new VelocityProfileCubic(pos1[2], pos2[2], duration);

  this.duration = duration;
}


VelocityProfileCubic3D.prototype.setProfileDuration = function(newduration) 
{
  this.profile0.setProfileDuration(newduration);
  this.profile1.setProfileDuration(newduration);
  this.profile2.setProfileDuration(newduration);

  this.duration = newduration;
}


VelocityProfileCubic3D.prototype.getPos = function(time)
{
  var res = V3.$(0.0, 0.0, 0.0);
  res[0] = this.profile0.getPos(time);
  res[1] = this.profile1.getPos(time);
  res[2] = this.profile2.getPos(time);

  return res;
}


VelocityProfileCubic3D.prototype.getVel = function(time) 
{
  var res = V3.$(0.0, 0.0, 0.0);
  res[0] = this.profile0.getVel(time);
  res[1] = this.profile1.getVel(time);
  res[2] = this.profile2.getVel(time);

  return res;
}
