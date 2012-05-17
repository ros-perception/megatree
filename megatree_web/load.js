
var do_include = function(path){
  var lastScript = document.getElementsByTagName("head")[0].lastChild;
  //var fullUrl = lastScript.src.substring(0, lastScript.src.lastIndexOf('/') + 1) + path;
  var fullUrl = path;
  document.write('<' + 'script');
  document.write(' language="javascript"');
  document.write(' type="text/javascript"');
  document.write(' src="' + fullUrl + '">');
  document.write('</' + 'script' + '>');
}


do_include('js/jquery-1.7.1.min.js');
do_include('js/jquery-ui-1.8.17.custom.min.js');
//do_include('js/jquery.popupWindow.js');
do_include('js/ui/jquery.ui.core.js');
do_include('js/ui/jquery.ui.widget.js');
do_include('js/ui/jquery.ui.mouse.js');
do_include('js/ui/jquery.ui.progressbar.js');
do_include('js/ui/jquery.ui.resizable.js');
do_include('js/ui/jquery.ui.dialog.js');

do_include('js/point_cloud.js');
do_include('js/mjs.js');
do_include('js/orbitcam.js');
do_include('js/lru_cache.js');
do_include('js/metadata.js');
do_include('js/cell.js');
do_include('js/geometry.js');
do_include('js/frustum_querier.js');
do_include('js/megatree.js');
do_include('js/utm.js');
do_include('js/selection.js');
do_include('js/road_detection.js');
do_include('js/jquery-impromptu.js');
do_include('js/measure.js');

do_include('js/velocityprofile_trap.js');
do_include('js/velocityprofile_trap3d.js');
do_include('js/velocityprofile_mapped.js');
do_include('js/velocityprofile_mapped_double.js');
do_include('js/velocityprofile_mapped3d.js');
do_include('js/velocityprofile_cubic.js');
do_include('js/velocityprofile_cubic3d.js');
do_include('js/velocityprofile_double_cubic.js');
do_include('js/velocityprofile_slowdown.js');
do_include('js/index.js');

