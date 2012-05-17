
generatePowers = function(n, x)
{
  var powers = new Array();
  powers.push(1.0);
  for (var i=1; i<=n; i++)
    powers.push(powers[i-1]*x);
  return powers;
}


VelocityProfileCubic = function(pos1, pos2, duration)
{
  this.pos1 = pos1;
  this.pos2 = pos2;
  this.vel1 = 0.0;
  this.vel2 = 0.0;

  this.duration = duration;
  this.coefficients = this.getCubicSplineCoefficients(duration);
}


VelocityProfileCubic.prototype.getVel = function(time)
{
  // create powers of time:                                                                              
  var t = generatePowers(3, time);

  var vel = t[0]*this.coefficients[1] +
            2.0*t[1]*this.coefficients[2] +
	    3.0*t[2]*this.coefficients[3];

   return vel;
}


VelocityProfileCubic.prototype.setProfileDuration = function(duration)
{
  this.duration = duration;
  this.coefficients = this.getCubicSplineCoefficients(duration);
}


VelocityProfileCubic.prototype.getCubicSplineCoefficients = function(time)
{
  var coefficients = new Array();

  if (time == 0.0)
  {
    coefficients.push(this.pos2);
    coefficients.push(this.vel2);
    coefficients.push(0.0);
    coefficients.push(0.0);
  }
  else
  {
    var T = generatePowers(3, time);

    coefficients.push(this.pos1);
    coefficients.push(this.vel1);
    coefficients.push((-3.0*this.pos1 + 3.0*this.pos2 - 2.0*this.vel1*T[1] - this.vel2*T[1]) / T[2]);
    coefficients.push((2.0*this.pos1 - 2.0*this.pos2 + this.vel1*T[1] + this.vel2*T[1]) / T[3]);
  }
  return coefficients;
}

