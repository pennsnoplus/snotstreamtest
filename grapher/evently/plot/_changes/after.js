function() {
 // #placeholder is now defined since the mustache.html has been rendered
    $$(this).placeholder = $("#placeholder", this); // store #placeholder in $$(this)
  var data=$$(this).data; // data now has the result of the view 
  var items=[];
  var qhl = [];
  var qhs = [];
  var qlx = [];
  var i=0;
  $.log(data);
  data.rows.map(function(r) { // this goes through the veiw, r.value is the doc
  	$.log(r);
  	if(r.key != undefined){
		if(r.value.qhl != undefined){
			qhl.push([r.key, r.value.qhl]);
		}
		else {
			qhl.push([r.key, NULL]);
		}
		if(r.value.qhs != undefined){
			qhs.push([r.key, r.value.qhs]);
		}
		else {
			qhs.push([r.key, NULL]);
		}
		if(r.value.qlx != undefined){
			qlx.push([r.key, r.value.qlx]);
		}
		else {
			qlx.push([r.key, NULL]);
		}
	}
  	  /*
	  if(r.value.message !=undefined){
	    var p=[];
	    p.push(i);
	    if(r.value.message=="hi") p.push(1);
	    else p.push(-1);
		items.push(p);
	    i++;
	    }
		*/
	  /*
	  if(r.value.mb_id !=undefined&& r.value.pass !=undefined){
		var p = [];
		p.push(i);
		if(r.value.pass=="yes") p.push(1);
		else p.push(-1);
		items.push(p);
		i++;
	  }
	  */
    });
  items.push(qhl);
  items.push(qhs);
  items.push(qlx);
  $.log(items);
  //$.plot($$(this).placeholder, [items] ,$$(this).evently.flot.options);
  $.plot($$(this).placeholder, items, $$(this).evently.flot.options);

}
