var redis = require("../../Client-Libraries/NodeJS/node_redis-0.8.3/"),
	client = redis.createClient();

client.on("error", function (err) {
	console.log("Error " + err);
});

client.set(['bench_key', '0'], function(err,reply){
	var start = +new Date;
	var done = 0;
	
	for(var i=0;i<100000;i++){
		client.incr('bench_key',function(err, reply){
			done++;
			if(done==100000){
				var dt = new Date - start;
				console.log('Speed: '+Math.round(100000/(dt/1000))+' ops/sec')
				client.quit();
			}
		});
	}
});
