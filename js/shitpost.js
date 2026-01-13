window.REKAV_ROOT = (sessionStorage.getItem('rekav_root_session') === 'true');
const RESOLVE_IMAGE = window.REKAV_ROOT;
const TILE_COLS = 16;
const TILE_ROWS = 9;
const TILE_COUNT = TILE_COLS * TILE_ROWS;

document.addEventListener("DOMContentLoaded", async () => {
    const container = document.getElementById("shitpost-container");
    if (!container) return;

    let posts = [];

    try {
        const response = await fetch("shitposts.json");
        if (!response.ok) throw new Error("Failed to load shitposts");
        posts = await response.json();
    } catch (err) {
        console.error(err);
        container.innerHTML = '<p style="color:#f44;text-align:center;">Failed to load the chaos...</p>';
        return;
    }
    
    const isDesktop = window.innerWidth > 768;

	posts.forEach((post, index) => {
		const article = document.createElement("article");
		article.className = "preview-item";

		// Make the whole card clickable if articlePath exists
		if (post.articlePath) {
		    article.style.cursor = "pointer";
		    article.title = "Click to read full post"; // optional tooltip
		    article.addEventListener("click", () => {
		        window.location.href = post.articlePath;
		    });
		}

		article.innerHTML = `
		    <div class="preview-media"></div>
		    <h2>${escapeHtml(post.title)}</h2>
		    <time datetime="${post.date}">${formatDate(post.date)}</time>
		    <p>${escapeHtml(post.content)}</p>
		`;

		container.appendChild(article);
		initTileChaos(article, index + 1, isDesktop);
	});
});

// Basic HTML escape
function escapeHtml(unsafe) {
    return unsafe
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

function formatDate(isoDate) {
    const date = new Date(isoDate);
    return date.toLocaleDateString("en-US", { 
        month: "short", 
        day: "numeric", 
        year: "numeric" 
    });
}

function initTileChaos(article, imgIndex, isDesktop) {
    const media = article.querySelector(".preview-media");
    if (!media) return;

    const folder = isDesktop 
        ? `assets/shitpost/image_${imgIndex}/`
        : `assets/shitpost/image_${imgIndex}_small/`;

    media.style.position = "relative";
    media.style.overflow = "hidden";

    const tiles = [];
    let loaded = 0;

    for (let i = 0; i < TILE_COUNT; i++) {
        const img = new Image();
        img.src = `${folder}tile_${i.toString().padStart(3, "0")}.jpg`;

        img.onload = () => {
            tiles[i] = img;
            loaded++;
            if (loaded === TILE_COUNT) {
                startChaos(media, tiles);
            }
        };

        img.onerror = () => console.warn(`Tile ${i} failed to load for post ${imgIndex}`);
    }
}

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
        setTimeout(() => { resolving = true; }, 2500);
    }

    elements.forEach((el, index) => {
        const correctX = (index % TILE_COLS) * tileW;
        const correctY = Math.floor(index / TILE_COLS) * tileH;

        const move = () => {
            if (el.dataset.locked === "true") return;

            let x, y;
            if (resolving && Math.random() < 0.25) {
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
