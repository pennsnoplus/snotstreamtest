function() {
 // #placeholder is now defined since the mustache.html has been rendered
    $$(this).placeholder = $("#placeholder", this); // store #placeholder in $$(this)
  var data=$$(this).data; // data now has the result of the view 
  var items=[];
  var i=0;
  data.rows.map(function(r) { // this goes through the veiw, r.value is the doc
	  if(r.value.message !=undefined){
	    var p=[];
	    p.push(i);
	    if(r.value.message=="hi") p.push(1);
	    else p.push(-1);
		items.push(p);
	    i++;
	    }
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
    $.plot($$(this).placeholder, [items] ,$$(this).evently.flot.options);

}
