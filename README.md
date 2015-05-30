# pebble-binary-basic

Build/Installation
- Setup Pebble SDK 3.0
- run 'peble build'
- copy build/pebble-binary-basic.pbw to phone
- Open pebble-binary-basic.pbw on phone; Pebble app will ask you to install to watch

Usage

Usage of the app is pretty straight forward. Each column encodes "binary coded decimal" to show the 4 digits of the time, where a black shaded box represents a 1 and a white shaded box represents a 0. Inside each square is (From left to right starting at the top) Wind Speed, Barometric Pressure, Relative Humidity, Minutes since last weather update, Sunrise, Sunset, current conditions graphic (sunny, rainy, cloudy, etc), temperature, Day of Week, Month of Year, Day of Month, Seconds

There is currently no configuration settings. To change from Metric to Imperial units, edit weather.js and main.c accordingly.
