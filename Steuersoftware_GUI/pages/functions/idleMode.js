
const outputParameters = document.getElementsByClassName('parameterOutput');

const pressureCalibrationButton = document.getElementById('pressureCalibrationButton');
const logDataButton = document.getElementById('logData');
let logDataActive = false;

function openManualMode() {
    window.ipcRenderer.send('sendCommand', [`${window.commands.changeState},m;`])
    window.ipcRenderer.send('openManualModeHtml');
}

function openAutomaticMode() {
    window.ipcRenderer.send('sendCommand', [`${window.commands.changeState},a;`])
    window.ipcRenderer.send('openAutomaticModeHtml');
}

function performHoming() {
  if(window.ipcRenderer.sendSync('openDialog')){
    window.ipcRenderer.send('sendCommand', [`${window.commands.performHoming};`])
  }
}

function calibratePressure(){
  window.ipcRenderer.send('sendCommand', [`${window.commands.calibratePressure};`]);
}

function logData(){
  logDataActive = !logDataActive;
  if(logDataActive){
    logDataButton.innerText = 'logging Data';
    logDataButton.style.backgroundColor = "salmon";
  } else {
    logDataButton.innerText = 'log Data';
    logDataButton.style.backgroundColor = "rgb(50, 163, 111)";
  }
  window.ipcRenderer.send('sendCommand', [`${window.commands.logDataIdle},${(logDataActive) ? 1 : 0};`]);
}

window.ipcRenderer.on('receivedData', (event, arg) =>{
  let cmdCommand = arg.substring(0,arg.length - 2).split(',');
  if(cmdCommand[0] == window.commands.sendParameter){
    window.updateOutput([outputParameters,cmdCommand]);
  }
  else {
    console.log(arg);
  }
})