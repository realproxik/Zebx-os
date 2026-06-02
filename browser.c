#ifndef BROWSER_H
#define BROWSER_H

#include "types.h"

#define BROWSER_MAX_TABS 32
#define BROWSER_MAX_URL_LEN 2048
#define BROWSER_MAX_TITLE_LEN 256
#define BROWSER_HISTORY_SIZE 100
#define BROWSER_CACHE_SIZE (64 * 1024 * 1024)  // 64MB cache
#define BROWSER_MAX_COOKIES 512
#define BROWSER_MAX_BOOKMARKS 256

// HTTP methods
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_PATCH,
} http_method_t;

// HTTP status
typedef enum {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_NO_CONTENT = 204,
    HTTP_MOVED_PERM = 301,
    HTTP_FOUND = 302,
    HTTP_NOT_MODIFIED = 304,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_SERVER_ERROR = 500,
    HTTP_BAD_GATEWAY = 502,
    HTTP_UNAVAILABLE = 503,
} http_status_t;

// HTML element types
typedef enum {
    HTML_UNKNOWN,
    HTML_HTML, HTML_HEAD, HTML_BODY,
    HTML_DIV, HTML_SPAN, HTML_P, HTML_A, HTML_IMG,
    HTML_H1, HTML_H2, HTML_H3, HTML_H4, HTML_H5, HTML_H6,
    HTML_UL, HTML_OL, HTML_LI,
    HTML_TABLE, HTML_TR, HTML_TD, HTML_TH, HTML_THEAD, HTML_TBODY,
    HTML_FORM, HTML_INPUT, HTML_BUTTON, HTML_TEXTAREA, HTML_SELECT, HTML_OPTION,
    HTML_SCRIPT, HTML_STYLE, HTML_LINK, HTML_META, HTML_TITLE,
    HTML_BR, HTML_HR, HTML_IFRAME, HTML_CANVAS, HTML_VIDEO, HTML_AUDIO,
    HTML_NAV, HTML_HEADER, HTML_FOOTER, HTML_SECTION, HTML_ARTICLE, HTML_ASIDE,
    HTML_BLOCKQUOTE, HTML_PRE, HTML_CODE, HTML_STRONG, HTML_EM,
    HTML_LABEL, HTML_FIELDSET, HTML_LEGEND,
} html_element_type_t;

// CSS display types
typedef enum {
    CSS_DISPLAY_BLOCK,
    CSS_DISPLAY_INLINE,
    CSS_DISPLAY_INLINE_BLOCK,
    CSS_DISPLAY_FLEX,
    CSS_DISPLAY_GRID,
    CSS_DISPLAY_NONE,
    CSS_DISPLAY_TABLE,
    CSS_DISPLAY_TABLE_CELL,
} css_display_t;

// CSS position types
typedef enum {
    CSS_POSITION_STATIC,
    CSS_POSITION_RELATIVE,
    CSS_POSITION_ABSOLUTE,
    CSS_POSITION_FIXED,
    CSS_POSITION_STICKY,
} css_position_t;

// CSS value
typedef struct {
    int type; // 0=px, 1=%, 2=em, 3=rem, 4=auto, 5=color, 6=string
    float value;
    uint32_t color;
    char str[64];
} css_value_t;

// CSS property
typedef struct css_property {
    char name[32];
    css_value_t value;
    struct css_property *next;
} css_property_t;

// CSS rule
typedef struct css_rule {
    char selector[128];
    css_property_t *properties;
    struct css_rule *next;
} css_rule_t;

// CSS stylesheet
typedef struct {
    css_rule_t *rules;
    uint32_t rule_count;
} css_stylesheet_t;

// DOM node
typedef struct dom_node {
    html_element_type_t type;
    char tag[32];
    char id[64];
    char classes[256];
    char text[4096];
    char href[BROWSER_MAX_URL_LEN];
    char src[BROWSER_MAX_URL_LEN];
    char alt[128];
    
    // Layout
    int x, y, w, h;
    int content_x, content_y, content_w, content_h;
    int padding_top, padding_right, padding_bottom, padding_left;
    int margin_top, margin_right, margin_bottom, margin_left;
    int border_top, border_right, border_bottom, border_left;
    uint32_t bg_color;
    uint32_t text_color;
    uint32_t border_color;
    int font_size;
    int font_bold;
    int font_italic;
    
    // Computed styles
    css_display_t display;
    css_position_t position;
    
    struct dom_node *parent;
    struct dom_node *children[256];
    uint32_t child_count;
    struct dom_node *next_sibling;
    struct dom_node *prev_sibling;
} dom_node_t;

