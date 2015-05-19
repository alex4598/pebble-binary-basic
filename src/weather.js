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

  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      latitude + "&lon=" + longitude;
      console.log("URL:" + url);

  xhrRequest(url, 'GET', 
    function(responseText) {
     var json = JSON.parse(responseText);
     var temperature = Math.round(json.main.temp - 273.15);
     var conditions = json.weather[0].main;  
     var conditions_id = json.weather[0].id;
     var sunrise = json.sys.sunrise;
     var sunset = json.sys.sunset;
     var wind = Math.round(json.wind.speed);
     var pressure = Math.round(json.main.pressure);
     var humidity = Math.round(json.main.humidity);
     
     var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions,
        "KEY_CONDITIONS_ID": conditions_id,
        "KEY_SUNRISE": sunrise,
        "KEY_SUNSET": sunset,
        "KEY_WIND": wind,
        "KEY_PRESSURE": pressure,
        "KEY_HUMIDITY": humidity  
      };

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
