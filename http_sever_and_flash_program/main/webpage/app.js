/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;
var wifiConnectInterval = null;

// Variables para rastrear estado PIR y PWM con holdtime
var pirLastActivationTime = 0;
var pwmLastActivationTime = 0;
var PIR_HOLDTIME_MS = 4000;  // 4 segundos como en el firmware
var PWM_HOLDTIME_MS = 4000;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	getUpdateStatus();
	startDHTSensorInterval();
	$("#connect_wifi").on("click", function(){
		checkCredentials();
	}); 
	
	$("#select_mode").on("change", function() {
	  var m = parseInt($(this).val(), 10);
	  if (m === 0) {
	    $("#automatic_options").hide();
	    $("#manual_options").show();
	    $("#ProgrammingMode").hide();
	  } else if (m === 1) {
	    $("#automatic_options").show();
	    $("#manual_options").hide();
	    $("#ProgrammingMode").hide();
	  } else if (m === 2) {
	    // PROGRAMADO
	    $("#automatic_options").hide();
	    $("#manual_options").hide();
	    $("#ProgrammingMode").show();
	    fetchProgramRegisters();  // Cargar la tabla de registros
	  }
	});
	$("#apply_mode").on("click", function() {
	  var mode = parseInt($("#select_mode").val(), 10);
	  var minT = parseFloat($("#temp_min").val());
	  var maxT = parseFloat($("#temp_max").val());
	  var payload = JSON.stringify({ mode: mode, temp_min: minT, temp_max: maxT });
	  $.ajax({
	    url: '/mode.json',
	    dataType: 'json',
	    method: 'POST',
	    cache: false,
	    data: payload,
	    contentType: 'application/json',
	    success: function() { console.log('Mode AUTOMATICO applied'); },
	    error: function(xhr, status, err) { console.error('Mode error', xhr.responseText); }
	  });
	});
	
	// Botón para aplicar modo MANUAL con PWM
	$("#apply_manual_mode").on("click", function() {
	  var mode = 0; // MANUAL
	  var pwm = parseInt($("#pwm_slider").val());
	  var payload = JSON.stringify({ mode: mode, pwm: pwm });
	  $.ajax({
	    url: '/mode.json',
	    dataType: 'json',
	    method: 'POST',
	    cache: false,
	    data: payload,
	    contentType: 'application/json',
	    success: function() { 
	      console.log('Mode MANUAL applied with PWM ' + pwm + '%'); 
	    },
	    error: function(xhr, status, err) { 
	      console.error('Mode error', xhr.responseText); 
	    }
	  });
	});
	
	// Controlar el slider PWM manual (actualiza display en tiempo real)
	$("#pwm_slider").on("input", function() {
	  var val = $(this).val();
	  $("#pwm_slider_value").text(val + "%");
	});
	
	// Inicializar visibilidad
	var initialMode = parseInt($("#select_mode").val(), 10);
	if (initialMode === 0) {
	  $("#manual_options").show();
	  $("#automatic_options").hide();
	  $("#ProgrammingMode").hide();
	} else if (initialMode === 1) {
	  $("#automatic_options").show();
	  $("#manual_options").hide();
	  $("#ProgrammingMode").hide();
	} else if (initialMode === 2) {
	  $("#automatic_options").hide();
	  $("#manual_options").hide();
	  $("#ProgrammingMode").show();
	  fetchProgramRegisters();
	}
});   

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() 
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) 
	{		
        var response = JSON.parse(xhr.responseText);
						
	 	document.getElementById("latest_firmware").innerHTML = response.compile_date + " V 1.0 " + response.compile_time

		// If flashing was complete it will return a 1, else -1
		// A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) 
		{
    		// Set the countdown timer time
            seconds = 10;
            // Start the countdown timer
            otaRebootTimer();
        } 
        else if (response.ota_update_status == -1)
		{
            document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

/**
 * Gets DHT22 sensor temperature and humidity values for display on the web page.
 */


function getregValues()
{
	$.getJSON('/readreg.json', function(data) {
		$("#reg_1").text(data["reg1"]);
		$("#reg_2").text(data["reg2"]);
		$("#reg_3").text(data["reg3"]);
		$("#reg_4").text(data["reg4"]);
		$("#reg_5").text(data["reg5"]);
	});
}

function getDHTSensorValues()
{
	$.getJSON('/dhtSensor.json', function(data) {
		$("#temperature_reading").text(data["temp"]);
	});
}

/**
 * Sets the interval for getting the updated DHT22 sensor values.
 */

function startDHTSensorInterval()
{
	setInterval(getDHTSensorValues, 5000);    
}


/**
 * Clears the connection status interval.
 */
function stopWifiConnectStatusInterval()
{
	if (wifiConnectInterval != null)
	{
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/**
 * Gets the WiFi connection status.
 */
function getWifiConnectStatus()
{
	var xhr = new XMLHttpRequest();
	var requestURL = "/wifiConnectStatus";
	xhr.open('POST', requestURL, false);
	xhr.send('wifi_connect_status');
	
	if (xhr.readyState == 4 && xhr.status == 200)
	{
		var response = JSON.parse(xhr.responseText);
		
		document.getElementById("wifi_connect_status").innerHTML = "Connecting...";
		
		if (response.wifi_connect_status == 2)
		{
			document.getElementById("wifi_connect_status").innerHTML = "<h4 class='rd'>Failed to Connect. Please check your AP credentials and compatibility</h4>";
			stopWifiConnectStatusInterval();
		}
		else if (response.wifi_connect_status == 3)
		{
			document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>Connection Success!</h4>";
			stopWifiConnectStatusInterval();
		}
	}
}

/**
 * Starts the interval for checking the connection status.
 */
function startWifiConnectStatusInterval()
{
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * Connect WiFi function called using the SSID and password entered into the text fields.
 */
function connectWifi()
{
	// Get the SSID and password
	/*selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	$.ajax({
		url: '/wifiConnect.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		data: {'timestamp': Date.now()}
	});
	*/
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	// Create an object to hold the data to be sent in the request body
	var requestData = {
	  'selectedSSID': selectedSSID,
	  'pwd': pwd,
	  'timestamp': Date.now()
	};
	
	// Serialize the data object to JSON
	var requestDataJSON = JSON.stringify(requestData);
	
	$.ajax({
	  url: '/wifiConnect.json',
	  dataType: 'json',
	  method: 'POST',
	  cache: false,
	  data: requestDataJSON, // Send the JSON data in the request body
	  contentType: 'application/json', // Set the content type to JSON
	  success: function(response) {
		// Handle the success response from the server
		console.log(response);
	  },
	  error: function(xhr, status, error) {
		// Handle errors
		console.error(xhr.responseText);
	  }
	});


	//startWifiConnectStatusInterval();
}

/**
 * Checks credentials on connect_wifi button click.
 */
function checkCredentials()
{
	errorList = "";
	credsOk = true;
	
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	if (selectedSSID == "")
	{
		errorList += "<h4 class='rd'>SSID cannot be empty!</h4>";
		credsOk = false;
	}
	if (pwd == "")
	{
		errorList += "<h4 class='rd'>Password cannot be empty!</h4>";
		credsOk = false;
	}
	
	if (credsOk == false)
	{
		$("#wifi_connect_credentials_errors").html(errorList);
	}
	else
	{
		$("#wifi_connect_credentials_errors").html("");
		connectWifi();    
	}
}

/**
 * Shows the WiFi password if the box is checked.
 */
function showPassword()
{
	var x = document.getElementById("connect_pass");
	if (x.type === "password")
	{
		x.type = "text";
	}
	else
	{
		x.type = "password";
	}
}


function send_register()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();
    hours = $("#hours").val();
    minutes = $("#minutes").val();
    
    // Create an array for selected days
    var selectedDays = [];
    if ($("#day_mon").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_tue").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_wed").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_thu").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_fri").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_sat").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_sun").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");

    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'hours': hours,
        'minutes': minutes,
        'selectedDays': selectedDays,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regchange.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });

    // Print the resulting JSON to the console (for testing)
    //console.log(requestDataJSON);
}