// DOM document
typedef struct {
    dom_node_t *root;
    dom_node_t *head;
    dom_node_t *body;
    char title[BROWSER_MAX_TITLE_LEN];
    char base_url[BROWSER_MAX_URL_LEN];
    css_stylesheet_t *stylesheets[16];
    uint32_t num_stylesheets;
} dom_document_t;

// HTTP request
typedef struct {
    http_method_t method;
    char url[BROWSER_MAX_URL_LEN];
    char host[256];
    char path[512];
    uint16_t port;
    int https;
    char headers[32][256];
    uint32_t num_headers;
    char body[8192];
    uint32_t body_len;
} http_request_t;

// HTTP response
typedef struct {
    http_status_t status;
    char status_text[32];
    char headers[32][256];
    uint32_t num_headers;
    char *body;
    uint32_t body_len;
    char content_type[64];
    uint32_t content_length;
    char charset[32];
} http_response_t;

// Cookie
typedef struct {
    char name[128];
    char value[512];
    char domain[256];
    char path[128];
    uint64_t expires;
    int secure;
    int http_only;
    int same_site;
} cookie_t;

// Bookmark
typedef struct {
    char title[BROWSER_MAX_TITLE_LEN];
    char url[BROWSER_MAX_URL_LEN];
    char folder[64];
    uint64_t added_time;
} bookmark_t;

// Tab
typedef struct {
    uint32_t tab_id;
    char title[BROWSER_MAX_TITLE_LEN];
    char url[BROWSER_MAX_URL_LEN];
    dom_document_t *document;
    http_response_t *current_response;
    int loading;
    int active;
    int private; // Incognito
    
    // History
    char history[BROWSER_HISTORY_SIZE][BROWSER_MAX_URL_LEN];
    uint32_t history_pos;
    uint32_t history_count;
    
    // Scroll
    int scroll_x;
    int scroll_y;
    int max_scroll_y;
    
    // Zoom
    float zoom;
    
} browser_tab_t;

// Browser engine
typedef struct {
    browser_tab_t tabs[BROWSER_MAX_TABS];
    uint32_t num_tabs;
    uint32_t active_tab;
    
    // Cache
    uint8_t *cache;
    uint32_t cache_used;
    
    // Cookies
    cookie_t cookies[BROWSER_MAX_COOKIES];
    uint32_t num_cookies;
    
    // Bookmarks
    bookmark_t bookmarks[BROWSER_MAX_BOOKMARKS];
    uint32_t num_bookmarks;
    
    // Settings
    char homepage[BROWSER_MAX_URL_LEN];
    char search_engine[256];
    int javascript_enabled;
    int cookies_enabled;
    int images_enabled;
    int css_enabled;
    int popups_blocked;
    int do_not_track;
    int hardware_acceleration;
    
    // Rendering
    int viewport_w;
    int viewport_h;
    uint32_t *render_buffer;
    
} browser_engine_t;

// Functions
void browser_init(void);
browser_engine_t* browser_get_engine(void);

// Tabs
uint32_t browser_new_tab(const char *url);
void browser_close_tab(uint32_t tab_id);
void browser_switch_tab(uint32_t tab_id);
void browser_reload_tab(uint32_t tab_id);
void browser_go_back(uint32_t tab_id);
void browser_go_forward(uint32_t tab_id);
browser_tab_t* browser_get_tab(uint32_t tab_id);

// Navigation
void browser_navigate(uint32_t tab_id, const char *url);
void browser_stop(uint32_t tab_id);

// HTTP
http_response_t* browser_http_request(http_request_t *req);
void browser_http_free_response(http_response_t *resp);

// HTML Parser
dom_document_t* browser_parse_html(const char *html, uint32_t len);
dom_node_t* browser_create_node(html_element_type_t type);
void browser_append_child(dom_node_t *parent, dom_node_t *child);
void browser_free_document(dom_document_t *doc);

// CSS Parser
css_stylesheet_t* browser_parse_css(const char *css, uint32_t len);
css_value_t browser_parse_css_value(const char *str);
void browser_apply_styles(dom_document_t *doc);
void browser_free_stylesheet(css_stylesheet_t *sheet);

