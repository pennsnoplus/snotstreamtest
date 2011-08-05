function(data) {// data is the view result specified in query.json
    // what gets returned goes to the mustache.html
   $$(this).data=data; // this stores data to be used in after.js
  return {
    dimensions : $$(this).evently.flot.dimensions // looks at eh flot.json file
  }
};
