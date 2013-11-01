function process(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/benchmarks.html');
	var html = tmpl.render({page:'benchmarks'});
	res.send(html);
}

global.express.all('/benchmarks/', process);
global.express.all('/benchmarks', process);