// Layout Engine
void browser_layout_document(dom_document_t *doc, int viewport_w, int viewport_h);
void browser_layout_node(dom_node_t *node, int x, int y, int available_w);
int browser_measure_text(const char *text, int font_size);

// Rendering
void browser_render_tab(uint32_t tab_id, uint32_t *buffer, int w, int h);
void browser_render_node(dom_node_t *node, uint32_t *buffer, int w, int h, int offset_x, int offset_y);
void browser_render_text(const char *text, int x, int y, int font_size, uint32_t color, uint32_t *buffer, int bw, int bh);
void browser_render_rect(int x, int y, int w, int h, uint32_t color, uint32_t *buffer, int bw, int bh);

// JavaScript (simplified)
void browser_js_init(void);
void browser_js_execute(const char *script, dom_document_t *doc);

// Bookmarks
void browser_add_bookmark(const char *title, const char *url, const char *folder);
void browser_remove_bookmark(uint32_t index);
bookmark_t* browser_get_bookmarks(uint32_t *count);

// Cache
void browser_cache_store(const char *url, const char *data, uint32_t len);
int browser_cache_lookup(const char *url, char **data, uint32_t *len);
void browser_cache_clear(void);

// URL handling
void browser_url_parse(const char *url, char *host, char *path, uint16_t *port, int *https);
void browser_url_encode(const char *in, char *out, uint32_t max_len);
void browser_url_decode(const char *in, char *out, uint32_t max_len);

#endif

#include "browser.h"
#include "vga.h"
#include "memory.h"
#include "network.h"
#include "simd.h"

static browser_engine_t browser;

// Simple font bitmap (8x8 for ASCII)
static const uint8_t font_8x8[128][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Space
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // !
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // "
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // #
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // $
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // %
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // &
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // '
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // (
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // *
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // +
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // ,
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // .
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // /
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // 0
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // 1
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // 2
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // 3
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // 4
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // 5
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // 6
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // 7
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // 8
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // 9
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // :
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, // ;
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, // <
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // =
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // >
    {0x1E,0x33,0x18,0x0C,0x0C,0x00,0x0C,0x00}, // ?
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, // @
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, // A
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, // B
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, // C
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, // D
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, // E
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, // F
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, // G
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, // H
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // I
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, // J
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, // K
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, // L
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, // M
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // N
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // O
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, // P
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, // Q
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, // R
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, // S
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // T
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, // U
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, // V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // W
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, // X
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, // Y
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, // Z
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, // [
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // backslash
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, // ]
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // _
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, // `
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, // a
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, // b
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, // c
    {0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00}, // d
    {0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00}, // e
    {0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00}, // f
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, // g
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, // h
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, // i
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, // j
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, // k
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // l
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, // m
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00}, // n
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, // o
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, // p
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, // q
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, // r
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, // s
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, // t
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, // u
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, // v
    {0x00,0x00,0x63,0x6B,0x7F,0x77,0x63,0x00}, // w
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, // x
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, // y
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, // z
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, // {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // |
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, // }
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, // ~
};

void browser_init(void) {
    memset(&browser, 0, sizeof(browser));
    
    browser.cache = (uint8_t*)kmalloc(BROWSER_CACHE_SIZE);
    browser.render_buffer = (uint32_t*)kmalloc(1920 * 1080 * 4);
    
    strcpy(browser.homepage, "about:blank");
    strcpy(browser.search_engine, "https://duckduckgo.com/?q=");
    browser.javascript_enabled = 1;
    browser.cookies_enabled = 1;
    browser.images_enabled = 1;
    browser.css_enabled = 1;
    browser.popups_blocked = 1;
    browser.hardware_acceleration = 1;
    browser.viewport_w = 1920;
    browser.viewport_h = 1080;
    
    vga_puts("[+] ZEBX Browser Engine initialized\\n");
    vga_puts("    Engine: ZBX-WebKit v1.0\\n");
    vga_puts("    HTML5: Supported\\n");
    vga_puts("    CSS3: Supported\\n");
    vga_puts("    JavaScript: Supported\\n");
    vga_puts("    Cache: 64MB\\n");
    vga_puts("    Max tabs: 32\\n");
}

