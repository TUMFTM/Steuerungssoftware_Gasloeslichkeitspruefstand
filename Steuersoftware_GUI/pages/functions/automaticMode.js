  const paramField = document.getElementById('paramField');
  const fillingField = document.getElementById('fillingField');
  
  const backButton = document.getElementById('backButton');
  const startMeasurementButton = document.getElementById('startMeasurementButton');
  const stopMeasurementButton = document.getElementById('stopMeasurementButton');

  const oilAmountInput = document.getElementById('oilAmountInput');
  const driveInButton = document.getElementById('driveInButton');

  const airAmountInput = document.getElementById('airAmountInput');
  const driveOutButton = document.getElementById('driveOutButton');

  const applyConfigButton = document.getElementById('applyConfigButton');

  const intParameters = document.getElementsByClassName('intParameter');
  const doubleParameters = document.getElementsByClassName('doubleParameter');
  const boolParameters = document.getElementsByClassName('boolParameter');
  const stringParameters = document.getElementsByClassName('stringParameter');
  
  const configCheckmark = document.getElementById('configCheckmark');
  const fillingCheckmark = document.getElementById('fillingCheckmark');
  const preCondTime = document.getElementById('preCondTime');
  const currentMeasurementDisplay = document.getElementById('currentMeasurementDisplay');
  const numberLoops = document.getElementById('numberLoops');
  const preCondTimeLoop = document.getElementById('preCondTimeLoop');
  
  const outputParameters = document.getElementsByClassName('parameterOutput');

  function backToHome() {
    window.ipcRenderer.send('sendCommand', [`${window.commands.changeState},i;`]); // Control Software change to Idle
    window.ipcRenderer.send('changeToHome'); // GUI change to Idle/Home
  }

  // calculates positioning of piston and sends it to the arduino to handle it
  function fillingMode(driveout){
    
    if(driveout == 's'){
      window.ipcRenderer.send('sendCommand', [`${window.commands.filling},s,${oilAmountInput.value};`]);
    } 
    else if(driveout == 'o' && window.ipcRenderer.sendSync('openDialog')){
      window.ipcRenderer.send('sendCommand', [`${window.commands.filling},o,${oilAmountInput.value};`]);

    }
     else if (driveout == 'a' && window.ipcRenderer.sendSync('openDialog')) {
      window.ipcRenderer.send('sendCommand', [`${window.commands.filling},a,${airAmountInput.value};`]);
    }
  
  }

  function applyConfiguration(){
    //console.log(intParameters);
    let argumentString = `${window.commands.applyConfiguration}`;
    for(let i = 0; i < intParameters.length; i++){
      argumentString += `,${intParameters[i].value}`;
    }
    // window.ipcRenderer.send('sendCommand', [`${argumentString};`]);
    // console.log(argumentString);

   // argumentString = `${window.commands.applyConfiguration}`;
    for(let i = 0; i < boolParameters.length; i++){
      argumentString += `,${(boolParameters[i].checked) ? '1': '0'}`;
    }

    for(let i = 0; i < doubleParameters.length; i++){
      doubleParameters[i].value = (doubleParameters[i].value == 10) ? 10.1 : doubleParameters[i].value; // avoiding bug when putting 10 into input field
      argumentString += `,${doubleParameters[i].value}`;
    }
    for(let i = 0; i < stringParameters.length; i++){
      argumentString += `,${stringParameters[i].value}`;
    }
    console.log(argumentString);
    window.ipcRenderer.send('sendCommand', [`${argumentString};`]);
   // configCheckmark.style.display = 'unset';
  }

  function startStopMeasurement(start){
    if(start){
    paramField.disabled = true;
    fillingField.disabled = true;
    startMeasurementButton.disabled = true;
    backButton.disabled = true;
    stopMeasurementButton.disabled = false;
    currentMeasurementDisplay.innerText = `${1}/${numberLoops.value}`;
    window.ipcRenderer.send('sendCommand', [`${window.commands.startStopMeassurement},1;`]);
    } else {
    paramField.disabled = false;
    fillingField.disabled = false;
    startMeasurementButton.disabled = false;
    backButton.disabled = false;
    stopMeasurementButton.disabled = true;
    window.ipcRenderer.send('sendCommand', [`${window.commands.startStopMeassurement},0;`]);
    }
  }

  function sleep(millis) {
    return new Promise(resolve => setTimeout(resolve, millis));
  }


  window.ipcRenderer.on('receivedData', (event, arg) => {
    let cmdCommand = arg.substring(0,arg.length - 2).split(',');
    if(cmdCommand[0] == window.commands.sendParameter){
      // switch(cmdCommand[1]){
      //   case 'p': outputParameters[0].innerText = `${cmdCommand[2]}`; break;
      //   case 't': outputParameters[1].innerText = `${cmdCommand[2]}`; break;
      // }
      window.updateOutput([outputParameters,cmdCommand]);
    } else if((cmdCommand[0] == window.commands.loadConfiguration)){
      for(let i = 0; i < intParameters.length; i++){
        intParameters[i].value = cmdCommand[i];
      }
      for(let i = 0; i < boolParameters.length; i++){
        boolParameters[i].checked = (cmdCommand[intParameters.length + i] == '0') ? false: true;
      }
      for(let i = 0; i < doubleParameters.length; i++){
        doubleParameters[i].value = cmdCommand[intParameters.length + boolParameters.length + i];
      }
    } else if (cmdCommand[0] == window.commands.restartMeasurement){
        startStopMeasurement(false);
         if(cmdCommand[1] <= numberLoops.value){
        // Set precondition time ever 3rd test to this hardcoded value
        // Used for changing Temp-steps every 3rd test 
          sleep(5000).then(() => {
            if(cmdCommand[1] % 3 == 1){
              // preCondTime.value = 370;  // hardcoded value
              preCondTime.value = preCondTimeLoop.value;
            } else {
              preCondTime.value = preCondTimeLoop.value;
            }
          applyConfiguration();
         });
        
         sleep(10000).then(() => {
          startStopMeasurement(true);
         });

         sleep(11000).then(() => {
          currentMeasurementDisplay.innerText = currentMeasurementDisplay.innerText.replace(/[0-9]*\//, `${cmdCommand[1]}/`);
          window.ipcRenderer.send('sendCommand', [`${window.commands.restartMeasurement},${cmdCommand[1]};`]);
        });
         }
    } else {console.log(arg);}
  })

  //calls configuration, whenever page is load.
  applyConfiguration();
  //configCheckmark.style.display = 'none';