/**
 * toogle led function.
 */
function read_reg()
{

	
	$.ajax({
		url: '/readreg.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		//headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		//data: {'timestamp': Date.now()}
	});
//	var xhr = new XMLHttpRequest();
//	xhr.open("POST", "/toogle_led.json");
//	xhr.setRequestHeader("Content-Type", "application/json");
//	xhr.send(JSON.stringify({data: "mi información"}));
}


function erase_register()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();



    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regerase.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });

    // Print the resulting JSON to the console (for testing)
    //console.log(requestDataJSON);
}

function toogle_led() 
{	
	$.ajax({
		url: '/toogle_led.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

function brigthness_up() 
{	
	$.ajax({
		url: '/toogle_led.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

function updateSensorsStatus() {
  $.getJSON('/sensorsStatus.json', function(data) {
    var now = Date.now();
    
    // Actualizar temperatura (siempre mostrar)
    if (data.temperature !== undefined) {
      $("#sensor_temp").text(parseFloat(data.temperature).toFixed(1) + " °C");
    }
    
    // PIR: si está activo, registrar tiempo; mantener "ON" durante 4 segundos
    if (data.pir !== undefined) {
      if (data.pir == 1) {
        pirLastActivationTime = now;
        $("#sensor_pir").text("ON");
        $("#sensor_pir").css("color", "#00ff00");  // Verde
      } else {
        // PIR inactivo: mostrar "OFF" solo si pasaron los 4 segundos
        if (now - pirLastActivationTime >= PIR_HOLDTIME_MS) {
          $("#sensor_pir").text("OFF");
          $("#sensor_pir").css("color", "#888888");  // Gris
        } else {
          // Aún en periodo holdtime: mantener "ON"
          $("#sensor_pir").text("ON");
          $("#sensor_pir").css("color", "#00ff00");
        }
      }
    }
    
    // Modo (siempre mostrar)
    if (data.mode !== undefined) {
      var modeText = (data.mode == 0) ? "MANUAL" : (data.mode == 1) ? "AUTOMATICO" : "PROGRAMADO";
      $("#modo_actual").text(modeText);
    }
    
    // PWM: si es > 0, registrar tiempo; mantener valor durante 4 segundos
    if (data.pwm_pct !== undefined) {
      var pwmValue = Math.round(data.pwm_pct);
      if (pwmValue > 0) {
        pwmLastActivationTime = now;
        $("#pwm_actual").text(pwmValue + " %");
        $("#pwm_actual").css("color", "#00ff00");  // Verde
      } else {
        // PWM 0: mostrar "0 %" solo si pasaron los 4 segundos
        if (now - pwmLastActivationTime >= PWM_HOLDTIME_MS) {
          $("#pwm_actual").text("0 %");
          $("#pwm_actual").css("color", "#888888");  // Gris
        } else {
          // Aún en periodo holdtime: mostrar último valor registrado
          var lastPwmValue = parseInt($("#pwm_actual").text());
          if (isNaN(lastPwmValue)) lastPwmValue = 0;
          if (lastPwmValue === 0) {
            $("#pwm_actual").text("0 %");
          }
          // Mantener color verde mientras esté en holdtime
          $("#pwm_actual").css("color", "#00ff00");
        }
      }
    }
  }).fail(function(){ /* ignore transient errors */ });
}

// Iniciar polling cada 500ms (más frecuente para sincronización exacta con holdtime)
updateSensorsStatus();
setInterval(updateSensorsStatus, 500);

// Controlar PWM manual en modo MANUAL
function setManualPWM(pwm_percent) {
  var payload = JSON.stringify({ pwm: parseInt(pwm_percent) });
  $.ajax({
    url: '/manual_pwm.json',
    dataType: 'json',
    method: 'POST',
    cache: false,
    data: payload,
    contentType: 'application/json',
    success: function() { 
      console.log('Manual PWM set to ' + pwm_percent + '%');
    },
    error: function(xhr, status, err) { 
      console.error('Manual PWM error', xhr.responseText); 
    }
  });
}

// Control del slider PWM manual (cuando está en modo MANUAL)
$("#pwm_slider").on("input", function() {
  var val = $(this).val();
  $("#pwm_slider_value").text(val + "%");
  setManualPWM(val);
});

// Programado: listar / editar / seleccionar registros
async function fetchProgramRegisters() {
  try {
    const resp = await fetch('/program_registers.json');
    if (!resp.ok) throw new Error('HTTP ' + resp.status);
    const data = await resp.json();
    renderProgramTable(data.registers || [], data.selected);
  } catch (e) {
    console.error('fetchProgramRegisters:', e);
  }
}

function renderProgramTable(registers, selected) {
  const container = document.getElementById('programs_container');
  if (!container) return;
  
  // Limitar a solo 3 registros
  const maxRegisters = Math.min(3, registers.length);
  
  let html = '<div class="prog-wrapper"><table class="prog-table"><thead><tr><th>#</th><th>Inicio</th><th>Fin</th><th>Días de la Semana</th><th>Acción</th><th>Activo</th></tr></thead><tbody>';
  
  for (let idx = 0; idx < maxRegisters; idx++) {
    const r = registers[idx];
    const startH = (r.start_hour !== undefined && r.start_hour !== -1) ? String(r.start_hour).padStart(2, '0') : '00';
    const startM = (r.start_min !== undefined && r.start_min !== -1) ? String(r.start_min).padStart(2, '0') : '00';
    const endH = (r.end_hour !== undefined && r.end_hour !== -1) ? String(r.end_hour).padStart(2, '0') : '00';
    const endM = (r.end_min !== undefined && r.end_min !== -1) ? String(r.end_min).padStart(2, '0') : '00';
    const weekdays = r.weekdays || 0;
    
    html += `<tr data-idx="${idx}" class="prog-row">
      <td class="prog-num">${idx + 1}</td>
      <td class="prog-time">
        <input type="number" min="0" max="23" class="start_h time-input" value="${startH}" placeholder="HH"> : 
        <input type="number" min="0" max="59" class="start_m time-input" value="${startM}" placeholder="MM">
      </td>
      <td class="prog-time">
        <input type="number" min="0" max="23" class="end_h time-input" value="${endH}" placeholder="HH"> : 
        <input type="number" min="0" max="59" class="end_m time-input" value="${endM}" placeholder="MM">
      </td>
      <td class="weekdays-cell">` +
        ['Lun','Mar','Mié','Jue','Vie','Sab','Dom'].map((d, bit) => {
          const checked = (weekdays & (1 << bit)) ? 'checked' : '';
          return `<label class="day-label"><input type="checkbox" class="wd" data-bit="${bit}" ${checked}><span>${d}</span></label>`;
        }).join('') +
      `</td>
      <td class="prog-action">
        <button class="save_reg btn-save">Guardar</button>
      </td>
      <td class="prog-radio">
        <input type="radio" name="sel_reg" class="sel_reg" value="${idx}" ${selected === idx ? 'checked' : ''}>
      </td>
    </tr>`;
  }
  
  html += '</tbody></table></div>';
  html += '<div class="prog-buttons"><button id="deselect_prog" class="btn-deselect">Deseleccionar</button></div>';
  container.innerHTML = html;

  // bind save buttons
  container.querySelectorAll('.save_reg').forEach(btn => {
    btn.addEventListener('click', async (ev) => {
      ev.preventDefault();
      const tr = ev.target.closest('tr');
      const idx = Number(tr.dataset.idx);
      
      const start_h = Number(tr.querySelector('.start_h').value) || 0;
      const start_m = Number(tr.querySelector('.start_m').value) || 0;
      const end_h = Number(tr.querySelector('.end_h').value) || 0;
      const end_m = Number(tr.querySelector('.end_m').value) || 0;
      
      let weekdays = 0;
      tr.querySelectorAll('.wd').forEach(cb => {
        if (cb.checked) {
          weekdays |= (1 << Number(cb.dataset.bit));
        }
      });
      
      console.log(`Guardando registro ${idx}:`, {
        start_hour: start_h,
        start_min: start_m,
        end_hour: end_h,
        end_min: end_m,
        weekdays: weekdays.toString(2).padStart(7, '0')
      });
      
      const regObj = {
        idx: idx,
        start_hour: start_h,
        start_min: start_m,
        end_hour: end_h,
        end_min: end_m,
        weekdays: weekdays
      };
      
      await saveProgramRegister(idx, regObj);
    });
  });

  // bind select radio
  container.querySelectorAll('.sel_reg').forEach(radio => {
    radio.addEventListener('change', async (ev) => {
      const val = Number(ev.target.value);
      await selectRegister(val);
      setTimeout(fetchProgramRegisters, 200);
    });
  });

  const deselectBtn = document.getElementById('deselect_prog');
  if (deselectBtn) {
    deselectBtn.addEventListener('click', async () => {
      await selectRegister(255);
      setTimeout(fetchProgramRegisters, 200);
    });
  }
}

async function saveProgramRegister(idx, regObj) {
  try {
    console.log('Guardando registro', idx, ':', regObj);
    
    const resp = await fetch('/program_register.json', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(regObj)
    });
    
    if (!resp.ok) {
      const errorText = await resp.text();
      console.error('HTTP Error:', resp.status, errorText);
      throw new Error('HTTP ' + resp.status);
    }
    
    const data = await resp.json();
    console.log('Respuesta del servidor:', data);
    
    if (data.ok === 1) {
      console.log('✓ Registro guardado correctamente');
      // Mostrar notificación visual
      alert('✓ Registro ' + (idx + 1) + ' guardado correctamente');
      // Recargar después de un pequeño delay
      setTimeout(fetchProgramRegisters, 300);
    } else {
      console.error('Respuesta inesperada:', data);
      alert('Error: Respuesta inesperada del servidor');
    }
  } catch (e) {
    console.error('Error al guardar:', e);
    alert('Error al guardar: ' + e.message);
  }
}

async function selectRegister(selected) {
  try {
    const resp = await fetch('/select_register.json', {
      method: 'POST',
      headers: { 'Content-Type':'application/json' },
      body: JSON.stringify({ selected })
    });
    if (!resp.ok) throw new Error('HTTP ' + resp.status);
    console.log('selected register ->', selected);
  } catch (e) {
    console.error('selectRegister:', e);
  }
}

// initialize program UI on page load
document.addEventListener('DOMContentLoaded', ()=>{
  // ensure container exists; if not, create one under body end
  if (!document.getElementById('programs_container')) {
    const div = document.createElement('div');
    div.id = 'programs_container';
    document.body.appendChild(div);
  }
  fetchProgramRegisters();
});

























