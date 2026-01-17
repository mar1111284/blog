#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base64.h"
#include "data_codec.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// List of allowed pages
char *available_pages[] = {
    "gallery",
    "random",
    "shitpost",
    "about"
};
int page_count = sizeof(available_pages) / sizeof(available_pages[0]);

ManEntry man_db[] = {
    {
        "translate",
        "--------------------------------------------------\n"
        "                   TRANSLATE                      \n"
        "--------------------------------------------------\n\n"
        "translate <source> <target> <text>\n"
        "  Sends <text> to the translator and prints the result in the terminal.\n\n"
        "Usage:\n"
        "  translate en fr Hello world\n"
        "  translate es en 'Hola mundo'\n\n"
        "Examples:\n"
        "  translate en fr Hello world\n"
        "  translate es en Goodbye!\n"
        "  translate ja en 'How are you?'\n\n"
        "Notes:\n"
        "  - Translations are handled asynchronously.\n"
        "  - Result is automatically printed in the terminal.\n"
        "  - You must specify both <source> and <target> languages.\n"
        "  - MyMemory API is used; short phrases may return unexpected results.\n"
        "  - Machine translation fallback is forced (mt=1) for reliability.\n"
        "  - Supported languages include:\n"
        "      en (English), fr (French), es (Spanish), de (German), it (Italian),\n"
        "      pt (Portuguese), nl (Dutch), ru (Russian), ja (Japanese), zh (Chinese)\n"
        "  - Use quotes for multi-word text.\n\n"
        "--------------------------------------------------\n"
        "        Type 'translate <src> <tgt> <text>'         \n"
        "--------------------------------------------------\n"
		"\n"
    },
	{
		"weather",
		"--------------------------------------------------\n"
		"                     WEATHER                       \n"
		"--------------------------------------------------\n\n"
		"weather <city>\n"
		"  Fetches the weather forecast for the specified city and displays it in the terminal.\n\n"
		"Usage:\n"
		"  weather Paris\n"
		"  weather Sydney\n\n"
		"Available Cities:\n"
		"  Paris, London, New_York, Tokyo, Sydney, Moscow, Berlin, Toronto, Rio, Cape_Town\n\n"
		"Examples:\n"
		"  weather Paris\n"
		"  weather Tokyo\n\n"
		"Notes:\n"
		"  - The command queries the Open-Meteo API asynchronously.\n"
		"  - The result includes a global info block (latitude, longitude, elevation, timezone)\n"
		"    and hourly forecast.\n"
		"  - Forecast data includes:\n"
		"      * UV index (hourly)\n"
		"      * Precipitation (hourly, mm)\n"
		"      * Temperature (hourly, if enabled)\n"
		"  - Results are automatically printed in the terminal once received.\n"
		"  - Only predefined common cities are supported; type 'weather' without arguments\n"
		"    to see the list.\n"
		"  - Graphs for UV and Temperature are displayed in ASCII format for easy reading.\n\n"
		"--------------------------------------------------\n"
		"              Type 'weather <city>'               \n"
		"--------------------------------------------------\n"
		"\n"
	},
	{
		"to_ascii",
		"--------------------------------------------------\n"
		"                   TO_ASCII                       \n"
		"--------------------------------------------------\n\n"
		"to_ascii <link> [options]\n"
		"  Converts an online image into ASCII art.\n"
		"  By default, it displays the ASCII preview in the terminal.\n"
		"  Optional flags allow exporting to a high-resolution PNG.\n\n"
		"Usage:\n"
		"  to_ascii <image_url> [options]\n\n"
		"Options:\n"
		"  download=1       Export the ASCII art as a high-resolution PNG.\n"
		"  wide=<number>    Number of characters per line (ASCII width). Default: 130\n"
		"  font_size=<num>  Font size for the exported PNG. Default: 6\n"
		"  bg=<color>       Background color for PNG. Options: black, white, red, green, blue, pink, purple. Default: black\n"
		"  color=<color>    Font color for PNG. Options same as bg. Default: white\n"
		"  name=<filename>  Output PNG file name. Default: ascii_highres.png\n\n"
		"Examples:\n"
		"  to_ascii https://i.imgur.com/example.jpg\n"
		"  to_ascii https://picsum.photos/800/600 download=1 wide=300 font_size=15 bg=pink color=purple name=output.png\n\n"
		"Notes:\n"
		"  - Copy/paste of images is not supported.\n"
		"  - Images must be accessible via direct URL.\n"
		"  - Some websites block image access due to CORS restrictions.\n"
		"  - Working sources usually include Imgur, Picsum, Wikimedia.\n\n"
		"Image Tips:\n"
		"  - Use small to medium images for faster processing.\n"
		"  - High-contrast photos give the best results.\n"
		"  - Avoid flat colors or very dark images.\n"
		"  - Increasing 'wide' and 'font_size' improves export resolution.\n\n"
		"--------------------------------------------------\n"
		"          Type 'to_ascii <image_url> [options]'   \n"
		"--------------------------------------------------\n"
		"\n"
	},

};
int man_db_size = sizeof(man_db) / sizeof(man_db[0]);

City cities[] = {
    {"Paris", 48.8566, 2.3522},
    {"London", 51.5074, -0.1278},
    {"New_York", 40.7128, -74.0060},
    {"Tokyo", 35.6895, 139.6917},
    {"Sydney", -33.8688, 151.2093},
    {"Moscow", 55.7558, 37.6173},
    {"Berlin", 52.52, 13.4050},
    {"Toronto", 43.6532, -79.3832},
    {"Rio", -22.9068, -43.1729},
    {"Cape_Town", -33.9249, 18.4241}
};
int city_count = sizeof(cities) / sizeof(cities[0]);

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



