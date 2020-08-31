
const velocityInput = document.getElementById('velocityInput');
const accelerationInput = document.getElementById('accelerationInput');
const outputParameters = document.getElementsByClassName('parameterOutput');

const stirrerCheckbox = document.getElementById('stirrerCheckbox');
const moveUpButton = document.getElementById('moveUpButton');
const moveDownButton = document.getElementById('moveDownButton');


function changeToHome(){
  window.ipcRenderer.send('sendCommand', [`${window.commands.changeState},i;`]);
  window.ipcRenderer.send('changeToHome');
}

function rotate(up){
    (up) ? window.ipcRenderer.send('sendCommand', [`${window.commands.rotate},u,${stirrerCheckbox.checked ? '127' : '1'};`]):
     window.ipcRenderer.send('sendCommand', [`${window.commands.rotate},d,${stirrerCheckbox.checked ? '127' : '1'};`]);
}

function checkboxClicked(){
  if(stirrerCheckbox.checked){
    moveUpButton.innerText = 'rotate right';
    moveDownButton.innerText = 'rotate left';
  } else {
    moveUpButton.innerText = 'move up';
    moveDownButton.innerText = 'move down';
  }

  setObject('v');
  setObject('a');
}
function stopMotor(){
  window.ipcRenderer.send('sendCommand', [`${window.commands.rotate},s,${stirrerCheckbox.checked ? '127' : '1'};`]);
}

function setObject(objectToSet){
  switch(objectToSet){
    case 'v': window.ipcRenderer.send('sendCommand', [`${Math.abs(window.commands.setObject)},v,4,${velocityInput.value}, ${stirrerCheckbox.checked ? '127' : '1'};`]); break; //set velocity
    case 'a': window.ipcRenderer.send('sendCommand', [`${Math.abs(window.commands.setObject)},a,4,${accelerationInput.value}, ${stirrerCheckbox.checked ? '127' : '1'};`]); break; //set acceleration
  }
}

window.ipcRenderer.on('receivedData', (event, arg) =>{
  let cmdCommand = arg.substring(0,arg.length - 2).split(',');
  if(cmdCommand[0] == window.commands.sendParameter){
    window.updateOutput([outputParameters,cmdCommand]);
  } else {
  console.log(arg);
  }
})

window.ipcRenderer.send('sendCommand', [`${Math.abs(window.commands.setObject)},v,4,${200},1;`]);
window.ipcRenderer.send('sendCommand', [`${Math.abs(window.commands.setObject)},a,4,${30000},1;`]);
window.ipcRenderer.send('sendCommand', [`${Math.abs(window.commands.setObject)},v,4,${1200},127;`]);
window.ipcRenderer.send('sendCommand', [`${Math.abs(window.commands.setObject)},a,4,${30000},127;`]);