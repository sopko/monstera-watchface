// Cache Fahrenheit on the watch; display conversion works immediately offline.
var Clay = require('@rebble/clay');
var clay = new Clay(require('./config'), null, {autoHandleEvents:false});
var settings = {Celsius:0,ShowSteps:1,ShowWeather:1,ShowBattery:1,ShowHeart:1};
function applySettings(values) {
  Object.keys(settings).forEach(function(key) {
    if (!Object.prototype.hasOwnProperty.call(values,key)) return;
    var value=values[key];
    if(value && typeof value==='object') value=value.value;
    settings[key]=(value===true || value===1 || value==='1')?1:0;
  });
}
var hasStoredSettings=false;
try { var saved=JSON.parse(localStorage.getItem('clay-settings')); if(saved) {applySettings(saved);hasStoredSettings=true;} } catch(e) {}
function sendSettings() {
  Pebble.sendAppMessage(settings, function() { weather(); }, function() {
    console.log('Settings delivery failed; will retry when the watch requests weather or reconnects.');
  });
}
Pebble.addEventListener('showConfiguration',function() { Pebble.openURL(clay.generateUrl()); });
Pebble.addEventListener('webviewclosed',function(e) {
  if(!e.response || e.response==='CANCELLED') return;
  try { applySettings(clay.getSettings(e.response,false)); hasStoredSettings=true; sendSettings(); }
  catch(error) { console.log('Settings response unavailable: '+error); }
});
var busy = false;
function weather() {
  if (busy || !settings.ShowWeather) return;
  busy = true;
  navigator.geolocation.getCurrentPosition(function(pos) {
    var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + pos.coords.latitude.toFixed(2) +
      '&longitude=' + pos.coords.longitude.toFixed(2) +
      '&current=temperature_2m,weather_code&temperature_unit=fahrenheit&forecast_days=1';
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true); xhr.timeout = 20000;
    xhr.onload = function() {
      busy = false;
      if (xhr.status !== 200 || !settings.ShowWeather) return;
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
Pebble.addEventListener('ready', function() { if(hasStoredSettings) sendSettings(); else weather(); });
Pebble.addEventListener('appmessage', function(e) { if(e.payload.FetchWeather) { if(hasStoredSettings) sendSettings(); else weather(); } });
