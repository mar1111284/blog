#include "translate.h"
#include "global.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/emscripten.h>
#endif

#ifdef __EMSCRIPTEN__
#ifdef __EMSCRIPTEN__

int poll_translate_result(void) {
    if (!_translation_pending) {
        return 0;
    }

    char buf[512];
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_translate_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) {
            console.warn("poll_translate_result: result too large for buffer");
            stringToUTF8("__TOO_LARGE__", out_ptr, maxlen);
            return;
        }

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_translate_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {

        if (strcmp(buf, "__TOO_LARGE__") == 0) {
            add_log("poll_translate_result: translation result too large", LOG_ERROR);
            return 1;
        }
		add_terminal_line("\n", LINE_FLAG_NONE);
		char line[MAX_LINE_LENGTH];
		snprintf(line, sizeof(line), "Translation: %s", buf);
		add_terminal_line(line, LINE_FLAG_SYSTEM);
		add_terminal_line("\n", LINE_FLAG_NONE);
        _translation_pending = 0;
        reset_current_input();
        return 1;
    }

    return 0;
}
#endif


#endif
