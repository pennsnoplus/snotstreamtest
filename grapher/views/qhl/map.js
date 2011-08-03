function(doc){
	if(doc.qhl && doc.ts){
		emit(doc.ts, doc.qhl);
	}
}
