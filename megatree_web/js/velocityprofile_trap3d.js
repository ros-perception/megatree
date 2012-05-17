

VelocityProfileTrap3D = function(pos1, pos2, maxvel, maxacc)
{
  this.profile0 = new VelocityProfileTrap(pos1[0], pos2[0], maxvel, maxacc);
  this.profile1 = new VelocityProfileTrap(pos1[1], pos2[1], maxvel, maxacc);
  this.profile2 = new VelocityProfileTrap(pos1[2], pos2[2], maxvel, maxacc);

  // give profiles common durations
  this.duration = 0.0;
  this.duration = Math.max(this.profile0.duration, this.profile1.duration, this.profile2.duration);
  console.log("durations " + this.profile0.duration + " "  + this.profile1.duration + " " + this.profile2.duration);

  this.profile0.setProfileDuration(this.duration);
  this.profile1.setProfileDuration(this.duration);
  this.profile2.setProfileDuration(this.duration);
}


VelocityProfileTrap3D.prototype.setProfileDuration = function(newduration) 
{
  this.profile0.setProfileDuration(newduration);
  this.profile1.setProfileDuration(newduration);
  this.profile2.setProfileDuration(newduration);

  this.duration = newduration;
}


VelocityProfileTrap3D.prototype.getPos = function(time)
{
  var res = V3.$(0.0, 0.0, 0.0);
  res[0] = this.profile0.getPos(time);
  res[1] = this.profile1.getPos(time);
  res[2] = this.profile2.getPos(time);

  return res;
}


VelocityProfileTrap3D.prototype.getVel = function(time) 
{
  var res = V3.$(0.0, 0.0, 0.0);
  res[0] = this.profile0.getVel(time);
  res[1] = this.profile1.getVel(time);
  res[2] = this.profile2.getVel(time);

  return res;
}
