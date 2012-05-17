

VelocityProfileMapped3D = function(pos1, pos2, mapping, maxvel, maxacc)
{
  this.length = V3.length(V3.sub(pos2, pos1));
  this.dir = V3.scale(V3.sub(pos2, pos1), 1.0/this.length);
  this.offset = V3.clone(pos1);
  
  this.profile = new VelocityProfileMapped(0, this.length, mapping, maxvel, maxacc);
  this.duration = this.profile.duration;
}


VelocityProfileMapped3D.prototype.setProfileDuration = function(newduration) 
{
  this.duration = newduration;
  this.profile.setProfileDuration(newduration);
}


VelocityProfileMapped3D.prototype.getPos = function(time)
{
  if (this.length == 0.0)
    return this.offset;
  else{
    var mapped = this.profile.getPos(time);
    return V3.add(V3.scale(this.dir, mapped), this.offset);
  }
}



