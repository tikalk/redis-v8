console.log('docs page');

function process(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/docs.html');
	var html = tmpl.render({page:'docs'});
	res.send(html);
}

global.express.all('/docs', process);
global.express.all('/docs/', process);
