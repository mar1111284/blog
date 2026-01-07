const IS_ROOT = (sessionStorage.getItem('rekav_root_session') === 'true');
const RESOLVE_IMAGE = IS_ROOT;
const TILE_COLS = 16;
const TILE_ROWS = 9;

document.addEventListener("DOMContentLoaded", () => {


    const TILE_COUNT = TILE_COLS * TILE_ROWS;
    const isDesktop = window.innerWidth > 768;

    const articles = document.querySelectorAll(".shitpost");

    articles.forEach((article, articleIndex) => {

        const container = article.querySelector(".shitpost-media");
        if (!container) return;

        const imgIndex = articleIndex + 1;
        const folder = isDesktop
            ? `assets/shitpost/image_${imgIndex}/`
            : `assets/shitpost/image_${imgIndex}_small/`;

        container.style.position = "relative";
        container.style.overflow = "hidden";

        const tiles = [];
        let loaded = 0;

        for (let i = 0; i < TILE_COUNT; i++) {
            const img = new Image();
            img.src = `${folder}tile_${i.toString().padStart(3, "0")}.jpg`;
            img.onload = () => {
                tiles[i] = img;
                loaded++;
                if (loaded === TILE_COUNT) startChaos(container, tiles);
            };
        }
    });

    function startChaos(container, tiles) {

        const tileW = tiles[0].naturalWidth;
        const tileH = tiles[0].naturalHeight;
        const fullW = tileW * TILE_COLS;
        const fullH = tileH * TILE_ROWS;

        container.style.width = `${fullW}px`;
        container.style.height = `${fullH}px`;

        const elements = [];

        for (let i = 0; i < TILE_COUNT; i++) {
            const el = document.createElement("img");
            el.src = tiles[i].src;

            el.style.position = "absolute";
            el.style.width = `${tileW}px`;
            el.style.height = `${tileH}px`;
            el.style.left = "0";
            el.style.top = "0";
            el.style.pointerEvents = "none";
            el.style.transition = "transform 0.7s cubic-bezier(0.2,0.8,0.4,1)";

            container.appendChild(el);

            el.dataset.locked = "false";

            el.style.transform = `translate(
                ${Math.random() * fullW}px,
                ${Math.random() * fullH}px
            )`;

            elements.push(el);
        }

        let resolving = false;

        if (RESOLVE_IMAGE) {
            // start resolving after a delay
            setTimeout(() => resolving = true, 2500);
        }

        elements.forEach((el, index) => {

            const correctX = (index % TILE_COLS) * tileW;
            const correctY = Math.floor(index / TILE_COLS) * tileH;

            const move = () => {

                if (el.dataset.locked === "true") return;

                let x, y;

                if (resolving && Math.random() < 0.25) {
                    // LOCK this tile forever
                    x = correctX;
                    y = correctY;
                    el.dataset.locked = "true";
                } else {
                    const r = Math.floor(Math.random() * TILE_COUNT);
                    x = (r % TILE_COLS) * tileW;
                    y = Math.floor(r / TILE_COLS) * tileH;
                }

                el.style.transform = `translate(${x}px, ${y}px)`;

                setTimeout(move, 500 + Math.random() * 700);
            };

            setTimeout(move, Math.random() * 1200);
        });
    }
});

