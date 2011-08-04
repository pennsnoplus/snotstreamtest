function(query_data){
	var stash = {
		items: query_data.rows.map(function(r){
			return r.value;
		})
	};
	$$(this).recent_items = stash.items; // make the data accessible to after.js
	$.log("recent_items:");
	$.log($$(this).recent_items);
	return stash;
}
