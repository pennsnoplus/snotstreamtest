function(){
	$$(this).qlx_graph = $("#qlx_graph", this);
	var data = $$(this).qlx;
	var items = [];
	var qlx = [];
	data.map(function(r){
		if(r.timestamp != undefined){
			if(r.qlx_value != undefined){
				qlx.push([r.timestamp, r.qlx_value]);
			}
			else{
				qlx.push([r.timestamp, NULL]);
			}
		}
	});
	items.push({label: "QLX", color: "blue", data: qlx, shadowSize: 0});
	$.plot($$(this).qlx_graph, items, $$(this).evently.flot.options);
}
