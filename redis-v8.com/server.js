var express = require('express');
var app = express();

global.express = app;

var swig  = require('swig');
swig.init({
	allowErrors: false,
	autoescape: true,
	cache: false,
	encoding: 'utf8',
	filters: {},
	root: './Templates/',
	tags: {},
	extensions: {},
	tzOffset: 0
});

app.use(express.static(__dirname + '/static/',{ maxAge: 600000 }));
app.use(express.compress());
app.use(express.bodyParser());

global.swig = swig;
require('./App/index.js');
require('./App/docs.js');
require('./App/quick-start.js');
require('./App/downloads.js');
require('./App/author.js');
require('./App/benchmarks.js');
require('./App/roadmap.js');
require('./App/try.js');


app.listen(3000);
console.log('Listening on port 3000');