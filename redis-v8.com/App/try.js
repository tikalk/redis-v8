var redis = require(__dirname+'/../Lib/node_redis-0.8.3/');
var client = redis.createClient('/tmp/redis.sock');

client.on('error', function(err){
	console.log('redis client error', err);
});

function page(req, res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/try.html');
	var html = tmpl.render({page:'try'});
	res.send(html);
}

global.express.get('/try/', page);
global.express.get('/try', page);

global.express.post('/try', function(req, res){
	if(!req.body.code){
		res.send('{"Error": "No code..."}');
		return;
	}
	var start = +new Date;
	client.js([req.body.code], function(err, result){
		if(err){
			console.log(err, typeof err);
			res.send(JSON.stringify({ERROR: err.toString()}));
			return;
		}
		res.send(JSON.stringify({result: result, time: (+new Date)-start}));
		return;
	});
});