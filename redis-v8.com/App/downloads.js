function process(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/downloads.html');
	var html = tmpl.render({page:'downloads'});
	res.send(html);
}

global.express.all('/downloads/', process);
global.express.all('/downloads', process);