const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
let events={}, sent=[], requests=[], locationError=false;
const storage={};
function Clay(){this.getSettings=(response)=>{const v=JSON.parse(response);storage["clay-settings"]=JSON.stringify(v);return v;};this.generateUrl=()=>"data:text/html,settings";}
const context={console,Date,Math,JSON,require:(name)=>name==="@rebble/clay"?Clay:require("../src/pkjs/config"),localStorage:{getItem:k=>storage[k]||null},navigator:{geolocation:{getCurrentPosition(ok,fail){locationError?fail():ok({coords:{latitude:40.7128,longitude:-74.006}});}}},Pebble:{addEventListener(n,f){events[n]=f;},openURL(url){assert.ok(url);},sendAppMessage(m,ok){sent.push({...m});if(ok)ok();}},XMLHttpRequest:function(){requests.push(this);this.open=(method,url)=>{this.url=url;};this.send=()=>{};}};
vm.runInNewContext(fs.readFileSync('src/pkjs/index.js','utf8'),context);
context.weather(); assert.equal(requests.length,1);
context.weather();assert.equal(requests.length,1);
requests[0].status=200;requests[0].responseText=JSON.stringify({current:{temperature_2m:71.6,weather_code:0}});requests[0].onload();
assert.equal(sent[0].Temperature,72);assert.ok(requests[0].url.includes('temperature_unit=fahrenheit'));
context.weather();requests[1].status=200;requests[1].responseText='invalid';requests[1].onload();assert.equal(sent.length,1);
context.weather();requests[2].ontimeout();context.weather();assert.equal(requests.length,4);
requests[3].status=503;requests[3].onload();assert.equal(sent.length,1);
locationError=true;context.weather();locationError=false;context.weather();assert.equal(requests.length,5);
console.log('Weather: success, duplicate suppression, malformed response, timeout recovery, HTTP failure, and location recovery passed.');

const before=requests.length;
events.webviewclosed({response:JSON.stringify({Celsius:'1',ShowWeather:false,ShowSteps:false,ShowBattery:false,ShowHeart:false})});
assert.equal(requests.length,before);assert.equal(sent.at(-1).Celsius,1);assert.equal(sent.at(-1).ShowBattery,0);
requests.at(-1).status=200;requests.at(-1).responseText=JSON.stringify({current:{temperature_2m:32,weather_code:0}});
const count=sent.length;requests.at(-1).onload();assert.equal(sent.length,count);
events.ready();assert.equal(requests.length,before);
events.webviewclosed({response:'CANCELLED'});assert.equal(requests.length,before);
events.webviewclosed({response:JSON.stringify({Celsius:'0',ShowWeather:true,ShowSteps:true,ShowBattery:true,ShowHeart:true})});
assert.equal(sent.at(-1).Celsius,0);assert.equal(sent.at(-1).ShowSteps,1);assert.equal(requests.length,before+1);
assert.ok(storage['clay-settings']);
console.log('Settings: toggle delivery, disabled weather suppression including in-flight responses, cancellation, and re-enable refresh passed.');
