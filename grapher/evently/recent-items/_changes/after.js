function(){
	//alert("set _value = 500");
	//$$(this)._value = 500;
	//$.log("set _value = 500");
	$$(this).recent_items_graph = $("#recent_items_graph", this);
	$.log($$(this).recent_items_graph);
	var data = $$(this).recent_items;
	var items = [];
	var qhl = [];
	var qhs = [];
	var qlx = [];
	var i = 0;
	//$.log(data);
	$.log("data=");
	$.log(data);
	data.map(function(r){
		//$.log(r);
		if(r.ts != undefined){
			if(r.qhl != undefined){
				qhl.push([r.ts, r.qhl]);
			}
			else {
				qhl.push([r.ts, NULL]);
			}

			if(r.qhs != undefined){
				qhs.push([r.ts, r.qhs]);
			}
			else {
				qhs.push([r.ts, NULL]);
			}

			if(r.qlx != undefined){
				qlx.push([r.ts, r.qlx]);
			}
			else {
				qlx.push([r.ts, NULL]);
			}
		}
	});
	items.push({label: "QHL", color: "green", data: qhl, shadowSize: 0, hoverable: true});
	items.push({label: "QHS", color: "red", data: qhs, shadowSize: 0, hoverable: true});
	items.push({label: "QLX", color: "blue", data: qlx, shadowSize: 0, hoverable: true});
	//$.log(items);
	$.plot($$(this).recent_items_graph, items, $$(this).evently.flot.options);
	$$(this).qhl_qhs_graph = $("#qhl_qhs_graph", this);
	$.plot($$(this).qhl_qhs_graph, items.slice(0,2), $$(this).evently.flot.options);
	$$(this).qlx_graph = $("#qlx_graph", this);
	$.plot($$(this).qlx_graph, items.slice(2), $$(this).evently.flot.options);
}
