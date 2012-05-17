sign = function(v)
{
  if (v < 0)
    return -1.0;
  else
   return 1.0;
}



VelocityProfileMapped = function(pos1, pos2, mapping, maxvel, maxacc)
{
  this.mapping = mapping;
  this.length = Math.abs(pos2 - pos1);
  this.offset = pos1;
  if (this.length == 0.0)
    this.duration = 0.0;
  else{
    this.dir    = sign(pos2 - pos1);

    this.profile = new VelocityProfileTrap(0.0, 1.0, maxvel, maxacc);
    this.duration = this.profile.duration;
  }
}


VelocityProfileMapped.prototype.setProfileDuration = function(newduration) 
{
  this.duration = newduration;
  if (this.length != 0.0)
    this.profile.setProfileDuration(newduration);
}


VelocityProfileMapped.prototype.getPos = function(time)
{
  if (this.length == 0.0){
    return this.offset;
  }
  else{
    var value = this.profile.getPos(time);
    return this.mapping(value) * this.length * this.dir + this.offset;    
  }
}
