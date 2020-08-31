const { ipcRenderer } = require('electron');

window.ipcRenderer = ipcRenderer;
window.updateOutput = (args) =>{
  let params = args[0];
  let cmdCommand = args[1];
  for(let i = 0; i < params.length; i++){
    params[i].innerText = cmdCommand[i+1];
  }
};
window.commands = {
  rotate: 1,
  setObject: 2,
  changeState: 3,
  filling: 4,
  applyConfiguration: 5,
  startStopMeassurement: 6,
  calibratePressure: 7,
  performHoming: 8,
  sendParameter: 9,
  loadConfiguration: 10,
  logDataIdle: 11,
  restartMeasurement: 12
};