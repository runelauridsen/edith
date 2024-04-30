typedef enum edith_clipboard_part_flags {
    EDITH_CLIPBOARD_PART_FLAG_NEWLINE = 1,
} edith_clipboard_part_flags;

typedef struct edith_clipboard_part edith_clipboard_part;
struct edith_clipboard_part {
    str data;
    edith_clipboard_part_flags flags;
    edith_clipboard_part *next;
};

typedef struct edith_clipboard_part_list edith_clipboard_part_list;
struct edith_clipboard_part_list {
    edith_clipboard_part *first;
    edith_clipboard_part *last;
    i64 count;
};

typedef struct edith_clipboard edith_clipboard;
struct edith_clipboard {
    arena arena;
    edith_clipboard_part_list parts;
};

static void edith_clipboard_reset(edith_clipboard *clipboard) {
    arena_reset(&clipboard->arena);
    clipboard->parts.first = null;
    clipboard->parts.last  = null;
    clipboard->parts.count = 0;
}

