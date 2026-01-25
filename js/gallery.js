// js/gallery.js — Stable version: JSON + size mapping + lightbox
document.addEventListener('DOMContentLoaded', async () => {
    const grid = document.getElementById('gallery-grid');
    const lightbox = document.getElementById('lightbox');
    const lightboxImg = document.getElementById('lightbox-img');
    const closeBtn = document.getElementById('close-btn');

    const sizeMapping = {
        'landscape-small':  'normal',
        'landscape-medium': 'wide',
        'landscape-large':  'big',
        'square-small':     'normal',
        'square-medium':    'big',
        'square-large':     'big',
        'portrait-small':   'normal',
        'portrait-medium':  'tall',
        'portrait-large':   'tall'
    };
    
    // Color mode toggle
	const toggle = document.getElementById('color-mode-toggle');
	const galleryGrid = document.getElementById('gallery-grid');

	// Apply initial state
	if (!toggle.checked) {
		galleryGrid.classList.add('grayscale-mode');
	}

	// Listen for changes
	toggle.addEventListener('change', () => {
		if (toggle.checked) {
		    galleryGrid.classList.remove('grayscale-mode');
		} else {
		    galleryGrid.classList.add('grayscale-mode');
		}
	});

    try {
        const response = await fetch('gallery.json');
        if (!response.ok) throw new Error('Failed to load gallery data');

        const data = await response.json();
        let images = data.images;

        images = images.sort(() => Math.random() - 0.5);

		images.forEach(item => {
			const galleryItem = document.createElement('div');
			const cssClass = sizeMapping[item.size] || 'normal';
			galleryItem.className = `gallery-item ${cssClass}`;

			const img = document.createElement('img');
			img.src = item.url;
			img.alt = item.title || 'Rekav gallery image';
			img.loading = 'lazy';

			img.addEventListener('load', () => {
				galleryItem.classList.add('loaded');
			}, { once: true });

			galleryItem.appendChild(img);
			grid.appendChild(galleryItem);

			galleryItem.addEventListener('click', () => {
				lightboxImg.src = item.url;
				lightbox.classList.add('active');
				document.body.style.overflow = 'hidden';
			});
		});
    } catch (error) {
        console.error('Gallery error:', error);
        grid.innerHTML = '<p style="color:#ff6b6b; text-align:center; padding:40px;">Failed to load images</p>';
    }

    function closeLightbox() {
        lightbox.classList.remove('active');
        document.body.style.overflow = '';
    }

    closeBtn.addEventListener('click', closeLightbox);
    lightbox.addEventListener('click', (e) => {
        if (e.target === lightbox) closeLightbox();
    });
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && lightbox.classList.contains('active')) {
            closeLightbox();
        }
    });
});
