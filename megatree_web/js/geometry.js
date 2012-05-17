
function NodeGeometry(lo, hi)
{
  this.level = 1;
  this.lo = lo;
  this.hi = hi;
}

NodeGeometry.prototype.getCenter = function(){
  return V3.$((this.hi[0] + this.lo[0])/2.0,
	      (this.hi[1] + this.lo[1])/2.0,
	      (this.hi[2] + this.lo[2])/2.0);
}

NodeGeometry.prototype.getChild = function(to_child)
{
  var res = new NodeGeometry(new Float32Array(3), new Float32Array(3));
  res.level = this.level + 1;

  var mid0 = (this.lo[0] + this.hi[0]) / 2.0;
  var mid1 = (this.lo[1] + this.hi[1]) / 2.0;
  var mid2 = (this.lo[2] + this.hi[2]) / 2.0;
  
  res.lo[0] = (to_child & (1 << X_BIT)) ? mid0	: this.lo[0];
  res.hi[0] = (to_child & (1 << X_BIT)) ? this.hi[0]	: mid0;
  res.lo[1] = (to_child & (1 << Y_BIT)) ? mid1	: this.lo[1];
  res.hi[1] = (to_child & (1 << Y_BIT)) ? this.hi[1]	: mid1;
  res.lo[2] = (to_child & (1 << Z_BIT)) ? mid2	: this.lo[2];
  res.hi[2] = (to_child & (1 << Z_BIT)) ? this.hi[2]	: mid2;

  return res;
}
