//
//  HTML PAGE
//

const char PAGE_AdminGeneralSettings[] PROGMEM =  R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>General Settings</strong>
<hr>
<form action="" method="post">
<table border="0"  cellspacing="0" cellpadding="3" >

<tr>
	  <td align="left" colspan="2"><b>Heating Setpoint</b></td>
</tr>

<tr>
	<td align="left"> Temperature:</td>
	<td><input type="number" id="heatsp" name="heatsp" size="2" value="" min="35" max="80">F</td>
</tr>

<tr>
	  <td align="left" colspan="2"><b>Heating Hysteresis<b></td>
<tr>
	<td align="left"> Temperature:</td>
	<td><input type="number" id="heathist" name="heathist" size="2" value="" min="2" max="6">F</td>
</tr>
<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>
<script>

 

window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				setValues("/admin/generalvalues");
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}



</script>
)=====";


// Functions for this Page

void send_general_html()
{
	if (server.args() > 0 )  // Save Settings
	{
		String temp = "";
		for ( uint8_t i = 0; i < server.args(); i++ ) {
			if (server.argName(i) == "heatsp") config.HeatSP = server.arg(i).toInt(); 
			if (server.argName(i) == "heathist") config.HeatHist = server.arg(i).toInt();
		}
		WriteConfig();
	}
	server.send ( 200, "text/html", PAGE_AdminGeneralSettings ); 
	Serial.println(__FUNCTION__); 
}

void send_general_configuration_values_html()
{
	String values ="";
	values += "heatsp|" +   (String) config.HeatSP +  "|input\n";
	values += "heathist|" +  (String)  config.HeatHist +  "|input\n";
	server.send ( 200, "text/plain", values);
	Serial.println(__FUNCTION__); 
}
