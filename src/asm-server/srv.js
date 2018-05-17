const http = require('http');
var mongo = require('mongodb');
const hostname = '127.0.0.1';
const port = 3000;
const ServerURL = "mongodb://localhost:27017/mydb";
var Client = mongo.MongoClient;

const DB_NAME = "testdb";
const COLLECTION_CLIENT = "client2";
const COLLECTION_NAME = "test2";
var exec = require('child_process').exec;


const server = http.createServer((req, res) => {
	res.statusCode = 200;
	res.setHeader('Content-Type', 'text/html');
	var url = req.url;
	var array = Array();
	var a = url.split("/");
	for(var v in a)
	{
		if(a[v].length)
			array.push(a[v]);
	}
	res.write("<html><body>");
	var Command = array[0];
	if(Command != "add")
	{
		res.write("Command '" + Command);
		res.write("'         Known [show, full, add]<br>");
	}
	console.log("got commmand "+ Command + " url " + req.url);

	var Client = mongo.MongoClient;
	Client.connect(ServerURL, function(err, client) {
		if (err) 
			throw err;
		var db = client.db(DB_NAME);
		var collection = db.collection(COLLECTION_NAME);
		var collectionClient = db.collection(COLLECTION_CLIENT);
		// collection.remove({});
		// collectionClient.remove({});
		var xx = 0;
		if(Command == "full")
		{
			let cursor = collection.find({});
			cursor.each(function(err,obj)
			{
				if(obj)
				{
					res.write("" + xx  + " :: " + JSON.stringify(obj) + "<br>");
					xx++;
				}
				else
				{
					xx = 0;
					let c1 = collectionClient.find();
					c1.each(function(err,obj)
					{
						if(obj)
						{
							res.write("client " + xx  + " :: " + JSON.stringify(obj) + "<br>");
							xx++;
						}
						else
						{
							res.end("</body></html>");
							client.close();
						}
					});
				}

			});

		}
		else if(Command == "show")
		{
			let cursor = collection.find({});
			cursor.each(function(err,obj)
			{
				if(obj)
				{
					res.write("" + xx  + " :: " + JSON.stringify(obj));
					res.write("<a href='dis/"+ obj.code + "'>dis</a><br>");
				}
				else
				{
					res.end("</body></html>");
					client.close();
				}
				xx++;

			});

		}
		else if(Command == "add")
		{
			let U = [];
			if(array.length < 3)
			{
				res.end("failed to add, to few args (" + array.length + ")");
			}
			else
			{
				try{
					let client = req.headers['x-forwarded-for'] || req.connection.remoteAddress;
					let version = array[1];
					res.write("Thank you for reporting.<br>");
					res.write("client:" + client + "<br>");
					res.write("version:" + version + "<br>");
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
							let d = {version:version, code:code, client:client};
							let r = collectionClient.updateOne(d, {$inc:{count:1}}, {upsert:true} );
							U.push(r);
							r.then(function(r)
							{
								if(r.upsertedId)
								{
									console.log("adding to stats " + code + " \n");
									collection.updateOne( {version:version, code:code}, { $inc:{count:1} }, {upsert:true} ); 
								}
								else
								{
									console.log("did not add to stats\n");
								}
							});
						}
						else
						{
							res.write("Invalid '" + code + "'<br>");
						}
					}
					res.end("</body></html>");
				}
				catch(e)
				{
					console.log(e);
				}
				
			}
			Promise.all(U).then(function(){client.close()});
			// client.close();
		}
		else if(Command == "dis")
		{
			if(array.length < 2)
			{
				res.end("failed to add, to few args (" + array.length + ")</body></html>");
			}
			else
			{
				let code = array[1];
				res.write("dissing "+ code + "\n");
				let asm = ".byte ";
				for(var i = 0; i < code.length; i += 2)
				{
				 	asm += "0x" + code[i] + code[i+1];
				 	if(i + 2 < code.length)
				 		asm += ",";
				}
				exec('echo "'+ asm + '" | as ', function callback(error, stdout, stderr){	
					exec("objdump -d", function callback(error, stdout, stderr){		
						res.write("<pre><code>");
						res.write(stdout);
						xx = 0;
						let cursor = collectionClient.find({code:code});
						cursor.each(function(err,obj)
						{
							if(obj)
							{
								res.write("" + obj.version + " : "+ obj.client + "\n");
								xx++;
							}
							else
							{
								res.write("</code></pre>");
								res.end("</body></html>");
								client.close();
							}
						});
					});
				});
			}
		}
		else
		{
			res.write("unknown command '" + Command + "'");
			res.end("</body></html>");

			client.close();
		}
	});

});


server.listen(port, hostname, () => {
	console.log(`Server running at http://${hostname}:${port}/`);
});
