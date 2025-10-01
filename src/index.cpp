#include "index.h"

// ======== WEB PANEL ========
const char PAGE_INDEX[] PROGMEM = R"html(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover" />
    <title>Builder Panel</title>
    <style>
      :root {
        --bg: #000;
        --ink: #9aff9a;
        --muted: #4caf50;
        --dim: #2b572c;
        --warn: #ffd166;
        --err: #ff6b6b;
        --accent: #00e676;
      }
      * { box-sizing: border-box }
      html, body { height: 100% }
      body {
        margin: 0;
        background: var(--bg);
        color: var(--ink);
        font: 13px monospace;
      }
      body::before {
        content: "";
        position: fixed;
        inset: 0;
        pointer-events: none;
        background: #222;
        background-size: 100% 2px, 100% 100%;
        mix-blend-mode: screen;
        opacity: .35;
      }
      .wrap { height: calc(100svh - 30px); display: flex; gap: 8px; padding: 8px }
      .pane { display: flex }
      .pane.center { width: 75%; }
      .frame { background: #000; border: 1px solid var(--dim); padding: 6px; min-width: 200px }
      .right>.frame { min-width: 320px }
      .center>.frame { width: 100%; }
      .title { color: var(--muted); margin-bottom: 4px }
      .title::before { content: "+-----[" }
      .title::after  { content: "]-----+" }
      .palette { overflow: auto }
      .pill {
        display: flex; gap: 1ch; align-items: center;
        padding: 6px 4px; border-bottom: 1px dashed var(--dim);
        cursor: grab; color: var(--ink)
      }
      .pill .tag { color: var(--muted) }
      .pill:hover { background: #001900 }
      .board { overflow: auto; flex: 1 }
      .lane { border: 1px dashed var(--dim); padding: 6px; height: auto; min-height: 100%; }
      .block {
        margin: 6px 0; padding: 6px; border: 1px solid var(--dim);
        position: relative; display: grid; grid-template-columns: auto 1fr auto;
        gap: 1ch; align-items: start; background: #000;
      }
      .block.compact { grid-template-columns: auto auto 1fr; align-items: center }
      .block .hdr { color: var(--muted); min-width: 80px; text-align: right; }
      .drag { cursor: grab; color: #79ff79 }
      .kill { cursor: pointer; color: var(--err); background: transparent; border: 0; text-align: right; }
      .fields { display: grid; gap: 6px }
      .fields.cols0 { display:none }
      .fields.cols1 { grid-template-columns: minmax(14ch, 1fr) }
      .fields.cols2 { grid-template-columns: repeat(2, minmax(14ch, 1fr)) }
      @media (max-width:599px) {
        .fields.cols1 { grid-template-columns: minmax(16ch, 1fr) }
        .fields.cols2 { grid-template-columns: repeat(2, minmax(12ch, 1fr)) }
      }
      @media (min-width:600px) and (max-width:1199px) {
        .fields.cols2 { grid-template-columns: repeat(2, minmax(14ch, 1fr)) }
      }
      .field { display: flex; gap: .5ch; align-items: center }
      .field span { color: var(--muted) }
      .meta>.field>span { min-width: 55px }
      .field input, .field select, textarea, .meta input, .meta select {
        background: #001400; color: var(--ink); border: 1px solid var(--dim);
        padding: 4px 6px; outline: none; width: 100%;
      }
      .field input:focus, .field select:focus, textarea:focus, .meta input:focus, .meta select:focus {
        border-color: var(--accent); box-shadow: 0 0 0 1px var(--accent) inset;
      }
      .bar { display: flex; gap: 8px; flex-wrap: wrap; padding-top: 6px; border-top: 1px solid var(--dim); margin-top: 6px }
      .btn { background: #001100; color: var(--ink); border: 1px solid var(--dim); padding: 6px 10px; cursor: pointer }
      .btn.primary { border-color: var(--accent); color: #b8ffb8 }
      .btn.warn { border-color: #a68a00; color: #ffe38c }
      .btn.good { border-color: #0f0; color: #b8ffb8 }
      .btn:active { transform: translateY(1px) }
      .meta { display: grid; grid-template-columns: 1fr 1fr; gap: 6px }
      .code { display: flex; flex-direction: column; min-height: 0; height: 100%; }
      #jsonOut { height: 100%; }
      .status { padding: 4px 8px; border-top: 1px solid var(--dim); color: #8fff8f;
        display: flex; justify-content: space-between; gap: 1ch }
    </style>
  </head>
  <body>
    <div class="wrap">
      <section class="pane left">
        <div class="frame">
          <div class="title">blocks</div>
          <div class="palette mono" id="palette">
            <div class="pill" draggable="true" data-kind="wait"><span class="tag">time</span> WAIT :: ms</div>
            <div class="pill" draggable="true" data-kind="type"><span class="tag">kbd </span> TYPE :: text</div>
            <div class="pill" draggable="true" data-kind="key"><span class="tag">kbd </span> KEY :: code</div>
            <div class="pill" draggable="true" data-kind="move"><span class="tag">mouse</span> MOVE :: dx,dy</div>
            <div class="pill" draggable="true" data-kind="click"><span class="tag">mouse</span> CLICK</div>
            <div class="pill" draggable="true" data-kind="scroll"><span class="tag">mouse</span> SCROLL :: dy</div>
            <div class="pill" draggable="true" data-kind="tap"><span class="tag">touch</span> TAP :: x,y</div>
          </div>
        </div>
      </section>
      <section class="pane center">
        <div class="frame" style="display:flex;flex-direction:column;min-height:0">
          <div class="title">steps</div>
          <div class="board mono">
            <div class="lane" id="lane"></div>
          </div>
          <div class="bar"><button class="btn" id="clearSteps">clear</button><button class="btn warn" id="importJson">import json</button><button class="btn primary" id="exportJson">export json</button><button class="btn good" id="saveToDevice">save → esp32</button><button class="btn" id="runOnDevice">run</button></div>
        </div>
      </section>
      <aside class="pane right">
        <div class="frame" style="display:flex;flex-direction:column;min-height:0;flex-wrap:nowrap;">
          <div class="title">script</div>
          <div class="meta mono" style="padding-bottom:6px;border-bottom:1px solid var(--dim);margin-bottom:6px;display:flex;flex-direction:column;">
            <div class="field"><span>name</span><input id="name" value="full_capabilities_demo" /></div>
            <div class="field"><span>repeat</span><input id="repeat" type="number" min="1" value="1" /></div>
            <div class="field"><span>mode</span><select id="mode">
                <option value="relative">relative</option>
                <option value="absolute">absolute</option>
              </select></div>
            <div class="field"><span>ref.w</span><input id="refw" type="number" value="1080" /></div>
            <div class="field"><span>ref.h</span><input id="refh" type="number" value="2400" /></div>
          </div>
          <div class="code mono"><textarea id="jsonOut" spellcheck="false"></textarea></div>
          <div class="bar"><button class="btn" id="copyJson">copy</button><button class="btn" id="downloadJson">download</button></div>
        </div>
      </aside>
    </div>
    <div class="status mono"><span>Terminal::builder</span><span id="stat"></span></div>
    <script>
      const lane = document.getElementById('lane');
      const palette = document.getElementById('palette');
      const jsonOut = document.getElementById('jsonOut');
      const stat = document.getElementById('stat') || { textContent: "" };
      const nameEl = document.getElementById('name');
      const modeEl = document.getElementById('mode');
      const repEl = document.getElementById('repeat');
      const refwEl = document.getElementById('refw');
      const refhEl = document.getElementById('refh');
      const echo = (s) => stat.textContent = s || "";

      // Dropdown options for KEY block
      const KEY_OPTIONS = [
        "ENTER","TAB","ESC","BACKSPACE","SPACE",
        "UP","DOWN","LEFT","RIGHT",
        "HOME","END","PAGEUP","PAGEDOWN","DELETE","INSERT",
        "CTRL+C","CTRL+V","CTRL+X","CTRL+Z","CTRL+A","CTRL+S",
        "CTRL+W","CTRL+T",
        "CTRL+SHIFT+ESC",
        "WIN+R","WIN+E","WIN+D"
      ];

      function uid() { return Math.random().toString(36).slice(2, 9); }

      function blockTemplate(kind, data = {}) {
        const id = uid();
        const el = document.createElement('div');
        el.className = 'block';
        el.draggable = true;
        el.dataset.kind = kind;
        el.dataset.id = id;
        const drag = Object.assign(document.createElement('div'), { className: 'drag', textContent: '::' });
        const hdr  = Object.assign(document.createElement('div'), { className: 'hdr',  textContent: '[' + kind.toUpperCase() + ']' });
        const kill = Object.assign(document.createElement('button'), { className: 'kill', type: 'button', textContent: 'x', title: 'remove' });
        const fields = Object.assign(document.createElement('div'), { className: 'fields' });

        const addField = (label, init, type = 'number', attrs = {}) => {
          const wrap = document.createElement('div');
          wrap.className = 'field';
          wrap.title = label;
          const span = document.createElement('span');
          span.textContent = label.padEnd(5, ' ');
          let input;
          if (type === 'select') {
            input = document.createElement('select');
            (attrs.options || []).forEach(v => {
              const o = document.createElement('option');
              o.value = v;
              o.textContent = v;
              input.appendChild(o);
            });
            input.value = (init ?? '');
          } else {
            input = document.createElement('input');
            input.type = type;
            Object.entries(attrs).forEach(([k, v]) => input.setAttribute(k, v));
            input.value = (init ?? '');
          }
          wrap.append(span, input);
          fields.appendChild(wrap);
          return input;
        };

        let fCount = 0;
        if (kind === 'wait') { addField('ms', data.ms ?? 300); fCount = 1; }
        if (kind === 'type') { addField('text', data.text ?? '', 'text', { placeholder: 'Hello' }); fCount = 1; }
        if (kind === 'key')  {
          // SELECT box instead of text input
          addField('code', data.code ?? 'ENTER', 'select', { options: KEY_OPTIONS });
          fCount = 1;
        }
        if (kind === 'move') { addField('dx', data.dx ?? 20); addField('dy', data.dy ?? 10); fCount = 2; }
        if (kind === 'click'){ fCount = 0; }
        if (kind === 'scroll'){ addField('dy', data.dy ?? -400); addField('dur', data.duration ?? 250); fCount = 2; }
        if (kind === 'tap')  { addField('x', data.x ?? 540); addField('y', data.y ?? 1200); fCount = 2; }

        fields.classList.add(fCount === 0 ? 'cols0' : (fCount === 1 ? 'cols1' : 'cols2'));
        if (fCount === 0) el.classList.add('compact');

        kill.onclick = () => { el.remove(); syncOut(); saveLocal(); };
        el.append(drag, hdr, kill, fields);

        el.addEventListener('dragstart', e => { e.dataTransfer.setData('text/plain', el.dataset.id); el.style.opacity = .5; });
        el.addEventListener('dragend',   () => { el.style.opacity = ''; });

        el.querySelectorAll('input,select').forEach(inp => inp.addEventListener('input', () => { syncOut(); saveLocal(); }));
        return el;
      }

      palette.addEventListener('dragstart', e => {
        const pill = e.target.closest('.pill');
        if (!pill) return;
        e.dataTransfer.setData('text/plain', pill.dataset.kind);
      });
      lane.addEventListener('dragover', e => e.preventDefault());
      lane.addEventListener('drop', e => {
        e.preventDefault();
        const src = e.dataTransfer.getData('text/plain');
        if (!src) return;
        if (["wait","type","key","move","click","scroll","tap"].includes(src)) {
          lane.appendChild(blockTemplate(src, {}));
        } else {
          const moving = [...lane.children].find(b => b.dataset.id === src);
          const after = document.elementFromPoint(e.clientX, e.clientY)?.closest('.block');
          if (!moving) return;
          if (!after || after === moving) lane.appendChild(moving);
          else lane.insertBefore(moving, after.nextSibling);
        }
        syncOut(); saveLocal();
      });

      function readBlock(el) {
        const kind = el.dataset.kind;
        const inputs = el.querySelectorAll('.field');
        const val = name => {
          const inp = [...inputs].find(l => l.title === name)?.querySelector('input,select');
          if (!inp) return undefined;
          return (inp.tagName === 'INPUT' && inp.type === 'number') ? Number(inp.value) : inp.value;
        };
        if (kind === 'wait')   return { wait:   { ms: Number(val('ms')) } };
        if (kind === 'type')   return { type:   { text: String(val('text') || '') } };
        if (kind === 'key')    return { key:    { code: String(val('code') || 'ENTER') } };
        if (kind === 'move')   return { move:   { dx: Number(val('dx') || 0), dy: Number(val('dy') || 0) } };
        if (kind === 'click')  return { click:  {} };
        if (kind === 'scroll') return { scroll: { dy: Number(val('dy') || 0), duration: Number(val('dur') || 0) } };
        if (kind === 'tap')    return { tap:    { x: Number(val('x') || 0), y: Number(val('y') || 0) } };
        return null;
      }

      function toJSON() {
        const steps = [...lane.children].map(readBlock).filter(Boolean);
        return {
          name: (nameEl.value || 'script').trim(),
          mode: modeEl.value,
          reference: { w: Number(refwEl.value || 1080), h: Number(refhEl.value || 2400) },
          repeat: Math.max(1, Number(repEl.value || 1)),
          steps
        };
      }

      function fromJSON(obj) {
        if (!obj || !obj.steps) return;
        nameEl.value = obj.name || nameEl.value;
        modeEl.value = obj.mode || modeEl.value;
        repEl.value  = obj.repeat || repEl.value;
        if (obj.reference) {
          refwEl.value = obj.reference.w ?? refwEl.value;
          refhEl.value = obj.reference.h ?? refhEl.value;
        }
        lane.innerHTML = '';
        (obj.steps || []).forEach(step => {
          const k = Object.keys(step)[0];
          const v = step[k] || {};
          lane.appendChild(blockTemplate(k, v));
        });
        syncOut();
      }

      function syncOut() { jsonOut.value = JSON.stringify(toJSON(), null, 2); }
      function saveLocal() {
        localStorage.setItem('tb_meta', JSON.stringify({
          name: nameEl.value, mode: modeEl.value, repeat: repEl.value, w: refwEl.value, h: refhEl.value
        }));
        localStorage.setItem('tb_steps', jsonOut.value);
        echo("saved locally");
      }
      function restoreLocal() {
        try {
          const m = localStorage.getItem('tb_meta');
          if (m) {
            const o = JSON.parse(m);
            nameEl.value = o.name || nameEl.value;
            modeEl.value = o.mode || modeEl.value;
            repEl.value  = o.repeat || repEl.value;
            refwEl.value = o.w || refwEl.value;
            refhEl.value = o.h || refhEl.value;
          }
          const s = localStorage.getItem('tb_steps');
          if (s) { const obj = JSON.parse(s); fromJSON(obj); return true; }
        } catch {}
        return false;
      }
      async function loadFromDevice() {
        try {
          const r = await fetch('/api/get', { cache: 'no-store' });
          if (!r.ok) throw 0;
          const txt = await r.text();
          const obj = JSON.parse(txt);
          fromJSON(obj);
          echo("loaded from device");
          return true;
        } catch (e) { echo("device load failed; using local/default"); return false; }
      }
      async function saveToDevice() {
        try {
          syncOut();
          const r = await fetch('/api/save', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: jsonOut.value });
          echo(r.ok ? "saved → esp32" : "save failed");
        } catch { echo("save failed"); }
      }
      async function runOnDevice() {
        try {
          const r = await fetch('/api/run', { method: 'POST' });
        echo(r.ok ? "running…" : "run failed");
        } catch { echo("run failed"); }
      }
      document.getElementById('clearSteps').onclick  = () => { lane.innerHTML = ''; syncOut(); saveLocal(); };
      document.getElementById('exportJson').onclick = syncOut;
      document.getElementById('copyJson').onclick   = async () => { syncOut(); await navigator.clipboard.writeText(jsonOut.value); echo("copied"); };
      document.getElementById('downloadJson').onclick = () => {
        syncOut();
        const a = document.createElement('a');
        a.href = URL.createObjectURL(new Blob([jsonOut.value], { type: 'application/json' }));
        a.download = (nameEl.value || 'script') + '.json';
        a.click();
        URL.revokeObjectURL(a.href);
        echo("downloaded");
      };
      document.getElementById('importJson').onclick = async () => {
        const txt = prompt('paste JSON:'); if (!txt) return;
        try {
          const obj = JSON.parse(txt);
          if (!obj.steps) throw 0;
          fromJSON(obj); saveLocal(); echo("import ok");
        } catch { alert('invalid JSON'); echo("import failed"); }
      };
      document.getElementById('saveToDevice').onclick = saveToDevice;
      document.getElementById('runOnDevice').onclick  = runOnDevice;

      const DEFAULT_OBJ = {
        name: "hello",
        mode: "relative",
        reference: { w: 1080, h: 2400 },
        repeat: 1,
        steps: [ { wait: { ms: 400 } }, { type: { text: "Hello_from_ESP32" } }, { key: { code: "ENTER" } } ]
      };
      window.addEventListener('load', async () => {
        const ok = await loadFromDevice();
        if (!ok) {
          if (!restoreLocal()) { fromJSON(DEFAULT_OBJ); echo("default loaded"); }
        }
      });
    </script>
  </body>
</html>
)html";