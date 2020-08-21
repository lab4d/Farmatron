/*
function count (timeleft){
    if (var time < timeleft){
        time = time + 1;
    }
    else {
        time = 0;
    }
    
    document.getElementById("progressBar").value = time;
    
}
*/
function setActive() {
	var xhttp = new XMLHttpRequest();
	var text = document.getElementById("active").value;

		xhttp.open("GET", "setActive?active="+text, true);
		xhttp.send();

}
function setFan() {
	var xhttp = new XMLHttpRequest();
	var text = document.getElementById("fan").value;

		xhttp.open("GET", "setFan?fan="+text, true);
		xhttp.send();

}
function setPassive() {
	var xhttp = new XMLHttpRequest();
	var text = document.getElementById("passive").value;

		xhttp.open("GET", "setPassive?passive="+text, true);
		xhttp.send();

}

function setUpload() {
  var xhttp = new XMLHttpRequest();
  var checkBox = document.getElementById("Check1");
  var text = document.getElementById("upload");
  if (checkBox.checked == true){
    xhttp.open("GET", "setUpload?upload=1", true);
  } 
  else {
    xhttp.open("GET", "setUpload?upload=0", true);
  }
  xhttp.send();
}


setInterval(function() {
  getTemp();
  getHum();
  //getDaylight();
  getActive();
  getPassive();
  getUpload();
  getPump();
  getFan();
  
}, 2000); 


function getPump() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == 0){
        document.getElementById("water").outerHTML = '<span id="water" class="iconify green icon" data-icon="mdi:water-pump-off" data-inline="false"></span>'
        //count(10);
      }
      else if (this.responseText == 1){
        document.getElementById("water").outerHTML = '<span id="water" class="iconify blue2 icon" data-icon="mdi:water-pump" data-inline="false"></span>';
        
      }
    }
  };
  xhttp.open("GET", "getPump", true);
  xhttp.send();
}
function getFan() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == 0){
        document.getElementById("fan").outerHTML = '<a id="fan" class="iconify green icon" data-icon="mdi:fan-off" onclick="setFan()" data-inline="false"></a>'
      }
      else if (this.responseText == 1){
        document.getElementById("fan").outerHTML = '<a id="fan" class="iconify blue2 icon" data-icon="mdi:fan" onclick="setFan()" data-inline="false"></a>'
      }
    }
  };
  xhttp.open("GET", "getFan", true);
  xhttp.send();
}

function getTemp() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "getTemp", true);
  xhttp.send();
}
function getHum() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "getHum", true);
  xhttp.send();
}
function getDaylight() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("daylight").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "getDaylight", true);
  xhttp.send();
}
function getActive() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("activeG").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "getActive", true);
  xhttp.send();
}
function getPassive() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("passiveG").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "getPassive", true);
  xhttp.send();
}
function getUpload() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("upload").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "getUpload", true);
  xhttp.send();
}

