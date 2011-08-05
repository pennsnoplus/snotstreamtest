function(e, data, reset) {
	if (e.from) $.log("triggered from: " + e.from);
	
	if (reset) {
		$$(this).data = [];
	}
	
	if (data && data.data) { 
		$$(this).data.push({ label: data.label, data : data.data });
	} else if (data && data.series) {
		for (i=0;i<data.series.length;i++) {
			$$(this).data.push({ label: data.series[i].label, data : data.series[i].data });
		}
	}

	if ($$(this).data && $$(this).data.length > 0) {
		$.plot($$(this).placeholder, $$(this).data, $$(this).evently.flot.options);
	}
	return false;
}