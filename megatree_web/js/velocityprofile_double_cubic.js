

VelocityProfileDoubleCubic = function(pos1, pos2, pos3, duration)
{
  console.log("Double");

  // assign duration to right profiles
  this.d1 = Math.abs(pos1 - pos2);
  this.d2 = Math.abs(pos2 - pos3);

  this.profile0 = new VelocityProfileCubic(pos1, pos2, duration * this.d1 / (this.d1+this.d2));
  this.profile1 = new VelocityProfileCubic(pos2, pos3, duration * this.d2 / (this.d1+this.d2));

  this.duration = duration;
}


VelocityProfileDoubleCubic.prototype.setProfileDuration = function(newduration) 
{
  this.profile0.setProfileDuration(newduration * this.d1 / (this.d1+this.d2));
  this.profile1.setProfileDuration(newduration * this.d2 / (this.d1+this.d2));

  this.duration = newduration;
}


VelocityProfileDoubleCubic.prototype.getPos = function(time)
{
  if (time < this.duration * this.d1 / (this.d1+this.d2))
    return this.profile0.getPos(time);
  else
    return this.profile1.getPos(time);   
}


VelocityProfileDoubleCubic.prototype.getVel = function(time) 
{
  if (time < this.duration * this.d1 / (this.d1+this.d2))
    return this.profile0.getVel(time);
  else
    return this.profile1.getVel(time);   
}
