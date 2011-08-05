function(){
	$$(this).qhl_graph = $("#qhl_graph", this);
	var data = $$(this).qhl;
	var items = [];
	var qhl = [];
	data.map(function(r){
		if(r.timestamp != undefined){
			if(r.qhl_value != undefined){
				qhl.push([r.timestamp, r.qhl_value]);
			}
			else{
				qhl.push([r.timestamp, NULL]);
			}
		}
	});
	items.push({label: "QHL", color: "green", data: qhl, shadowSize: 0});
	$.plot($$(this).qhl_graph, items, $$(this).evently.flot.options);
}
