var particle = new Particle();
      var token = "55e9973b571f594ebc4cc2c2aabff1f1dda8a17f";


      console.log("Page Running");


      particle.getEventStream({ deviceId: '42005f001051353338363333', auth: token }).then(function(stream) {
        stream.on('Temperature', function(data) {
        
          document.getElementById("TempID").innerHTML = data.data;

        });
      });


      particle.getEventStream({ deviceId: '42005f001051353338363333', auth: token }).then(function(stream) {
        stream.on('Photoresistor Reading', function(data) {

          document.getElementById("photoID").innerHTML = data.data;

        });
      });


      particle.getEventStream({ deviceId: '42005f001051353338363333', auth: token }).then(function(stream) {
        stream.on('Moisture Sensor Reading', function(data) {

          document.getElementById("MoistureID").innerHTML = data.data;

        });
      });

      var dangerButton = document.getElementById("dangerbutton");
      dangerButton.addEventListener("click", actuate);

      var functionData = {
        deviceId: '42005f001051353338363333',
        name: "actuate",
        argument: "",
        auth: token
      }

      function onSuccess(){
        console.log("actuate success");
      }

      function onFailure(){
        console.log("get fucked");
      }

      function actuate(){
        particle.callFunction(functionData).then(onSuccess, onFailure); 
        console.log("button click");
      }