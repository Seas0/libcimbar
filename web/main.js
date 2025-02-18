const Main = function() {
let _interval = 20;
let _pauseCount = 0;

let _showStats = true;
let _colorBits = 2;
let _counter = 0;
let _frameCount = 0;
let _renderTime = 0;
let _shaking = false;

let _fileQueue = [];
let _currentFileIndex = -1;

// internal helpers
function toggleFullscreen()
{
  if (document.fullscreenElement) {
    return document.exitFullscreen();
  }
  else {
    return document.documentElement.requestFullscreen();
  }
}

function importFile(f, index)
{
  const fileReader = new FileReader();
  fileReader.onload = (event) => {
    const imageData = new Uint8Array(event.target.result);
    const numBytes = imageData.length * imageData.BYTES_PER_ELEMENT;
    const dataPtr = Module._malloc(numBytes);
    const dataOnHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, numBytes);
    dataOnHeap.set(imageData);
    Main.encode(f.name, dataOnHeap, index);
    Module._free(dataPtr);

    Main.setHTML("current-file", f.name);
  };
  fileReader.onerror = () => {
    console.error('Unable to read file ' + f.name + '.');
  };

  fileReader.readAsArrayBuffer(f);
}

// public interface, exposed as Main object
return {
  init : function(canvas)
  {
    Module._initialize_GL(1040, 1040);
    Main.resize();
    Main.check_GL_enabled(canvas);
  },

  check_GL_enabled : function(canvas)
  {
    if (canvas.getContext("2d")) {
       var elem = document.getElementById('dragdrop');
       elem.classList.add("error");
    }
  },

  resize : function()
  {
    // reset zoom
    const canvas = document.getElementById('canvas');
    // TODO: account for the nav bar
    const width = window.innerWidth - 10;
    const height = window.innerHeight - 10;
    Main.scaleCanvas(canvas, width, height);
    Main.alignInvisibleClick(canvas);
  },

  toggleFullscreen : function()
  {
    toggleFullscreen().then(Main.resize);
    Main.togglePause(true);
  },

  togglePause : function(pauseCount)
  {
    // pause is a cooldown counter.
    // We pause to help autofocus, but we don't want to do it forever...
    if (pauseCount === undefined) {
       pauseCount = !Main.isPaused();
    }
    _pauseCount = pauseCount ? 15 : 0;
  },

  isPaused : function()
  {
     return _pauseCount > 0;
  },

  scaleCanvas : function(canvas, width, height)
  {
    let dim = height < width ? height : width;
    console.log("scaling canvas to " + dim + "²");
    canvas.style.width = dim + "px";
    canvas.style.height = dim + "px";
  },

  alignInvisibleClick : function(canvas)
  {
     // reset cursor invisible zone to
     // the canvas size and position
     canvas = canvas || document.getElementById('canvas');
     const canvas_position = canvas.getBoundingClientRect();
     const invisible_click = document.getElementById("invisible_click");
     invisible_click.style.width = canvas.style.width;
     invisible_click.style.height = canvas.style.height;
     invisible_click.style.top = canvas_position.top + "px";
     invisible_click.style.left = canvas_position.left + "px";
     invisible_click.style.zoom = canvas.style.zoom;
  },

  encode : function(filename, data, fileIndex)
  {
    console.log("encoding " + filename);
    // const encode_id = fileIndex >= 0 ? fileIndex + 109 : -1;
    const encode_id = fileIndex >= 0 ? fileIndex : -1;
    const res = Module._encode(data.byteOffset, data.length, encode_id);
    console.log("encoder returns: " + res);
    Main.setTitle(filename);
    Main.setActive(true);
  },

  dragDrop : function(event)
  {
    console.log("drag drop?");
    console.log(event);
    const files = event.dataTransfer.files;
    if (files && files.length > 0) {
      _fileQueue = Array.from(files);
      _currentFileIndex = 0;
      importFile(_fileQueue[0], 0);
      Main.updateQueueStatus();
    }
    Main.setMode('B');
  },

  clickNav : function()
  {
    document.getElementById("nav-button").focus();
  },

  blurNav : function(pauseCount)
  {
    if (pauseCount === undefined)
      pauseCount = 1;

    document.getElementById("nav-button").blur();
    document.getElementById("nav-content").blur();
    document.getElementById("nav-find-file-link").blur();
    Main.togglePause(pauseCount);
  },

  clickFileInput : function()
  {
    document.getElementById("file_input").click();
  },

  fileInput : function(ev)
  {
    console.log("file input: " + ev);
    const files = document.getElementById('file_input').files;
    if (files && files.length > 0) {
      _fileQueue = Array.from(files);
      _currentFileIndex = 0;
      importFile(_fileQueue[0], 0);
      Main.updateQueueStatus();
    }
    Main.blurNav(false);
    Main.setMode('B');
  },

  nextFrame : function()
  {
    _counter += 1;
    if (_pauseCount > 0) {
      _pauseCount -= 1;
    }
    let start = performance.now();
    if (!Main.isPaused()) {
      Module._render();
      _frameCount = Module._next_frame();
    }
    let elapsed = performance.now() - start;
    let nextInterval = _interval>elapsed? _interval-elapsed : 0;
    setTimeout(Main.nextFrame, nextInterval);

    if (_showStats && _frameCount) {
      _renderTime += elapsed;
      Main.setHTML( "status", elapsed + " : " + _frameCount + " : " + Math.ceil(_renderTime/_frameCount));
    }
    if ( !(_counter & 31) ) {
      Main.resize();
    }
  },

  setActive : function(active)
  {
    // hide cursor when there's a barcode active
    const invisible_click = document.getElementById("invisible_click");
    invisible_click.classList.remove("active");
    invisible_click.classList.add("active");
  },

  setMode : function(mode_str)
  {
    const is_4c = (mode_str == "4C");
    // colorBits, ecc, compression, shaking, legacyMode
    // colorBits: [0, 3]
    // ecc: [0, 149], 255 = default
    // compression: [0, 150], 255 = default
    // shaking: bool
    // legacyMode: bool
    Module._configure(_colorBits, 255, 255, _shaking, is_4c);

    const nav = document.getElementById("nav-container");
    if (is_4c) {
      nav.classList.remove("mode-b");
      nav.classList.add("mode-4c");
    } else if (mode_str == "B") {
      nav.classList.add("mode-b");
      nav.classList.remove("mode-4c");
    } else {
      nav.classList.remove("mode-b");
      nav.classList.remove("mode-4c");
    }
  },

  setHTML : function(id, msg)
  {
    document.getElementById(id).innerHTML = msg;
  },

  setTitle : function(msg)
  {
    document.title = "Cimbar: " + msg;
  },

  setInterval : function(interval)
  {
    _interval = interval;
  },

  setShaking : function(shaking)
  {
    _shaking = shaking;
  },

  setStats : function(showStats)
  {
    _showStats = showStats;
  },

  setColorBits : function(colorBits)
  {
    _colorBits = colorBits;
  },

  start : function()
  {
    Main.nextFrame();
  },

  previousFile: function() {
    if (_currentFileIndex > 0) {
      _currentFileIndex--;
      importFile(_fileQueue[_currentFileIndex], _currentFileIndex);
      Main.updateQueueStatus();
    }
    Main.blurNav(false);
  },

  nextQueuedFile: function() {
    if (_currentFileIndex < _fileQueue.length - 1) {
      _currentFileIndex++;
      importFile(_fileQueue[_currentFileIndex], _currentFileIndex);
      Main.updateQueueStatus();
    }
    Main.blurNav(false);
  },

  updateQueueStatus: function() {
    const total = _fileQueue.length;
    if (total === 0) {
      Main.setHTML("queue-status", "No files queued");
      return;
    }
    const current = _currentFileIndex + 1;
    Main.setHTML("queue-status", `File ${current}/${total}`);
  },

};
}();

