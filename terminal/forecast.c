#include "forecast.h"
#include "global.h"


#ifdef __EMSCRIPTEN__
int poll_forecast_result(void) {
    if (!weather_forecast_pending) return 0;

    // Internal buffer for JS → C string
    char buf[8192];  // big enough for multi-line text
    buf[0] = '\0';

    EM_ASM({
        const out_ptr = $0;
        const maxlen = $1;

        const res = sessionStorage.getItem("rekav_weather_result");
        if (!res) return;

        const len = lengthBytesUTF8(res) + 1;
        if (len > maxlen) return;

        stringToUTF8(res, out_ptr, maxlen);
        sessionStorage.removeItem("rekav_weather_result");
    }, buf, sizeof(buf));

    if (buf[0] != '\0') {
        weather_forecast_pending = 0;

        // Split by '\n' and print line by line
        char *line = buf;
        while (*line) {
            char *next = strchr(line, '\n');
            if (!next) next = line + strlen(line);

            char tmp[512];
            int len = next - line;
            if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
            memcpy(tmp, line, len);
            tmp[len] = '\0';

            // Print each line to terminal
            //add_line(tmp);

            line = (*next) ? next + 1 : next;
        }

        //add_line(""); // extra space after forecast
        return 1;
    }

    return 0;
}
#endif
