typedef struct ui_toast ui_toast;
struct ui_toast {
    u8 text[256];
    u64 text_len;
    i64 born_tick;
    i64 dead_tick;
    ui_toast *next;
    ui_toast *prev;
    ui_animated_rect animated_rect;
};

typedef struct ui_toast_array ui_toast_array;
struct ui_toast_array {
    ui_toast *v;
    u64 count;
};

typedef struct ui_toast_ctx ui_toast_ctx;
struct ui_toast_ctx {
    ui_toast_array storage;

    struct {
        ui_toast *first;
        ui_toast *last;
    } active;

    ui_toast *first_free;
};