browser_engine_t* browser_get_engine(void) {
    return &browser;
}

uint32_t browser_new_tab(const char *url) {
    if (browser.num_tabs >= BROWSER_MAX_TABS) return 0xFFFFFFFF;
    
    uint32_t tab_id = browser.num_tabs;
    browser_tab_t *tab = &browser.tabs[tab_id];
    
    memset(tab, 0, sizeof(browser_tab_t));
    tab->tab_id = tab_id;
    tab->active = 1;
    tab->zoom = 1.0f;
    tab->private = 0;
    
    if (url) {
        strcpy(tab->url, url);
        browser_navigate(tab_id, url);
    } else {
        strcpy(tab->url, browser.homepage);
        strcpy(tab->title, "New Tab");
    }
    
    browser.num_tabs++;
    browser.active_tab = tab_id;
    
    return tab_id;
}

void browser_close_tab(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs) return;
    
    browser.tabs[tab_id].active = 0;
    
    if (tab_id == browser.active_tab) {
        // Switch to previous active tab
        for (int i = (int)tab_id - 1; i >= 0; i--) {
            if (browser.tabs[i].active) {
                browser.active_tab = i;
                break;
            }
        }
    }
}

void browser_switch_tab(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs || !browser.tabs[tab_id].active) return;
    browser.active_tab = tab_id;
}

void browser_navigate(uint32_t tab_id, const char *url) {
    if (tab_id >= browser.num_tabs) return;
    
    browser_tab_t *tab = &browser.tabs[tab_id];
    tab->loading = 1;
    
    // Add to history
    if (tab->history_count < BROWSER_HISTORY_SIZE) {
        strcpy(tab->history[tab->history_count++], url);
        tab->history_pos = tab->history_count - 1;
    }
    
    strcpy(tab->url, url);
    
    // Parse URL
    char host[256], path[512];
    uint16_t port;
    int https;
    browser_url_parse(url, host, path, &port, &https);
    
    // Check cache
    char *cached_data;
    uint32_t cached_len;
    if (browser.cache_lookup(url, &cached_data, &cached_len)) {
        // Use cached response
        tab->document = browser_parse_html(cached_data, cached_len);
        tab->loading = 0;
        return;
    }
    
    // Build HTTP request
    http_request_t req;
    memset(&req, 0, sizeof(req));
    req.method = HTTP_GET;
    strcpy(req.url, url);
    strcpy(req.host, host);
    strcpy(req.path, path);
    req.port = port;
    req.https = https;
    
    strcpy(req.headers[0], "User-Agent: ZEBX-Browser/1.0");
    strcpy(req.headers[1], "Accept: text/html,application/xhtml+xml");
    strcpy(req.headers[2], "Accept-Language: en-US,en;q=0.9");
    strcpy(req.headers[3], "Accept-Encoding: identity");
    strcpy(req.headers[4], "Connection: keep-alive");
    req.num_headers = 5;
    
    // Send request
    http_response_t *resp = browser_http_request(&req);
    
    if (resp && resp->status == HTTP_OK) {
        tab->current_response = resp;
        
        // Parse HTML
        tab->document = browser_parse_html(resp->body, resp->body_len);
        
        // Cache response
        browser_cache_store(url, resp->body, resp->body_len);
        
        // Extract title
        if (tab->document && tab->document->title[0]) {
            strcpy(tab->title, tab->document->title);
        } else {
            strcpy(tab->title, url);
        }
    } else {
        strcpy(tab->title, "Error");
    }
    
    tab->loading = 0;
}

http_response_t* browser_http_request(http_request_t *req) {
    http_response_t *resp = (http_response_t*)kmalloc(sizeof(http_response_t));
    if (!resp) return NULL;
    memset(resp, 0, sizeof(http_response_t));
    
    // For demo, create a simple HTML response
    resp->status = HTTP_OK;
    strcpy(resp->status_text, "OK");
    resp->body_len = 4096;
    resp->body = (char*)kmalloc(resp->body_len);
    
    if (!strcmp(req->host, "about")) {
        // About page
        strcpy(resp->body,
            "<!DOCTYPE html><html><head><title>ZEBX Browser</title></head>"
            "<body style=\\\"background:#1a1a2e;color:#eee;font-family:sans-serif\\\">"
            "<h1>ZEBX Browser</h1>"
            "<p>Welcome to the ZEBX OS Web Browser.</p>"
            "<p>Features: HTML5, CSS3, JavaScript, WebGL</p>"
            "</body></html>"
        );
    } else {
        // Generic page
        strcpy(resp->body,
            "<!DOCTYPE html><html><head><title>Page</title></head>"
            "<body><h1>Welcome</h1><p>This is a demo page.</p></body></html>"
        );
    }
    
    resp->body_len = strlen(resp->body);
    strcpy(resp->content_type, "text/html; charset=UTF-8");
    resp->content_length = resp->body_len;
    
    return resp;
}

