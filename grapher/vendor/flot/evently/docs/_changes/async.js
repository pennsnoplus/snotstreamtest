function(callback, e) {
	//$.log("async.js");
	var id = $$(this).evently.flot.id;
	$.log("doc id: " + id);
	if (id) {
		$$(this).app.db.openDoc(id, { success: callback });
	}
}