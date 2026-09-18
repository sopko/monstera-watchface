module.exports = [
  {type:'heading',defaultValue:'Monstera Settings'},
  {type:'section',items:[
    {type:'select',messageKey:'Celsius',label:'Temperature unit',defaultValue:'0',options:[
      {label:'Fahrenheit (°F)',value:'0'}, {label:'Celsius (°C)',value:'1'}
    ]},
    {type:'toggle',messageKey:'ShowWeather',label:'Show weather',defaultValue:true},
    {type:'toggle',messageKey:'ShowSteps',label:'Show step counter',defaultValue:true},
    {type:'toggle',messageKey:'ShowBattery',label:'Show battery',description:'Battery icon and percentage.',defaultValue:true},
    {type:'toggle',messageKey:'ShowHeart',label:'Show heart rate',defaultValue:true}
  ]},
  {type:'submit',defaultValue:'Save Settings'}
];
