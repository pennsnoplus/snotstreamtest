// the async result should already be a document
// completely conforming to the flot specs...
function(data) {
  //reset the plot
  var source = "evently.docs.changes.data";
  $(this).trigger({ type: "refreshPlot", from : source }, [data, true]);

  return {};
};