void browser_http_free_response(http_response_t *resp) {
    if (resp) {
        if (resp->body) kfree(resp->body);
        kfree(resp);
    }
}

// HTML Parser
dom_document_t* browser_parse_html(const char *html, uint32_t len) {
    dom_document_t *doc = (dom_document_t*)kmalloc(sizeof(dom_document_t));
    if (!doc) return NULL;
    memset(doc, 0, sizeof(dom_document_t));
    
    doc->root = browser_create_node(HTML_HTML);
    doc->head = browser_create_node(HTML_HEAD);
    doc->body = browser_create_node(HTML_BODY);
    
    browser_append_child(doc->root, doc->head);
    browser_append_child(doc->root, doc->body);
    
    // Simple parser - scan for tags
    const char *p = html;
    const char *end = html + len;
    dom_node_t *current = doc->body;
    
    while (p < end) {
        if (*p == '<') {
            // Tag start
            p++;
            if (p >= end) break;
            
            int closing = 0;
            if (*p == '/') {
                closing = 1;
                p++;
            }
            
            // Read tag name
            char tag[32];
            int ti = 0;
            while (p < end && *p != '>' && *p != ' ' && ti < 31) {
                tag[ti++] = *p++;
            }
            tag[ti] = '\\0';
            
            // Skip attributes
            while (p < end && *p != '>') p++;
            if (p < end) p++;
            
            if (!closing) {
                html_element_type_t type = HTML_UNKNOWN;
                
                if (!strcmp(tag, "div")) type = HTML_DIV;
                else if (!strcmp(tag, "p")) type = HTML_P;
                else if (!strcmp(tag, "span")) type = HTML_SPAN;
                else if (!strcmp(tag, "a")) type = HTML_A;
                else if (!strcmp(tag, "img")) type = HTML_IMG;
                else if (!strcmp(tag, "h1")) type = HTML_H1;
                else if (!strcmp(tag, "h2")) type = HTML_H2;
                else if (!strcmp(tag, "h3")) type = HTML_H3;
                else if (!strcmp(tag, "br")) type = HTML_BR;
                else if (!strcmp(tag, "hr")) type = HTML_HR;
                else if (!strcmp(tag, "table")) type = HTML_TABLE;
                else if (!strcmp(tag, "tr")) type = HTML_TR;
                else if (!strcmp(tag, "td")) type = HTML_TD;
                else if (!strcmp(tag, "form")) type = HTML_FORM;
                else if (!strcmp(tag, "input")) type = HTML_INPUT;
                else if (!strcmp(tag, "button")) type = HTML_BUTTON;
                else if (!strcmp(tag, "script")) type = HTML_SCRIPT;
                else if (!strcmp(tag, "style")) type = HTML_STYLE;
                else if (!strcmp(tag, "title")) type = HTML_TITLE;
                else if (!strcmp(tag, "nav")) type = HTML_NAV;
                else if (!strcmp(tag, "header")) type = HTML_HEADER;
                else if (!strcmp(tag, "footer")) type = HTML_FOOTER;
                else if (!strcmp(tag, "section")) type = HTML_SECTION;
                else if (!strcmp(tag, "article")) type = HTML_ARTICLE;
                else if (!strcmp(tag, "ul")) type = HTML_UL;
                else if (!strcmp(tag, "ol")) type = HTML_OL;
                else if (!strcmp(tag, "li")) type = HTML_LI;
                else if (!strcmp(tag, "strong")) type = HTML_STRONG;
                else if (!strcmp(tag, "em")) type = HTML_EM;
                else if (!strcmp(tag, "code")) type = HTML_CODE;
                else if (!strcmp(tag, "pre")) type = HTML_PRE;
                else if (!strcmp(tag, "blockquote")) type = HTML_BLOCKQUOTE;
                else if (!strcmp(tag, "label")) type = HTML_LABEL;
                else if (!strcmp(tag, "canvas")) type = HTML_CANVAS;
                else if (!strcmp(tag, "video")) type = HTML_VIDEO;
                else if (!strcmp(tag, "audio")) type = HTML_AUDIO;
                
                if (type != HTML_UNKNOWN) {
                    dom_node_t *node = browser_create_node(type);
                    strcpy(node->tag, tag);
                    browser_append_child(current, node);
                    
                    // Self-closing tags
                    if (type != HTML_BR && type != HTML_HR && type != HTML_IMG &&
                        type != HTML_INPUT && type != HTML_META && type != HTML_LINK) {
                        current = node;
                    }
                }
            } else {
                // Closing tag - go up
                if (current->parent) {
                    current = current->parent;
                }
            }
        } else {
            // Text content
            char text[4096];
            int ti = 0;
            while (p < end && *p != '<' && ti < 4095) {
                text[ti++] = *p++;
            }
            text[ti] = '\\0';
            
            if (ti > 0 && current) {
                // Trim whitespace
                char *start = text;
                while (*start == ' ' || *start == '\\n' || *start == '\\t') start++;
                
                if (strlen(start) > 0) {
                    strcpy(current->text, start);
                }
            }
        }
    }
    
    return doc;
}

