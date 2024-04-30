static bool edith_mem_eq(u8 *a, u8 *b, u64 len) {
    return memcmp(a, b, len) == 0;
}

static bool edith_mem_eq_nocase(u8 *a, u8 *b, u64 len) {
    bool ret = true;
    for (u64 i = 0; i < len; i++) {
        if (u8_to_lower(a[i]) != u8_to_lower(b[i])) {
            ret = false;
            break;
        }
    }
    return ret;
}

static u8 *edith_memmem(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len) {
    if (haystack_len == 0) return null;
    if (needle_len == 0) return haystack;
    if (haystack_len < needle_len) return null;

    u8 first = needle[0];
    u8 last  = needle[needle_len - 1];

    for (u64 i = 0; i < haystack_len - needle_len + 1; i++) {
        if (haystack[i] == first && haystack[i + needle_len - 1] == last) {
            if (edith_mem_eq(haystack + i, needle, needle_len)) {
                return haystack + i;
            }
        }
    }

    return null;
}

static u8 *edith_memmem_nocase(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len) {
    if (haystack_len == 0) return null;
    if (needle_len == 0) return haystack;
    if (haystack_len < needle_len) return null;

    u8 first = u8_to_lower(needle[0]);
    u8 last  = u8_to_lower(needle[needle_len - 1]);

    for (u64 i = 0; i < haystack_len - needle_len + 1; i++) {
        if (u8_to_lower(haystack[i]) == first &&
            u8_to_lower(haystack[i + needle_len - 1]) == last) {
            if (edith_mem_eq_nocase(haystack + i, needle, needle_len)) {
                return haystack + i;
            }
        }
    }

    return null;
}

static u8 *edith_memmem_rev(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len) {
    if (haystack_len == 0) return null;
    if (needle_len == 0) return haystack;

    u8 first = needle[0];
    u8 last  = needle[needle_len - 1];

    for (u64 j = 0; j < haystack_len - needle_len + 1; j++) {
        u64 i = (haystack_len - needle_len) - j - 1;
        if (haystack[i] == first && haystack[i + needle_len - 1] == last) {
            if (edith_mem_eq(haystack + i, needle, needle_len)) {
                return haystack + i;
            }
        }
    }

    return null;
}

static u8 *edith_memmem_nocase_rev(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len) {
    if (haystack_len == 0) return null;
    if (needle_len == 0) return haystack;

    u8 first = u8_to_lower(needle[0]);
    u8 last  = u8_to_lower(needle[needle_len - 1]);

    for (u64 j = 0; j < haystack_len - needle_len + 1; j++) {
        u64 i = (haystack_len - needle_len) - j - 1;
        if (u8_to_lower(haystack[i]) == first &&
            u8_to_lower(haystack[i + needle_len - 1]) == last) {
            if (edith_mem_eq_nocase(haystack + i, needle, needle_len)) {
                return haystack + i;
            }
        }
    }

    return null;
}

static i64 edith_memmem_str(str haystack, str needle, dir dir, bool case_insensitive) {
    u8 *m = null;
    if (0);
    else if (dir == DIR_FORWARD  && !case_insensitive) m = edith_memmem(haystack.v, haystack.len, needle.v, needle.len);
    else if (dir == DIR_BACKWARD && !case_insensitive) m = edith_memmem_rev(haystack.v, haystack.len, needle.v, needle.len);
    else if (dir == DIR_FORWARD  &&  case_insensitive) m = edith_memmem_nocase(haystack.v, haystack.len, needle.v, needle.len);
    else if (dir == DIR_BACKWARD &&  case_insensitive) m = edith_memmem_nocase_rev(haystack.v, haystack.len, needle.v, needle.len);
    else assert(false);

    if (m) {
        return m - haystack.v;
    } else {
        return -1;
    }
}

