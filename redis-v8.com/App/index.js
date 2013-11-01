console.log('index page');

global.express.all('/',function(req,res){
	var tmpl = global.swig.compileFile(__dirname+'/../Templates/index.html');
	var html = tmpl.render({page:'index'});
	res.send(html);
})