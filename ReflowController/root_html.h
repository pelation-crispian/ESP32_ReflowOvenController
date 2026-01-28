const char ROOT_HTML[] PROGMEM = R"=====(
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
    <style>
      :root {
        --bg: #f4efe6;
        --bg-top: #f7f2ea;
        --panel: #fbf7f0;
        --ink: #1f1c18;
        --muted: #6d6154;
        --line: #d8cbb8;
        --accent: #e0592a;
        --accent-2: #1e9fb3;
        --warn: #e0a12a;
        --danger: #b42318;
        --shadow: rgba(31, 28, 24, 0.12);
      }
      * {
        box-sizing: border-box;
      }
      body {
        margin: 0;
        font-family: "IBM Plex Mono", "Space Mono", Menlo, Monaco, "Courier New", monospace;
        background:
          radial-gradient(circle at 12% 8%, rgba(255,255,255,0.65), transparent 40%),
          radial-gradient(circle at 92% 2%, rgba(255,255,255,0.35), transparent 35%),
          linear-gradient(160deg, var(--bg-top) 0%, var(--bg) 100%);
        background-attachment: fixed;
        color: var(--ink);
      }
      .page {
        max-width: 1200px;
        margin: 0 auto;
        padding: 16px;
      }
      .layout {
        display: grid;
        grid-template-columns: minmax(320px, 420px) 1fr;
        gap: 16px;
        align-items: start;
      }
      .stack {
        display: flex;
        flex-direction: column;
        gap: 12px;
      }
      .panel {
        background: var(--panel);
        border: 1px solid var(--line);
        border-radius: 8px;
        padding: 14px;
        box-shadow: 0 6px 16px var(--shadow);
        animation: panelIn 0.45s ease-out both;
      }
      .stack .panel:nth-child(1) { animation-delay: 40ms; }
      .stack .panel:nth-child(2) { animation-delay: 80ms; }
      .stack .panel:nth-child(3) { animation-delay: 120ms; }
      .stack .panel:nth-child(4) { animation-delay: 160ms; }
      .chart-panel { animation-delay: 100ms; }
      @keyframes panelIn {
        from { opacity: 0; transform: translateY(6px); }
        to { opacity: 1; transform: translateY(0); }
      }
      .panel h2 {
        margin: 0 0 10px 0;
        font-size: 12px;
        text-transform: uppercase;
        letter-spacing: 0.18em;
        color: var(--muted);
      }
      .panel-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 10px;
      }
      .status-hero {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        flex-wrap: wrap;
        border-top: 1px dashed var(--line);
        border-bottom: 1px dashed var(--line);
        padding: 12px 0;
      }
      .temp-block {
        min-width: 200px;
      }
      .eyebrow {
        font-size: 10px;
        letter-spacing: 0.2em;
        text-transform: uppercase;
        color: var(--muted);
      }
      .temp-big {
        font-size: 44px;
        font-weight: 600;
        letter-spacing: -0.02em;
      }
      .temp-big .unit {
        font-size: 18px;
        margin-left: 6px;
        color: var(--muted);
      }
      .subline {
        margin-top: 4px;
        font-size: 12px;
        color: var(--muted);
      }
      .status-pills {
        display: flex;
        flex-direction: column;
        gap: 8px;
        align-items: flex-start;
      }
      .pill {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 4px 10px;
        border-radius: 999px;
        border: 1px solid var(--line);
        font-size: 11px;
        letter-spacing: 0.16em;
        text-transform: uppercase;
        color: var(--muted);
        background: #f4ede2;
      }
      .pill.state {
        background: #1f1c18;
        color: #f7f2ea;
        border-color: #1f1c18;
      }
      .pill.state.paused {
        background: var(--warn);
        border-color: var(--warn);
        color: #1f1c18;
      }
      .pill.hot {
        background: #ffebe1;
        border-color: #e9b8a7;
        color: #b44418;
      }
      .pill.cool {
        background: #e3f6f8;
        border-color: #b8dfe5;
        color: #1a7a8b;
      }
      .pill.off {
        background: #efe6d8;
        border-color: #d8cbb8;
        color: #7b6f60;
      }
      .status-grid {
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 10px 12px;
        margin-top: 10px;
      }
      .stat {
        display: flex;
        flex-direction: column;
        gap: 2px;
      }
      .stat .label {
        font-size: 10px;
        letter-spacing: 0.18em;
        text-transform: uppercase;
        color: var(--muted);
      }
      .stat .value {
        font-size: 14px;
      }
      .controls-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
        gap: 10px 12px;
      }
      label {
        font-size: 10px;
        letter-spacing: 0.18em;
        text-transform: uppercase;
        color: var(--muted);
      }
      input, select, button {
        font-size: 13px;
        padding: 7px 9px;
        font-family: inherit;
      }
      input, select {
        width: 100%;
        background: #f8f3ea;
        border: 1px solid var(--line);
        border-radius: 4px;
        color: var(--ink);
      }
      button {
        background: var(--accent);
        border: 1px solid #c64f22;
        color: #fff;
        border-radius: 4px;
        cursor: pointer;
        text-transform: uppercase;
        letter-spacing: 0.14em;
        font-size: 11px;
      }
      button.secondary {
        background: #2e2a27;
        border-color: #2e2a27;
      }
      button.warning {
        background: var(--warn);
        border-color: var(--warn);
        color: #1f1c18;
      }
      button.danger {
        background: var(--danger);
        border-color: var(--danger);
      }
      button:active {
        transform: translateY(1px);
      }
      .inline {
        display: flex;
        gap: 8px;
        align-items: center;
        flex-wrap: wrap;
      }
      .muted {
        color: var(--muted);
        font-size: 11px;
        letter-spacing: 0.08em;
        text-transform: uppercase;
      }
      #chart_div {
        width: 100%;
        height: 420px;
      }
      #export {
        display: inline-block;
        margin-top: 8px;
        color: var(--accent);
        text-decoration: none;
        font-size: 12px;
        letter-spacing: 0.12em;
        text-transform: uppercase;
      }
      #export:hover {
        text-decoration: underline;
      }
      @media (max-width: 900px) {
        .layout {
          grid-template-columns: 1fr;
        }
        #chart_div {
          height: 300px;
        }
      }
    </style>
    <script>
      google.charts.load('current', { 'packages': ['line', 'corechart'] });
      let chartdata;
      let classicChart;
      let lastState = "";
      let lastPaused = false;

      function initChart() {
        chartdata = new google.visualization.DataTable();
        chartdata.addColumn('number', 'Seconds');
        chartdata.addColumn('number', 'Temperature');
        chartdata.addColumn({type: 'string', role: 'annotation'});
        chartdata.addColumn('number', 'Setpoint');
        chartdata.addColumn('number', 'Power');
        classicChart = new google.visualization.LineChart(document.getElementById('chart_div'));
      }

      function drawChart() {
        const options = {
          series: {
            0: { targetAxisIndex: 0 },
            1: { targetAxisIndex: 0, lineDashStyle: [6, 4] },
            2: { targetAxisIndex: 1 }
          },
          backgroundColor: 'transparent',
          chartArea: { left: 52, top: 20, width: '80%', height: '70%' },
          colors: ['#e0592a', '#b06a36', '#1e9fb3'],
          hAxis: {
            title: 'Time (s)',
            textStyle: { color: '#6d6154' },
            titleTextStyle: { color: '#6d6154' },
            gridlines: { color: '#e5d9c7' }
          },
          vAxes: {
            0: {
              title: 'Temp (°C)',
              viewWindow: { max: 300, min: 0 },
              textStyle: { color: '#6d6154' },
              titleTextStyle: { color: '#6d6154' },
              gridlines: { color: '#e5d9c7' }
            },
            1: {
              title: 'Power (%)',
              viewWindow: { max: 100, min: 0 },
              textStyle: { color: '#6d6154' },
              titleTextStyle: { color: '#6d6154' },
              gridlines: { color: '#efe6d8' }
            }
          },
          legend: {
            position: 'bottom',
            textStyle: { color: '#6d6154', fontSize: 11 }
          },
          lineWidth: 2,
          annotations: {
            alwaysOutside: true,
            style: 'line',
            textStyle: { bold: true }
          }
        };
        classicChart.draw(chartdata, options);
      }

      function apiGet(path) {
        const sep = path.indexOf('?') === -1 ? '?' : '&';
        return fetch(path + sep + 't=' + Date.now(), { cache: 'no-store' })
          .then(res => res.json());
      }

      function formatTime(seconds) {
        const m = Math.floor(seconds / 60);
        const s = Math.floor(seconds % 60);
        return m.toString().padStart(2, '0') + ':' + s.toString().padStart(2, '0');
      }

      function updateStatus() {
        apiGet('/api/status').then(data => {
          const setText = (id, text) => {
            const el = document.getElementById(id);
            if (el) el.textContent = text;
          };
          const setPill = (id, text, cls) => {
            const el = document.getElementById(id);
            if (!el) return;
            el.textContent = text;
            el.className = 'pill ' + cls;
          };

          setText('st-state', data.state + (data.paused ? ' (Paused)' : ''));
          setText('st-temp', data.temp.toFixed(1) + ' °C');
          setText('st-temp-big', data.temp.toFixed(1));
          setText('st-ramp', data.ramp.toFixed(2) + ' °C/s');
          setText('st-setpoint', data.setpoint.toFixed(1) + ' °C');
          setText('st-power', data.power + ' %');
          setText('st-profile', data.profileName + ' (#' + data.profileId + ')');
          setText('st-remaining', formatTime(data.remaining || 0));

          const statePill = document.getElementById('st-state-pill');
          if (statePill) {
            statePill.textContent = data.state + (data.paused ? ' PAUSED' : '');
            statePill.className = 'pill state' + (data.paused ? ' paused' : '');
          }
          setPill('st-heater', data.heaterOn ? 'HEATER ON' : 'HEATER OFF', data.heaterOn ? 'hot' : 'off');
          setPill('st-fan', data.fanOn ? 'FAN ON' : 'FAN OFF', data.fanOn ? 'cool' : 'off');

          let label = null;
          if (data.state !== lastState || data.paused !== lastPaused) {
            label = data.paused ? 'Paused' : data.state;
            lastState = data.state;
            lastPaused = data.paused;
          }
          if (data.state === 'Idle' || data.state === 'Settings' || data.state === 'Edit' || data.state === 'Manual') {
            const rows = chartdata.getNumberOfRows();
            if (rows > 0) { chartdata.removeRows(0, rows); }
          } else if (data.paused && !label) {
            // keep chart static while paused
          } else {
            chartdata.addRow([data.time / 1000.0, data.temp, label, data.setpoint, data.power]);
          }
          drawChart();
        });
      }

      function loadProfiles() {
        apiGet('/api/profiles').then(list => {
          const select = document.getElementById('profile-select');
          select.innerHTML = '';
          list.forEach(item => {
            const opt = document.createElement('option');
            opt.value = item.id;
            opt.textContent = item.id + ' - ' + item.name;
            select.appendChild(opt);
          });
        });
      }

      function loadSettings() {
        apiGet('/api/settings').then(data => {
          document.getElementById('profile-select').value = data.profileId;
          document.getElementById('profile-name').value = data.profile.name;
          document.getElementById('profile-soak-temp').value = data.profile.soakTemp;
          document.getElementById('profile-soak-duration').value = data.profile.soakDuration;
          document.getElementById('profile-peak-temp').value = data.profile.peakTemp;
          document.getElementById('profile-peak-duration').value = data.profile.peakDuration;
          document.getElementById('profile-ramp-up').value = data.profile.rampUpRate;
          document.getElementById('profile-ramp-down').value = data.profile.rampDownRate;

          document.getElementById('pid-kp').value = Number(data.pid.kp).toFixed(3);
          document.getElementById('pid-ki').value = Number(data.pid.ki).toFixed(3);
          document.getElementById('pid-kd').value = Number(data.pid.kd).toFixed(3);

          document.getElementById('const-setpoint').value = data.constTemp.setpoint;
          document.getElementById('const-beep').value = data.constTemp.beepMinutes;

          document.getElementById('tune-output').value = data.tuning.output;
          document.getElementById('tune-noise').value = data.tuning.noiseBand;
          document.getElementById('tune-step').value = data.tuning.outputStep;
          document.getElementById('tune-lookback').value = data.tuning.lookbackSec;
          document.getElementById('guard-lead').value = data.control.guardLeadSec;
          document.getElementById('guard-hyst').value = data.control.guardHystC;
          document.getElementById('guard-rise').value = data.control.guardMinRiseCps;
          document.getElementById('guard-max').value = data.control.guardMaxSetpointC;
          document.getElementById('const-slew').value = data.control.constSlewCps;
          document.getElementById('integral-band').value = data.control.integralBandC;

          const fanValue = data.fanOverride === 1 ? 'on' : (data.fanOverride === 0 ? 'off' : 'auto');
          document.getElementById('fan-override').value = fanValue;
        });
      }

      function initControls() {
        document.getElementById('btn-start-cycle').onclick = () => apiGet('/api/action?cmd=start_cycle');
        document.getElementById('btn-start-const').onclick = () => apiGet('/api/action?cmd=start_const');
        document.getElementById('btn-start-tune').onclick = () => apiGet('/api/action?cmd=start_tune');
        document.getElementById('btn-stop').onclick = () => apiGet('/api/action?cmd=stop');
        document.getElementById('btn-pause').onclick = () => apiGet('/api/action?cmd=pause');
        document.getElementById('btn-resume').onclick = () => apiGet('/api/action?cmd=resume');

        document.getElementById('btn-const-apply').onclick = () => {
          const sp = document.getElementById('const-setpoint').value;
          const beep = document.getElementById('const-beep').value;
          apiGet('/api/consttemp?setpoint=' + sp + '&beepMinutes=' + beep).then(loadSettings);
        };
        document.querySelectorAll('.btn-beep-preset').forEach(btn => {
          btn.onclick = () => {
            const minutes = btn.getAttribute('data-min');
            document.getElementById('const-beep').value = minutes;
          };
        });

        document.getElementById('btn-manual-apply').onclick = () => {
          const power = document.getElementById('manual-power').value;
          apiGet('/api/action?cmd=manual_power&power=' + power);
        };
        document.getElementById('btn-manual-stop').onclick = () => apiGet('/api/action?cmd=manual_stop');

        document.getElementById('btn-fan-apply').onclick = () => {
          const v = document.getElementById('fan-override').value;
          apiGet('/api/action?cmd=fan&value=' + v);
        };

        document.getElementById('btn-profile-load').onclick = () => {
          const id = document.getElementById('profile-select').value;
          apiGet('/api/profile/select?id=' + id).then(() => {
            loadSettings();
            loadProfiles();
          });
        };
        document.getElementById('btn-profile-save').onclick = () => {
          const params = new URLSearchParams({
            name: document.getElementById('profile-name').value,
            soakTemp: document.getElementById('profile-soak-temp').value,
            soakDuration: document.getElementById('profile-soak-duration').value,
            peakTemp: document.getElementById('profile-peak-temp').value,
            peakDuration: document.getElementById('profile-peak-duration').value,
            rampUpRate: document.getElementById('profile-ramp-up').value,
            rampDownRate: document.getElementById('profile-ramp-down').value
          });
          apiGet('/api/profile/update?' + params.toString()).then(() => {
            loadSettings();
            loadProfiles();
          });
        };

        document.getElementById('btn-pid-save').onclick = () => {
          const params = new URLSearchParams({
            kp: document.getElementById('pid-kp').value,
            ki: document.getElementById('pid-ki').value,
            kd: document.getElementById('pid-kd').value
          });
          apiGet('/api/pid?' + params.toString()).then(loadSettings);
        };

        document.getElementById('btn-tune-save').onclick = () => {
          const params = new URLSearchParams({
            output: document.getElementById('tune-output').value,
            noiseBand: document.getElementById('tune-noise').value,
            outputStep: document.getElementById('tune-step').value,
            lookbackSec: document.getElementById('tune-lookback').value
          });
          apiGet('/api/tune?' + params.toString()).then(loadSettings);
        };

        document.getElementById('btn-control-save').onclick = () => {
          const params = new URLSearchParams({
            guardLeadSec: document.getElementById('guard-lead').value,
            guardHystC: document.getElementById('guard-hyst').value,
            guardMinRiseCps: document.getElementById('guard-rise').value,
            guardMaxSetpointC: document.getElementById('guard-max').value,
            constSlewCps: document.getElementById('const-slew').value,
            integralBandC: document.getElementById('integral-band').value
          });
          apiGet('/api/control?' + params.toString()).then(loadSettings);
        };

        document.getElementById('export').onclick = function () {
          const csvFormattedDataTable = google.visualization.dataTableToCsv(chartdata).replace(/[,]/g, ";");
          const encodedUri = 'data:application/csv;charset=utf-8,' +
            'Time;Temp;State;Setpoint;Power\n' + encodeURIComponent(csvFormattedDataTable);
          this.href = encodedUri;
          this.download = 'Reflow_' + (new Date().toISOString().substring(0, 16).replace(/[\-:T]/g, "_")) + '.csv';
          this.target = '_blank';
        };
      }

      google.charts.setOnLoadCallback(function () {
        initChart();
        initControls();
        loadProfiles();
        loadSettings();
        updateStatus();
        setInterval(updateStatus, 1000);
      });
    </script>
  </head>
  <body>
    <div class="page">
      <div class="layout">
        <div class="stack">
          <div class="panel status-panel">
            <div class="panel-header">
              <h2>Status</h2>
              <div id="st-state-pill" class="pill state">-</div>
            </div>
            <div class="status-hero">
              <div class="temp-block">
                <div class="eyebrow">Current Temp</div>
                <div class="temp-big"><span id="st-temp-big">--</span><span class="unit">°C</span></div>
                <div class="subline">Setpoint <span id="st-setpoint">-</span></div>
              </div>
              <div class="status-pills">
                <div id="st-heater" class="pill">HEATER -</div>
                <div id="st-fan" class="pill">FAN -</div>
              </div>
            </div>
            <div class="status-grid">
              <div class="stat"><span class="label">State</span><span class="value" id="st-state">-</span></div>
              <div class="stat"><span class="label">Ramp</span><span class="value" id="st-ramp">-</span></div>
              <div class="stat"><span class="label">Power</span><span class="value" id="st-power">-</span></div>
              <div class="stat"><span class="label">Profile</span><span class="value" id="st-profile">-</span></div>
              <div class="stat"><span class="label">Remaining</span><span class="value" id="st-remaining">-</span></div>
              <div class="stat"><span class="label">Temp</span><span class="value" id="st-temp">-</span></div>
            </div>
          </div>
          <div class="panel">
            <h2>Live Controls</h2>
            <div class="controls-grid">
              <div class="inline">
                <button id="btn-start-cycle">Start Cycle</button>
                <button class="secondary" id="btn-start-const">Start Const</button>
                <button class="secondary" id="btn-start-tune">Start AutoTune</button>
              </div>
              <div class="inline">
                <button class="warning" id="btn-pause">Pause</button>
                <button class="secondary" id="btn-resume">Resume</button>
                <button class="danger" id="btn-stop">Stop</button>
              </div>
              <div>
                <label>Constant Temp (°C)</label>
                <input id="const-setpoint" type="number" min="0" max="350" step="1" />
              </div>
              <div>
                <label>Beep After (min)</label>
                <input id="const-beep" type="number" min="0" max="720" step="1" />
              </div>
              <div class="inline">
                <button class="secondary btn-beep-preset" data-min="60">1 hr</button>
                <button class="secondary btn-beep-preset" data-min="120">2 hr</button>
                <button class="secondary btn-beep-preset" data-min="240">4 hr</button>
                <button class="secondary btn-beep-preset" data-min="480">8 hr</button>
                <button class="secondary btn-beep-preset" data-min="1440">24 hr</button>
              </div>
              <div class="inline">
                <button id="btn-const-apply">Apply Constant Temp</button>
              </div>
              <div></div>
              <div>
                <label>Manual Power (%)</label>
                <input id="manual-power" type="number" min="0" max="100" step="1" value="0" />
              </div>
              <div class="inline">
                <button id="btn-manual-apply">Apply Manual Power</button>
                <button class="secondary" id="btn-manual-stop">Stop Manual</button>
              </div>
              <div>
                <label>Fan Override</label>
                <select id="fan-override">
                  <option value="auto">Auto</option>
                  <option value="on">On</option>
                  <option value="off">Off</option>
                </select>
              </div>
              <div class="inline">
                <button id="btn-fan-apply">Apply Fan</button>
              </div>
            </div>
            <div class="muted">Changes sync to the TFT on the next update tick.</div>
          </div>
          <div class="panel">
            <h2>Profile</h2>
            <div class="controls-grid">
              <div>
                <label>Active Profile</label>
                <select id="profile-select"></select>
              </div>
              <div class="inline">
                <button id="btn-profile-load">Load</button>
              </div>
              <div>
                <label>Name</label>
                <input id="profile-name" type="text" maxlength="10" />
              </div>
              <div>
                <label>Soak Temp (°C)</label>
                <input id="profile-soak-temp" type="number" min="0" max="350" step="1" />
              </div>
              <div>
                <label>Soak Duration (s)</label>
                <input id="profile-soak-duration" type="number" min="0" max="20000" step="1" />
              </div>
              <div>
                <label>Peak Temp (°C)</label>
                <input id="profile-peak-temp" type="number" min="0" max="350" step="1" />
              </div>
              <div>
                <label>Peak Duration (s)</label>
                <input id="profile-peak-duration" type="number" min="0" max="20000" step="1" />
              </div>
              <div>
                <label>Ramp Up (°C/s)</label>
                <input id="profile-ramp-up" type="number" min="0.1" max="20" step="0.01" />
              </div>
              <div>
                <label>Ramp Down (°C/s)</label>
                <input id="profile-ramp-down" type="number" min="0.1" max="20" step="0.01" />
              </div>
              <div class="inline">
                <button id="btn-profile-save">Save Profile</button>
              </div>
            </div>
          </div>
          <div class="panel">
            <h2>PID + Guard + AutoTune</h2>
            <div class="controls-grid">
              <div>
                <label>Kp</label>
                <input id="pid-kp" type="number" step="0.001" />
              </div>
              <div>
                <label>Ki</label>
                <input id="pid-ki" type="number" step="0.001" />
              </div>
              <div>
                <label>Kd</label>
                <input id="pid-kd" type="number" step="0.001" />
              </div>
              <div class="inline">
                <button id="btn-pid-save">Save PID</button>
              </div>
              <div>
                <label>Autotune Output (%)</label>
                <input id="tune-output" type="number" min="0" max="100" step="1" />
              </div>
              <div>
                <label>Noise Band (°C)</label>
                <input id="tune-noise" type="number" min="0" max="20" step="1" />
              </div>
              <div>
                <label>Output Step (%)</label>
                <input id="tune-step" type="number" min="0" max="100" step="1" />
              </div>
              <div>
                <label>Lookback (s)</label>
                <input id="tune-lookback" type="number" min="1" max="300" step="1" />
              </div>
              <div class="inline">
                <button id="btn-tune-save">Save AutoTune</button>
              </div>
              <div>
                <label>Guard Lead (s)</label>
                <input id="guard-lead" type="number" min="0" max="120" step="0.01" />
              </div>
              <div>
                <label>Guard Hyst (°C)</label>
                <input id="guard-hyst" type="number" min="0" max="20" step="0.01" />
              </div>
              <div>
                <label>Guard Min Rise (°C/s)</label>
                <input id="guard-rise" type="number" min="0" max="5" step="0.01" />
              </div>
              <div>
                <label>Guard Max Set (°C)</label>
                <input id="guard-max" type="number" min="0" max="350" step="0.01" />
              </div>
              <div>
                <label>Const Slew (°C/s)</label>
                <input id="const-slew" type="number" min="0" max="20" step="0.01" />
              </div>
              <div>
                <label>I Band (°C)</label>
                <input id="integral-band" type="number" min="0" max="200" step="0.01" />
              </div>
              <div class="inline">
                <button id="btn-control-save">Save Guard</button>
              </div>
            </div>
          </div>
        </div>
        <div class="panel chart-panel">
          <h2>Live Graph</h2>
          <div id="chart_div"></div>
          <a id="export" href="#">Download CSV</a>
        </div>
      </div>
    </div>
  </body>
</html>
)=====";
