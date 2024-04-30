////////////////////////////////////////////////////////////////
// rune: Symbol list

static lang_c_symbol *lang_c_symbol_list_push(lang_c_symbol_list *symbols, str name, lang_c_symbol_kind kind) {
    lang_c_symbol *added = arena_push_struct(&lang_c_symbol_arena, lang_c_symbol);
    added->name = name;
    added->kind = kind;
    slist_push(symbols, added);
    symbols->count++;
    return added;
}

////////////////////////////////////////////////////////////////
// rune: String conversions

static str_list lang_c_str_list_from_type(lang_c_type *type, arena *arena) {
    str_list list = { 0 };
    switch (type->kind) {
        case LANG_C_TYPE_KIND_BASIC: {
            str_list_push(&list, arena, type->text);
        } break;

        case LANG_C_TYPE_KIND_POINTER: {
            str_list_push(&list, arena, type->text);
            str_list_push(&list, arena, str("*"));
        } break;

        case LANG_C_TYPE_KIND_ARRAY: {
            str_list_push(&list, arena, str("["));
            str_list_push(&list, arena, type->text);
            str_list_push(&list, arena, str("]"));
        } break;

        case LANG_C_TYPE_KIND_FUNC: {
            str_list_push(&list, arena, str("func("));
            for_list (lang_c_symbol, arg, type->args) {
                str_list_join(&list, lang_c_str_list_from_symbol(arg, arena));
                if (arg != type->args.last) {
                    str_list_push(&list, arena, str(", "));
                }
            }
            str_list_push(&list, arena, str(") -> "));
        } break;
    }
    return list;
}

static str_list lang_c_str_list_from_type_list(lang_c_type_list types, arena *arena) {
    str_list list = { 0 };
    for_list (lang_c_type, type, types) {
        str_list_join(&list, lang_c_str_list_from_type(type, arena));
    }
    return list;
}

static str_list lang_c_str_list_from_symbol(lang_c_symbol *symbol, arena *arena) {
    str_list list = { 0 };
    str_list_push(&list, arena, symbol->name);
    str_list_push(&list, arena, str(": "));
    str_list_join(&list, lang_c_str_list_from_type_list(symbol->type, arena));
    return list;
}

static str_list lang_c_str_list_from_symbol_list(lang_c_symbol_list symbols, arena *arena) {
    str_list list = { 0 };
    for_list (lang_c_symbol, symbol, symbols) {
        str_list_join(&list, lang_c_str_list_from_symbol(symbol, arena));
        if (symbol != symbols.last) {
            str_list_push(&list, arena, str(", "));
        }
    }
    return list;
}

static str lang_c_str_from_type(lang_c_type *type, arena *arena) {
    str_list list = lang_c_str_list_from_type(type, arena);
    str s = str_list_concat(&list, arena);
    return s;
}

static str lang_c_str_from_type_list(lang_c_type_list types, arena *arena) {
    str_list list = lang_c_str_list_from_type_list(types, arena);
    str s = str_list_concat(&list, arena);
    return s;
}

static str lang_c_str_from_symbol(lang_c_symbol *symbol, arena *arena) {
    str_list list = lang_c_str_list_from_symbol(symbol, arena);
    str s = str_list_concat(&list, arena);
    return s;
}

static str lang_c_str_from_symbol_list(lang_c_symbol_list symbols, arena *arena) {
    str_list list = lang_c_str_list_from_symbol_list(symbols, arena);
    str s = str_list_concat(&list, arena);
    return s;
}

static lang_c_symbol_kind lang_c_symbol_kind_from_type(lang_c_type *type) {
    switch (type ? type->kind : 0) {
        case LANG_C_TYPE_KIND_BASIC:   return LANG_C_SYMBOL_KIND_VAR;
        case LANG_C_TYPE_KIND_POINTER: return LANG_C_SYMBOL_KIND_VAR;
        case LANG_C_TYPE_KIND_ARRAY:   return LANG_C_SYMBOL_KIND_VAR;
        case LANG_C_TYPE_KIND_FUNC:    return LANG_C_SYMBOL_KIND_FUNC;
        default:                    return 0;
    }
}

static str lang_c_str_from_type_kind(lang_c_type_kind type_kind) {
    switch (type_kind) {
        case LANG_C_TYPE_KIND_NONE:    return str("LANG_C_TYPE_KIND_NONE");
        case LANG_C_TYPE_KIND_BASIC:   return str("LANG_C_TYPE_KIND_BASIC");
        case LANG_C_TYPE_KIND_POINTER: return str("LANG_C_TYPE_KIND_POINTER");
        case LANG_C_TYPE_KIND_ARRAY:   return str("LANG_C_TYPE_KIND_ARRAY");
        case LANG_C_TYPE_KIND_FUNC:    return str("LANG_C_TYPE_KIND_FUNC");
        default:                    return str("INVALID");
    }
}

static str lang_c_str_from_symbol_kind(lang_c_symbol_kind symbol_kind) {
    switch (symbol_kind) {
        case LANG_C_SYMBOL_KIND_NONE:  return str("LANG_C_SYMBOL_KIND_NONE");
        case LANG_C_SYMBOL_KIND_TYPE:  return str("LANG_C_SYMBOL_KIND_TYPE");
        case LANG_C_SYMBOL_KIND_FUNC:  return str("LANG_C_SYMBOL_KIND_FUNC");
        case LANG_C_SYMBOL_KIND_VAR:   return str("LANG_C_SYMBOL_KIND_VAR");
        default:                    return str("INVALID");
    }
}

static void lang_c_debug_print_level(i64 level) {
    static readonly str colors[] = {
        STR(ANSI_FG_BLUE),
        STR(ANSI_FG_MAGENTA),
        STR(ANSI_FG_RED),
        STR(ANSI_FG_YELLOW),
        STR(ANSI_FG_GREEN),
        STR(ANSI_FG_CYAN),
    };

    print(ANSI_FAINT);
    for_n (i64, i, level) {
        print(colors[i % countof(colors)]);

        if (i + 1 < level) {
            print("│ ");
        } else {
            print("├─");
        }
    }
    print(ANSI_RESET);
}

static void lang_c_debug_print_symbols(lang_c_symbol_list symbols, i64 level); // TODO(rune): Cleanup

static void lang_c_debug_print_type(lang_c_type *type, i64 level) {
    lang_c_debug_print_level(level);
    print("type ");
    print(ANSI_FG_CYAN);
    print("%(literal) ", type->text);
    print(ANSI_FG_GRAY);
    print("%", lang_c_str_from_type_kind(type->kind));
    print(ANSI_FG_DEFAULT);
    print("\n");

    if (type->args.count > 0) {
        lang_c_debug_print_symbols(type->args, level + 1);
    }
}

static void lang_c_debug_print_symbol(lang_c_symbol *symbol, i64 level) {
    lang_c_debug_print_level(level);
    print("symbol ");
    print(ANSI_FG_CYAN);
    print("%(literal) ", symbol->name);
    print(ANSI_FG_GRAY);
    print("%", lang_c_str_from_symbol_kind(symbol->kind));
    print(ANSI_FG_DEFAULT);
    print("\n");

    for_list (lang_c_type, it, symbol->type) {
        lang_c_debug_print_type(it, level + 1);
    }
}

static void lang_c_debug_print_symbols(lang_c_symbol_list symbols, i64 level) {
    lang_c_debug_print_level(level);
    println("symbol-list");
    for_list (lang_c_symbol, symbol, symbols) {
        lang_c_debug_print_symbol(symbol, level + 1);
    }
}