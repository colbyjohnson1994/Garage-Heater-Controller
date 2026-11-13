const char PAGE_Manifest_json[] PROGMEM = R"=====(
{
  "name": "Garage Heater",
  "short_name": "Heater",
  "description": "Garage heater controller",
  "start_url": "/",
  "scope": "/",
  "display": "standalone",
  "orientation": "portrait",
  "background_color": "#0f172a",
  "theme_color": "#1e293b",
  "icons": [
    {
      "src": "/favicon.svg",
      "sizes": "any",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
}
)=====";

const char PAGE_ServiceWorker_js[] PROGMEM = R"=====(
const CACHE = 'heater-v1';

self.addEventListener('install', e => {
  e.waitUntil(self.skipWaiting());
});

self.addEventListener('activate', e => {
  e.waitUntil(self.clients.claim());
});

self.addEventListener('fetch', e => {
  // Network-first; fall back to cache only for navigation
  e.respondWith(
    fetch(e.request).catch(() => caches.match(e.request))
  );
});
)=====";
