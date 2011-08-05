function(){
	$$(this).qhs_graph = $("#qhs_graph", this);
	var data = $$(this).qhs;
	var items = [];
	var qhs = [];
	data.map(function(r){
		if(r.timestamp != undefined){
			if(r.qhs_value != undefined){
				qhs.push([r.timestamp, r.qhs_value]);
			}
			else{
				qhs.push([r.timestamp, NULL]);
			}
		}
	});
	items.push({label: "QHS", color: "red", data: qhs, shadowSize: 0});
	$.plot($$(this).qhs_graph, items, $$(this).evently.flot.options);
}
