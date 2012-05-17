sign = function(v)
{
  if (v < 0)
    return -1.0;
  else
   return 1.0;
}


VelocityProfileSlowdown = function(pos1, vel1, maxacc)
{
  this.startpos = pos1;
  this.startvel = vel1;
  this.maxacc = maxacc;

  this.s = sign(this.startvel);
  this.duration = this.s*this.startvel / this.maxacc;
  this.endpos = this.startpos + this.startvel*this.duration - this.s*this.duration*this.duration*this.maxacc/2.0;

  console.log("Slowdown profile with initial velcity of " + this.startvel + " will last for " + this.duration + " seconds");
}


VelocityProfileSlowdown.prototype.setProfileDuration = function(newduration) 
{
  var factor = this.duration/newduration;
  assert(factor <= 1);

  this.maxacc *= factor;
  this.duration = newduration;
}


VelocityProfileSlowdown.prototype.getPos = function(time)
{
  if (time < 0) {
    return this.startpos;
  } 
  else if (time<this.duration) {
    return this.startpos + this.startvel*time - this.s*time*time*this.maxacc/2.0;
  } 
  else {
    return this.endpos;
  }
}


VelocityProfileSlowdown.prototype.getVel = function(time)
{
  if (time < 0) {
    return this.startvel;
  } 
  else if (time<this.duration) {
    return this.startvel - this.s*time*this.maxacc;
  } 
  else {
    return 0.0;
  }
}
