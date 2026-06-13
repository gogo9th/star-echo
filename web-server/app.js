var express = require('express');
var bodyParser = require('body-parser');
var multer =  require('multer');
var path = require('path');
const https = require("https");
const fs = require('fs');
const crypto = require('crypto');
const { execSync } = require('child_process');
const UPLOAD_PATH = './public/uploads';
const DATA_FILE = path.resolve(__dirname, 'data', 'pics.json');
const CONVERTER_PATH = '../star_echo';
const argparse = require('argparse');

const FILTERS = [
	'livecafe',
	'cathedral',
	'studio',
	'rock',
	'classical',
	'jazz',
	'dance',
	'ballad',
	'club',
	'rnb',
	'cafe',
	'concert'
];

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

function loadPics() {
	try {
		var data = JSON.parse(fs.readFileSync(DATA_FILE, 'utf8'));
		return Array.isArray(data) ? data : [];
	}
	catch (err) {
		if (err.code === 'ENOENT') {
			return [];
		}
		console.log('error reading pics data', err);
		return [];
	}
}

function savePics(data) {
	fs.mkdirSync(path.dirname(DATA_FILE), {recursive: true});
	var tempFile = DATA_FILE + '.tmp';
	fs.writeFileSync(tempFile, JSON.stringify(data, null, 2));
	fs.renameSync(tempFile, DATA_FILE);
}

function deletePics(query) {
	if (!query) {
		savePics([]);
		return;
	}

	var data = loadPics().filter(function (item) {
		return Object.keys(query).some(function (key) {
			return item[key] !== query[key];
		});
	});
	savePics(data);
}

function savePic(picpath) {
	var data = loadPics();
	var id = crypto.randomBytes(12).toString('hex');
	var item = {_id: id, id: id, picpath: picpath};
	data.push(item);
	savePics(data);
	return item;
}

function escapeShellPathPart(value) {
	return value.replace(/["\\$`]/g, '\\$&');
}

function selectedFilters(value) {
	var values = Array.isArray(value) ? value : (value ? [value] : []);
	return values.filter(function (filter) {
		return FILTERS.indexOf(filter) !== -1;
	});
}

function convertedFilename(inputPath, filter) {
	return inputPath.substr(0, inputPath.lastIndexOf('.')) + ' - ' + filter.toUpperCase() + '.flac';
}

function convertFile(inputPath, filter) {
	var outputPath = convertedFilename(inputPath, filter);
	deletePics({picpath: outputPath});

	var cmd = 'rm -f "' + escapeShellPathPart('public/' + outputPath) + '"';
	console.log("CMD0: " + cmd);
	execSync(cmd);

	cmd = CONVERTER_PATH + ' -n -s 5 -i "' + escapeShellPathPart('public/' + inputPath) + '" -f ' + filter + ' -o "' + escapeShellPathPart('public/' + outputPath) + '"';
	console.log("CMD1: " + cmd);
	execSync(cmd);

	return savePic(outputPath);
}

function sendUploadError(req, res, msg) {
	if (req.body && req.body.ajax === '1') {
		res.status(400).json({error: msg});
		return;
	}
	res.redirect('/error?msg=' + encodeURIComponent('"' + msg + '"'));
}

app.set('views',path.resolve(__dirname,'views'));
app.set('view engine','ejs');

var pathh = path.resolve(__dirname,'public');
app.use(express.static(pathh));
app.use(bodyParser.urlencoded({extended:false}));

app.get('/thumbnail.png',(req,res)=>{
	res.sendFile(path.resolve(__dirname,'views','thumbnail.png'));
});


app.get('/',(req,res)=>{
console.log("Received");
	if (!req.query.id)
	{	deletePics();
		//console.log("Deleted All");
	}
	else
		console.log(req.query.id);

	var data = loadPics();
	if (data.length == 0){
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
		deletePics();
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

app.post('/', upload.single('pic'), (req,res)=>{
	if (!req.file || !req.file.originalname) 
	{	sendUploadError(req, res, 'File is not uploaded');
		return;
	}
	var x = 'uploads/'+req.file.originalname;
	var ext = path.extname(x).toLowerCase();
	if (ext !== '.wma' && ext !== '.wav' && ext !== '.mp3' && ext !== '.flac')
	{
		sendUploadError(req, res, 'Only the following file extensions can be uploaded: .mp3, .flac, .wav, .wma');
		return;
	}

	var filters = selectedFilters(req.body.filter || req.body.filters);
	if (filters.length === 0)
	{
		sendUploadError(req, res, 'Choose at least one filter');
		return;
	}

	if (req.body.reset === '1')
	{
		deletePics();
	}

	try {
		var data_id;
		var results = [];
		for (var i = 0; i < filters.length; i++)
		{
			var data = convertFile(x, filters[i]);
			data_id = data.id;
			results.push(data);
		}

		if (req.body.ajax === '1')
		{
			res.json({id: data_id, data: results});
			return;
		}
		res.redirect('/?id=' + data_id);
	}
	catch (err) {
		console.log(err);
		sendUploadError(req, res, 'Something is wrong with your file: ' + req.file.originalname);
	}
});

app.get('/error',(req,res)=>{
	deletePics();
	res.render('error',{data: req.query.msg});
});

app.get('/download/:id',(req,res)=>{
	var data = loadPics().filter(function (item) {
		return item._id === req.params.id;
	});
	if (!data[0])
	{
		res.redirect('/error?msg="The file does not exist');
		return;
	}
	else{
		var x= __dirname+'/public/'+data[0].picpath;
		res.download(x);
	}
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