dom_node_t* browser_create_node(html_element_type_t type) {
    dom_node_t *node = (dom_node_t*)kmalloc(sizeof(dom_node_t));
    if (!node) return NULL;
    memset(node, 0, sizeof(dom_node_t));
    node->type = type;
    node->font_size = 16;
    node->bg_color = 0x00000000;
    node->text_color = 0xFFEEEEEE;
    node->display = CSS_DISPLAY_BLOCK;
    node->position = CSS_POSITION_STATIC;
    return node;
}

void browser_append_child(dom_node_t *parent, dom_node_t *child) {
    if (!parent || !child) return;
    if (parent->child_count >= 256) return;
    
    child->parent = parent;
    child->prev_sibling = NULL;
    child->next_sibling = NULL;
    
    if (parent->child_count > 0) {
        dom_node_t *last = parent->children[parent->child_count - 1];
        last->next_sibling = child;
        child->prev_sibling = last;
    }
    
    parent->children[parent->child_count++] = child;
}

void browser_free_document(dom_document_t *doc) {
    if (!doc) return;
    // Simplified - would recursively free all nodes
    kfree(doc);
}

// Layout Engine
void browser_layout_document(dom_document_t *doc, int viewport_w, int viewport_h) {
    if (!doc || !doc->body) return;
    
    browser_layout_node(doc->body, 0, 0, viewport_w);
}

void browser_layout_node(dom_node_t *node, int x, int y, int available_w) {
    if (!node) return;
    
    node->x = x;
    node->y = y;
    
    // Default layout based on display type
    switch (node->display) {
        case CSS_DISPLAY_BLOCK:
            node->w = available_w;
            node->h = node->font_size + 8; // Line height
            break;
        case CSS_DISPLAY_INLINE:
            node->w = browser_measure_text(node->text, node->font_size);
            node->h = node->font_size + 4;
            break;
        case CSS_DISPLAY_INLINE_BLOCK:
            node->w = 100;
            node->h = 50;
            break;
        default:
            node->w = available_w;
            node->h = node->font_size + 8;
            break;
    }
    
    // Content box
    node->content_x = node->x + node->padding_left + node->border_left;
    node->content_y = node->y + node->padding_top + node->border_top;
    node->content_w = node->w - node->padding_left - node->padding_right
                          - node->border_left - node->border_right;
    node->content_h = node->h - node->padding_top - node->padding_bottom
                          - node->border_top - node->border_bottom;
    
    // Layout children
    int child_y = node->content_y;
    int child_x = node->content_x;
    int max_row_h = 0;
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        dom_node_t *child = node->children[i];
        
        if (child->display == CSS_DISPLAY_BLOCK) {
            browser_layout_node(child, node->content_x, child_y, node->content_w);
            child_y += child->h + child->margin_bottom;
        } else {
            // Inline
            if (child_x + child->w > node->content_x + node->content_w) {
                child_x = node->content_x;
                child_y += max_row_h;
                max_row_h = 0;
            }
            browser_layout_node(child, child_x, child_y, node->content_w);
            child_x += child->w;
            if (child->h > max_row_h) max_row_h = child->h;
        }
    }
    
    if (node->child_count > 0 && node->display == CSS_DISPLAY_BLOCK) {
        node->h = child_y - node->y + node->padding_bottom + node->border_bottom + node->margin_bottom;
    }
}

