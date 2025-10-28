#ifndef INCLUDE_REMIMU
#define INCLUDE_REMIMU 1

#ifndef REMIMU_FUNC_VISIBILITY
#define REMIMU_FUNC_VISIBILITY static inline
#endif

#ifndef REMIMU_CONST_VISIBILITY
#define REMIMU_CONST_VISIBILITY static const
#endif

#ifndef REMIMU_ITERATION_LIMIT
#define REMIMU_ITERATION_LIMIT 0 // Set to non-zero to enable an interation limit
#endif

#ifndef REMIMU_ASSERT
#define REMIMU_ASSERT(x) assert(x)
#endif

/************

    REMIMU: SINGLE HEADER C/C++ REGEX LIBRARY

    Compatible with C99 and C++11 and later standards. Uses backtracking and relatively standard regex syntax.

    #include "remimu.h"

FUNCTIONS

    // Returns 0 on success, or -1 on invalid or unsupported regex, or -2 on not enough tokens given to parse regex.
    int regex_parse(
        const char * pattern,       // Regex pattern to parse.
        RegexToken * tokens,        // Output buffer of token_count regex tokens.
        int16_t * token_count,      // Maximum allowed number of tokens to write
        int32_t flags               // Optional bitflags.
    )

    // Returns match length, or -1 on no match, or -2 on out of memory, or -3 if the regex is invalid.
    int64_t regex_match(
        const RegexToken * tokens,  // Parsed regex to match against text.
        const char * text,          // Text to match against tokens.
        size_t start_i,             // index value to match at.
        uint16_t cap_slots,         // Number of allowed capture info output slots.
        int64_t * cap_pos,          // Capture position info output buffer.
        int64_t * cap_span          // Capture length info output buffer.
    )

    void print_regex_tokens(
        RegexToken * tokens     // Regex tokens to spew to stdout, for debugging.
    )

PERFORMANCE

    On simple cases, Remimu's match speed is similar to PCRE2. Regex parsing/compilation is also much faster (around 4x to 10x), so single-shot regexes are often faster than PCRE2.

    HOWEVER: Remimu is a pure backtracking engine, and has `O(2^x)` complexity on regexes with catastrophic backtracking. It can be much, much, MUCH slower than PCRE2. Beware!

    Remimu uses length-checked fixed memory buffers with no recursion, so memory usage is statically known.

FEATURES

    - Lowest-common-denominator common regex syntax
    - Based on backtracking (slow in the worst case, but fast in the best case)
    - 8-bit only, no utf-16 or utf-32
    - Statically known memory usage (no heap allocation or recursion)
    - Groups with or without capture, and with or without quantifiers
    - Supported escapes:
    - - 2-digit hex: e.g. \x00, \xFF, or lowercase, or mixed case
    - - \r, \n, \t, \v, \f (whitespace characters)
    - - \d, \s, \w, \D, \S, \W (digit, space, and word character classes)
    - - \b, \B word boundary and non-word-boundary anchors (not fully supported in zero-size quantified groups, but even then, usually supported)
    - - Escaped literal characters: {}[]-()|^$*+?:./\
    - - - Escapes work in character classes, except for 'b'
    - Character classes, including disjoint ranges, proper handling of bare [ and trailing -, etc
    - - Dot (.) matches all characters, including newlines, unless REMIMU_FLAG_DOT_NO_NEWLINES is passed as a flag to regex_parse
    - - Dot (.) only matches at most one byte at a time, so matching \r\n requires two dots (and not using REMIMU_FLAG_DOT_NO_NEWLINES)
    - Anchors (^ and $)
    - - Same support caveats as \b, \B apply
    - Basic quantifiers (*, +, ?)
    - - Quantifiers are greedy by default.
    - Explicit quantifiers ({2}, {5}, {5,}, {5,7})
    - Alternation e.g. (asdf|foo)
    - Lazy quantifiers e.g. (asdf)*? or \w+?
    - Possessive greedy quantifiers e.g. (asdf)*+ or \w++
    - - NOTE: Capture groups for and inside of possessive groups return no capture information.
    - Atomic groups e.g. (?>(asdf))
    - - NOTE: Capture groups inside of atomic groups return no capture information.

NOT SUPPORTED

    - Strings with non-terminal null characters
    - Unicode character classes (matching single utf-8 characters works regardless)
    - Exact POSIX regex semantics (posix-style greediness etc)
    - Backreferences
    - Lookbehind/Lookahead
    - Named groups
    - Most other weird flavor-specific regex stuff
    - Capture of or inside of possessive-quantified groups (still take up a capture slot, but no data is returned)

USAGE

    // minimal:

    RegexToken tokens[1024];
    int16_t token_count = 1024;
    int e = regex_parse("[0-9]+\\.[0-9]+", tokens, &token_count, 0);
    assert(!e);

    int64_t match_len = regex_match(tokens, "23.53) ", 0, 0, 0, 0);

    // with captures:

    RegexToken tokens[256];
    int16_t token_count = sizeof(tokens)/sizeof(tokens[0]);
    int e = regex_parse("((a)|(b))++", tokens, &token_count, 0);
    assert(!e);

    int64_t cap_pos[5];
    int64_t cap_span[5];
    memset(cap_pos, 0xFF, sizeof(cap_pos));
    memset(cap_span, 0xFF, sizeof(cap_span));

    int64_t matchlen = regex_match(tokens, "aaaaaabbbabaqa", 0, 5, cap_pos, cap_span);

    // for debugging
    print_regex_tokens(tokens);

LICENSE

    Creative Commons Zero, public domain.

*/

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

REMIMU_CONST_VISIBILITY int REMIMU_FLAG_DOT_NO_NEWLINES = 1;

REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_NORMAL      = 0;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_OPEN        = 1;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_NCOPEN      = 2;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_CLOSE       = 3;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_OR          = 4;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_CARET       = 5;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_DOLLAR      = 6;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_BOUND       = 7;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_NBOUND      = 8;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_KIND_END         = 9;

REMIMU_CONST_VISIBILITY uint8_t REMIMU_MODE_POSSESSIVE  = 1;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_MODE_LAZY        = 2;
REMIMU_CONST_VISIBILITY uint8_t REMIMU_MODE_INVERTED    = 128; // temporary; gets cleared later

typedef struct _RegexToken {
    uint8_t kind;
    uint8_t mode;
    uint16_t count_lo;
    uint16_t count_hi; // 0 means no limit
    uint16_t mask[16]; // for groups: mask 0 stores group-with-quantifier number (quantifiers are +, *, ?, {n}, {n,}, or {n,m})
    int16_t pair_offset; // from ( or ), offset in token list to matching paren. TODO: move into mask maybe
} RegexToken;

static int remimu_nibble_hex_to_bin(char hex, uint8_t *bin)
{
    if (hex >= '0' && hex <= '9')
    {
        *bin = hex - '0';
        return 0;
    }
    if (hex >= 'A' && hex <= 'F')
    {
        *bin = hex - 'A' + 10;
        return 0;
    }
    if (hex >= 'a' && hex <= 'f')
    {
        *bin = hex - 'a' + 10;
        return 0;
    }
    return -1; // invalid hex digit
}

