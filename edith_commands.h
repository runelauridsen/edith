typedef struct edith_editor_tab edith_editor_tab;
struct edith_editor_tab {
    edith_textview v;
    str file_name;
    edith_editor_tab *next;
    edith_editor_tab *prev;
};

typedef struct edith_editor_tab_list edith_editor_tab_list;
struct edith_editor_tab_list {
    edith_editor_tab *first;
    edith_editor_tab *last;
};

typedef enum edith_cmd_kind {
    EDITH_CMD_KIND_NONE = 0,

    // rune: Tabs/panes

    EDITH_CMD_KIND_OPEN_FILE,
    EDITH_CMD_KIND_SAVE_TAB,
    EDITH_CMD_KIND_CLOSE_TAB,
    EDITH_CMD_KIND_SELECT_TAB,
    EDITH_CMD_KIND_NEXT_TAB,
    EDITH_CMD_KIND_PREV_TAB,

    EDITH_CMD_KIND_NEXT_PANE,
    EDITH_CMD_KIND_PREV_PANE,

    EDITH_CMD_KIND_SAVE_ACTIVE_TAB,
    EDITH_CMD_KIND_CLOSE_ACTIVE_TAB,

    EDITH_CMD_KIND_FIND_NEXT,
    EDITH_CMD_KIND_FIND_PREV,

    EDITH_CMD_KIND_CLOSE_DIALOG,
    EDITH_CMD_KIND_GOTO_LINE,
    EDITH_CMD_KIND_GOTO_POS,
    EDITH_CMD_KIND_SELECT_RANGE,

    EDITH_CMD_KIND_SHOW_OPEN_FILE_DIALOG,
    EDITH_CMD_KIND_SHOW_GOTO_LINE_DIALOG,
    EDITH_CMD_KIND_SHOW_FIND_DIALOG,
    EDITH_CMD_KIND_SHOW_FIND_ALL_DIALOG,
    EDITH_CMD_KIND_SHOW_COMMAND_DIALOG,

    // rune: Move

    EDITH_CMD_KIND_MOVE_LEFT,
    EDITH_CMD_KIND_MOVE_RIGHT,
    EDITH_CMD_KIND_MOVE_UP,
    EDITH_CMD_KIND_MOVE_DOWN,
    EDITH_CMD_KIND_MOVE_BY_WORD_LEFT,
    EDITH_CMD_KIND_MOVE_BY_WORD_RIGHT,
    EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_UP,
    EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_DOWN,
    EDITH_CMD_KIND_MOVE_TO_LINE_LEFT,
    EDITH_CMD_KIND_MOVE_TO_LINE_RIGHT,
    EDITH_CMD_KIND_MOVE_BY_PAGE_DOWN,
    EDITH_CMD_KIND_MOVE_BY_PAGE_UP,
    EDITH_CMD_KIND_MOVE_TO_DOCUMENT_START,
    EDITH_CMD_KIND_MOVE_TO_DOCUMENT_END,

    // rune: Select

    EDITH_CMD_KIND_SELECT_LEFT,
    EDITH_CMD_KIND_SELECT_RIGHT,
    EDITH_CMD_KIND_SELECT_UP,
    EDITH_CMD_KIND_SELECT_DOWN,

    EDITH_CMD_KIND_SELECT_BY_WORD_LEFT,
    EDITH_CMD_KIND_SELECT_BY_WORD_RIGHT,
    EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_UP,
    EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_DOWN,
    EDITH_CMD_KIND_SELECT_TO_LINE_LEFT,
    EDITH_CMD_KIND_SELECT_TO_LINE_RIGHT,
    EDITH_CMD_KIND_SELECT_BY_PAGE_DOWN,
    EDITH_CMD_KIND_SELECT_BY_PAGE_UP,
    EDITH_CMD_KIND_SELECT_TO_DOCUMENT_START,
    EDITH_CMD_KIND_SELECT_TO_DOCUMENT_END,

    EDITH_CMD_KIND_SELECT_ALL,

    // rune: Delete

    EDITH_CMD_KIND_DELETE_LEFT,
    EDITH_CMD_KIND_DELETE_RIGHT,
    EDITH_CMD_KIND_DELETE_BY_WORD_LEFT,
    EDITH_CMD_KIND_DELETE_BY_WORD_RIGHT,
    EDITH_CMD_KIND_DELETE_TO_LINE_LEFT,
    EDITH_CMD_KIND_DELETE_TO_LINE_RIGHT,

    // rune: Undo/redo

    EDITH_CMD_KIND_UNDO,
    EDITH_CMD_KIND_REDO,

    // rune: Cursors.

    EDITH_CMD_KIND_ADD_CURSOR_ABOVE,
    EDITH_CMD_KIND_ADD_CURSOR_BELOW,
    EDITH_CMD_KIND_ADD_CURSOR_AT_NEXT_OCCURENCE,
    EDITH_CMD_KIND_ADD_CURSOR_AT_PREV_OCCURENCE,
    EDITH_CMD_KIND_CLEAR_CURSORS,
    EDITH_CMD_KIND_FLIP_CURSOR,

    // rune: Misc move

    EDITH_CMD_KIND_MOVE_TO_NEXT_OCCURENCE,
    EDITH_CMD_KIND_MOVE_TO_PREV_OCCURENCE,

    // rune: Clipboard

    EDITH_CMD_KIND_CUT,
    EDITH_CMD_KIND_COPY,
    EDITH_CMD_KIND_PASTE,

    // rune: Misc

    EDITH_CMD_KIND_EXIT,
    EDITH_CMD_KIND_SUBMIT_STR,

} edith_cmd_kind;

