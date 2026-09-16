// Open-Meteo current weather; Fahrenheit matches the supplied reference.
var busy = false;
function weather() {
  if (busy) return;
  busy = true;
  navigator.geolocation.getCurrentPosition(function(pos) {
    var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + pos.coords.latitude.toFixed(2) +
      '&longitude=' + pos.coords.longitude.toFixed(2) +
      '&current=temperature_2m,weather_code&temperature_unit=fahrenheit&forecast_days=1';
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true); xhr.timeout = 20000;
    xhr.onload = function() {
      busy = false;
      if (xhr.status !== 200) return;
      try {
        var data = JSON.parse(xhr.responseText).current;
        if (!data || typeof data.temperature_2m !== 'number' || typeof data.weather_code !== 'number') return;
        Pebble.sendAppMessage({Temperature: Math.round(data.temperature_2m), WeatherCode: data.weather_code,
          WeatherTime: Math.floor(Date.now()/1000)});
      } catch(e) { console.log('Weather response unavailable: ' + e); }
    };
    xhr.onerror = xhr.ontimeout = function() { busy = false; };
    xhr.send();
  }, function() { busy = false; }, {timeout:15000,maximumAge:1800000});
}
Pebble.addEventListener('ready', weather);
Pebble.addEventListener('appmessage', function(e) { if(e.payload.FetchWeather) weather(); });
