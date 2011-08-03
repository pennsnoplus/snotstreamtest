function(query_data){
	var stash = {
		name: "Peter",
		items: query_data.rows.map(function(r){
			return r.value;
		})
	};
	$.log(stash.items);
	return stash;
}
