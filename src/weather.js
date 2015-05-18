var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var latitude = String(pos.coords.latitude);
  var longitude = String(pos.coords.longitude);
    
  // Coordinates 
    console.log("Latitude is " + latitude);
    console.log("Longitude is " + longitude);
  
  // Construct URL
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      latitude + "&lon=" + longitude;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
        console.log("Temperature:" + temperature);

      // Conditions
      var conditions = json.weather[0].main;      
        console.log("Conditions:" + conditions);
        
      // Sunrise and Sunset
     var sunrise = json.sys.sunrise;
     var sunset = json.sys.sunset;
        console.log("Sunrise:" + sunrise);
        console.log("Sunset:" + sunset);
     
     // Wind, Pressure and Humidity
     var wind = json.wind.speed;
     var pressure = json.main.pressure;
     var humidity = json.main.humidity;
        console.log("Wind:" + wind);
        console.log("Pressure:" + pressure);
        console.log("Humidity:" + humidity);
        
     
      
      // Assemble dictionary
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions,
        "KEY_LATITUDE":  latitude,
        "KEY_LONGITUDE": longitude,
        "KEY_SUNRISE": sunrise,
        "KEY_SUNSET": sunset,
        "KEY_WIND": wind,
        "KEY_PRESSURE": pressure,
        "KEY_HUMIDITY": humidity
        
          
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready");

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received");
    getWeather();
  }                     
);
