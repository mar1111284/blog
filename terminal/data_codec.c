#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base64.h"
#include "data_codec.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// ---- Base64 encoded JSON preview data ----
const char *data_encoded =
	"WyAgCiAgeyAKICAgICJ0aXRsZSI6ICJObyBFc2NhcGUsIEp1c3QgRnVybml0dXJlIiwK"
	"ICAgICJkYXRlIjogIjIwMjYtMDEtMDMiLAogICAgImNvbnRlbnQiOiAiTGVhdmUgdG9v"
	"IHNvb24gYW5kIHN0YXkgbXlzdGVyaW91cywgc3RheSB0b28gbG9uZyBhbmQgYmVjb21l"
	"IHBhcnQgb2YgdGhlIHVwaG9sc3RlcnkuIiwKICAgICJpbWFnZUluZGV4IjogMSwKICAg"
	"ICJhcnRpY2xlUGF0aCI6ICJwYWdlcy9hcnRpY2xlX3Rlc3QuaHRtbCIKICB9LAogIHsK"
	"ICAgICJ0aXRsZSI6ICJTY2hyw7ZkaW5nZXLDpXMgQXdrd2FyZCBSb21hbmNlIiwKICAg"
	"ICJkYXRlIjogIjIwMjYtMDEtMDQiLAogICAgImNvbnRlbnQiOiAiV2XigJlyZSBib3Ro"
	"IGRhdGluZyBhbmQgZ2hvc3RpbmcgdW50aWwgc29tZW9uZSBvcGVucyB0aGUgY2hhdC4g"
	"U3BvaWxlcjogRG9u4oCZdCBvcGVuIGl0LiIsCiAgICAiaW1hZ2VJbmRleCI6IDIKICB9"
	"LAogIHsKICAgICJ0aXRsZSI6ICJFbW90aW9uYWwgSGFuZ292ZXIiLAogICAgImRhdGUi"
	"OiAiMjAyNi0wMS0wNSIsCiAgICAiY29udGVudCI6ICJPdXQgb2Ygc2lnaHQ/IE1vcmUg"
	"bGlrZSBteSBicmFpbiByZXJ1bnMgZXZlcnkgcG9zc2libGUgY29udmVyc2F0aW9uIGlu"
	"IEhEIGRvb20gbW9kZS4iLAogICAgImltYWdlSW5kZXgiOiAzCiAgfSwKICB7CiAgICAi"
	"dGl0bGUiOiAiMyBBTSBCcmFpbiBVcGRhdGUiLAogICAgImRhdGUiOiAiMjAyNi0wMS0w"
	"NyIsCiAgICAiY29udGVudCI6ICJSYW5kb20gMjAxMiBlbWJhcnJhc3NtZW50IHRyaWdn"
	"ZXJlZCBhIHN5c3RlbSB1cGdyYWRlOiBjcmluZ2UgdjIuMCBpbnN0YWxsZWQuIiwKICAg"
	"ICJpbWFnZUluZGV4IjogNAogIH0KXQ==";
	
// ---- Fake Articles in HTML, Base64 Encoded ----

// Article 0
const char *article_0 =
    "PGgxPlRpdGxlIG9mIEFydGljbGUgMDA8L2gxPg0KPHA+VGhpcyBpcyBhIHNhbXBsZSBhcnRpY2xlIGZvciB0ZXN0aW5nIHJlYWxseSBsb25nIHRleHQuIFRoaXMgY29udGVudCBpcyBkdW1teSBidXQgaXQgaGFzIG11bHRpcGxlIHBhcmFncmFwaHMuPC9wPg0KPHA+U29tZSBtb3JlIHRleHQgdG8gc2ltdWxhdGUgdGhlIGxvbmcgdGV4dC4gUGFyYWdyYXBoIGZvdXIgYmFycyBhbmQgc2FtcGxlIGNvbnRlbnQuPC9wPg0KPGgyPkFuIG90aGVyIFNlY3Rpb248L2gyPg0KPHA+VGhpcyBwYXJhZ3JhcGggYWRkaXRpb25hbCBjb250ZW50IHRvIHNpY21pdWxhdGUgdGhpcyBhcnRpY2xlLiBBbiBpbWFnZSBvciBvdGhlciBlbGVtZW50IGlzIG5vdCBhY3R1YWwgYnV0IG1vY2sgdGV4dC4gVGhpcyBzaG93cyB3aGF0IHRoZSBhcnRpY2xlIG1heSBoYW5kbGUgYnVzaW5lc3MgYW5kIGZvcm1hdCBsb2dpYy4gPC9wPg==";