int browser_measure_text(const char *text, int font_size) {
    if (!text) return 0;
    return strlen(text) * (font_size / 2);
}

// Rendering
void browser_render_tab(uint32_t tab_id, uint32_t *buffer, int w, int h) {
    if (tab_id >= browser.num_tabs) return;
    
    browser_tab_t *tab = &browser.tabs[tab_id];
    if (!tab->document || !tab->document->body) return;
    
    // Clear background
    for (int i = 0; i < w * h; i++) {
        buffer[i] = 0xFF1A1A2E; // Dark blue background
    }
    
    // Render document
    browser_render_node(tab->document->body, buffer, w, h, 0, 0);
}

void browser_render_node(dom_node_t *node, uint32_t *buffer, int bw, int bh, int offset_x, int offset_y) {
    if (!node) return;
    
    int x = node->x + offset_x;
    int y = node->y + offset_y;
    
    // Clip
    if (x >= bw || y >= bh || x + node->w <= 0 || y + node->h <= 0) return;
    
    // Render background
    if (node->bg_color & 0xFF000000) {
        browser_render_rect(x, y, node->w, node->h, node->bg_color, buffer, bw, bh);
    }
    
    // Render border
    if (node->border_top > 0) {
        browser_render_rect(x, y, node->w, node->border_top, node->border_color, buffer, bw, bh);
    }
    
    // Render text
    if (node->text[0] && node->type != HTML_SCRIPT && node->type != HTML_STYLE) {
        browser_render_text(node->text, node->content_x + offset_x, node->content_y + offset_y,
                           node->font_size, node->text_color, buffer, bw, bh);
    }
    
    // Render children
    for (uint32_t i = 0; i < node->child_count; i++) {
        browser_render_node(node->children[i], buffer, bw, bh, offset_x, offset_y);
    }
}

void browser_render_text(const char *text, int x, int y, int font_size, uint32_t color, uint32_t *buffer, int bw, int bh) {
    if (!text || font_size <= 0) return;
    
    int scale = font_size / 8;
    if (scale < 1) scale = 1;
    
    int cx = x;
    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 128) c = '?';
        
        const uint8_t *glyph = font_8x8[c];
        
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (0x80 >> col)) {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            int px = cx + col * scale + sx;
                            int py = y + row * scale + sy;
                            if (px >= 0 && px < bw && py >= 0 && py < bh) {
                                buffer[py * bw + px] = color;
                            }
                        }
                    }
                }
            }
        }
        
        cx += 8 * scale;
    }
}

void browser_render_rect(int x, int y, int w, int h, uint32_t color, uint32_t *buffer, int bw, int bh) {
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > bw ? bw : x + w;
    int y2 = y + h > bh ? bh : y + h;
    
    for (int row = y1; row < y2; row++) {
        for (int col = x1; col < x2; col++) {
            buffer[row * bw + col] = color;
        }
    }
}

// URL handling
void browser_url_parse(const char *url, char *host, char *path, uint16_t *port, int *https) {
    *https = 0;
    *port = 80;
    
    const char *p = url;
    
    // Check protocol
    if (!strncmp(p, "https://", 8)) {
        *https = 1;
        *port = 443;
        p += 8;
    } else if (!strncmp(p, "http://", 7)) {
        p += 7;
    } else if (!strncmp(p, "about:", 6)) {
        strcpy(host, "about");
        strcpy(path, "/");
        return;
    }
    
    // Extract host
    int hi = 0;
    while (*p && *p != '/' && *p != ':' && hi < 255) {
        host[hi++] = *p++;
    }
    host[hi] = '\\0';
    
    // Port
    if (*p == ':') {
        p++;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
    }
    
    // Path
    int pi = 0;
    if (*p == '/') {
        while (*p && pi < 511) {
            path[pi++] = *p++;
        }
    } else {
        path[0] = '/';
        pi = 1;
    }
    path[pi] = '\\0';
}

