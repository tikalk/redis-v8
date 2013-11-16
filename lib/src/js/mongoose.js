console.log('mongoose.js');

//mongoose
mongoose = (function(){
	var modelNames = {};
	/*
		Schema - http://mongoosejs.com/docs/api.html#schema_Schema
	*/
	function Schema(definition){
		console.log('Schema()');
		this.indexTypes = function(){
			//The allowed index types
			throw "not implemented";
		}
		this.reserved = 'on, emit, _events, db, init, isNew, errors, schema, options, modelName, collection, _pres, _posts, toObject'.split(', ');
		this.methods = {};
	}
	Schema.prototype = {
		add: function(obj, prefix){
			//Adds key path / schema type pairs to this schema.
			throw "not implemented";
		},
		eachPath: function(fn){
			//The callback is passed the pathname and schemaType as arguments on each iteration.
			throw "not implemented";
		},
		get: function(key){
			//Gets a schema option.
			throw "not implemented";
		},
		index: function(fields, options){
			//Defines an index (most likely compound) for this schema.
			throw "not implemented";
		},
		indexes: function(){
			//Compiles indexes from fields and schema-level indexes
			throw "not implemented";
		},
		method: function(method, fn){
			//Adds an instance method to documents constructed from Models compiled from this schema.
			this.methods[method] = fn; 
		},
		path: function(path, constructor){
			//Gets/sets schema paths.
			throw "not implemented";
		},
		pathType: function(path){
			//Returns the pathType of path for this schema.
			throw "not implemented";
		},
		plugin: function(plugin, opts){
			//Registers a plugin for this schema.
			throw "not implemented";
		},
		post: function(method, fn){
			//Defines a post for the document
			throw "not implemented";
		},
		pre: function(method, callback){
			//Defines a pre hook for the document.
			throw "not implemented";
		},
		requiredPaths: function(){
			//Returns an Array of path strings that are required by this schema.
			throw "not implemented";
		},
		set: function(key, value){
			//Sets/gets a schema option.
			throw "not implemented";
		},
		static: function(name, fn){
			//Adds static "class" methods to Models compiled from this schema.
			throw "not implemented";
		},
		virtual: function(name, options){
			//Creates a virtual type with the given name.
			throw "not implemented";
		},
		virtualpath: function(name){
			//Returns the virtual type with the given name.
			throw "not implemented";
		}
	};
	
	/*
		model - http://mongoosejs.com/docs/api.html#index_Mongoose-model
	
		Mongoose#model(name, [schema], [collection], [skipInit])

		Defines a model or retrieves it.

		Parameters:

		name <String> model name
		[schema] <Schema>
		[collection] <String> name (optional, induced from model name)
		[skipInit] <Boolean> whether to skip initialization (defaults to false)
	*/
	function model(name, schema, collection, skipInit){
		
	}
	
	/*
		Document - http://mongoosejs.com/docs/api.html#document-js
		private api
		* @param {Object} obj the values to set
		* @param {Object} [opts] optional object containing the fields which were selected in the query returning this document and any populated paths data
		* @param {Boolean} [skipId] bool, should we auto create an ObjectId _id
		* @inherits NodeJS EventEmitter http://nodejs.org/api/events.html#events_class_events_eventemitter
		* @event `init`: Emitted on a document after it has was retreived from the db and fully hydrated by Mongoose.
		* @event `save`: Emitted when the document is successfully saved
		* @api private
	*/
	function Document(obj, fields, skipId) {
		
	}
	
	Document.prototype = {
	};
	
	/*
		Model - http://mongoosejs.com/docs/api.html#model_Model
		Model constructor

		Parameters:

		doc <Object> values with which to create the document
		Inherits:
			Document
		
		Events:
			error: If listening to this event, it is emitted when a document was saved without passing a callback and an error occurred. If not listening, the event bubbles to the connection used to create this Model.

			index: Emitted after Model#ensureIndexes completes. If an error occurred it is passed with the event.

		Provides the interface to MongoDB collections as well as creates document instances.
	*/
	function Model(doc){
		
	}
	Model.prototype = {
		
	};
	
	return {
		Schema: Schema,
		model: model,
		Model: Model,
		modelNames: function(){
			return Object.keys(modelNames);
		},
		Types: {
			String: String,
			Number: Number,
			Boolean: Boolean,
			Bool: Boolean,
			Array: Array
		}
	};
})();

console.log('mongoose', mongoose);


var blogSchema = new mongoose.Schema({
	title:	String,
	author: String,
	body:	 String,
	views: Number,
	// comments: [{ body: String, date: Date }],
	// date: { type: Date, default: Date.now },
	hidden: Boolean,
	// meta: {
	// 	votes: Number,
	// 	favs:	Number
	// }
});

blogSchema.method('getTitle', function(){
	return this.title;
});

var Blog = mongoose.model('Blog', blogSchema)
var blog = new Blog({ title: 'Post title', author: 'h0x91B', body: 'Hello!', hidden: false });
console.log('blog.getTitle()', blog.getTitle());
blog.save(function(err){
	if(err) throw err;
});
console.log(blog.title);