#ifndef FORECAST_H
#define FORECAST_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// State
extern int weather_forecast_pending;

int poll_forecast_result(void);


#endif /* FORECAST_H */