// Article 1
const char *article_1 =
    "PGgxPkZha2UgQXJ0aWNsZSAxPC9oMT4NCjxwPkFub3RoZXIgc2FtcGxlIGFydGljbGUgd2l0aCBtb3JlIHRleHQgdG8gc2hvdyBkaWZmZXJlbnQgcGFyYWdyYXBoIGxlbmd0aHMuIFRoaXMgaXMgZm9yIHRlc3Rpbmcgd29ya2Zsb3cgd2l0aCBzdHlsZXMuPC9wPg0KPHA+TW9yZSBwYXJhZ3JhcGhzLCBtdWx0aXBsZSBwYXJhZ3JhcGhzIHRvIHNpbXVsYXRlIGxvbmcgdGV4dCBjb250ZW50LiBQcmV0ZW5kIGZvciBhIGxvbmcgc2FtcGxlIGFyY2hpdmUgYmFzZSBvbiB0aGUgcGxhY2Ugd2l0aCBkYXRhLg0KPC9wPg0KPGgyPkNvbXBsZXggU2VjdGlvbjwvaDI+DQogPHA+VGhpcyBvbmUgaXMgbWVhbnQgdG8gdGVzdCBiYXNlNjQgZGVjb2RpbmcuIFRoaXMgaXMgdXNlZnVsIGZvciB3b3JraW5nIHdpdGggV0FTTSB0byBwaXBlIGRhdGEgdGhyb3VnaCBhcnRpY2xlcy48L3A+DQo=";

// Article 2
const char *article_2 =
    "PGgxPkZha2UgQXJ0aWNsZSAyPC9oMT4NCjxwPkZpbmFsIHNhbXBsZSBhcnRpY2xlLCBsaWtlIGEgd2Vic2l0ZSBhcnRpY2xlLg0KVGhpcyBvbmUgaXMgbG9uZ2VyIHRoYW4gdGhlIG90aGVycywgY2FwdHVyaW5nIG11bHRpcGxlIGV2ZW50cyBhbmQgcGFyYWdyYXBoIHJlZmVyZW5jZXMuPC9wPg0KPHA+VGhpcyBhcnRpY2xlIG5lZWRzIHRvIGJlIHJlYWQgd2l0aCB0aGUgd2Vic2l0ZSBvciBtb2NrIHNhbXBsZSBjb250ZW50LiBQYXJhZ3JhcGggYW5kIGRldGFpbGVkIGxpc3RzIHdpdGggc3RydWN0dXJlIGFyZSBpbmNsdWRlZC48L3A+DQo=";



// ---- Decode a base64 string and push to sessionStorage ----
void push_base64_to_storage(const char *b64data, const char *storage_key) {
    if (!b64data || !storage_key) return;

    size_t encoded_len = strlen(b64data);
    size_t max_decoded = (encoded_len * 3) / 4 + 4;

    unsigned char *decoded = malloc(max_decoded);
    if (!decoded) return;

    int decoded_len = base64_decode(b64data, decoded, max_decoded);
    if (decoded_len <= 0) {
        free(decoded);
        return;
    }

    decoded[decoded_len] = '\0'; // null-terminate to make JS-safe string

#ifdef __EMSCRIPTEN__
    EM_ASM({
        const key = UTF8ToString($1);
        const value = UTF8ToString($0);
        sessionStorage.setItem(key, value);
        console.log("SessionStorage set:", key, value);
    }, decoded, storage_key);
#endif

    free(decoded);
}



