function(query_data){
	var stash = {
		dimensions: $$(this).evently.flot.dimensions,
		items: query_data.rows.map(function(r){
			return {
				timestamp: r.key,
				qlx_value: r.value
			};
		})
	};
	$$(this).qlx = stash.items; // make the data accessible to after.js
	$.log("qlx:");
	$.log($$(this).qlx);
	return stash;
}
