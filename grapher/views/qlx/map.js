function(doc){
	if(doc.qlx && doc.ts){
		emit(doc.ts, doc.qlx);
	}
}
