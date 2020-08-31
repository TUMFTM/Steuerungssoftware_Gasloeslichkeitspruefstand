const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');
const port = new SerialPort('/dev/ttyACM0', {
    baudRate: 9600,
    autoOpen: true
  });
const readLineParser = port.pipe(new Readline({ delimiter: '\n' }))

process.env.NODE_ENV = 'production';

port.on('open', onOpen);
readLineParser.on('data', onData);

let win = null;

commands = {
  rotate: 1,
  setObject: 2,
  changeState: 3,
  filling: 4,
  applyConfiguration: 5,
  startStopMeassurement: 6,
  calibratePressure: 7,
  performHoming: 8,
  sendParameter: 9
};

const options = {
  type: 'question',
  buttons: ['Ja', 'Nein'],
  defaultId: 2,
  title: 'Question',
  message: 'Sind Sie sicher ?',
  detail: '',
  checkboxChecked: false,
};


function createWindow () {
  // Create the browser window.
  win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })
  // and load the idleMode.html of the app.
  //port.pause();
  win.setMenuBarVisibility(false);
  win.loadFile('./pages/idleMode.html')
}

function onQuit(){
  port.write(`${commands.changeState},i;`);
  if(win && port && port.isOpen){
   port.pause();
  }
}

app.on('ready', createWindow);
app.on('quit', onQuit);

ipcMain.on('openManualModeHtml', (event,arg) => {
  //port.resume();
    port.flush(err => {
    console.log('resumed port!');
    win.loadFile('pages/manualMode.html');
    });
});

ipcMain.on('openAutomaticModeHtml', (event,arg) => {
  win.loadFile('pages/automaticMode.html');
});

ipcMain.on('changeToHome', (event,arg) => {
  //port.pause();
  //console.log('paused port!');
  win.loadFile('pages/idleMode.html');
});

ipcMain.on('sendCommand', (event, commands) => {
  console.log(commands[0]);
  port.write(commands[0]);
  port.drain(value =>{});
})

// port functions
function onOpen() {
  port.flush();
  console.log("port geöffnet");
}

function onData(data){
  //win.webContents.executeJavaScript('"console.log("' + data + '")"');
  win.webContents.send('receivedData', data);
}

ipcMain.on('openDialog', (event) =>{
  event.returnValue = (dialog.showMessageBoxSync(win,options) === 0)? true : false;
})

ipcMain.on('updateOutput', (event, args) => {
  let params = args[0];
  let cmdCommand = args[1];
  for(let i = 0; i < params.length; i++){
    params[i].innerText = cmdCommand[i+1];
  }
})