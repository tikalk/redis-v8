//Example DB JS script
function thisFuncHaveName(){
	console.log('example.js', 1, 2, 3);
	abc();
}

function crash(){
	thisFuncHaveName();
}

function MakeMillionKeys(){
	for(var i=0;i<1000000;i++){
		redis.set('KEY'+i,'some value');
	}
}

function getRandom100keys(){
	var ret = Array(100);
	for(var i=0;i<100;i++){
		var key = 'KEY'+Math.floor(Math.random()*1000000);
		var value = redis.get(key);
		ret[i] = {
			key: key,
			value: value
		}
	}
	return ret;
}
