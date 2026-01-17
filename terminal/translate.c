#include "translate.h"
#include "global.h"

#ifdef __EMSCRIPTEN__
int poll_translate_result(void) {
    if (!translation_pending) return 0;

    // small internal buffer
    char buf[512];
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_translate_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) return;

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_translate_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {
        translation_pending = 0;
        //add_line(buf);  // prints automatically
        return 1;
    }

    return 0;
}
#endif
