////////////////////////////////////////////////////////////////
// rune: Low level needle-in-a-haystack

static u8 *edith_memmem(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len);
static u8 *edith_memmem_rev(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len);
static u8 *edith_memmem_nocase(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len);
static u8 *edith_memmem_nocase_rev(u8 *haystack, u64 haystack_len, u8 *needle, u64 needle_len);
static i64 edith_memmem_str(str haystack, str needle, dir dir, bool case_insensitive);

////////////////////////////////////////////////////////////////
// rune: Incremental gapbuffer search

static i64 edith_next_occurence_in_gapbuffer(edith_gapbuffer *gb, i64_range search_range, str needle, dir dir, bool case_insensitive, arena *temp);
