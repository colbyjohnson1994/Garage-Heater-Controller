const char PAGE_Style_css[] PROGMEM = R"=====(
:root {
  --bg: #0f172a;
  --card: #1e293b;
  --text: #f1f5f9;
  --accent: #3b82f6;
  --on: #22c55e;
  --off: #ef4444;
  --muted: #94a3b8;
}
* { box-sizing: border-box; }
body {
  font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  background: var(--bg);
  color: var(--text);
  margin: 0;
  padding: 0;
  line-height: 1.5;
}
.topnav {
  position: sticky;
  top: 0;
  background: #1e293b;
  padding: 10px 16px;
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
  justify-content: center;
  border-bottom: 1px solid #334155;
  z-index: 100;
}
.topnav a {
  color: var(--accent);
  text-decoration: none;
  font-weight: 500;
  font-size: 0.9rem;
}
.topnav a:hover { text-decoration: underline; }
.container { max-width: 720px; margin: 0 auto; padding: 16px; }
h1 { font-size: 1.8rem; margin: 0 0 20px; text-align: center; }
.card {
  background: var(--card);
  border-radius: 16px;
  padding: 24px;
  margin-bottom: 16px;
  box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3);
}
.temp-big {
  font-size: 4.5rem;
  font-weight: 700;
  text-align: center;
  margin: 10px 0;
  letter-spacing: -2px;
}
.temp-unit { font-size: 1.5rem; color: var(--muted); }
.status-badge {
  display: inline-block;
  padding: 6px 16px;
  border-radius: 999px;
  font-weight: 600;
  font-size: 0.95rem;
}
.status-on { background: rgba(34,197,94,0.2); color: var(--on); }
.status-off { background: rgba(239,68,68,0.2); color: var(--off); }
.switch {
  position: relative;
  display: inline-block;
  width: 60px;
  height: 34px;
}
.switch input { opacity: 0; width: 0; height: 0; }
.slider {
  position: absolute;
  cursor: pointer;
  top: 0; left: 0; right: 0; bottom: 0;
  background-color: #475569;
  transition: .3s;
  border-radius: 34px;
}
.slider:before {
  position: absolute;
  content: "";
  height: 26px; width: 26px;
  left: 4px; bottom: 4px;
  background-color: white;
  transition: .3s;
  border-radius: 50%;
}
input:checked + .slider { background-color: var(--on); }
input:checked + .slider:before { transform: translateX(26px); }
.btn {
  display: inline-block;
  background: var(--accent);
  color: white;
  padding: 12px 24px;
  border-radius: 10px;
  text-decoration: none;
  font-weight: 600;
  border: none;
  cursor: pointer;
  margin: 4px;
}
.btn:hover { filter: brightness(1.1); }
.btn-secondary { background: #475569; }
input[type="number"], input[type="text"], input[type="time"], input[type="password"] {
  background: #0f172a;
  border: 1px solid #334155;
  color: var(--text);
  padding: 10px 12px;
  border-radius: 8px;
  font-size: 1rem;
  width: 100%;
  max-width: 200px;
}
label { display: block; margin: 12px 0 4px; color: var(--muted); font-size: 0.9rem; }
.row { display: flex; gap: 16px; align-items: center; flex-wrap: wrap; }
.nav { text-align: center; margin-top: 24px; }
.nav a { color: var(--accent); margin: 0 12px; text-decoration: none; }
.weather-icon { font-size: 2.5rem; }
.rate { font-size: 0.9rem; color: var(--muted); text-align: center; margin-top: 4px; }
.rate.up { color: #ef4444; }
.rate.down { color: #3b82f6; }
)=====";
