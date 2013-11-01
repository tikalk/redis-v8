function process(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/author.html');
	var html = tmpl.render({page:'author'});
	res.send(html);
};

global.express.all('/author/', process);
global.express.all('/author', process);