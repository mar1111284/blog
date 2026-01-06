// js/index.js

document.addEventListener('DOMContentLoaded', () => {
    const terminalBody = document.getElementById('terminal-body');
    const terminal = document.getElementById('terminal');

    let currentInput = '';

    // Create hidden input for real keyboard capture (especially mobile)
    const hiddenInput = document.createElement('input');
    hiddenInput.type = 'text';
    hiddenInput.style.position = 'absolute';
    hiddenInput.style.opacity = '0';
    hiddenInput.style.height = '1px';
    hiddenInput.style.width = '1px';
    hiddenInput.style.border = 'none';
    hiddenInput.style.outline = 'none';
    hiddenInput.autocapitalize = 'off';
    hiddenInput.autocorrect = 'off';
    hiddenInput.spellcheck = false;
    terminal.appendChild(hiddenInput);

    const appendLine = (html) => {
        const line = document.createElement('div');
        line.innerHTML = html;
        terminalBody.appendChild(line);
        terminalBody.scrollTop = terminalBody.scrollHeight;
    };

    const escapeHtml = (text) => {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    };

    const renderPrompt = () => {
        const livePrompt = terminalBody.querySelector('.live-prompt');
        if (livePrompt) livePrompt.remove();

        const promptLine = document.createElement('div');
        promptLine.className = 'live-prompt';
        promptLine.innerHTML = `
            <span class="prompt">guest</span><span style="color:#ffb86c;">@rekav</span>:~$ </span>
            <span class="input">${escapeHtml(currentInput)}</span>
            <span class="cursor">█</span>
        `;
        terminalBody.appendChild(promptLine);
        terminalBody.scrollTop = terminalBody.scrollHeight;
    };

    const newPrompt = () => {
        currentInput = '';
        renderPrompt();
        hiddenInput.value = ''; // sync
        hiddenInput.focus();    // bring up keyboard on mobile
    };

    // === COMMAND PARSER ===
    const commands = {
        help: () => {
            appendLine('<span style="color: #ff79c6;">Available commands:</span>');
            appendLine('  <span class="hint">help</span>     – Show this help message');
            appendLine('  <span class="hint">clear</span>    – Clear the terminal (coming soon)');
            appendLine('  <span class="hint">shitpost</span> – View latest shitposts');
            appendLine('  <span class="hint">random</span>   – Random thoughts & rants');
            appendLine('  <span class="hint">gallery</span>  – Image gallery');
            appendLine('  <span class="hint">about</span>    – Who the hell is Rekav?');
            appendLine('');
        },
    };

    const handleCommand = (input) => {
        const trimmed = input.trim();
        if (trimmed === '') return;

        const parts = trimmed.split(' ');
        const cmd = parts[0].toLowerCase();

        if (commands[cmd]) {
            commands[cmd](parts.slice(1));
        } else {
            appendLine(`<span style="color: #ff5555;">bash: ${escapeHtml(cmd)}: command not found</span>`);
            appendLine(`<span style="color: #6272a4;">Did you mean something else? Try</span> <span class="hint">help</span>`);
        }
    };

    // === INITIALIZATION ===
    appendLine('<span style="color: #ff79c6;">Rekav\'s terminal interface version 0.1</span>');
    appendLine('Type <span class="hint">help</span> for available commands.');
    appendLine('');
    newPrompt();

    // Focus hidden input when terminal is tapped
    terminal.addEventListener('click', () => {
        hiddenInput.focus();
    });

    // Sync hidden input → currentInput on any change
    hiddenInput.addEventListener('input', (e) => {
        currentInput = e.target.value;
        renderPrompt();
    });

    // Handle Enter key (from hidden input)
    hiddenInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();

            // Freeze the line
            const livePrompt = terminalBody.querySelector('.live-prompt');
            if (livePrompt) {
                livePrompt.innerHTML = `<span class="prompt">guest</span><span style="color:#ffb86c;">@rekav</span>:~$ </span><span class="input">${escapeHtml(currentInput)}</span>`;
                livePrompt.classList.remove('live-prompt');
            }

            handleCommand(currentInput);
            newPrompt();
        }
    });

    // Optional: support Backspace/Delete if needed (input event already handles most)
});
