#define ui_stack_block(T)           concat(ui_stack_block_, T)
#define ui_stack(T)                 concat(ui_stack_, T)
#define ui_stack_push(T, ...)       concat(ui_stack_push_, T)(__VA_ARGS__)
#define ui_stack_pop(T, ...)        concat(ui_stack_pop_, T)(__VA_ARGS__)
#define ui_stack_get(T, ...)        concat(ui_stack_get_, T)(__VA_ARGS__)
#define ui_stack_set(T, ...)        concat(ui_stack_set_, T)(__VA_ARGS__)
#define ui_stack_get_prev(T, ...)   concat(ui_stack_get_prev_, T)(__VA_ARGS__)
#define ui_stack_init(T, ...)       concat(ui_stack_init_, T)(__VA_ARGS__)

typedef struct ui_stack_block(T) ui_stack_block(T);
struct ui_stack_block(T) {
    T storage[256];
    u64 count_used;
    ui_stack_block(T) *next;
    ui_stack_block(T) *prev;
};

typedef struct ui_stack(T) ui_stack(T);
struct ui_stack(T) {
    ui_stack_block(T) first_block;
    ui_stack_block(T) *head_block;
    T top;
    u64 count;
    arena *arena;
};

static T ui_stack_get(T, ui_stack(T) *stack) {
    return stack->top;
}

static void ui_stack_set(T, ui_stack(T) *stack, T value) {
    stack->top = value;
}

static T ui_stack_get_prev(T, ui_stack(T) *stack) {
    ui_stack_block(T) *block = stack->head_block;
    assert(block->count_used > 0);
    T ret = block->storage[block->count_used - 1];
    return ret;
}

static void ui_stack_push(T, ui_stack(T) *stack) {
    if (stack->head_block->count_used == countof(stack->head_block->storage)) {
        if (stack->head_block->next) {
            assert(stack->head_block->next->count_used == 0);
            stack->head_block = stack->head_block->next;
        } else {
            ui_stack_block(T) *new_block = arena_push_struct(stack->arena, ui_stack_block(T));
            new_block->prev = stack->head_block;
            stack->head_block->next = new_block;
            stack->head_block = new_block;
        }
    }

    stack->head_block->storage[stack->head_block->count_used++] = stack->top;
    stack->count++;
}

static void ui_stack_pop(T, ui_stack(T) *stack) {
    assert(stack->count > 1);
    assert(stack->head_block->count_used > 0);

    stack->top = stack->head_block->storage[stack->head_block->count_used - 1];
    stack->head_block->count_used--;
    if (stack->head_block->count_used == 0) {
        assert(stack->head_block->prev);
        stack->head_block = stack->head_block->prev;
    }
    stack->count--;
}

static void ui_stack_init(T, ui_stack(T) *stack, arena *arena, T initial_value) {
    stack->arena = arena;
    stack->top = initial_value;
    stack->count = 1;
    stack->head_block = &stack->first_block;
    stack->head_block->storage[0] = initial_value;
    stack->head_block->count_used = 1;
    stack->head_block->next = null;
    stack->head_block->prev = null;
}

#undef T
