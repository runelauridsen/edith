#define yo_stack_block(T)           concat(yo_stack_block_, T)
#define yo_stack(T)                 concat(yo_stack_, T)
#define yo_stack_push(T, ...)       concat(yo_stack_push_, T)(__VA_ARGS__)
#define yo_stack_pop(T, ...)        concat(yo_stack_pop_, T)(__VA_ARGS__)
#define yo_stack_get(T, ...)        concat(yo_stack_get_, T)(__VA_ARGS__)
#define yo_stack_set(T, ...)        concat(yo_stack_set_, T)(__VA_ARGS__)
#define yo_stack_get_prev(T, ...)   concat(yo_stack_get_prev_, T)(__VA_ARGS__)
#define yo_stack_init(T, ...)       concat(yo_stack_init_, T)(__VA_ARGS__)

typedef struct yo_stack_block(T) yo_stack_block(T);
struct yo_stack_block(T) {
    T storage[64];
    u64 count_used;
    yo_stack_block(T) *next;
    yo_stack_block(T) *prev;
};

typedef struct yo_stack(T) yo_stack(T);
struct yo_stack(T) {
    yo_stack_block(T) first_block;
    yo_stack_block(T) *head_block;
    T top;
    u64 count;
    arena *arena;
};

static T yo_stack_get(T, yo_stack(T) *stack) {
    return stack->top;
}

static void yo_stack_set(T, yo_stack(T) *stack, T value) {
    stack->top = value;
}

static T yo_stack_get_prev(T, yo_stack(T) *stack) {
    yo_stack_block(T) *block = stack->head_block;
    assert(block->count_used > 0);
    T ret = block->storage[block->count_used - 1];
    return ret;
}

static void yo_stack_push(T, yo_stack(T) *stack) {
    if (stack->head_block->count_used == countof(stack->head_block->storage)) {
        if (stack->head_block->next) {
            assert(stack->head_block->next->count_used == 0);
            stack->head_block = stack->head_block->next;
        } else {
            yo_stack_block(T) *new_block = arena_push_struct(stack->arena, yo_stack_block(T));
            new_block->prev = stack->head_block;
            stack->head_block->next = new_block;
            stack->head_block = new_block;
        }
    }

    stack->head_block->storage[stack->head_block->count_used++] = stack->top;
    stack->count++;
}

static void yo_stack_pop(T, yo_stack(T) *stack) {
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

static void yo_stack_init(T, yo_stack(T) *stack, arena *arena, T initial_value) {
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