static i64 edith_next_occurence_in_gapbuffer(edith_gapbuffer *gb, i64_range search_range, str needle, dir dir, bool case_insensitive, arena *temp) {
    assert(needle.len > 0);

    i64 ret = -1;

    ////////////////////////////////////////////////
    // Find ranges of haystacks.

    str       buf       = str_make(gb->buf, gb->buf_cap);
    i64_range gap       = gb->gap;
    i64       gap_len   = gap.max - gap.min;

    i64_range before_gap = { 0,                        gap.min };
    i64_range across_gap = { gap.min - needle.len + 1, gap.min + needle.len - 1 };
    i64_range after_gap  = { gap.min,                  buf.len - gap_len };

    i64_range haystack_before_gap = { max(before_gap.min, search_range.min), min(before_gap.max, search_range.max) };
    i64_range haystack_across_gap = { max(across_gap.min, search_range.min), min(across_gap.max, search_range.max) };
    i64_range haystack_after_gap  = { max(after_gap.min,  search_range.min), min(after_gap.max,  search_range.max) };

    i64 haystack_before_len = haystack_before_gap.max - haystack_before_gap.min;
    i64 haystack_across_len = haystack_across_gap.max - haystack_across_gap.min;
    i64 haystack_after_len  = haystack_after_gap.max  - haystack_after_gap.min;

    if (dir == DIR_FORWARD) {

        ////////////////////////////////////////////////
        // Search before gap.

        if (ret == -1 && haystack_before_len >= needle.len) {
            str haystack = substr_range(buf, haystack_before_gap);
            ret = edith_memmem_str(haystack, needle, dir, case_insensitive);
            if (ret != -1) {
                ret += haystack_before_gap.min;
            }
        }

        ////////////////////////////////////////////////
        // Search across gap.

        if (ret == -1 && haystack_across_len >= needle.len) {
            // TODO(rune): This is a very lazy approach. Check if it's a performance problem.

            arena_scope_begin(temp);

            str haystack = edith_str_from_gapbuffer_range(gb, haystack_across_gap, temp);
            ret = edith_memmem_str(haystack, needle, dir, case_insensitive);
            if (ret != -1) {
                ret += haystack_across_gap.min;
            }

            arena_scope_end(temp);
        }

        ////////////////////////////////////////////////
        // Search after gap.

        if (ret == -1 && haystack_after_len >= needle.len) {
            i64_range raw_range = { haystack_after_gap.min + gap_len, haystack_after_gap.max + gap_len };
            str haystack = substr_range(buf, raw_range);
            ret = edith_memmem_str(haystack, needle, dir, case_insensitive);
            if (ret != -1) {
                ret += haystack_after_gap.min;
            }
        }
    } else {
        assert(dir == DIR_BACKWARD);

        ////////////////////////////////////////////////
        // Search after gap.

        if (ret == -1 && haystack_after_len >= needle.len) {
            i64_range raw_range = { haystack_after_gap.min + gap_len, haystack_after_gap.max + gap_len };
            str haystack = substr_range(buf, raw_range);
            ret = edith_memmem_str(haystack, needle, dir, case_insensitive);
            if (ret != -1) {
                ret += haystack_after_gap.min;
            }
        }

        ////////////////////////////////////////////////
        // Search across gap.

        if (ret == -1 && haystack_across_len >= needle.len) {
            // TODO(rune): This is a very lazy approach. Check if it's a performance problem.

            arena_scope_begin(temp);

            str haystack = edith_str_from_gapbuffer_range(gb, haystack_across_gap, temp);
            ret = edith_memmem_str(haystack, needle, dir, case_insensitive);
            if (ret != -1) {
                ret += haystack_across_gap.min;
            }

            arena_scope_end(temp);
        }

        ////////////////////////////////////////////////
        // Search before gap.

        if (ret == -1 && haystack_before_len >= needle.len) {
            str haystack = substr_range(buf, haystack_before_gap);
            ret = edith_memmem_str(haystack, needle, dir, case_insensitive);
            if (ret != -1) {
                ret += haystack_before_gap.min;
            }
        }
    }

    return ret;
}
