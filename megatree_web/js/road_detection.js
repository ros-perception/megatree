
function detectRoads(tree_name, box, progressCallback, successCallback, failureCallback)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/algorithm/road_detection" + 
	   "?l_x=" + Math.min(box[0][0], box[1][0]) + "&l_y=" + Math.min(box[0][1], box[1][1]) + "&l_z=" + Math.min(box[0][2], box[1][2]) + 
	   "&h_x=" + Math.max(box[0][0], box[1][0]) + "&h_y=" + Math.max(box[0][1], box[1][1]) + "&h_z=" + Math.max(box[0][2], box[1][2]) + 
	   "&tree_name=" + tree_name, true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      console.log("Road detection started creating a new tree " + xhr.response);
      pollRoadDetectionProgress(xhr.response, 
				function(progress){progressCallback(progress);},
				successCallback);
    }
    else {
      console.log("Road detection failed");
      failureCallback();
    }
  };
  xhr.send();
  


  return xhr;
}



function pollRoadDetectionProgress(tree_name, progressCallback, finishedCallback)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/algorithm/progress_monitor" + 
	   "?tree_name=" + tree_name, true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      var progress = parseFloat(xhr.response);
      console.log("Road detection is " + progress*100 + " % complete.");
      if (progress < 1.0){
	  progressCallback(xhr.response);
	  setTimeout(function(){pollRoadDetectionProgress(tree_name, progressCallback, finishedCallback)}, 1000); 
      }
      else{
	  progressCallback(xhr.response);
	  pollRoadDetectionCompletion(tree_name, finishedCallback);
      }
    }
    else {
      console.log("Polling road detection progress failed, trying again");
      setTimeout(function(){pollRoadDetectionProgress(tree_name, progressCallback, finishedCallback)}, 1000); 
    }
  };
  xhr.send();
  
  return xhr;
}
 


function pollRoadDetectionCompletion(tree_name, finishedCallback)
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/algorithm/completion_monitor" + 
	   "?tree_name=" + tree_name, true);
  xhr.responseType = "text";
  xhr.onload = function (evt) {
    if (xhr.status == 200) {
      var finished = parseInt(xhr.response);
      if (finished){
	  console.log("Road detection tree generation is complete");
	  finishedCallback(tree_name);
      }
	else{
	  console.log("Road detection is generating tree " + tree_name);
	  setTimeout(function(){pollRoadDetectionCompletion(tree_name, finishedCallback)}, 1000); 
      }
    }
    else {
      console.log("Polling road detection completion failed, trying again");
      setTimeout(function(){pollRoadDetectionCompletion(tree_name, finishedCallback)}, 1000); 
    }
  };
  xhr.send();
  
  return xhr;
}