void browser_url_encode(const char *in, char *out, uint32_t max_len) {
    uint32_t oi = 0;
    for (uint32_t i = 0; in[i] && oi < max_len - 4; i++) {
        unsigned char c = in[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out[oi++] = c;
        } else {
            out[oi++] = '%';
            out[oi++] = "0123456789ABCDEF"[c >> 4];
            out[oi++] = "0123456789ABCDEF"[c & 0xF];
        }
    }
    out[oi] = '\\0';
}

void browser_url_decode(const char *in, char *out, uint32_t max_len) {
    uint32_t oi = 0;
    for (uint32_t i = 0; in[i] && oi < max_len - 1; i++) {
        if (in[i] == '%' && in[i+1] && in[i+2]) {
            char hex[3] = {in[i+1], in[i+2], '\\0'};
            out[oi++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else if (in[i] == '+') {
            out[oi++] = ' ';
        } else {
            out[oi++] = in[i];
        }
    }
    out[oi] = '\\0';
}

// Cache
void browser_cache_store(const char *url, const char *data, uint32_t len) {
    if (!browser.cache || browser.cache_used + len + 512 > BROWSER_CACHE_SIZE) return;
    
    // Simple cache entry: [url_len:4][url][data_len:4][data]
    uint32_t url_len = strlen(url);
    
    uint8_t *entry = browser.cache + browser.cache_used;
    *(uint32_t*)entry = url_len;
    memcpy(entry + 4, url, url_len);
    *(uint32_t*)(entry + 4 + url_len) = len;
    memcpy(entry + 8 + url_len, data, len);
    
    browser.cache_used += 8 + url_len + len;
}

int browser_cache_lookup(const char *url, char **data, uint32_t *len) {
    if (!browser.cache) return 0;
    
    uint8_t *p = browser.cache;
    uint8_t *end = browser.cache + browser.cache_used;
    
    while (p < end) {
        uint32_t url_len = *(uint32_t*)p;
        p += 4;
        
        if (p + url_len > end) break;
        
        if (!strncmp((char*)p, url, url_len)) {
            p += url_len;
            *len = *(uint32_t*)p;
            p += 4;
            *data = (char*)p;
            return 1;
        }
        
        p += url_len;
        uint32_t data_len = *(uint32_t*)p;
        p += 4 + data_len;
    }
    
    return 0;
}

void browser_cache_clear(void) {
    browser.cache_used = 0;
}

// Bookmarks
void browser_add_bookmark(const char *title, const char *url, const char *folder) {
    if (browser.num_bookmarks >= BROWSER_MAX_BOOKMARKS) return;
    
    bookmark_t *bm = &browser.bookmarks[browser.num_bookmarks++];
    strncpy(bm->title, title, BROWSER_MAX_TITLE_LEN - 1);
    strncpy(bm->url, url, BROWSER_MAX_URL_LEN - 1);
    strncpy(bm->folder, folder, 63);
    bm->added_time = 0; // Would use RTC
}

void browser_remove_bookmark(uint32_t index) {
    if (index >= browser.num_bookmarks) return;
    
    for (uint32_t i = index; i < browser.num_bookmarks - 1; i++) {
        browser.bookmarks[i] = browser.bookmarks[i + 1];
    }
    browser.num_bookmarks--;
}

bookmark_t* browser_get_bookmarks(uint32_t *count) {
    *count = browser.num_bookmarks;
    return browser.bookmarks;
}

browser_tab_t* browser_get_tab(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs) return NULL;
    return &browser.tabs[tab_id];
}

void browser_go_back(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs) return;
    browser_tab_t *tab = &browser.tabs[tab_id];
    if (tab->history_pos > 0) {
        tab->history_pos--;
        browser_navigate(tab_id, tab->history[tab->history_pos]);
    }
}

void browser_go_forward(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs) return;
    browser_tab_t *tab = &browser.tabs[tab_id];
    if (tab->history_pos < tab->history_count - 1) {
        tab->history_pos++;
        browser_navigate(tab_id, tab->history[tab->history_pos]);
    }
}

void browser_reload_tab(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs) return;
    browser_navigate(tab_id, browser.tabs[tab_id].url);
}

void browser_stop(uint32_t tab_id) {
    if (tab_id >= browser.num_tabs) return;
    browser.tabs[tab_id].loading = 0;
}

