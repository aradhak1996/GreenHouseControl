document.addEventListener("DOMContentLoaded", function(event) {


  console.log("page loaded");
  var particle = new Particle();

  var accessToken = "3dfcb3b37093bbb15ef02131fd2a52fb020ae103"
  var deviceID = "3d003e000247363333343435";

  var justLoaded = false;

  var valveStatus;
  var temperatureStatus;
  var moistureStatus;
  var lightStatus;
  var prevTemp;

  var isWatering = false;
  var isMCEnabled = false;

  var tempUnit = "F";

  var manualControls = document.getElementById("manualcontrols");
  var openValve = document.getElementById("openvalve");
  var closeValve = document.getElementById("closevalve");
  var toggleButton = document.getElementById("togglebutton");
  var valveOnOff = document.getElementById("valvestatus");

  var mainpage = document.getElementById("mainpage");
  var mainPageConditions = document.getElementById("conditions");
  var wateringCondition = document.getElementById("wateringcondition");
  var wateringMessage = document.getElementById("wateringmessage");
  var advancedControlsPage = document.getElementById("advancedcontrols");
  var loadingPage = document.getElementById("loadingpage");
  var toAdvancedControls = document.getElementById("toadvancedpage");
  var toMainPage = document.getElementById("tomainpage");
  var mainAdvancedControls = document.getElementById("mainadvancedcontrols");
  var schemePage = document.getElementById("schemepage");

  //Divs for status bar values
  var moistureBar = document.getElementById("moistureBar")
  var lightBar = document.getElementById("lightBar");
  var tempBar = document.getElementById("tempBar");

  //Values associated with bars
  var tempText = document.getElementById("tempvalue");
  var lightText = document.getElementById("lightpercent");
  var moistureText = document.getElementById("waterpercent");

  var fahrenheitSelection = document.getElementById("fahrenheit");
  var celsiusSelection = document.getElementById("celsius");
  var scheme1Selection = document.getElementById("scheme1");
  var scheme2Selection = document.getElementById("scheme2");
  var schemepagetransition = document.getElementById("moreinfo");

  var schemeToAdvanced = document.getElementById("schemetoadv");

  mainpage.hidden = false;
  loadingPage.hidden = true;

  schemepagetransition.addEventListener("click", function(){
    schemePage.hidden = false;
    advancedControlsPage.hidden = true;
  })

  schemeToAdvanced.addEventListener("click", function(){
    schemePage.hidden = true;
    advancedControlsPage.hidden = false;
  })



  fahrenheitSelection.addEventListener("click", toFahrenheit);
  celsiusSelection.addEventListener("click", toCelsius);
  scheme1Selection.addEventListener("click", lowerThresh);
  scheme2Selection.addEventListener("click", higherThresh);

  toAdvancedControls.addEventListener("click", function(){
    mainpage.hidden = true;
    advancedControlsPage.hidden = false;
  });

  toMainPage.addEventListener("click", function(){
    advancedControlsPage.hidden = true;
    mainpage.hidden = false;
  })

  toggleButton.addEventListener("click", function(){
    manualControls.hidden = !manualControls.hidden;
    mainAdvancedControls.hidden = !mainAdvancedControls.hidden;
  })
  openValve.addEventListener("click", valveOn);
  closeValve.addEventListener("click", valveOff);

  function newStateEvent(objectContainingData){
    console.log(objectContainingData.data);

    if(justLoaded){
      mainpage.hidden = false;
    }

    justLoaded = false;

    loadingpage.hidden = true;
    var parsed = JSON.parse(objectContainingData.data);
    moistureStatus = parseInt(parsed.moisture);
    lightStatus = parseInt(parsed.light);
    valveStatus = parseInt(parsed.valve);
    var temperature = parsed.temperature;
    temperatureStatus = "";
    if(temperature){
      for(var i=0; i<4; i++){
        temperatureStatus += temperature[i];
      }
      prevTemp = temperatureStatus;
    }
    else {
      temperatureStatus = prevTemp;
    }

    temperatureStatus = parseFloat(temperatureStatus);
    if(parsed.mcenabled == "true"){
      isMCEnabled = true;
    }
    if(parsed.mcenabled == "false"){
      isMCEnabled = false;
    }

    tempUnit = parsed.temparg;

    updateUIElements();


  }

  function newWateringEvent(objectContainingData){
    var event = objectContainingData.data;
    if(event == "started"){
      isWatering = true;
      flashWatering();
    }
    if(event == "stopped"){
      isWatering = false;
      location.reload();
    }
  }

  function flashWatering(){
    mainpage.hidden = false;
    advancedControlsPage.hidden = true;
    mainPageConditions.hidden = true;
    wateringCondition.hidden = false;
  }

  function updateUIElements(){
    setTempBar(temperatureStatus);
    setMoistureBar(moistureStatus);
    setLightBar(lightStatus);
  }

  function setTempBar(tempvalue){
    var tempString = temperatureStatus.toString();
    if(tempUnit == "F"){
      var intTemp = Math.floor(tempvalue) - 6;
    }
    if(tempUnit == "C"){
      var intTemp = Math.floor(tempvalue*1.8+32) - 6;
    }
    var barValue = intTemp.toString() + "%";
    if(isNaN(tempvalue)){
      tempBar.style.width = "2%";
      tempText.innerHTML = "...";
    }
    else {
      tempBar.style.width = barValue;
      tempText.innerHTML = tempString + " " + tempUnit;
    }


  }

  function setMoistureBar(soilval){

    var barMoisture = Math.floor(100-((soilval - 1300)/20)).toString();
    if(soilval <= 2100){
      moistureText.innerHTML = "Very wet";
    }
    else if(soilval > 2100 && soilval <= 2400){
      moistureText.innerHTML = "Healthy";
    }
    else if(soilval > 2400 && soilval <= 2700){
      moistureText.innerHTML = "Drying";
    }
    else {
      moistureText.innerHTML = "Very dry";
    }
    moistureBar.style.width = barMoisture + "%";
  }

  function setLightBar(lightval){
    var barVal = Math.floor((lightval/10)).toString();
    lightText.innerHTML = barVal + "%";
    lightBar.style.width = barVal + "%";
  }


  function waitForUpdate(){
    mainpage.hidden = true;
    advancedControlsPage.hidden = true;
    var functionData = {
       deviceId:deviceID,
       name: "pubState",
       argument: "",
       auth: accessToken
    }

    function onSuccess(e){
      mainpage.hidden = false;
    }
    function onFailure(e){
      console.log("wait for update call failed");
    }

    particle.callFunction(functionData).then(onSuccess, onFailure);
  }

  function toFahrenheit(){
    var functionData = {
       deviceId:deviceID,
       name: "CtoF",
       argument: "",
       auth: accessToken
    }

    function onSuccess(e){console.log("set temp unit success")}
    function onFailure(e){console.log("set temp unit failure")}
    particle.callFunction(functionData).then(onSuccess, onFailure);

  }

  function toCelsius(){
    var functionData = {
       deviceId:deviceID,
       name: "FtoC",
       argument: "",
       auth: accessToken
    }

    function onSuccess(e){console.log("set temp unit success")}
    function onFailure(e){console.log("set temp unit failure")}
    particle.callFunction(functionData).then(onSuccess, onFailure);

  }

  function lowerThresh(){
    var functionData = {
       deviceId:deviceID,
       name: "setHigh",
       argument: "2450",
       auth: accessToken
    }
    function onSuccess(e){console.log("set scheme1 success")}
    function onFailure(e){console.log("set scheme1 failure")}
    particle.callFunction(functionData).then(onSuccess, onFailure);
  }

  function higherThresh(){
    var functionData = {
       deviceId:deviceID,
       name: "setHigh",
       argument: "2750",
       auth: accessToken
    }
    function onSuccess(e){console.log("set scheme2 success")}
    function onFailure(e){console.log("set scheme2 failure")}
    particle.callFunction(functionData).then(onSuccess, onFailure);
  }


  function setTempArg(unit){
    if(arg == "F"){
      var functionData = {
         deviceId:deviceID,
         name: "CtoF",
         argument: "",
         auth: accessToken
      }
    }
    if(arg == "C"){
      var functionData = {
         deviceId:deviceID,
         name: "FtoC",
         argument: "",
         auth: accessToken
      }
    }
    function onSuccess(e){console.log("set temp unit success")}
    function onFailure(e){console.log("set temp unit failure")}
    particle.callFunction(functionData).then(onSuccess, onFailure);
  }

  particle.getEventStream({ deviceId: deviceID, name: "state", auth: accessToken}).then(function(stream) {
    stream.on('event', newStateEvent);
  });

  particle.getEventStream({ deviceId: deviceID, name: "wateringStatus", auth: accessToken}).then(function(stream) {
    stream.on('event', newWateringEvent);
  });

  function valveOn(){
    var functionData = {
       deviceId:deviceID,
       name: "actuate",
       argument: "open",
       auth: accessToken
    }

    function onSuccess(e) {
      console.log("valve successfully opened");
      valveOnOff.innerHTML = "Open";
    }
    function onFailure(e) { console.log("valve open failure")}
    particle.callFunction(functionData).then(onSuccess,onFailure)
  }

  function valveOff(){
    var functionData = {
       deviceId:deviceID,
       name: "actuate",
       argument: "close",
       auth: accessToken
    }

    function onSuccess(e) {
      console.log("valve successfully opened");
      valveOnOff.innerHTML = "Closed"
    }
    function onFailure(e) { console.log("valve close failure")}
    particle.callFunction(functionData).then(onSuccess,onFailure)

  }

  function pubState(){
    var functionData = {
       deviceId:deviceID,
       name: "pubState",
       argument: "close",
       auth: accessToken
    }

    function onSuccess(e) { console.log("valve successfully closed") }
    function onFailure(e) { console.log("valve close failure")}
    particle.callFunction(functionData).then(onSuccess,onFailure)

  }

  console.log("page loaded");
  loadingPage.hidden = false;
  mainpage.hidden = true;
  advancedControlsPage.hidden = true;
  manualControls.hidden = true;



});
