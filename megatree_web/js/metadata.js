function MetaData(data)
{
  // set defaults
  this.version = 0;
  this.subtree_width = 0;
  this.subfolder_depth = 0;
  this.min_cell_size = 0;
  this.root_size = 0;
  this.root_center = V3.$(0.0, 0.0, 0.0);
  this.camera_origin = V3.$(0.0, 0.0, 0.0);
  this.camera_distance = 0.0;
  this.camera_yaw = 0.0;
  this.camera_pitch = 0.0;
  this.with_viewpoint = false;

  // parse metadata
  var lines = data.split("\n");
  for (l in lines){
   if (lines[l].length > 0){
     var line = lines[l].split(" = ");
     var nr = parseFloat(line[1]);
     switch (line[0]){
       case "version":
	 this.version = nr;
         break;
       case "min_cell_size":
	 this.min_cell_size = nr;
         break;
       case "subtree_width":
	 this.subtree_width = nr;
         break;
       case "subfolder_depth":
	 this.subfolder_depth = nr;
         break;
       case "tree_center_x":
	 this.root_center[0] = nr;
         break;
       case "tree_center_y":
	 this.root_center[1] = nr;
         break;
       case "tree_center_z":
	 this.root_center[2] = nr;
         break;
       case "tree_size":
	 this.root_size = nr;
         break;
       case "default_camera_center_x":
	 this.with_viewpoint = true;
	 this.camera_origin[0] = nr;
         break;
       case "default_camera_center_y":
	 this.with_viewpoint = true;
	 this.camera_origin[1] = nr;
         break;
       case "default_camera_center_z":
	 this.with_viewpoint = true;
	 this.camera_origin[2] = nr;
         break;
       case "default_camera_distance":
	 this.with_viewpoint = true;
	 this.camera_distance = nr;
         break;
       case "default_camera_yaw":
	 this.with_viewpoint = true;
	 this.camera_yaw = nr;
         break;
       case "default_camera_pitch":
	 this.with_viewpoint = true;
	 this.camera_pitch = nr;
         break;
       default:
         console.log("Unknown line while parsing metadata " + line);
     }
   }
  }
  if (this.subfolder_depth < 30){
      $.prompt("Web megatree does not work with subfolder depth less than 30!");
  }
}


MetaData.prototype.toString = function()
{
  var res = "";
  res += "  version: " + this.version + "\n";
  res += "  min_cell_size: " + this.min_cell_size + "\n";
  res += "  subtree_width: " + this.subtree_width + "\n";
  res += "  subfolder_depth: " + this.subfolder_depth + "\n";
  res += "  tree_center: " + this.root_center[0] + " " + this.root_center[1] + " " + this.root_center[2] + "\n";
  res += "  tree_size: " + this.root_size + "\n";
  res += "  camera_origin: " + this.camera_origin[0] + " " + this.camera_origin[1] + " " + this.camera_origin[2] + "\n";
  res += "  camera_distance: " + this.camera_distance + "\n";
  res += "  camera_yaw: " + this.camera_yaw + "\n";
  res += "  camera_pitch: " + this.camera_pitch + "\n";
  return res;
}

 
