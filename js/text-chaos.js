window.REKAV_ROOT = (sessionStorage.getItem('rekav_root_session') === 'true');
const RESOLVE_TEXT = window.REKAV_ROOT ;

const CHAOS_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+=-[]{};:,.<>?";

document.addEventListener("DOMContentLoaded", () => {

    const targets = document.querySelectorAll(
        ".shitpost h2, .shitpost time, .shitpost p"
    );

    targets.forEach(el => chaosifyElement(el));
});

function chaosifyElement(element) {

    const originalText = element.textContent;
    const chars = originalText.split("");

    const state = chars.map(ch => ({
        original: ch,
        current: randomChar(),
        locked: ch === " " // spaces lock immediately
    }));

    element.textContent = "";

    let resolving = false;
    if (RESOLVE_TEXT) {
        setTimeout(() => resolving = true, 1000);
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
                // mutate until correct
                if (char.current === char.original) {
                    char.locked = true;
                    output += char.original;
                } else {
                    char.current = randomChar();
                    output += char.current;
                }
            } else {
                // infinite chaos
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

