function process(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/roadmap.html');
	var html = tmpl.render({page:'roadmap'});
	res.send(html);
}
global.express.all('/roadmap/', process);
global.express.all('/roadmap', process);