typedef enum edith_cmd_arg_kind {
    EDITH_CMD_ARG_KIND_NONE,
    EDITH_CMD_ARG_KIND_STR,
    EDITH_CMD_ARG_KIND_TAB,
    EDITH_CMD_ARG_KIND_I64,

    EDITH_CMD_ARG_KIND_SMALL_DATA,
    EDITH_CMD_ARG_KIND_BIG_DATA,
} edith_cmd_arg_kind;

typedef struct edith_cmd_arg_spec edith_cmd_arg_spec;
struct edith_cmd_arg_spec {
    edith_cmd_arg_kind kind;
    str name;
};

typedef enum edith_cmd_flags {
    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR = 1,
    EDITH_CMD_FLAG_HIDE_FROM_LISTER = 2,
} edith_cmd_flags;

typedef struct edith_cmd_spec edith_cmd_spec;
struct edith_cmd_spec {
    str name;
    edith_cmd_flags flags;
    edith_cmd_arg_spec args[4];
};

////////////////////////////////////////////////////////////////
// rune: Ring buffer

typedef struct edith_ring edith_ring;
struct edith_ring {
    u8 *base;
    i64 write_pos;
    i64 read_pos;
    i64 size;
};

// rune: Producer
static i64 edith_ring_avail_for_write(edith_ring *ring);
static i64 edith_ring_write(edith_ring *ring, void *src, i64 write_size);

// rune: Consumer
static i64 edith_ring_avail(edith_ring *ring);
static i64 edith_ring_peek(edith_ring *ring, void *dst, i64 peek_off, i64 peek_size);
static i64 edith_ring_skip(edith_ring *ring, void *dst, i64 read_size);

typedef struct edith_cmd_ring edith_cmd_ring;
struct edith_cmd_ring {
    edith_ring ring;
};

#define EDITH_CMD_PACKET_MAGIC (0xdeadbeef)

typedef struct edith_cmd_packet edith_cmd_packet;
struct edith_cmd_packet {
    u32 magic;
    edith_cmd_kind kind;
    i64 args[4];
    i64 body_size;
};

