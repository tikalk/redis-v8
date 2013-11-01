function process(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/quick-start.html');
	var html = tmpl.render({page:'quick-start'});
	res.send(html);
}

global.express.all('/quick-start/', process);
global.express.all('/quick-start', process);
