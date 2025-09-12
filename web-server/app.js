var express = require('express');
var mongoose = require('mongoose');
var bodyParser = require('body-parser');
var multer =  require('multer');
var path = require('path');
const async = require('async');

const https = require("https");
const fs = require('fs');
const { execSync } = require('child_process');
const UPLOAD_PATH = './public/uploads';
const CONVERTER_PATH = '../q2cathedral';
const VOLUME_MATCHER_PATH = 'python3 ../volume-matcher.py';
const argparse = require('argparse');

var picSchema= new mongoose.Schema({
	picpath:String
})


var storage = multer.diskStorage({
	destination:function(req,file,cb){
		cb(null, UPLOAD_PATH)
	},
	filename:function(req,file,cb){
		cb(null,file.originalname)
	}
})
var upload = multer({storage:storage})

var app = express();

mongoose.connect('mongodb://localhost:27017/pics',{useNewUrlParser: true, useUnifiedTopology: true})
.then(()=>console.log('connected to mongoose')).catch(err=>console.log('error ocured',err));

var picModel = mongoose.model('picsdemo',picSchema);

app.set('views',path.resolve(__dirname,'views'));
app.set('view engine','ejs');

var pathh = path.resolve(__dirname,'public');
app.use(express.static(pathh));
app.use(bodyParser.urlencoded({extended:false}));


app.get('/',(req,res)=>{
console.log("Received");
	if (!req.query.id)
	{	picModel.deleteMany({}).exec();
		//console.log("Deleted All");
	}
	else
		console.log(req.query.id);

	picModel.find((err,data)=>{
		if(err){
			console.log(err);
		}
		else if (data.length == 0){
			res.render('home',{data:{}});
		}
		else
		{
			for (var i = 0; i < data.length; i++)
			{	// if a previous visitor, then continuously keep the database	
				if (data[i].id === req.query.id) 
				{	//console.log("Match");
					// ??? if the id is ever registered, then let her download ALL music files in the server
					res.render('home',{data:data});
					return;
				}
			}
			picModel.deleteMany({}).exec();
//console.log("No Match");
			// Delete files in the database
			/*fs.readdir(UPLOAD_PATH, (err, files) => {
			  if (err) throw err;

			  for (const file of files) {
				 fs.unlink(path.join(UPLOAD_PATH, file), err => {
					if (err) throw err;
				 });
			  }
			});
			*/
			res.render('home',{data:{}});
		}
	});
});


app.post('/', upload.single('pic'), (req,res)=>{
	if (!req.file || !req.file.originalname) 
	{	res.redirect('/error?msg="File is not uploaded"');
		return;
	}
	var x = 'uploads/'+req.file.originalname;
	var ext = path.extname(x);
	if (ext !== '.wma' && ext !== '.wav' && ext !== '.mp3' && ext !== '.flac')
	{
		res.redirect('/error?msg="Only the following file extensions can be uploaded: .mp3, .flac, .wav, .wma"');
		return;
	}
	var x_converted = [];
	//if (req.query.filter == 'livecafe')
	x_converted[0] = {filename: x.substr(0, x.lastIndexOf('.')) + ' - LIVE CAFE.flac', filter: '-f ch,13,10'};
	x_converted[1] = {filename: x.substr(0, x.lastIndexOf('.')) + ' - CATHEDRAL.flac', filter: ''};
	//x_converted[2] = {filename: x.substr(0, x.lastIndexOf('.')) + ' - OPERA2.flac', filter: '-f rnb -f ch,10,10'};
	//x_converted[3] = {filename: x.substr(0, x.lastIndexOf('.')) + ' - RnB.flac', filter: '-f rnb'};

		//.replace(/[\\$'"]/g, "\\$&")

	//res.count = 0;
	//res.countMax = x_converted.length;
	var data_id;
	var isError = false;
	async.each(x_converted, function(obj, callback) {
	//console.log(obj);
	//console.log(picModel.find({picpath: obj.filename}));
		picModel.deleteMany({picpath: obj.filename}).exec();
		var temp = new picModel( { picpath:obj.filename } );
		temp.save((err,data)=>{
			if(err) {
				console.log(err);
			} 

			var cmd = 'rm -f "public/' + obj.filename.replace('"', '\\"') + '"';
			console.log("CMD0: " + cmd);
			var stdout = execSync(cmd);

			var cmd = CONVERTER_PATH + ' -n -s 5 -i "public/' + x.replace('"', '\\"') + '" ' + obj.filter +  ' -o "public/' + obj.filename.replace('"', '\\"') + '"';
			console.log("CMD1: " + cmd);
			var stdout = execSync(cmd);

			//cmd = VOLUME_MATCHER_PATH + ' "public/' + x.replace('"', '\\"') +  '" "public/' + obj.filename.replace('"', '\\"') + '"';
			//console.log("CMD2: " + cmd);
			//var stdout = execSync(cmd);
			//cmd = 'mv "public/' + x_converted[i].replace('"', '\\"') + ' - GAINED.flac" ' +  ' "public/' + x_converted[i].replace('"', '\\"') + '"';
			//console.log("CMD3: " + cmd);
			//execSync(cmd); 
			//if (fs.existsSync('public/' + x_converted[i]))
			//{	//var temp_converted = new picModel( { picpath:x_converted } );
			//}
//console.log("id: " + data.id);		
			//picModel.find({id: data.id}).deleteOne().exec();
			//if (!fs.existsSync('public/' + obj.filename))
			//	isError = true;
			//else
			data_id = data.id;
			callback();
		});
	}, function (err){
		 if(err || isError) { 
			  //if any of your save produced error, handle it here
				console.log(err);
				res.redirect('/error?msg="Something is wrong with your file: ' + req.file.originalname + '"');
		 } 
		 else {
				// no errors, all four object should be in db
//console.log("data_id: " + data_id);
//console.log(res);
				res.redirect('/?id=' + data_id);
		 }
		}
	);
});

app.get('/error',(req,res)=>{
	picModel.deleteMany({}).exec();
	res.render('error',{data: req.query.msg});
});

app.get('/download/:id',(req,res)=>{
	picModel.find({_id:req.params.id},(err,data)=>{
		if(err){
			console.log(err);
		}
		else if (!data[0])
		{
			res.redirect('/error?msg="The file does not exist');
			return;
		}
		else{
			var x= __dirname+'/public/'+data[0].picpath;
			res.download(x);
		}
	});
});

var parser = new argparse.ArgumentParser({
    add_help: true,
    description: `Command-line utility to run an investment signal server.`
});
parser.add_argument('--http', {help: "HTTP?", action: 'store_true'});

var args = parser.parse_args();

port = 3000;
var IS_HTTPS = !(args.http);
if (IS_HTTPS)
{  https.createServer(
      { key: fs.readFileSync("./private-key.pem"),
         cert: fs.readFileSync("./certificate.pem"),
      }, app)
   .listen(httpsPort, () => { console.log('The main server is running at port ' + httpsPort) });
   app_shadow.all('*', (req, res, next) =>
   {
      let protocol = req.headers['x-forwarded-proto'] || req.protocol;
      if (protocol == 'https')
      {  next();
      }
      else
      {  let from = `${protocol}://${req.hostname}${req.url}`;
         let to = `https://${req.hostname}${req.url}`;

         // log and redirect
         //console.log(`[${req.method}]: ${from} -> ${to}`);
         res.redirect(to);
      }
   });
   app_shadow.listen(httpPort, () => console.log('The shadow server is running at port ' + httpPort));
}
else
	app.listen(port,()=>console.log('server running at port ' + port));

