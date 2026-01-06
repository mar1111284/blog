// js/index.js

document.addEventListener('DOMContentLoaded', () => {
    const terminalBody = document.getElementById('terminal-body');
    const terminal = document.getElementById('terminal');

    let currentInput = '';

    // Hidden input — now positioned at the bottom and fully invisible
    const hiddenInput = document.createElement('input');
    hiddenInput.type = 'text';
    hiddenInput.autocapitalize = 'off';
    hiddenInput.autocorrect = 'off';
    hiddenInput.spellcheck = false;

    // Critical styling to fix mobile issues
    Object.assign(hiddenInput.style, {
        position: 'fixed',          // Fixed so it doesn't push content
        bottom: '20px',             // Place it near bottom (inside terminal view)
        left: '-100px',             // Way off-screen horizontally
        width: '1px',
        height: '1px',
        opacity: '0',
        border: 'none',
        outline: 'none',
        background: 'transparent',
        caretColor: 'transparent',  // Hides the native blinking caret completely
        zIndex: '-1',
        //fontSize: '16px'            // Prevents zoom on iOS when focusing
    });

    document.body.appendChild(hiddenInput); // Attach to body, not terminal

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

        // Always scroll to bottom so user sees what they're typing
        terminalBody.scrollTop = terminalBody.scrollHeight;
    };

    const newPrompt = () => {
        currentInput = '';
        renderPrompt();
        hiddenInput.value = '';
        hiddenInput.focus();
    };

    // === COMMANDS ===
	const commands = {
		help: () => {
		    appendLine('<span style="color: #ff79c6;">Available commands:</span>');
		    appendLine('  <span class="hint">help</span>     – Show this help message');
		    appendLine('  <span class="hint">clear</span>    – Clear the terminal screen');
		    appendLine('  <span class="hint">shitpost</span> – View latest shitposts');
		    appendLine('  <span class="hint">random</span>   – Random thoughts & rants');
		    appendLine('  <span class="hint">gallery</span>  – Image gallery');
		    appendLine('  <span class="hint">about</span>    – Who the hell is Rekav?');
		    appendLine('');
		},

		clear: () => {
		    terminalBody.innerHTML = '';
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

    // Tap terminal → focus hidden input (triggers keyboard)
    terminal.addEventListener('click', () => {
        hiddenInput.focus();
    });

    // Real-time typing sync
    hiddenInput.addEventListener('input', (e) => {
        currentInput = e.target.value;
        renderPrompt();
    });

    // Enter key → execute
    hiddenInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();

            const livePrompt = terminalBody.querySelector('.live-prompt');
            if (livePrompt) {
                livePrompt.innerHTML = `<span class="prompt">guest</span><span style="color:#ffb86c;">@rekav</span>:~$ </span><span class="input">${escapeHtml(currentInput)}</span>`;
                livePrompt.classList.remove('live-prompt');
            }

            handleCommand(currentInput);
            newPrompt();
        }
    });

    // Bonus: Keep focus when scrolling on mobile
    terminalBody.addEventListener('scroll', () => {
        if (document.activeElement !== hiddenInput) {
            hiddenInput.focus();
        }
    });
});
