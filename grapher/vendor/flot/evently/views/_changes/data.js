// parse the data returned by the view
// this currently works like this:
// the view returns one 'master' document, containing the value
// and 'n' 'data' documents that contain: 'seq', 'value'
// these documents are used to generate a series like:
// { label : 'label', data : [ [x1, y1], [x2,y2],...]}
// finally this series is used to trigger the refreshPlot event
function(data) {

  var items = [];

  var label = "no title, add a label";
  data.rows.map(function(r) {
	  if (r.value.label) {
		label = r.value.label; 
	  } 
	  if (r.value.value && (r.value.seq != undefined)) {
		var p = [];
		p.push(r.value.seq);
		p.push(r.value.value);
		items.push(p);
	  }
    });

  var source = "evently.views.changes.data";
  $(this).trigger({ type: "refreshPlot", from : source }, [{ label : label, data : items }, true]);
}