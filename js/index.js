// js/index.js


document.addEventListener('DOMContentLoaded', () => {

// Clean up any old localStorage junk (just in case)
    localStorage.removeItem('rekav_root');
    localStorage.removeItem('rekav_root_persist');

    // Check if we have active root session from this tab
    if (sessionStorage.getItem('rekav_root_session') === 'true') {
        window.REKAV_ROOT = true;
    } else {
        window.REKAV_ROOT = false;
    }
	
    const terminalBody = document.getElementById('terminal-body');
    const terminal = document.getElementById('terminal');
    let awaitingPassword = false;
	const ROOT_PASSWORD = "rekav";
    let currentInput = '';
    let passwordPromptActive = false;
	
	// IOS Carret fix and style
    const hiddenInput = document.createElement('input');
    hiddenInput.type = 'text';
    hiddenInput.autocapitalize = 'off';
    hiddenInput.autocorrect = 'off';
    hiddenInput.spellcheck = false;

    Object.assign(hiddenInput.style, {
        position: 'fixed',
        bottom: '20px',             
        left: '-100px',           
        width: '1px',
        height: '1px',
        opacity: '0',
        border: 'none',
        outline: 'none',
        background: 'transparent',
        caretColor: 'transparent',
        zIndex: '-1',
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

		if (passwordPromptActive && awaitingPassword) {
		    // Password mode: show masked input
		    promptLine.innerHTML = `
		        <span style="color: #ff79c6;">Password for root: </span>
		        <span class="input">${'*'.repeat(currentInput.length)}</span>
		        <span class="cursor">█</span>
		    `;
		} else {
		    // Normal mode
		    const user = window.REKAV_ROOT ? '<span style="color: #ff5555; font-weight: bold;">root</span>' : 'guest';
		    const promptSymbol = window.REKAV_ROOT ? '#' : '$';
		    promptLine.innerHTML = `
		        <span class="prompt">${user}</span><span style="color:#ffb86c;">@rekav</span>:~${promptSymbol} 
		        <span class="input">${escapeHtml(currentInput)}</span>
		        <span class="cursor">█</span>
		    `;
		}

		terminalBody.appendChild(promptLine);
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
		date: () => {
		    const now = new Date();

		    // Format like classic Unix `date` command
		    const days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
		    const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
		                    'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

		    const dayName = days[now.getUTCDay()];
		    const monthName = months[now.getUTCMonth()];
		    const day = String(now.getUTCDate()).padStart(2, '0');
		    const hours = String(now.getUTCHours()).padStart(2, '0');
		    const minutes = String(now.getUTCMinutes()).padStart(2, '0');
		    const seconds = String(now.getUTCSeconds()).padStart(2, '0');
		    const year = now.getUTCFullYear();

		    const formattedDate = `${dayName} ${monthName} ${day} ${hours}:${minutes}:${seconds} UTC ${year}`;

		    appendLine(`<span style="color: #8be9fd;">${formattedDate}</span>`);
		},
		root: () => {
				if (window.REKAV_ROOT) {
				    appendLine('<span style="color: #f1fa8c;">You are already root.</span>');
				    return;
				}

				passwordPromptActive = true;
				awaitingPassword = true;  // you'll reuse this flag

				// Change the live prompt to show password entry (hidden input)
				const livePrompt = terminalBody.querySelector('.live-prompt');
				if (livePrompt) {
				    livePrompt.innerHTML = `
				        <span style="color: #ff79c6;">Password for root: </span>
				        <span class="input">${'*'.repeat(currentInput.length)}</span>
				        <span class="cursor">█</span>
				    `;
				}
			},

			quit: () => {
				if (window.REKAV_ROOT) {
					// Clear in-memory flag
					window.REKAV_ROOT = false;

					// Clear persistent storage
					sessionStorage.removeItem('rekav_root_session');  // revoke across all pages
					appendLine('<span style="color: #50fa7b;">Returned to guest session. Root access revoked.</span>');
				} else {
					appendLine('<span style="color: #6272a4;">You are not root.</span>');
				}
			},

	};

	const handleCommand = (input) => {
		const trimmed = input.trim();
		if (trimmed === '') return;

		// Special handling for password entry
		if (awaitingPassword && passwordPromptActive) {
		    if (trimmed === ROOT_PASSWORD) {
		        window.REKAV_ROOT = true;
		        // Save to sessionStorage so other pages know
        		sessionStorage.setItem('rekav_root_session', 'true');
		        appendLine('<span style="color: #50fa7b;">Root access granted.</span>');
		    } else {
		        appendLine('<span style="color: #ff5555;">root: Access denied</span>');
		    }
		    awaitingPassword = false;
		    passwordPromptActive = false;
		    newPrompt(); // back to normal prompt
		    return;
		}

		const parts = trimmed.split(' ');
		const cmd = parts[0].toLowerCase();

		if (commands[cmd]) {
		    commands[cmd](parts.slice(1));
		} else {
		    appendLine(`<span style="color: #ff5555;">bash: ${escapeHtml(cmd)}: command not found</span>`);
		    appendLine(`<span style="color: #6272a4;">Did you mean something else? Try</span> <span class="hint">help</span>`);
		}

		// After normal command execution, show new prompt (unless we're waiting for password)
		if (!awaitingPassword) {
		    newPrompt();
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
            const user = window.REKAV_ROOT ? "root" : "guest";
			if (livePrompt) {
				if (passwordPromptActive && awaitingPassword) {
					// In password mode: show the password prompt as final (with stars)
					livePrompt.innerHTML = `
						<span style="color: #ff79c6;">Password for root: </span>
						<span class="input">${'*'.repeat(currentInput.length)}</span>
					`;
				} else {
					// Normal command: show what was typed
					const user = window.REKAV_ROOT ? "root" : "guest";
					const promptSymbol = window.REKAV_ROOT ? '#' : '$';
					livePrompt.innerHTML = `
						<span class="prompt">${user}</span><span style="color:#ffb86c;">@rekav</span>:~${promptSymbol} ${escapeHtml(currentInput)}
					`;
				}
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
