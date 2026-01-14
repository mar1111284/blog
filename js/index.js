// js/index.js

document.addEventListener('DOMContentLoaded', () => {
    // Check if we have active root session from this tab
    if (sessionStorage.getItem('rekav_root_session') === 'true') {
        window.REKAV_ROOT = true;
    } else {
        window.REKAV_ROOT = false;
    }
});

