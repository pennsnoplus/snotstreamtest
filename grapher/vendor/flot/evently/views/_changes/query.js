function() {
	var startkey = $$(this).evently.flot.startkey;
	var endkey = $$(this).evently.flot.endkey;
	
	var view = { view : $$(this).evently.flot.view }
	if (startkey) {
		view.startkey = startkey;
	}
	if (endkey) {
		view.endkey = endkey;
	}
	
	return view;
}