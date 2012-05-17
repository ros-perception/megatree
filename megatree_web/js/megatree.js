
var X_BIT = 2;
var Y_BIT = 1;
var Z_BIT = 0;


function TreeInfo(name, metadata, color)
{
    this.enabled = true;
    this.name = name;
    this.metadata = metadata;
    this.override_color = false;
    this.color = color;
    this.root = name + "/f1";
    this.geom = getRootGeom(metadata);
}

function JobInfo(name)
{
    this.name = name;
    this.finished = false;
    this.failed = false;
    this.removed = false;
    this.tree_name = null;
}
JobInfo.prototype.remove = function()
{
  // find the corresponding tree
  if (this.tree_name){
    console.log("Searching for tree " + this.tree_name + " so we can remove it");
    for (var i=0; i<trees_info.length; i++){
      if (trees_info[i] && trees_info[i].name == this.tree_name){
        console.log("removing tree " + this.tree_name);
        trees_info[i] = null;
        triggerFrustumUpdate();
      }
    }
  }
  this.removed = true;
}


function getRootGeom(tree_metadata)
{
  if (tree_metadata == null){
      $.prompt("Can't get root geometry because metadata is not loaded");
      return new NodeGeometry(V3.$(0,0,0), V3.$(0,0,0));
  }
  else{
      var s = tree_metadata.root_size / 2.0;
      var c = tree_metadata.root_center;
      return new NodeGeometry(V3.$(c[0]-s ,c[1]-s, c[2]-s), V3.$(c[0]+s, c[1]+s, c[2]+s));
  }
}






////////////////////////////////////////////////////////////////////////////////
//   Comm
////////////////////////////////////////////////////////////////////////////////

function getMetaData(tree_name, successCallback, failureCallback)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/files/" + tree_name + "/metadata.ini", true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      console.log("Received metadata of tree " + tree_name);
      var metadata = new MetaData(xhr.response);
      successCallback(tree_name, metadata);
    }
    else {
      console.log("Failed to get metadata of tree " + tree_name);
      failureCallback();
    }
  };
  xhr.send();
  
  return xhr;
}


function getViews(tree_name, successCallback)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/files/" + tree_name + "/views.ini", true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      var views = new Array();
      var views_list = xhr.response.split("\n");
      for (var i=0; i<views_list.length; i++){
	var view = getViewpointFromQuery(views_list[i]);
	if (view){
          views.push(view);
	}
      }
      successCallback(tree_name, views);
    }
    else {
      console.log("Failed to get views of tree " + tree_name);
    }
  };
  xhr.send();
  
  return xhr;
}



function getSupportedTrees(successCallback, failureCallback)
{
  failureCallback = (typeof failureCallback == "undefiend") ? function() {} : failureCallback;

  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/files/supported_trees", true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      successCallback(xhr.response.split(","));
    }
    else {
      failureCallback();
    }
  };
  xhr.send();
  
  return xhr;
}



var totalCellBytesDownloaded = 0;

function getCell(cell_id, cell_geometry, successCallback, failureCallback)
{
  //console.log("Getting cell " + cell_id);

  failureCallback = (typeof failureCallback == "undefined") ? function() {} : failureCallback;

  // Shortcuts the query if the cell is in cache
  var from_cache = cellCache.get(cell_id);
  if (from_cache) {
    successCallback(from_cache);
    return null;
  }

  if (RUN_ON_CACHE_ONLY){
    return null;
  }

  // Unfortunately, jquery doesn't yet support arraybuffer responses,
  // so we use xhr manually.
  //
  // See: http://blog.vjeux.com/2011/javascript/jquery-binary-ajax.html
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/files/" + cell_id, true);
  xhr.responseType = "arraybuffer";

  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      if(xhr.response == null)
      {
        console.log("Null response for " + cell_id);
        console.log(xhr);
      }

      totalCellBytesDownloaded += xhr.response.byteLength;
      var cell_object = new Cell(cell_id, cell_geometry, xhr.response);
      cellCache.put(cell_id, cell_object);
      //cell_object.loadGL(ctx);
      //cell_object.toBox();

      //console.log("Children of file " + cell_id + " = " + cell_object.children.toString(2));
      successCallback(cell_object);
    }
    else {
      failureCallback();
    }
  };
  xhr.send();
  
  return xhr;
}


