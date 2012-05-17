 

VelocityProfileMappedDouble = function(pos1, pos2, pos3, mapping1, mapping2, max_vel, max_acc)
{
  // create two profiles
  this.profile0 = new VelocityProfileMapped(pos1, pos2, mapping1, max_vel, max_acc);
  this.profile1 = new VelocityProfileMapped(pos2, pos3, mapping2, max_vel, max_acc);

  this.duration = this.profile0.duration + this.profile1.duration;
}


VelocityProfileMappedDouble.prototype.setProfileDuration = function(newduration) 
{
  var factor = newduration / this.duration;

  this.profile0.setProfileDuration(this.profile0.duration * factor);
  this.profile1.setProfileDuration(this.profile1.duration * factor);

  this.duration = newduration;
}


VelocityProfileMappedDouble.prototype.getPos = function(time)
{
  if (time < this.profile0.duration)
    return this.profile0.getPos(time);
  else
    return this.profile1.getPos(time - this.profile0.duration);   
}


VelocityProfileMappedDouble.prototype.getVel = function(time) 
{
  if (time < this.profile0.duration)
    return this.profile0.getVel(time);
  else
    return this.profile1.getVel(time - this.profile0.duration);   
}