/// Returns a negative number on failure:
/// -1: Regex string is invalid or using unsupported features or too long.
/// -2: Provided buffer not long enough. Give up, or reallocate with more length and retry.
/// Returns 0 on success.
/// On call, token_count pointer must point to the number of tokens that can be written to the tokens buffer.
/// On successful return, the number of actually used tokens is written to token_count.
/// Sets token_count to zero if a regex is not created but no error happened (e.g. empty pattern).
/// Flags: Not yet used.
/// SAFETY: Pattern must be null-terminated.
/// SAFETY: tokens buffer must have at least the input token_count number of RegexToken objects. They are allowed to be uninitialized.
REMIMU_FUNC_VISIBILITY int regex_parse(const char * pattern, RegexToken * tokens, int16_t * token_count, int32_t flags)
{
    int64_t tokens_len = *token_count;
    uint64_t pattern_len = strlen(pattern);
    if (token_count == 0)
        return -2;

    // 0: normal
    // 1: just saw a backslash
    int esc_state = 0;

    // 0: init
    // 1: normal
    // 2: in char class, initial state
    // 3: in char class, but possibly looking for a range marker
    // 4: in char class, but just saw a range marker
    // 5: immediately after quantifiable token
    // 6: immediately after quantifier

    const int STATE_NORMAL    = 1;
    const int STATE_QUANT     = 2;
    const int STATE_MODE      = 3;
    const int STATE_CC_INIT   = 4;
    const int STATE_CC_NORMAL = 5;
    const int STATE_CC_RANGE  = 6;
    int state = STATE_NORMAL;

    int char_class_mem = -1;

    RegexToken token;

    #define _REGEX_CLEAR_TOKEN() do { \
        memset(&token, 0, sizeof(RegexToken)); \
        token.count_lo = 1; \
        token.count_hi = 2; \
    } while(0)

    _REGEX_CLEAR_TOKEN();

    #define _REGEX_DO_INVERT() do { \
        for (int n = 0; n < 16; n++) \
            token.mask[n] = ~token.mask[n]; \
        token.mode &= ~REMIMU_MODE_INVERTED; \
    } while (0)

    int16_t k = 0;

    #define _REGEX_PUSH_TOKEN() do { \
        if (k == 0 || tokens[k-1].kind != token.kind || (token.kind != REMIMU_KIND_BOUND && token.kind != REMIMU_KIND_NBOUND)) \
        { \
            if (token.mode & REMIMU_MODE_INVERTED) _REGEX_DO_INVERT(); \
            if (k >= tokens_len) \
            { \
                return -2; \
            } \
            tokens[k++] = token; \
            _REGEX_CLEAR_TOKEN(); \
        } \
    } while (0)

    #define _REGEX_SET_MASK(byte) do { token.mask[((uint8_t)(byte))>>4] |= 1 << ((uint8_t)(byte) & 0xF); } while (0)
    #define _REGEX_SET_MASK_ALL() do { \
        for (int n = 0; n < 16; n++) \
            token.mask[n] = 0xFFFF; \
    } while (0)

    // start with an invisible group specifier
    // (this allows the matcher to not need to have a special root-level alternation operator case)
    token.kind = REMIMU_KIND_OPEN;
    token.count_lo = 0;
    token.count_hi = 0;

    int paren_count = 0;

    for (uint64_t i = 0; i < pattern_len; i++)
    {
        char c = pattern[i];
        if (state == STATE_QUANT)
        {
            state = STATE_MODE;
            if (c == '?')
            {
                token.count_lo = 0;
                token.count_hi = 2; // first non-allowed amount
                continue;
            }
            else if (c == '+')
            {
                token.count_lo = 1;
                token.count_hi = 0; // unlimited
                continue;
            }
            else if (c == '*')
            {
                token.count_lo = 0;
                token.count_hi = 0; // unlimited
                continue;
            }
            else if (c == '{')
            {
                if (pattern[i+1] == 0 || pattern[i+1] < '0' || pattern[i+1] > '9')
                    state = STATE_NORMAL;
                else
                {
                    i += 1;
                    uint32_t val = 0;
                    while (pattern[i] >= '0' && pattern[i] <= '9')
                    {
                        val *= 10;
                        val += (uint32_t)(pattern[i] - '0');
                        if (val > 0xFFFF)
                        {
                            return -1; // unsupported length
                        }
                        i += 1;
                    }
                    token.count_lo = val;
                    token.count_hi = val + 1;
                    if (pattern[i] == ',')
                    {
                        token.count_hi = 0; // unlimited
                        i += 1;

                        if (pattern[i] >= '0' && pattern[i] <= '9')
                        {
                            uint32_t val2 = 0;
                            while (pattern[i] >= '0' && pattern[i] <= '9')
                            {
                                val2 *= 10;
                                val2 += (uint32_t)(pattern[i] - '0');
                                if (val2 > 0xFFFF)
                                {
                                    return -1; // unsupported length
                                }
                                i += 1;
                            }
                            if (val2 < val)
                            {
                                return -1; // unsupported length
                            }
                            token.count_hi = val2 + 1;
                        }
                    }

                    if (pattern[i] == '}')
                    {
                        // quantifier range parsed successfully
                        continue;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }

        if (state == STATE_MODE)
        {
            state = STATE_NORMAL;
            if (c == '?')
            {
                token.mode |= REMIMU_MODE_LAZY;
                continue;
            }
            else if (c == '+')
            {
                token.mode |= REMIMU_MODE_POSSESSIVE;
                continue;
            }
        }

        if (state == STATE_NORMAL)
        {
            if (esc_state == 1)
            {
                esc_state = 0;
                if (c == 'n')
                    _REGEX_SET_MASK('\n');
                else if (c == 'r')
                    _REGEX_SET_MASK('\r');
                else if (c == 't')
                    _REGEX_SET_MASK('\t');
                else if (c == 'v')
                    _REGEX_SET_MASK('\v');
                else if (c == 'f')
                    _REGEX_SET_MASK('\f');
                else if (c == 'x')
                {
                    if (pattern[i+1] == 0 || pattern[i+2] == 0)
                        return -1; // too-short hex pattern
                    uint8_t n0, n1;
                    if (remimu_nibble_hex_to_bin(pattern[i+1], &n0))
                        return -1; // invalid hex
                    if (remimu_nibble_hex_to_bin(pattern[i+2], &n1))
                        return -1; // invalid hex
                    _REGEX_SET_MASK((n0 << 4) | n1);
                    i += 2;
                    state = STATE_QUANT;
                }
                else if (c == '{' || c == '}' ||
                         c == '[' || c == ']' || c == '-' ||
                         c == '(' || c == ')' ||
                         c == '|' || c == '^' || c == '$' ||
                         c == '*' || c == '+' || c == '?' || c == ':' ||
                         c == '.' || c == '/' || c == '\\')
                {
                    _REGEX_SET_MASK(c);
                    state = STATE_QUANT;
                }
                else if (c == 'd' || c == 's' || c == 'w' ||
                         c == 'D' || c == 'S' || c == 'W')
                {
                    uint8_t is_upper = c <= 'Z';

                    uint16_t m[16];
                    memset(m, 0, sizeof(m));

                    if (is_upper)
                        c += 0x20;
                    if (c == 'd' || c == 'w')
                        m[3] |= 0x03FF; // 0~7
                    if (c == 's')
                    {
                        m[0] |= 0x3E00; // \t-\r (includes \n, \v, and \f in the middle. 5 enabled bits.)
                        m[2] |= 1; // ' '
                    }
                    if (c == 'w')
                    {
                        m[4] |= 0xFFFE; // A-O
                        m[5] |= 0x87FF; // P-Z_
                        m[6] |= 0xFFFE; // a-o
                        m[7] |= 0x07FF; // p-z
                    }

                    for (int j = 0; j < 16; j++)
                        token.mask[j] |= is_upper ? ~m[j] : m[j];

                    token.kind = REMIMU_KIND_NORMAL;
                    state = STATE_QUANT;
                }
                else if (c == 'b')
                {
                    token.kind = REMIMU_KIND_BOUND;
                    state = STATE_NORMAL;
                }
                else if (c == 'B')
                {
                    token.kind = REMIMU_KIND_NBOUND;
                    state = STATE_NORMAL;
                }
                else
                {
                    return -1; // unknown/unsupported escape sequence
                }
            }
            else
            {
                _REGEX_PUSH_TOKEN();
                if (c == '\\')
                {
                    esc_state = 1;
                }
                else if (c == '[')
                {
                    state = STATE_CC_INIT;
                    char_class_mem = -1;
                    token.kind = REMIMU_KIND_NORMAL;
                    if (pattern[i + 1] == '^')
                    {
                        token.mode |= REMIMU_MODE_INVERTED;
                        i += 1;
                    }
                }
                else if (c == '(')
                {
                    paren_count += 1;
                    state = STATE_NORMAL;
                    token.kind = REMIMU_KIND_OPEN;
                    token.count_lo = 0;
                    token.count_hi = 1;
                    if (pattern[i + 1] == '?' && pattern[i + 2] == ':')
                    {
                        token.kind = REMIMU_KIND_NCOPEN;
                        i += 2;
                    }
                    else if (pattern[i + 1] == '?' && pattern[i + 2] == '>')
                    {
                        token.kind = REMIMU_KIND_NCOPEN;
                        _REGEX_PUSH_TOKEN();

                        state = STATE_NORMAL;
                        token.kind = REMIMU_KIND_NCOPEN;
                        token.mode = REMIMU_MODE_POSSESSIVE;
                        token.count_lo = 1;
                        token.count_hi = 2;

                        i += 2;
                    }
                }
                else if (c == ')')
                {
                    paren_count -= 1;
                    if (paren_count < 0 || k == 0)
                        return -1; // unbalanced parens
                    token.kind = REMIMU_KIND_CLOSE;
                    state = STATE_QUANT;

                    int balance = 0;
                    ptrdiff_t found = -1;
                    for (ptrdiff_t l = k - 1; l >= 0; l--)
                    {
                        if (tokens[l].kind == REMIMU_KIND_NCOPEN || tokens[l].kind == REMIMU_KIND_OPEN)
                        {
                            if (balance == 0)
                            {
                                found = l;
                                break;
                            }
                            else
                                balance -= 1;
                        }
                        else if (tokens[l].kind == REMIMU_KIND_CLOSE)
                            balance += 1;
                    }
                    if (found == -1)
                        return -1; // unbalanced parens
                    ptrdiff_t diff = k - found;
                    if (diff > 32767)
                        return -1; // too long
                    token.pair_offset = -diff;
                    tokens[found].pair_offset = diff;
                    // phantom group for atomic group emulation
                    if (tokens[found].mode == REMIMU_MODE_POSSESSIVE)
                    {
                        _REGEX_PUSH_TOKEN();
                        token.kind = REMIMU_KIND_CLOSE;
                        token.mode = REMIMU_MODE_POSSESSIVE;
                        token.pair_offset = -diff - 2;
                        tokens[found - 1].pair_offset = diff + 2;
                    }
                }
                else if (c == '?' || c == '+' || c == '*' || c == '{')
                {
                    return -1; // quantifier in non-quantifier context
                }
                else if (c == '.')
                {
                    _REGEX_SET_MASK_ALL();
                    if (flags & REMIMU_FLAG_DOT_NO_NEWLINES)
                    {
                        token.mask[1] ^= 0x04; // \n
                        token.mask[1] ^= 0x20; // \r
                    }
                    state = STATE_QUANT;
                }
                else if (c == '^')
                {
                    token.kind = REMIMU_KIND_CARET;
                    state = STATE_NORMAL;
                }
                else if (c == '$')
                {
                    token.kind = REMIMU_KIND_DOLLAR;
                    state = STATE_NORMAL;
                }
                else if (c == '|')
                {
                    token.kind = REMIMU_KIND_OR;
                    state = STATE_NORMAL;
                }
                else
                {
                    _REGEX_SET_MASK(c);
                    state = STATE_QUANT;
                }
            }
        }
        else if (state == STATE_CC_INIT || state == STATE_CC_NORMAL || state == STATE_CC_RANGE)
        {
            if (c == '\\' && esc_state == 0)
            {
                esc_state = 1;
                continue;
            }
            uint8_t esc_c = 0;
            if (esc_state == 1)
            {
                esc_state = 0;
                if (c == 'n')
                    esc_c = '\n';
                else if (c == 'r')
                    esc_c = '\r';
                else if (c == 't')
                    esc_c = '\t';
                else if (c == 'v')
                    esc_c = '\v';
                else if (c == 'f')
                    esc_c = '\f';
                else if (c == 'x')
                {
                    if (pattern[i+1] == 0 || pattern[i+2] == 0)
                        return -1; // too-short hex pattern
                    uint8_t n0, n1;
                    if (remimu_nibble_hex_to_bin(pattern[i+1], &n0))
                        return -1; // invalid hex
                    if (remimu_nibble_hex_to_bin(pattern[i+2], &n1))
                        return -1; // invalid hex
                    esc_c = (n0 << 4) | n1;
                    i += 2;
                }
                else if (c == '{' || c == '}' ||
                         c == '[' || c == ']' || c == '-' ||
                         c == '(' || c == ')' ||
                         c == '|' || c == '^' || c == '$' ||
                         c == '*' || c == '+' || c == '?' || c == ':' ||
                         c == '.' || c == '/' || c == '\\')
                {
                    esc_c = c;
                }
                else if (c == 'd' || c == 's' || c == 'w' ||
                         c == 'D' || c == 'S' || c == 'W')
                {
                    if (state == STATE_CC_RANGE)
                    {
                        return -1; // range shorthands can't be part of a range
                    }
                    uint8_t is_upper = c <= 'Z';

                    uint16_t m[16];
                    memset(m, 0, sizeof(m));

                    if (is_upper)
                        c += 0x20;
                    if (c == 'd' || c == 'w')
                        m[3] |= 0x03FF; // 0~7
                    if (c == 's')
                    {
                        m[0] |= 0x3E00; // \t-\r (includes \n, \v, and \f in the middle. 5 enabled bits.)
                        m[2] |= 1; // ' '
                    }
                    if (c == 'w')
                    {
                        m[4] |= 0xFFFE; // A-O
                        m[5] |= 0x87FF; // P-Z_
                        m[6] |= 0xFFFE; // a-o
                        m[7] |= 0x07FF; // p-z
                    }

                    for (int j = 0; j < 16; j++)
                        token.mask[j] |= is_upper ? ~m[j] : m[j];

                    char_class_mem = -1; // range shorthands can't be part of a range
                    continue;
                }
                else
                {
                    return -1; // unknown/unsupported escape sequence
                }
            }
            if (state == STATE_CC_INIT)
            {
                uint8_t val = esc_c ? esc_c : (uint8_t)c;
                char_class_mem = val;
                _REGEX_SET_MASK(val);
                state = STATE_CC_NORMAL;
            }
            else if (state == STATE_CC_NORMAL)
            {
                if (c == ']' && esc_c == 0)
                {
                    char_class_mem = -1;
                    state = STATE_QUANT;
                    continue;
                }
                else if (c == '-' && esc_c == 0 && char_class_mem >= 0)
                {
                    state = STATE_CC_RANGE;
                    continue;
                }
                else
                {
                    uint8_t val = esc_c ? esc_c : (uint8_t)c;
                    char_class_mem = val;
                    _REGEX_SET_MASK(val);
                    state = STATE_CC_NORMAL;
                }
            }
            else if (state == STATE_CC_RANGE)
            {
                if (c == ']' && esc_c == 0)
                {
                    char_class_mem = -1;
                    _REGEX_SET_MASK('-');
                    state = STATE_QUANT;
                    continue;
                }
                else
                {
                    if (char_class_mem == -1)
                    {
                        return -1; // probably tried to use a character class shorthand as part of a range
                    }
                    uint8_t rhs = esc_c ? esc_c : (uint8_t)c;
                    if (rhs < (uint8_t)char_class_mem)
                    {
                        return -1; // range is in wrong order
                    }
                    for (uint8_t j = rhs; j > (uint8_t)char_class_mem; j--)
                        _REGEX_SET_MASK(j);
                    state = STATE_CC_NORMAL;
                    char_class_mem = -1;
                }
            }
        }
        else
            REMIMU_ASSERT(0);
    }
    if (paren_count > 0)
    {
        return -1; // unbalanced parens
    }
    if (esc_state != 0)
    {
        return -1; // open escape sequence
    }
    if (state >= STATE_CC_INIT)
    {
        return -1; // open character class
    }

    _REGEX_PUSH_TOKEN();

    // add invisible non-capturing group specifier
    token.kind = REMIMU_KIND_CLOSE;
    token.count_lo = 1;
    token.count_hi = 2;
    _REGEX_PUSH_TOKEN();

    // add end token (tells matcher that it's done)
    token.kind = REMIMU_KIND_END;
    _REGEX_PUSH_TOKEN();

    tokens[0].pair_offset = k - 2;
    tokens[k-2].pair_offset = -(k - 2);

    *token_count = k;

    // copy quantifiers from )s to (s (so (s know whether they're optional)
    // also take the opportunity to smuggle "quantified group index" into the mask field for the )
    uint64_t n = 0;
    for (int16_t k2 = 0; k2 < k; k2++)
    {
        if (tokens[k2].kind == REMIMU_KIND_CLOSE)
        {
            tokens[k2].mask[0] = n++;

            int16_t k3 = k2 + tokens[k2].pair_offset;
            tokens[k3].count_lo = tokens[k2].count_lo;
            tokens[k3].count_hi = tokens[k2].count_hi;
            tokens[k3].mask[0] = n++;
            tokens[k3].mode = tokens[k2].mode;

            //if (n > 65535)
            if (n > 1024)
                return -1; // too many quantified groups
        }
        else if (tokens[k2].kind == REMIMU_KIND_OR || tokens[k2].kind == REMIMU_KIND_OPEN || tokens[k2].kind == REMIMU_KIND_NCOPEN)
        {
            // find next | or ) and how far away it is. store in token
            int balance = 0;
            ptrdiff_t found = -1;
            for (ptrdiff_t l = k2 + 1; l < k; l++)
            {
                if (tokens[l].kind == REMIMU_KIND_OR && balance == 0)
                {
                    found = l;
                    break;
                }
                else if (tokens[l].kind == REMIMU_KIND_CLOSE)
                {
                    if (balance == 0)
                    {
                        found = l;
                        break;
                    }
                    else
                        balance -= 1;
                }
                else if (tokens[l].kind == REMIMU_KIND_NCOPEN || tokens[l].kind == REMIMU_KIND_OPEN)
                    balance += 1;
            }
            if (found == -1)
            {
                return -1; // unbalanced parens
            }
            ptrdiff_t diff = found - k2;
            if (diff > 32767)
            {
                return -1; // too long
            }

            if (tokens[k2].kind == REMIMU_KIND_OR)
                tokens[k2].pair_offset = diff;
            else
                tokens[k2].mask[15] = diff;
        }
    }

    #undef _REGEX_PUSH_TOKEN
    #undef _REGEX_SET_MASK
    #undef _REGEX_CLEAR_TOKEN

    return 0;
}

typedef struct _RegexMatcherState {
    uint32_t k;
    uint32_t group_state; // quantified group temp state (e.g. number of repetitions)
    uint32_t prev; // for )s, stack index of corresponding previous quantified state
#ifdef REGEX_STACK_SMOL
    uint32_t i;
    uint32_t range_min;
    uint32_t range_max;
#else
    uint64_t i;
    uint64_t range_min;
    uint64_t range_max;
#endif
} RegexMatcherState;

// NOTE: undef'd later
#define _REGEX_CHECK_MASK(K, byte) (!!(tokens[K].mask[((uint8_t)byte)>>4] & (1 << ((uint8_t)byte & 0xF))))

// Returns match length if text starts with a regex match.
// Returns -1 if the text doesn't start with a regex match.
// Returns -2 if the matcher ran out of memory or the regex is too complex.
// Returns -3 if the regex is somehow invalid.
// The first cap_slots capture positions and spans (lengths) will be written to cap_pos and cap_span. If zero, will not be written to.
// SAFETY: The text variable must be null-terminated, and start_i must be the index of a character within the string or its null terminator.
// SAFETY: Tokens array must be terminated by a REMIMU_KIND_END token (done by default by regex_parse).
// SAFETY: Partial capture data may be written even if the match fails.
REMIMU_FUNC_VISIBILITY int64_t regex_match(const RegexToken * tokens, const char * text, size_t start_i, uint16_t cap_slots, int64_t * cap_pos, int64_t * cap_span)
{
    (void)text;

#define IF_VERBOSE(X) { if (verbose) { X } }

#ifdef REGEX_STACK_SMOL
    const uint16_t stack_size_max = 256;
#else
    const uint16_t stack_size_max = 1024;
#endif
    const uint16_t aux_stats_size = 1024;
    if (cap_slots > aux_stats_size)
        cap_slots = aux_stats_size;

    // quantified group state
    uint8_t q_group_accepts_zero[aux_stats_size];
    uint32_t q_group_state[aux_stats_size]; // number of repetitions
    uint32_t q_group_stack[aux_stats_size]; // location of most recent corresponding ) on stack. 0 means nowhere

    uint16_t q_group_cap_index[aux_stats_size];
    memset(q_group_cap_index, 0xFF, sizeof(q_group_cap_index));

    uint64_t tokens_len = 0;
    uint32_t k = 0;
    uint16_t caps = 0;

    while (tokens[k].kind != REMIMU_KIND_END)
    {
        if (tokens[k].kind == REMIMU_KIND_OPEN && caps < cap_slots)
        {
            q_group_cap_index[tokens[k].mask[0]] = caps;
            q_group_cap_index[tokens[k + tokens[k].pair_offset].mask[0]] = caps;
            cap_pos[caps] = -1;
            cap_span[caps] = -1;
            caps += 1;
        }
        k += 1;
        if (tokens[k].kind == REMIMU_KIND_CLOSE || tokens[k].kind == REMIMU_KIND_OPEN || tokens[k].kind == REMIMU_KIND_NCOPEN)
        {
            if (tokens[k].mask[0] >= aux_stats_size)
            {
                return -2; // OOM: too many quantified groups
            }

            q_group_state[tokens[k].mask[0]] = 0;
            q_group_stack[tokens[k].mask[0]] = 0;
            q_group_accepts_zero[tokens[k].mask[0]] = 0;
        }
    }

    tokens_len = k;

    RegexMatcherState rewind_stack[stack_size_max];
    uint16_t stack_n = 0;

    uint64_t i = start_i;

    uint64_t range_min = 0;
    uint64_t range_max = 0;
    uint8_t just_rewinded = 0;

    #define _P_TEXT_HIGHLIGHTED() do { \
    } while (0)

    #define _REWIND_DO_SAVE_RAW(K, ISDUMMY) do { \
        if (stack_n >= stack_size_max) \
        { \
            return -2; \
        } \
        RegexMatcherState s; \
        memset(&s, 0, sizeof(RegexMatcherState)); \
        s.i = i; \
        s.k = (K); \
        s.range_min = range_min; \
        s.range_max = range_max; \
        s.prev = 0; \
        if (ISDUMMY) s.prev = 0xFAC7; \
        else if (tokens[s.k].kind == REMIMU_KIND_CLOSE) \
        { \
            s.group_state = q_group_state[tokens[s.k].mask[0]]; \
            s.prev = q_group_stack[tokens[s.k].mask[0]]; \
            q_group_stack[tokens[s.k].mask[0]] = stack_n; \
        } \
        rewind_stack[stack_n++] = s; \
        _P_TEXT_HIGHLIGHTED(); \
    } while (0)
    #define _REWIND_DO_SAVE_DUMMY(K) _REWIND_DO_SAVE_RAW(K, 1)
    #define _REWIND_DO_SAVE(K) _REWIND_DO_SAVE_RAW(K, 0)

    #define _REWIND_OR_ABORT() do { \
        if (stack_n == 0) \
            return -1; \
        stack_n -= 1; \
        while (stack_n > 0 && rewind_stack[stack_n].prev == 0xFAC7) stack_n -= 1; \
        just_rewinded = 1; \
        range_min = rewind_stack[stack_n].range_min; \
        range_max = rewind_stack[stack_n].range_max; \
        REMIMU_ASSERT(rewind_stack[stack_n].i <= i); \
        i = rewind_stack[stack_n].i; \
        k = rewind_stack[stack_n].k; \
        if (tokens[k].kind == REMIMU_KIND_CLOSE) \
        { \
            q_group_state[tokens[k].mask[0]] = rewind_stack[stack_n].group_state; \
            q_group_stack[tokens[k].mask[0]] = rewind_stack[stack_n].prev; \
        } \
        _P_TEXT_HIGHLIGHTED(); \
        k -= 1; \
    } while (0)
    // the -= 1 is because of the k++ in the for loop

    // used in boundary anchor checker
    uint64_t w_mask[16];
    memset(w_mask, 0, sizeof(w_mask));
    w_mask[3] = 0x03FF;
    w_mask[4] = 0xFFFE;
    w_mask[5] = 0x87FF;
    w_mask[6] = 0xFFFE;
    w_mask[7] = 0x07FF;
    #define _REGEX_CHECK_IS_W(byte) (!!(w_mask[((uint8_t)byte)>>4] & (1 << ((uint8_t)byte & 0xF))))

    int limit = REMIMU_ITERATION_LIMIT;
    for (k = 0; k < tokens_len; k++)
    {
        if (REMIMU_ITERATION_LIMIT)
        {
            if (limit-- == 0)
            {
                return -2;
            }
        }
        _P_TEXT_HIGHLIGHTED();
        if (tokens[k].kind == REMIMU_KIND_CARET)
        {
            if (i != 0)
                _REWIND_OR_ABORT();
            continue;
        }
        else if (tokens[k].kind == REMIMU_KIND_DOLLAR)
        {
            if (text[i] != 0)
                _REWIND_OR_ABORT();
            continue;
        }
        else if (tokens[k].kind == REMIMU_KIND_BOUND)
        {
            if (i == 0 && !_REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] == 0 && !_REGEX_CHECK_IS_W(text[i-1]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] != 0 && _REGEX_CHECK_IS_W(text[i-1]) == _REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
        }
        else if (tokens[k].kind == REMIMU_KIND_NBOUND)
        {
            if (i == 0 && _REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] == 0 && _REGEX_CHECK_IS_W(text[i-1]))
                _REWIND_OR_ABORT();
            else if (i != 0 && text[i] != 0 && _REGEX_CHECK_IS_W(text[i-1]) != _REGEX_CHECK_IS_W(text[i]))
                _REWIND_OR_ABORT();
        }
        else
        {
            // deliberately unmatchable token (e.g. a{0}, a{0,0})
            if (tokens[k].count_hi == 1)
            {
                if (tokens[k].kind == REMIMU_KIND_OPEN || tokens[k].kind == REMIMU_KIND_NCOPEN)
                    k += tokens[k].pair_offset;
                else
                    k += 1;
                continue;
            }

            if (tokens[k].kind == REMIMU_KIND_OPEN || tokens[k].kind == REMIMU_KIND_NCOPEN)
            {
                if (!just_rewinded)
                {
                    // need this to be able to detect and reject zero-size matches
                    //q_group_state[tokens[k].mask[0]] = i;

                    // if we're lazy and the min length is 0, we need to try the non-group case first
                    if ((tokens[k].mode & REMIMU_MODE_LAZY) && (tokens[k].count_lo == 0 || q_group_accepts_zero[tokens[k + tokens[k].pair_offset].mask[0]]))
                    {
                        range_min = 0;
                        range_max = 0;
                        _REWIND_DO_SAVE(k);
                        k += tokens[k].pair_offset; // automatic += 1 will put us past the matching )
                    }
                    else
                    {
                        range_min = 1;
                        range_max = 0;
                        _REWIND_DO_SAVE(k);
                    }
                }
                else
                {
                    just_rewinded = 0;

                    uint64_t orig_k = k;


                    if (range_min != 0)
                    {
                        k += range_min;
                        if (tokens[k-1].kind == REMIMU_KIND_OR)
                            k += tokens[k-1].pair_offset - 1;
                        else if (tokens[k-1].kind == REMIMU_KIND_OPEN || tokens[k-1].kind == REMIMU_KIND_NCOPEN)
                            k += tokens[k-1].mask[15] - 1;

                        if (tokens[k].kind == REMIMU_KIND_END) // unbalanced parens
                            return -3;

                        if (tokens[k].kind == REMIMU_KIND_CLOSE)
                        {
                            // do nothing and continue on if we don't need this group
                            if (tokens[k].count_lo == 0 || q_group_accepts_zero[tokens[k].mask[0]])
                            {
                                q_group_state[tokens[k].mask[0]] = 0;

                                if (!(tokens[k].mode & REMIMU_MODE_LAZY))
                                    q_group_stack[tokens[k].mask[0]] = 0;

                                continue;
                            }
                            // otherwise go to the last point before the group
                            else
                            {
                                _REWIND_OR_ABORT();
                                continue;
                            }
                        }

                        REMIMU_ASSERT(tokens[k].kind == REMIMU_KIND_OR);
                    }

                    ptrdiff_t k_diff = k - orig_k;
                    range_min = k_diff + 1;

                    _REWIND_DO_SAVE(k - k_diff);
                }
            }
            else if (tokens[k].kind == REMIMU_KIND_CLOSE)
            {
                // unquantified
                if (tokens[k].count_lo == 1 && tokens[k].count_hi == 2)
                {
                    // for captures
                    uint16_t cap_index = q_group_cap_index[tokens[k].mask[0]];
                    if (cap_index != 0xFFFF)
                        _REWIND_DO_SAVE_DUMMY(k);
                }
                // quantified
                else
                {
                    if (!just_rewinded)
                    {
                        uint32_t prev = q_group_stack[tokens[k].mask[0]];

                        range_max = tokens[k].count_hi;
                        range_max -= 1;
                        range_min = q_group_accepts_zero[tokens[k].mask[0]] ? 0 : tokens[k].count_lo;
                        //REMIMU_ASSERT(q_group_state[tokens[k + tokens[k].pair_offset].mask[0]] <= i);
                        //if (prev) REMIMU_ASSERT(rewind_stack[prev].i <= i);

                        // minimum requirement not yet met
                        if (q_group_state[tokens[k].mask[0]] + 1 < range_min)
                        {
                            q_group_state[tokens[k].mask[0]] += 1;
                            _REWIND_DO_SAVE(k);

                            k += tokens[k].pair_offset; // back to start of group
                            k -= 1; // ensure we actually hit the group node next and not the node after it
                            continue;
                        }
                        // maximum allowance exceeded
                        else if (tokens[k].count_hi != 0 && q_group_state[tokens[k].mask[0]] + 1 > range_max)
                        {
                            range_max -= 1;
                            _REWIND_OR_ABORT();
                            continue;
                        }

                        // fallback case to detect zero-length matches when we backtracked into the inside of this group
                        // after an attempted parse of a second copy of itself
                        uint8_t force_zero = 0;
                        if (prev != 0 && rewind_stack[prev].i > i)
                        {
                            // find matching open paren
                            size_t n = stack_n - 1;
                            while (n > 0 && rewind_stack[n].k != k + tokens[k].pair_offset)
                                n -= 1;
                            REMIMU_ASSERT(n > 0);
                            if (rewind_stack[n].i == i)
                                force_zero = 1;
                        }

                        // reject zero-length matches
                        if ((force_zero || (prev != 0 && rewind_stack[prev].i == i))) //  && q_group_state[tokens[k].mask[0]] > 0
                        {
                            q_group_accepts_zero[tokens[k].mask[0]] = 1;
                            _REWIND_OR_ABORT();
                            //range_max = q_group_state[tokens[k].mask[0]];
                            //range_min = 0;
                        }
                        else if (tokens[k].mode & REMIMU_MODE_LAZY) // lazy
                        {
                            // continue on to past the group; group retry is in rewind state
                            q_group_state[tokens[k].mask[0]] += 1;
                            _REWIND_DO_SAVE(k);
                            q_group_state[tokens[k].mask[0]] = 0;
                        }
                        else // greedy
                        {
                            // clear unwanted memory if possessive
                            if ((tokens[k].mode & REMIMU_MODE_POSSESSIVE))
                            {
                                uint32_t k2 = k;

                                // special case for first, only rewind to (, not to )
                                if (q_group_state[tokens[k].mask[0]] == 0)
                                    k2 = k + tokens[k].pair_offset;

                                if (stack_n == 0)
                                    return -1;
                                stack_n -= 1;

                                while (stack_n > 0 && rewind_stack[stack_n].k != k2)
                                    stack_n -= 1;

                                if (stack_n == 0)
                                    return -1;
                            }
                            // continue to next match if sane
                            if ((uint32_t)q_group_state[tokens[k + tokens[k].pair_offset].mask[0]] < (uint32_t)i)
                            {
                                q_group_state[tokens[k].mask[0]] += 1;
                                _REWIND_DO_SAVE(k);
                                k += tokens[k].pair_offset; // back to start of group
                                k -= 1; // ensure we actually hit the group node next and not the node after it
                            }
                        }
                    }
                    else
                    {
                        just_rewinded = 0;

                        if (tokens[k].mode & REMIMU_MODE_LAZY)
                        {
                            // lazy rewind: need to try matching the group again
                            _REWIND_DO_SAVE_DUMMY(k);
                            q_group_stack[tokens[k].mask[0]] = stack_n;
                            k += tokens[k].pair_offset; // back to start of group
                            k -= 1; // ensure we actually hit the group node next and not the node after it
                        }
                        else
                        {
                            // greedy. if we're going to go outside the acceptable range, rewind
                            //uint64_t old_i = i;
                            if (q_group_state[tokens[k].mask[0]] < range_min && !q_group_accepts_zero[tokens[k].mask[0]])
                            {
                                //i = old_i;
                                _REWIND_OR_ABORT();
                            }
                            // otherwise continue on to past the group
                            else
                            {
                                q_group_state[tokens[k].mask[0]] = 0;

                                // for captures
                                uint16_t cap_index = q_group_cap_index[tokens[k].mask[0]];
                                if (cap_index != 0xFFFF)
                                    _REWIND_DO_SAVE_DUMMY(k);
                            }
                        }
                    }
                }
            }
            else if (tokens[k].kind == REMIMU_KIND_OR)
            {
                k += tokens[k].pair_offset;
                k -= 1;
            }
            else if (tokens[k].kind == REMIMU_KIND_NORMAL)
            {
                if (!just_rewinded)
                {
                    uint64_t n = 0;
                    // do whatever the obligatory minimum amount of matching is
                    uint64_t old_i = i;
                    while (n < tokens[k].count_lo && text[i] != 0 && _REGEX_CHECK_MASK(k, text[i]))
                    {
                        i += 1;
                        n += 1;
                    }
                    if (n < tokens[k].count_lo)
                    {
                        i = old_i;
                        _REWIND_OR_ABORT();
                        continue;
                    }

                    if (tokens[k].mode & REMIMU_MODE_LAZY)
                    {
                        range_min = n;
                        range_max = tokens[k].count_hi - 1;
                        _REWIND_DO_SAVE(k);
                    }
                    else
                    {
                        uint64_t ilimit = tokens[k].count_hi;
                        if (ilimit == 0)
                            ilimit = ~ilimit;
                        range_min = n;
                        while (text[i] != 0 && _REGEX_CHECK_MASK(k, text[i]) && n + 1 < ilimit)
                        {
                            i += 1;
                            n += 1;
                        }
                        range_max = n;
                        if (!(tokens[k].mode & REMIMU_MODE_POSSESSIVE))
                            _REWIND_DO_SAVE(k);
                    }
                }
                else
                {
                    just_rewinded = 0;

                    if (tokens[k].mode & REMIMU_MODE_LAZY)
                    {
                        uint64_t ilimit = range_max;
                        if (ilimit == 0)
                            ilimit = ~ilimit;

                        if (_REGEX_CHECK_MASK(k, text[i]) && text[i] != 0 && range_min < ilimit)
                        {
                            i += 1;
                            range_min += 1;
                            _REWIND_DO_SAVE(k);
                        }
                        else
                        {
                            _REWIND_OR_ABORT();
                        }
                    }
                    else
                    {
                        if (range_max > range_min)
                        {
                            i -= 1;
                            range_max -= 1;
                            _REWIND_DO_SAVE(k);
                        }
                        else
                        {
                            _REWIND_OR_ABORT();
                        }
                    }
                }
            }
            else
            {
                REMIMU_ASSERT(0);
            }
        }
    }

    if (caps != 0)
    {
        fflush(stdout);
        for (size_t n = 0; n < stack_n; n++)
        {
            RegexMatcherState s = rewind_stack[n];
            int kind = tokens[s.k].kind;
            if (kind == REMIMU_KIND_OPEN || kind == REMIMU_KIND_CLOSE)
            {
                uint16_t cap_index = q_group_cap_index[tokens[s.k].mask[0]];
                if (cap_index == 0xFFFF)
                    continue;
                if (tokens[s.k].kind == REMIMU_KIND_OPEN)
                    cap_pos[cap_index] = s.i;
                else if (cap_pos[cap_index] >= 0)
                    cap_span[cap_index] = s.i - cap_pos[cap_index];
            }
        }
        // re-deinitialize capture positions that have no associated capture span
        for (size_t n = 0; n < caps; n++)
        {
            if (cap_span[n] == -1)
                cap_pos[n] = -1;
        }
    }

    #undef _REWIND_DO_SAVE
    #undef _REWIND_OR_ABORT
    #undef _REGEX_CHECK_IS_W
    #undef _P_TEXT_HIGHLIGHTED
    #undef IF_VERBOSE

    return i;
}

REMIMU_FUNC_VISIBILITY void print_regex_tokens(RegexToken * tokens)
{
}

#undef _REGEX_CHECK_MASK

#endif //INCLUDE_REMIMU
