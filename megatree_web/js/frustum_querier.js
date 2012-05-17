
// Extracts the frustum planes from a projection matrix.
//
// camera has -Z axis pointing forward (into the image)
function extract_frustum_planes(t)
{
  var norm = 0.0;
  var frustum = new Array(24);

  // Extract the numbers for the RIGHT plane 
  frustum[0] = t[0+3*4] - t[0+0*4];
  frustum[1] = t[3+1*4] - t[0+1*4];
  frustum[2] = t[3+2*4] - t[0+2*4];
  frustum[3] = t[3+3*4] - t[0+3*4];

  // Normalize the result 
  norm = Math.sqrt( frustum[0] * frustum[0] + frustum[1] * frustum[1] + frustum[2] * frustum[2] );
  frustum[0] /= norm;
  frustum[1] /= norm;
  frustum[2] /= norm;
  frustum[3] /= norm;

  // Extract the numbers for the LEFT plane 
  frustum[4] = t[0+3*4] + t[0+0*4];
  frustum[5] = t[3+1*4] + t[0+1*4];
  frustum[6] = t[3+2*4] + t[0+2*4];
  frustum[7] = t[3+3*4] + t[0+3*4];

  // Normalize the result 
  norm = Math.sqrt( frustum[4] * frustum[4] + frustum[5] * frustum[5] + frustum[6] * frustum[6] );
  frustum[4] /= norm;
  frustum[5] /= norm;
  frustum[6] /= norm;
  frustum[7] /= norm;

  // Extract the BOTTOM plane 
  frustum[8] = t[0+3*4] + t[1+0*4];
  frustum[9] = t[3+1*4] + t[1+1*4];
  frustum[10] = t[3+2*4] + t[1+2*4];
  frustum[11] = t[3+3*4] + t[1+3*4];

  // Normalize the result 
  norm = Math.sqrt( frustum[8] * frustum[8] + frustum[9] * frustum[9] + frustum[10] * frustum[10] );
  frustum[8] /= norm;
  frustum[9] /= norm;
  frustum[10] /= norm;
  frustum[11] /= norm;

  // Extract the TOP plane 
  frustum[12] = t[0+3*4] - t[1+0*4];
  frustum[13] = t[3+1*4] - t[1+1*4];
  frustum[14] = t[3+2*4] - t[1+2*4];
  frustum[15] = t[3+3*4] - t[1+3*4];

  // Normalize the result 
  norm = Math.sqrt( frustum[12] * frustum[12] + frustum[13] * frustum[13] + frustum[14] * frustum[14] );
  frustum[12] /= norm;
  frustum[13] /= norm;
  frustum[14] /= norm;
  frustum[15] /= norm;

  // Extract the NEAR plane 
  frustum[16] = t[0+3*4] + t[2+0*4];
  frustum[17] = t[3+1*4] + t[2+1*4];
  frustum[18] = t[3+2*4] + t[2+2*4];
  frustum[19] = t[3+3*4] + t[2+3*4];

  // Normalize the result 
  norm = Math.sqrt( frustum[16] * frustum[16] + frustum[17] * frustum[17] + frustum[18] * frustum[18] );
  frustum[16] /= norm;
  frustum[17] /= norm;
  frustum[18] /= norm;
  frustum[19] /= norm;

  // Extract the FAR plane 
  frustum[20] = t[0+3*4] - t[2+0*4];
  frustum[21] = t[3+1*4] - t[2+1*4];
  frustum[22] = t[3+2*4] - t[2+2*4];
  frustum[23] = t[3+3*4] - t[2+3*4];

  // Normalize the result 
  norm = Math.sqrt( frustum[20] * frustum[20] + frustum[21] * frustum[21] + frustum[22] * frustum[22] );
  frustum[20] /= norm;
  frustum[21] /= norm;
  frustum[22] /= norm;
  frustum[23] /= norm;

  return frustum;
}

// is any point of the cell inside the frustum
function isPartInsideFrustum(frustum_planes, cell_radius, pnt)
{
    // Determines if the node is inside the viewing frustum.
    for (var i = 0; i < 24; i += 4) {
	var n = frustum_planes[i+0] * pnt[0] +
                frustum_planes[i+1] * pnt[1] +
                frustum_planes[i+2] * pnt[2] +
                frustum_planes[i+3];
	if (n < -cell_radius) {  // Cell is completely outside the frustum
	    return false;
	}
    }
    return true;
}


