//
//  HTML PAGE
//

const char PAGE_ControlMainPage[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />

<center><strong>Heater Control</strong></center>
<hr>

<p style="text-align:center; font-size:3vw; font-weight: bold;">
Current Temperature
<br>
<font size="8vw">
<label id="temp"></label> F
</font>
</p>

<p style="text-align:center; font-size:3vw; font-weight: bold;">
Setpoint
<br>
<font size="8vw">
<label id="sp"></label> F
</font>
</p>

<p style="text-align:center; font-size:3vw; font-weight: bold;">
Heater Output
<br>
<font size="8vw">
<label id="status"></label>
</font>
</p>

<p />

<center>
<font size=":3vw" weight="bold">
Heater Enabled
</font>
<br>
<label class="switch">
  <input type="checkbox" id="state" onchange="toggleState()">
  <span class="slider"></span>
</label><br>
<p />
<a href="setpoints.html" style="width:250px" class="btn btn--m btn--blue" >Setpoints</a><br>
<a href="config.html" style="width:250px" class="btn btn--m btn--blue" >Network Configuration</a><br>
<a href="info.html"   style="width:250px"  class="btn btn--m btn--blue" >Network Information</a><br>
</center>


<script>
window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
        refreshVar();
    });
  });
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}

function refreshVar()
{
  setValues("/sendValues");
  setTimeout('refreshVar()', 1000);
}

function toggleState()
{
  setValues("/toggle");
}

</script>

)=====";


void send_status_values_html()
{
  String values ="";
  values += "temp|" +   (String) (int8_t)Temp_Avg +  "|label\n";
  values += "sp|" +  (String)  config.HeatSP +  "|label\n";
  values += "status|" +  (String)  (config.RelayState ? "On" : "Off") +  "|label\n";
  values += "state|" + (String) (config.HeatStatus ? "checked" : "") + "|chk\n";
  server.send ( 200, "text/plain", values);
  Serial.println(__FUNCTION__); 
}
