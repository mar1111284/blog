#include "forecast.h"
#include "global.h"


int poll_forecast_result(void) {
    if (!_weather_forecast_pending) return 0;

    char buf[8192];
    buf[0] = '\0';

	#ifdef __EMSCRIPTEN__
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
	#endif

    if (buf[0] != '\0') {
        char *line = buf;
        while (*line) {
            char *next = strchr(line, '\n');
            if (!next) next = line + strlen(line);
            char tmp[512];
            int len = next - line;
            if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
            memcpy(tmp, line, len);
            tmp[len] = '\0';
            add_terminal_line(tmp, LINE_FLAG_NONE);
            line = (*next) ? next + 1 : next;
        }
        add_terminal_line("", LINE_FLAG_NONE);
        _weather_forecast_pending = 0;
        reset_current_input();
        return 1;
    }

    return 0;
}

