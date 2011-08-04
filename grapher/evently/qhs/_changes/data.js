function(query_data){
	var stash = {
		items: query_data.rows.map(function(r){
			return {
				timestamp: r.key,
				qhs_value: r.value
			};
		})
	};
	$$(this).qhs = stash.items; // make the data accessible to after.js
	$.log("qhs:");
	$.log($$(this).qhs);
	return stash;
}