typedef struct edith_cmd edith_cmd;
struct edith_cmd {
    edith_cmd_kind kind;
    i64 args[4];
    str body;
};

static edith_cmd_ring edith_g_cmd_ring = { 0 };

static i64  edith_cmd_ring_write(edith_cmd_ring *queue, edith_cmd *cmd);
static i64  edith_cmd_ring_peek(edith_cmd_ring *queue, edith_cmd *cmd, u64 off, arena *arena);
static void edith_cmd_ring_skip(edith_cmd_ring *queue, i64 cmd_size);

////////////////////////////////////////////////////////////////
// rune: Command definitions

#define EDITH_CMD_ARG_SPEC(kind, name) { kind, STR(name) }

static readonly edith_cmd_spec edith_cmd_specs[] = {
    [0] = 0,
    [EDITH_CMD_KIND_OPEN_FILE]                    = { STR("Open file"),                               0,                                        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_STR, "File name"),          },  },
    [EDITH_CMD_KIND_SAVE_TAB]                     = { STR("Save tab"),                                0,                                        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_TAB, "Tab"),                },  },
    [EDITH_CMD_KIND_CLOSE_TAB]                    = { STR("Close tab"),                               0,                                        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_TAB, "Tab"),                },  },
    [EDITH_CMD_KIND_SELECT_TAB]                   = { STR("Select tab"),                              0,                                        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_TAB, "Tab"),                },  },
    [EDITH_CMD_KIND_NEXT_TAB]                     = { STR("Next tab"),                                0,                                                                                                                },
    [EDITH_CMD_KIND_PREV_TAB]                     = { STR("prev tab"),                                0,                                                                                                                },
    [EDITH_CMD_KIND_NEXT_PANE]                    = { STR("Next pane"),                               0,                                                                                                                },
    [EDITH_CMD_KIND_PREV_PANE]                    = { STR("Prev pane"),                               0,                                                                                                                },
    [EDITH_CMD_KIND_SAVE_ACTIVE_TAB]              = { STR("Save active tab"),                         EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_CLOSE_ACTIVE_TAB]             = { STR("Close active tab"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_FIND_NEXT]                    = { STR("Find next occurence"),                     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_FIND_PREV]                    = { STR("Find prev occurence"),                     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_GOTO_LINE]                    = { STR("Goto line number"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_I64, "Line number"),        },  },
    [EDITH_CMD_KIND_GOTO_POS]                     = { STR("Goto byte offset"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_I64, "Byte offset"),        },  },
    [EDITH_CMD_KIND_SELECT_RANGE]                 = { STR("Select byte offset range"),                EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,        { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_I64, "Start byte offset"), EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_I64, "End byte offset") }, },

    [EDITH_CMD_KIND_SHOW_OPEN_FILE_DIALOG]        = { STR("Show open file dialog"),                   0,                                                                                                                },
    [EDITH_CMD_KIND_SHOW_GOTO_LINE_DIALOG]        = { STR("Show goto line dialog"),                   EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SHOW_FIND_DIALOG]             = { STR("Show find dialog"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SHOW_FIND_ALL_DIALOG]         = { STR("Show find all dialog"),                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SHOW_COMMAND_DIALOG]          = { STR("Show command dialog"),                     0,                                                                                                                },
    [EDITH_CMD_KIND_CLOSE_DIALOG]                 = { STR("Close dialog"),                            0,                                                                                                                },

    [EDITH_CMD_KIND_MOVE_LEFT]                    = { STR("Move left"),                               EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_RIGHT]                   = { STR("Move right"),                              EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_UP]                      = { STR("Move up"),                                 EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_DOWN]                    = { STR("Move down"),                               EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_BY_WORD_LEFT]            = { STR("Move left by word"),                       EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_BY_WORD_RIGHT]           = { STR("Move right by word"),                      EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_UP]         = { STR("Move up by paragraph"),                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_BY_PARAGRAPH_DOWN]       = { STR("Move down by paragraph"),                  EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_TO_LINE_LEFT]            = { STR("Move to line start"),                      EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_TO_LINE_RIGHT]           = { STR("Move to line end"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_BY_PAGE_DOWN]            = { STR("Move down by page"),                       EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_BY_PAGE_UP]              = { STR("Move up by page"),                         EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_TO_DOCUMENT_START]       = { STR("Move to beginning of document"),           EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_TO_DOCUMENT_END]         = { STR("Move to end of document"),                 EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_LEFT]                  = { STR("Select left"),                             EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_RIGHT]                 = { STR("Select right"),                            EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_UP]                    = { STR("Select up"),                               EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_DOWN]                  = { STR("Select down"),                             EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_BY_WORD_LEFT]          = { STR("Select left by word"),                     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_BY_WORD_RIGHT]         = { STR("Select right by word"),                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_UP]       = { STR("Select up by paragraph"),                  EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_BY_PARAGRAPH_DOWN]     = { STR("Select down by paragraph"),                EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_TO_LINE_LEFT]          = { STR("Select to line start"),                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_TO_LINE_RIGHT]         = { STR("Select to line end"),                      EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_BY_PAGE_DOWN]          = { STR("Select down by page"),                     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_BY_PAGE_UP]            = { STR("Select up by page"),                       EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_TO_DOCUMENT_START]     = { STR("Select to beginning of document"),         EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_TO_DOCUMENT_END]       = { STR("Select to end of document"),               EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_SELECT_ALL]                   = { STR("Select whole document"),                   EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_DELETE_LEFT]                  = { STR("Delete left"),                             EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_DELETE_RIGHT]                 = { STR("Delete right"),                            EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_DELETE_BY_WORD_LEFT]          = { STR("Delete left by word"),                     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_DELETE_BY_WORD_RIGHT]         = { STR("Delete right by word"),                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_DELETE_TO_LINE_LEFT]          = { STR("Delete to line start"),                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_DELETE_TO_LINE_RIGHT]         = { STR("Delete to line end"),                      EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },

    [EDITH_CMD_KIND_UNDO]                         = { STR("Undo"),                                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_REDO]                         = { STR("Redo"),                                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },

    [EDITH_CMD_KIND_ADD_CURSOR_ABOVE]             = { STR("Add cursor above"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_ADD_CURSOR_BELOW]             = { STR("Add cursor below"),                        EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_ADD_CURSOR_AT_NEXT_OCCURENCE] = { STR("Add cursor at next occurence"),            EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_ADD_CURSOR_AT_PREV_OCCURENCE] = { STR("Add cursor at prev occurence"),            EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_CLEAR_CURSORS]                = { STR("Clear all cursors"),                       EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_FLIP_CURSOR]                  = { STR("Flip cursor"),                             EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },

    [EDITH_CMD_KIND_MOVE_TO_NEXT_OCCURENCE]       = { STR("Move to next occurence of selection"),     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_MOVE_TO_PREV_OCCURENCE]       = { STR("Move to prev occurence of selection"),     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },

    [EDITH_CMD_KIND_CUT]                          = { STR("Cut"),                                     EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_COPY]                         = { STR("Copy"),                                    EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_PASTE]                        = { STR("Paste"),                                   EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,                                                                                },
    [EDITH_CMD_KIND_EXIT]                         = { STR("Exit"),                                    0,                                                                                                                },

    [EDITH_CMD_KIND_SUBMIT_STR]                   = { STR("Insert string"),                           EDITH_CMD_FLAG_NEED_ACTIVE_EDITOR,               { EDITH_CMD_ARG_SPEC(EDITH_CMD_ARG_KIND_STR, "Data") }           },
};

////////////////////////////////////////////////////////////////
// rune: Misc

static edith_cmd_spec *edith_cmd_spec_from_kind(edith_cmd_kind kind);
static void edith_cmd_print(edith_cmd *cmd);
