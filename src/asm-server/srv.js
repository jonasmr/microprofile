const http = require('http');
var mongo = require('mongodb');
const hostname = '127.0.0.1';
const port = 3000;
const ServerURL = "mongodb://localhost:27017/mydb";
var Client = mongo.MongoClient;

const DB_NAME = "testdb";
const COLLECTION_NAME = "test";

const server = http.createServer((req, res) => {
	res.statusCode = 200;
	res.setHeader('Content-Type', 'text/plain');
	var url = req.url;
	var array = Array();
	var a = url.split("/");
	for(var v in a)
	{
		if(a[v].length)
			array.push(a[v]);
	}
	var Command = array[0];
	if(Command != "add")
	{
		res.write("Command '" + Command);
		res.write("'         Known [show, full, add]\n");
	}

	var Client = mongo.MongoClient;
	Client.connect(ServerURL, function(err, client) {
		if (err) 
			throw err;
		var db = client.db(DB_NAME);
		var collection = db.collection(COLLECTION_NAME);
		var xx = 0;
		if(Command == "full")
		{
			let cursor = collection.find({});
			cursor.each(function(err,obj)
			{
				if(obj)
					res.write("" + xx  + " :: " + JSON.stringify(obj)+ "\n");
				else
				{
					res.end("");
					client.close();
				}
				xx++;

			});

		}
		else if(Command == "show")
		{
			let cursor = collection.aggregate([
				{"$group" : {_id:{"code":"$code", "version":"$version"}, count:{$sum:1}}}
			]);
			cursor.each(function(err,obj)
			{
				if(obj)
					res.write("" + xx  + " :: " + JSON.stringify(obj)+ "\n");
				else
				{
					res.end("");
					client.close();
				}
				xx++;

			});
		}
		else if(Command == "add")
		{
			if(array.length < 3)
			{
				res.end("failed to add, to few args (" + array.length + ")");
			}
			else
			{
				try{
					let client = req.headers['x-forwarded-for'] || req.connection.remoteAddress;
					let version = array[1];
					res.write("Thank you for reporting. please dont refresh page\n");
					res.write("client:" + client + "\n");
					res.write("version:" + version + "\n");
					for(let i = 2; i < array.length; ++i)
					{
						let code = array[i].toLowerCase();
						let Reject = 0;
						for(let j = 0; j < code.length; ++j)
						{
							Reject = Reject || -1 == "0123456789abcdef".indexOf(code[j]);
						}
						if(!Reject)
						{
							collection.insertOne( {user:client,version:version, code:code} ); 
						}
						else
						{
							res.write("Invalid '" + code + "'\n");
						}
					}
					res.end("");
				}
				catch(e)
				{
					console.log(e);
				}
				
			}
			client.close();
		}
		else
		{
			res.end("unknown command '" + Command + "'");
			client.close();
		}
	});

});


server.listen(port, hostname, () => {
	console.log(`Server running at http://${hostname}:${port}/`);
});