// return -1: completely outside of frustum
// return 0: part inside, part outside of frustum
// return 1: completely inside of frustum
function isInsideFrustum(frustum_planes, cell_radius, pnt)
{
    // Determines if the node is inside the viewing frustum.
    var min_n = 2.0 * cell_radius;
    for (var i = 0; i < 24; i += 4) {
        // negative n = outside, positive n = inside
	var n = frustum_planes[i+0] * pnt[0] +
                frustum_planes[i+1] * pnt[1] +
                frustum_planes[i+2] * pnt[2] +
                frustum_planes[i+3];
	if (n < -cell_radius)   // Cell is completely outside the frustum
	    return -1;
        min_n = Math.min(min_n, n);
    }
    if (min_n > cell_radius)  // Cell is completely inside the frustum
      return 1; 
    return 0;   // Cell is part inside, part outside the frustum
}


// min distance is -cell_radius (completely outside frustum)
function distanceToFrustum(frustum_planes, cell_radius, pnt)
{
    // Determines if the node is inside the viewing frustum.
    var dist = 2.0*cell_radius;
    for (var i = 0; i < 24; i += 4) {
	var n = frustum_planes[i+0] * pnt[0] +
                frustum_planes[i+1] * pnt[1] +
                frustum_planes[i+2] * pnt[2] +
                frustum_planes[i+3];
	if (n <= -cell_radius)  // Outside the frustum
	    return -cell_radius;
        dist = Math.min(dist, n);
    }
    return dist;
}



function FrustumQuerier(projectionMatrix)
{
  this.setProjectionMatrix(projectionMatrix);
  this.inFlight = 0;

  this.resolution_correction = 1.0;
  this.cell_count = desired_cell_count;
  this.completed_full_frustum_query = false;
  this.resolution_step = 1.2;

}

FrustumQuerier.prototype.setProjectionMatrix = function(projectionMatrix)
{
  this.frustum_planes = extract_frustum_planes(projectionMatrix);
}


FrustumQuerier.prototype.cancel = function()
{
  for (var i = 0; i < this.storageRequests.length; ++i) 
      this.storageRequests[i].abort();
  
  this.inFlight = 0;
}


FrustumQuerier.prototype.getFrustum = function(trees_info, cameraMatrix, finishedCallback, progressCallback, G_M)
{
  // copy the parameters of this call into the class, to the call can re-trigger itself with the same parameters
  this.trees_info = trees_info;
  this.cameraMatrix = cameraMatrix;
  this.progressCallback = progressCallback;
  this.finishedCallback = finishedCallback;
  this.g_m = G_M;

  this.last_frustum_trigger_id = 0;
  this.triggerFrustum(0);
}




FrustumQuerier.prototype.triggerFrustum = function(id)
{
  if (this.last_frustum_trigger_id != 0 && id != this.last_frustum_trigger_id){ // we're in a self-induced loop
    console.log("Breaking self-induced loop");
    return;
  }
  this.last_frustum_trigger_id = id;

  // cancel all pending requests
  if (this.inFlight > 0) 
    this.cancel();

  // adapt resolution
  if (this.cell_count > desired_cell_count*this.resolution_step*1.1){
      this.resolution_correction *= this.resolution_step;
  }	
  else if (this.cell_count < desired_cell_count/this.resolution_step/1.1 && this.completed_full_frustum_query){      
      this.resolution_correction /= this.resolution_step;
      if (min_box_size_meters > this.g_m*this.resolution_correction){
	  this.resolution_correction *= this.resolution_step;
      }
  }
  this.corrected_g_m = this.g_m*this.resolution_correction;
  this.corrected_g_m_sq = this.corrected_g_m * this.corrected_g_m;

  // set state for this frustum query
  this.completed_full_frustum_query = false;
  this.cell_count = 0.0;
  this.queue = new Array();
  this.queue_index = 0;
  this.storageRequests = new Array();
  this.isProcessingQueue = true;
  this.interrupt_frustum_query = false;

  var this_ = this;

  // put tree roots on queue
  for (var i=0; i<trees_info.length; i++){ 
    var tree_info = trees_info[i];
    if (tree_info && tree_info.enabled){
      this.inFlight++;
      var xhr = getCell(tree_info.root, tree_info.geom,
                        // success
			function(child_cell){
                            // override color of cell if necessary
                            if (tree_info.override_color)
                                child_cell.override_color = tree_info.color;
                            else
                                child_cell.override_color = [-1, -1, -1];         
			    
                            // add cell to process queue
			    this.queue.push(child_cell);
			    this.inFlight--;
			    if (!this.isProcessingQueue)
				this.processQueue();
			}.bind(this_),
                        // failure
			function(){console.log("Failed to load cell " + tree_info.root);});
	if (xhr)
	    this.storageRequests.push(xhr);
    }
  }

  // start processing root cells
  if (this.queue.length > 0)
      this.processQueue();

  // root cells were not in cache, so start processing as soon as they arrive
  else
      this.isProcessingQueue = false;

  // returns results from cache 
  return this.queue;
}    


