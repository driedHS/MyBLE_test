'use strict';

var triggerLevel = 0;

var bluetoothDevice;
var speedLevelCharacteristic;
var mbedStateCharacteristic;
let speedLevel;
let speedLevelOld;
let mbedState = null;

const MY_SERVICE_UUID = '0ba61fc5-34b6-4d73-a562-1051c64e6665';
const SPEED_CHARACTERISTIC_UUID = '0ad84fb8-53a2-402e-9e3c-048f88367381';
const MBEDSTATE_CHARACTERISTIC_UUID = '49a99bde-9c9a-48e3-b852-62439e069af1';

async function onStartUpButtonClick() {
  try {
    if (!bluetoothDevice) {
      await requestDevice();
    }
    await connectDeviceAndCacheCharacteristics();

    /*
    console.log('Reading Battery Level...');
    await speedLevelCharacteristic.readValue();
    */
  } catch(error) {
    console.log('Argh! ' + error);
  }
}

async function requestDevice() {
  console.log('Requesting any Bluetooth Device...');
  bluetoothDevice = await navigator.bluetooth.requestDevice({
   // filters: [...] <- Prefer filters to save energy & show relevant devices.
      acceptAllDevices: true,
      optionalServices: [MY_SERVICE_UUID]});
  bluetoothDevice.addEventListener('gattserverdisconnected', onDisconnected);
}

async function connectDeviceAndCacheCharacteristics() {
  if (bluetoothDevice.gatt.connected && speedLevelCharacteristic) {
    return;
  }

  console.log('Connecting to GATT Server...');
  const server = await bluetoothDevice.gatt.connect();

  console.log('Getting Private Service...');
  const service = await server.getPrimaryService(MY_SERVICE_UUID);

  console.log('Getting Speed Level Characteristic...');
  speedLevelCharacteristic = await service.getCharacteristic(SPEED_CHARACTERISTIC_UUID);
  console.log('Getting MBED State Characteristic...');
  mbedStateCharacteristic = await service.getCharacteristic(MBEDSTATE_CHARACTERISTIC_UUID);

  mbedStateCharacteristic.addEventListener('characteristicvaluechanged',
      handleMbedStateChanged);
  
  //document.querySelector('#startNotifications').disabled = false;
  //document.querySelector('#stopNotifications').disabled = true;
}

/* This function will be called when `readValue` resolves and
 * characteristic value changes since `characteristicvaluechanged` event
 * listener has been added. */
function handleMbedStateChanged(event) {
  mbedState = event.target.value.getInt8(0);
  document.getElementById("display_mbed_state").textContent = "State : " + mbedState;
  console.log('> MBED State is ' + mbedState);
}

async function onStartNotificationsButtonClick() {
  try {
    console.log('Starting MBED State Notifications...');
    await mbedStateCharacteristic.startNotifications();

    console.log('> Notifications started');
    //document.querySelector('#startNotifications').disabled = true;
    //document.querySelector('#stopNotifications').disabled = false;
  } catch(error) {
    console.log('Argh! ' + error);
  }
}

async function onStopNotificationsButtonClick() {
  try {
    console.log('Stopping Mbed State Notifications...');
    await mbedStateCharacteristic.stopNotifications();

    console.log('> Notifications stopped');
    //document.querySelector('#startNotifications').disabled = false;
    //document.querySelector('#stopNotifications').disabled = true;
  } catch(error) {
    console.log('Argh! ' + error);
  }
}

async function inputSpeedLevel() {
  //update html
  speedLevel = document.getElementById('range_trigger').value;
  document.getElementById('display_trigger').textContent = "trigger : " + speedLevel;
}

async function inputSteering() {
  //update html
  let value = document.getElementById('range_steering').value;
  document.getElementById('display_steering').textContent = "steering : " + value;
}

async function onWriteSpeedLevel(){
  //write characteristic speed
  if (!speedLevelCharacteristic) {
    return;
  }
  if (speedLevel == speedLevelOld){
    return;
  }
  try {
    var value = new Int8Array(4);
    speedLevelOld = speedLevel;
    value[0] = (speedLevelOld & 0xFF000000) >> 24;
    value[1] = (speedLevelOld & 0x00FF0000) >> 16;
    value[2] = (speedLevelOld & 0x0000FF00) >> 8;
    value[3] = (speedLevelOld & 0x000000FF);
    await speedLevelCharacteristic.writeValueWithResponse(value);
    
    console.log('> Writing '+value[0]);
  } catch(error) {
    console.log('Argh! ' + error);
  }
}

function onResetButtonClick() {
  if (speedLevelCharacteristic) {
    speedLevelCharacteristic = null;
  }

  if (mbedStateCharacteristic) {
    mbedStateCharacteristic.removeEventListener('characteristicvaluechanged',
      handleMbedStateChanged);
    mbedStateCharacteristic = null;
  }

  mbedState = null;

  // Note that it doesn't disconnect device.
  bluetoothDevice = null;
  console.log('> Bluetooth Device reset');
}

async function onDisconnected() {
  console.log('> Bluetooth Device disconnected');
  try {
    await connectDeviceAndCacheCharacteristics()
  } catch(error) {
    console.log('Argh! ' + error);
  }
}
