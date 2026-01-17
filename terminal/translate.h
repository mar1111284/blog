#ifndef TRANSLATE_H
#define TRANSLATE_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// State
extern int translation_pending;

int poll_translate_result(void);

#endif /* TRANSLATE_H */