FrustumQuerier.prototype.processQueue = function()
{
  this.process_calls_in_a_row = 0;

  // proces the queue
  while (this.queue_index < this.queue.length && !this.interrupt_frustum_query){
    this.processCell(this.queue[this.queue_index]);
    this.queue_index++;
    if(this.process_calls_in_a_row > 50)
    {
      var this_ = this;
      setTimeout(function() {this.processQueue();}.bind(this_), 1);
      //this.progressCallback(this.queue);  // Can't do this
      return;
    }
  }
  if (this.process_calls_in_a_row > 0) {
    //console.log("Processed " + this.process_calls_in_a_row + " cells in a row");
  }
  this.isProcessingQueue = false;
  
  // give results
  if (this.inFlight > 0){
    var result = new Array();
    for (var i = 0; i < this.queue.length; ++i) {
      result[i] = this.queue[i].shallowCopy();
    }
    this.progressCallback(result);
    //this.progressCallback(this.queue);
  }
  else{
    if (this.cell_count < desired_cell_count / 1.5 && min_box_size_meters < this.corrected_g_m / this.resolution_step){ 
      var this_ = this;
      setTimeout(function(){this.triggerFrustum(1.0);}.bind(this_), 10);  // we want more cells
    }

    this.completed_full_frustum_query = true;
    var result = new Array();
    for (var i = 0; i < this.queue.length; ++i) {
      result[i] = this.queue[i].shallowCopy();
    }
    this.finishedCallback(result);
    //this.finishedCallback(this.queue);
  }
}



FrustumQuerier.prototype.processCell= function(cell)
{
  var this_ = this;
  if (( this.cell_count + this.inFlight) > desired_cell_count * 1.5){
    this.interrupt_frustum_query = true;
    this.cell_count = this.cell_count + this.inFlight;
    this.cancel();
    setTimeout(function(){this.triggerFrustum(-1.0);}.bind(this_), 10);  // we want fewer cells
    return;
  }
  this.cell_count++;
 

  // store show children in cell, this is pretty bad
  cell.show_children = cell.getChildren();

  // Tell the parent this cell has been loaded
  if (cell.parent_cell)
    cell.parent_cell.show_children &= ~(1 << cell.which_child);

  // Finished if this is a leaf
  if (cell.isLeaf()) 
    return;

  // cell center in camera frame
  var cell_in_camera = V3.mul4x4(this.cameraMatrix, cell.getCenter());
  var dist_sq = V3.lengthSquared(cell_in_camera);

  // Finished if this cell is small enough
  if (cell.node_size*cell.node_size <= this.corrected_g_m_sq * dist_sq) {
    return;
  }

  // Add the children of this cell to the queue
  for (var child = 0; child < 8; ++child) {
    if (cell.hasChild(child)) {
      var child_geom = cell.geometry.getChild(child);

      // If the cell is not completely inside frustum, compute more details for the cell
      if (cell.inside_frustum < 1){   // cell is not completely inside the frustum
	  // compute center of child cell in camera coordinates
	  var child_in_camera = V3.mul4x4(this.cameraMatrix, child_geom.getCenter());
          var child_in_frustum = isInsideFrustum(this.frustum_planes, cell.radius/2.0, child_in_camera);
      }
      else
        var child_in_frustum = cell.inside_frustum;
      
      // Load the child if it is inside the frustum
      if (cell.inside_frustum > 0 || child_in_frustum > -1){
          this.inFlight++;
	  var xhr = getCell(cell.id + child, child_geom,
                            // success
                            function(child_cell){
                                // override color of cell if necessary
                                child_cell.override_color = cell.override_color;
				
                                // tell cell who its parent is
				child_cell.parent_cell = cell;
				child_cell.inside_frustum = child_in_frustum;

                                // add cell to process queue
                                this.queue.push(child_cell);
                                this.inFlight--;
                                if (!this.isProcessingQueue)
                                {
                                    this.processQueue();
                                }
                                else
                                {
                                    this.process_calls_in_a_row++;
                                }
                            }.bind(this_),
                            // failure
			    function(){console.log("Failed to load cell " + (cell.id + child))});
	  if (xhr)
	      this.storageRequests.push(xhr);
      }
      else{
        // child is outside of frustum, so hide this part of the parent
        // TODO: this line should not be necessary, but it fixes this view:
        // http://bhc.willowgarage.com/?tree=tahoe_all&c_x=763241&c_y=4312897&c_z=2209.0708&c_d=29433.8805&c_pi=-0.6554054&c_ya=0.9046338
        cell.show_children &= ~(1 << child);   
      }
    }
    else{
      // parent has no child here, so it's empty space
      cell.show_children &= ~(1 << child);      
    }
  } 
}


