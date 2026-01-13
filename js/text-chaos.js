// text-chaos.js

window.REKAV_ROOT = (sessionStorage.getItem('rekav_root_session') === 'true');
const RESOLVE_TEXT = window.REKAV_ROOT;


const CHAOS_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+=-[]{};:,.<>?/'" +
    "äöüÄÖÜßéèêëáàâãíìîïóòôõúùûñç";

document.addEventListener("DOMContentLoaded", () => {

    processAllCurrentShitposts();

    const container = document.getElementById("shitpost-container");
    if (container) {
        const observer = new MutationObserver(() => {
            setTimeout(processAllCurrentShitposts, 100);
        });

        observer.observe(container, {
            childList: true,
            subtree: true
        });
    }
    setTimeout(processAllCurrentShitposts, 800);
    setTimeout(processAllCurrentShitposts, 2000);
    document.addEventListener('shitposts-loaded', processAllCurrentShitposts);
});

function processAllCurrentShitposts() {
    document.querySelectorAll('.preview-item:not([data-chaos-processed])').forEach(article => {
        article.dataset.chaosProcessed = 'true';

        const targets = article.querySelectorAll("h2, time, p");
        targets.forEach(el => {
            if (el.dataset.chaosified) return;
            el.dataset.chaosified = 'true';
            chaosifyElement(el);
        });
       
    });
}

function chaosifyElement(element) {

    const originalText = element.innerText;
    const chars = originalText.split("");
    const state = chars.map(ch => ({
        original: ch,
        current: randomChar(),
        locked: ch === " "
    }));

    element.textContent = "";

    let resolving = false;

    if (RESOLVE_TEXT) {
        setTimeout(() => {
            resolving = true;
        }, 1000);
    }

    function tick() {
        let output = "";

        for (let i = 0; i < state.length; i++) {
            const char = state[i];

            if (char.locked) {
                output += char.original;
                continue;
            }

            if (resolving) {

                if (char.current === char.original) {
                    char.locked = true;
                    output += char.original;
                } else {
                    char.current = randomChar();
                    output += char.current;
                }
            } else {

                char.current = randomChar();
                output += char.current;
            }
        }

        element.textContent = output;
        requestAnimationFrame(tick);
    }

    tick();
}

function randomChar() {
    return CHAOS_CHARS[Math.floor(Math.random() * CHAOS_CHARS.length)];
}
