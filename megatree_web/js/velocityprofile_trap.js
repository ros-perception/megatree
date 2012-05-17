sign = function(v)
{
  if (v < 0)
    return -1.0;
  else
   return 1.0;
}


VelocityProfileTrap = function(pos1, pos2, maxvel, maxacc)
{
  this.maxvel = maxvel;
  this.maxacc = maxacc;

  this.startpos = pos1;
  this.endpos   = pos2;
  this.time1 = maxvel/maxacc;

  var s = sign(this.endpos - this.startpos);
  var deltax1 = s*this.maxacc*this.time1*this.time1/2.0;
  var deltaT  = (this.endpos-this.startpos-2.0*deltax1) / (s*this.maxvel);

  // velocity will saturate
  if (deltaT > 0.0) {
    this.duration = 2.0 * this.time1 + deltaT;
    this.time2 = this.duration - this.time1;
  } 
  
  // never reach max velocity
  else {
    this.time1 = Math.sqrt((this.endpos-this.startpos)/s/this.maxacc);
    this.duration = this.time1*2.0;
    this.time2=this.time1;
  }
  this.a3 = s*this.maxacc/2.0;
  this.a2 = 0;
  this.a1 = this.startpos;
    
  this.b3 = 0;
  this.b2 = this.a2+2.0*this.a3*this.time1 - 2.0*this.b3*this.time1;
  this.b1 = this.a1+this.time1*(this.a2+this.a3*this.time1) - this.time1*(this.b2+this.time1*this.b3);
    
  this.c3 = -s*this.maxacc/2.0;
  this.c2 = this.b2+2.0*this.b3*this.time2 - 2.0*this.c3*this.time2;
  this.c1 = this.b1+this.time2*(this.b2+this.b3*this.time2) - this.time2*(this.c2+this.time2*this.c3);
}


VelocityProfileTrap.prototype.setProfileDuration = function(newduration) 
{
  if (newduration == this.duration)
    return;

  var factor = this.duration/newduration;
  if (factor > 1)
    console.log("WHATT??");
  //assert(factor <= 1);

  this.a2*=factor;
  this.a3*=factor*factor;
  this.b2*=factor;
  this.b3*=factor*factor;
  this.c2*=factor;
  this.c3*=factor*factor;
  this.duration = newduration;
  this.time1 /= factor;
  this.time2 /= factor;
}


VelocityProfileTrap.prototype.getPos = function(time)
{
  if (time < 0) {
    return this.startpos;
  } 
  else if (time<this.time1) {
    return this.a1+time*(this.a2+this.a3*time);
  } 
  else if (time<this.time2) {
    return this.b1+time*(this.b2+this.b3*time);
  } 
  else if (time<=this.duration) {
    return this.c1+time*(this.c2+this.c3*time);
  } 
  else {
    return this.endpos;
  }
}


VelocityProfileTrap.prototype.getVel = function(time) 
{
  if (time < 0) {
    return 0;
  } 
  else if (time<this.time1) {
    return this.a2+2.0*this.a3*time;
  } 
  else if (time<this.time2) {
    return this.b2+2.0*this.b3*time;
  } 
  else if (time<=this.duration) {
    return this.c2+2.0*this.c3*time;
  } 
  else {
    return 0.0;
  }
}
