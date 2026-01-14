// ------------------------------
// DOM-dependent initialization
// ------------------------------
document.addEventListener('DOMContentLoaded', () => {
    if (sessionStorage.getItem('rekav_root_session') === 'true') {
        window.REKAV_ROOT = true;
    } else {
        window.REKAV_ROOT = false;
    }
});


// js/index.js
// WASM ↔ JS translation bridge (sessionStorage based)
// Reliable, main-thread only, no direct WASM calls

// js/index.js
// Browser-safe translation bridge using MyMemory API

(function () {
    const REQ_KEY = "rekav_translate_request";
    const RES_KEY = "rekav_translate_result";

    async function handleTranslateRequest() {
        const raw = sessionStorage.getItem(REQ_KEY);
        if (!raw) return; // no request

        // Consume request immediately
        sessionStorage.removeItem(REQ_KEY);

        let req;
        try {
            req = JSON.parse(raw);
        } catch {
            sessionStorage.setItem(RES_KEY, "__ERROR__");
            return;
        }

        // Log incoming request
        console.log("[translate] request:", req.source, "→", req.target, req.text);

        try {
            // Build URL with source|target
			const url =
			  "https://api.mymemory.translated.net/get" +
			  "?q=" + encodeURIComponent(req.text) +
			  "&langpair=" + encodeURIComponent(req.source) + "|" + encodeURIComponent(req.target) +
			  "&mt=1"; // force machine translation fallback


            const res = await fetch(url);
            const data = await res.json();

            console.log("[translate] response:", data);

            const translated =
                data &&
                data.responseData &&
                data.responseData.translatedText;

            if (typeof translated === "string") {
                sessionStorage.setItem(RES_KEY, translated);
                console.log("[translate] translated:", translated);
            } else {
                sessionStorage.setItem(RES_KEY, "__FAILED__");
            }

        } catch (e) {
            console.error("Translation error:", e);
            sessionStorage.setItem(RES_KEY, "__ERROR__");
        }
    }

    // Polling loop: check every 100ms
    setInterval(handleTranslateRequest, 100);
})();

(function () {
    const REQ_KEY = "rekav_weather_request";
    const RES_KEY = "rekav_weather_result";

    const weatherCodes = {
        0: "Clear sky", 1: "Mainly clear", 2: "Partly cloudy", 3: "Overcast",
        45: "Fog", 48: "Depositing rime fog",
        51: "Drizzle: light", 53: "Drizzle: moderate", 55: "Drizzle: dense",
        61: "Rain: slight", 63: "Rain: moderate", 65: "Rain: heavy",
        71: "Snow fall: slight", 73: "Snow fall: moderate", 75: "Snow fall: heavy",
        80: "Rain showers: slight", 81: "Rain showers: moderate", 82: "Rain showers: violent",
        95: "Thunderstorm: slight/moderate", 96: "Thunderstorm + slight hail", 99: "Thunderstorm + heavy hail"
    };

    async function handleWeatherRequest() {
        const raw = sessionStorage.getItem(REQ_KEY);
        if (!raw) return;
        sessionStorage.removeItem(REQ_KEY);

        let req;
        try { req = JSON.parse(raw); } 
        catch { sessionStorage.setItem(RES_KEY, "__ERROR__"); return; }

        const { latitude, longitude, city } = req;

        try {
            const url = `https://api.open-meteo.com/v1/forecast` +
                        `?latitude=${latitude}&longitude=${longitude}` +
                        `&hourly=temperature_2m,uv_index,precipitation,weathercode` +
                        `&timezone=auto`;

            const res = await fetch(url);
            const data = await res.json();

            let text = "";
            text += "\n";
            text += "--------------------------------------------------\n";
            text += `Weather forecast for ${city}\n`;
            text += `Lat: ${data.latitude.toFixed(2)}, Lon: ${data.longitude.toFixed(2)}\n`;
            text += `Elevation: ${data.elevation} m, Timezone: ${data.timezone}\n`;

            const tempNow = data.hourly.temperature_2m[0];
            const uvNow = data.hourly.uv_index[0];
            const precNow = data.hourly.precipitation[0];
            const codeNow = data.hourly.weathercode[0];
            const descNow = weatherCodes[codeNow] || "Unknown";

            text += `Current Temp: ${tempNow.toFixed(1)}°C  UV: ${uvNow.toFixed(1)}  Precip: ${precNow.toFixed(1)} mm\n`;
            text += `Description: ${descNow}\n`;
            text += "--------------------------------------------------\n";
            text += "\n";

            // --- Graph function ---
            function generateGraph(values, unit, yMax = null) {
                const height = 10;
                const hours = values.length;
                const maxVal = yMax ?? Math.max(...values);
                const minVal = Math.min(...values);
                let graph = `${unit} (next ${hours}h):\n`;

                for (let row = height; row >= 0; row--) {
                    let line = "";
                    let scaleVal = minVal + ((row / height) * (maxVal - minVal));
                    line += scaleVal.toFixed(1).padStart(5) + " |"; // Y-axis

                    for (let i = 0; i < hours; i++) {
                        line += (values[i] >= scaleVal) ? "#" : " ";
                    }
                    graph += line + "\n";
                }

                // X-axis
                let xLine = "      ";
                for (let i = 0; i < hours; i++) {
                    xLine += (i % 2 === 0) ? String(i % 24).padStart(1, " ") : " ";
                }
                graph += xLine + "\n";
                return graph;
            }

            // UV graph
            const uvArray = data.hourly.uv_index.slice(0,24);
            text += generateGraph(uvArray, "UV", 11); // max UV 11
			text += "\n";
            // Temp graph
            const tempArray = data.hourly.temperature_2m.slice(0,24);
            text += generateGraph(tempArray, "Temp °C");
			text += "\n";
            sessionStorage.setItem(RES_KEY, text);
        } catch(e) {
            console.error(e);
            sessionStorage.setItem(RES_KEY, "__ERROR__");
        }
    }

    setInterval(handleWeatherRequest, 200);
})();