window.addEventListener('keydown', function(e) {
  e = e || event;
  if (e.target instanceof HTMLBodyElement) {
    if (e.key == 'Enter' || e.keyCode == 13 ||
        e.key == 'Tab' || e.keyCode == 9 ||
        e.key == 'Space' || e.keyCode == 32
    ) {
      Main.clickNav();
      e.preventDefault();
    }
    else if (e.key == 'Backspace' || e.keyCode == 8) {
      Main.togglePause(true);
      e.preventDefault();
    }
    else if (e.key == 'ArrowLeft' || e.keyCode == 37) {
      Main.previousFile();
      e.preventDefault();
    }
    else if (e.key == 'ArrowRight' || e.keyCode == 39) {
      Main.nextQueuedFile();
      e.preventDefault();
    }
  }
  else {
    if (e.key == 'Escape' || e.keyCode == 27 ||
        e.key == 'Backspace' || e.keyCode == 8 ||
        e.key == 'End' || e.keyCode == 35 ||
        e.key == 'Home' || e.keyCode == 36
    ) {
      Main.blurNav();
    }
    else if (e.key == 'Tab' || e.keyCode == 9 ||
            e.key == 'ArrowDown' || e.keyCode == 40
    ) {
      var nav = document.getElementById('nav-button');
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      if (nav.classList.contains('attention')) {
        nav.classList.remove('attention');
        links[0].classList.add('attention');
        return;
      }
      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          var next = i+1 == links.length? nav : links[i+1];
          links[i].classList.remove('attention');
          next.classList.add('attention');
          break;
        }
      }
    }
    else if (e.key == 'ArrowUp' || e.keyCode == 38)
    {
      var nav = document.getElementById('nav-button');
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      if (nav.classList.contains('attention')) {
        nav.classList.remove('attention');
        links[links.length-1].classList.add('attention');
        return;
      }

      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          var next = i == 0? nav : links[i-1];
          links[i].classList.remove('attention');
          next.classList.add('attention');
          break;
        }
      }
    }
    else if (e.key == 'Enter' || e.keyCode == 13 ||
             e.key == ' ' || e.keyCode == 32
    ) {
      var nav = document.getElementById('nav-button');
      if (nav.classList.contains('attention')) {
        Main.blurNav();
        return;
      }
      var links = document.getElementById('nav-content').getElementsByTagName('a');
      for (var i = 0; i < links.length; i++) {
        if (links[i].classList.contains('attention')) {
          links[i].click();
        }
      }
    }
  }
}, true);

window.addEventListener("touchstart", function(e) {
  e = e || event;
  Main.togglePause(true);
}, false);

window.addEventListener("touchend", function(e) {
  e = e || event;
  Main.togglePause(false);
}, false);

window.addEventListener("touchcancel", function(e) {
  e = e || event;
  Main.togglePause(false);
}, false);

window.addEventListener("dragover", function(e) {
  e = e || event;
  e.preventDefault();

  document.body.style["opacity"] = 0.5;
}, false);

window.addEventListener("dragleave", function(e) {
  e = e || event;
  e.preventDefault();

  document.body.style["opacity"] = 1.0;
}, false);

window.addEventListener("drop", function(e) {
  e = e || event;
  e.preventDefault();
  e.stopPropagation();
  Main.dragDrop(e);
  document.body.style["opacity"] = 1.0;
}, false);
