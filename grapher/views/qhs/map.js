function(doc){
	if(doc.qhs && doc.ts){
		emit(doc.ts, doc.qhs);
	}
}
