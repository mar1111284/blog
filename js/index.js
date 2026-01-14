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

// ------------------------------
// Terminal print helper
// ------------------------------
window.rekav_add_line = function(text) {
    const term = document.getElementById('terminal'); // your SDL overlay div
    if (term) term.innerText += text + "\n";
    console.log("Terminal:", text);
};

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





