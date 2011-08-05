function(query_data){
	var stash = {
		dimensions: $$(this).evently.flot.dimensions,
		items: query_data.rows.map(function(r){
			return {
				timestamp: r.key,
				qhl_value: r.value
			};
		})
	};
	$$(this).qhl = stash.items; // make the data accessible to after.js
	$.log("qhl:");
	$.log($$(this).qhl);
	return stash;
}
