function walkN(n, finishedCallback)
{
  var state = new Object();
  state.finishedCallback = finishedCallback;
  state.inFlight = 0;
  state.errors = 0;
  state.results = [];

  console.log("Walking for " + n);
  walkNRecursive(state, n, "f1", getRootGeom());
  timeout = 0;
}
var timeout = 1000;
// walkN(2, function() { console.log("asdf");})
function walkNRecursive(state, n, cell_id, geom)
{
  ++state.inFlight;

  //console.log("Requesting cell " + cell_id + " (" + (cell_id.length-1) + ") at " + n);
  getCell(cell_id,
	  function (cell) {
	    //console.log("Received cell " + cell_id);
	    --state.inFlight

	    // Checks if we're done
	    if (n == 1) {
	      // Sticks the geometry into the cell (still kinda gross)
	      cell.scale = geom.hi[0] - geom.lo[0];
	      cell.offset = V3.clone(geom.lo);
	      state.results.push(cell);

	      //console.log("Bottomed out at " + cell_id + " with " + state.inFlight + " in flight");
	      if (state.inFlight == 0) {
		if (state.errors > 0) {
		  console.warn("Tree walk had errors: " + state.errors);
		}
		state.finishedCallback(state.results, state.errors);
	      }
	    }
	    else {
	      // Descends to the children.
	      for (var child = 0; child < 8; ++child) {
		if (cell.hasChild(child)) {
		  walkNRecursive(state, n - 1, cell_id + child, geom.getChild(child));
		}
	      }
	    }
	  },
          function () {
	    --state.inFlight;
	    ++state.errors;
	    if (state.inFlight == 0) {
	      if (state.errors > 0) {
		console.warn("Tree walk had errors: " + state.errors);
	      }
	      state.finishedCallback(state.results, state.errors);
	    }
	  });
}
