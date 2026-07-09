#include "bgime/Utf.hpp"

#include <stdexcept>

namespace bgime::utf {

namespace cp {
// Turkish characters
inline constexpr char32_t tr_s_lower = 0x015F; // ş
inline constexpr char32_t tr_s_upper = 0x015E; // Ş
inline constexpr char32_t tr_c_lower = 0x00E7; // ç
inline constexpr char32_t tr_c_upper = 0x00C7; // Ç
inline constexpr char32_t tr_i_dot_upper = 0x0130; // İ
inline constexpr char32_t tr_i_lower = 0x0131; // ı
inline constexpr char32_t tr_u_lower = 0x00FC; // ü
inline constexpr char32_t tr_u_upper = 0x00DC; // Ü
inline constexpr char32_t tr_o_lower = 0x00F6; // ö
inline constexpr char32_t tr_o_upper = 0x00D6; // Ö
inline constexpr char32_t tr_g_lower = 0x011F; // ğ
inline constexpr char32_t tr_g_upper = 0x011E; // Ğ

// Bulgarian Cyrillic lowercase
inline constexpr char32_t bg_a = 0x0430; // а
inline constexpr char32_t bg_b = 0x0431; // б
inline constexpr char32_t bg_v = 0x0432; // в
inline constexpr char32_t bg_g = 0x0433; // г
inline constexpr char32_t bg_d = 0x0434; // д
inline constexpr char32_t bg_e = 0x0435; // е
inline constexpr char32_t bg_zh = 0x0436; // ж
inline constexpr char32_t bg_z = 0x0437; // з
inline constexpr char32_t bg_i = 0x0438; // и
inline constexpr char32_t bg_y = 0x0439; // й
inline constexpr char32_t bg_k = 0x043A; // к
inline constexpr char32_t bg_l = 0x043B; // л
inline constexpr char32_t bg_m = 0x043C; // м
inline constexpr char32_t bg_n = 0x043D; // н
inline constexpr char32_t bg_o = 0x043E; // о
inline constexpr char32_t bg_p = 0x043F; // п
inline constexpr char32_t bg_r = 0x0440; // р
inline constexpr char32_t bg_s = 0x0441; // с
inline constexpr char32_t bg_t = 0x0442; // т
inline constexpr char32_t bg_u = 0x0443; // у
inline constexpr char32_t bg_f = 0x0444; // ф
inline constexpr char32_t bg_h = 0x0445; // х
inline constexpr char32_t bg_ts = 0x0446; // ц
inline constexpr char32_t bg_ch = 0x0447; // ч
inline constexpr char32_t bg_sh = 0x0448; // ш
inline constexpr char32_t bg_sht = 0x0449; // щ
inline constexpr char32_t bg_hard = 0x044A; // ъ
inline constexpr char32_t bg_soft = 0x044C; // ь
inline constexpr char32_t bg_yu = 0x044E; // ю
inline constexpr char32_t bg_ya = 0x044F; // я

// Bulgarian Cyrillic uppercase
inline constexpr char32_t BG_A = 0x0410; // А
inline constexpr char32_t BG_B = 0x0411; // Б
inline constexpr char32_t BG_V = 0x0412; // В
inline constexpr char32_t BG_G = 0x0413; // Г
inline constexpr char32_t BG_D = 0x0414; // Д
inline constexpr char32_t BG_E = 0x0415; // Е
inline constexpr char32_t BG_ZH = 0x0416; // Ж
inline constexpr char32_t BG_Z = 0x0417; // З
inline constexpr char32_t BG_I = 0x0418; // И
inline constexpr char32_t BG_Y = 0x0419; // Й
inline constexpr char32_t BG_K = 0x041A; // К
inline constexpr char32_t BG_L = 0x041B; // Л
inline constexpr char32_t BG_M = 0x041C; // М
inline constexpr char32_t BG_N = 0x041D; // Н
inline constexpr char32_t BG_O = 0x041E; // О
inline constexpr char32_t BG_P = 0x041F; // П
inline constexpr char32_t BG_R = 0x0420; // Р
inline constexpr char32_t BG_S = 0x0421; // С
inline constexpr char32_t BG_T = 0x0422; // Т
inline constexpr char32_t BG_U = 0x0423; // У
inline constexpr char32_t BG_F = 0x0424; // Ф
inline constexpr char32_t BG_H = 0x0425; // Х
inline constexpr char32_t BG_TS = 0x0426; // Ц
inline constexpr char32_t BG_CH = 0x0427; // Ч
inline constexpr char32_t BG_SH = 0x0428; // Ш
inline constexpr char32_t BG_SHT = 0x0429; // Щ
inline constexpr char32_t BG_HARD = 0x042A; // Ъ
inline constexpr char32_t BG_SOFT = 0x042C; // Ь
inline constexpr char32_t BG_YU = 0x042E; // Ю
inline constexpr char32_t BG_YA = 0x042F; // Я
} // namespace cp

std::u32string utf8ToU32(std::string_view input) {
    std::u32string output;
    output.reserve(input.size());

    for (std::size_t i = 0; i < input.size();) {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        char32_t codepoint = 0;
        std::size_t extra = 0;

        if ((c & 0x80) == 0) {
            codepoint = c;
            extra = 0;
        } else if ((c & 0xE0) == 0xC0) {
            codepoint = c & 0x1F;
            extra = 1;
        } else if ((c & 0xF0) == 0xE0) {
            codepoint = c & 0x0F;
            extra = 2;
        } else if ((c & 0xF8) == 0xF0) {
            codepoint = c & 0x07;
            extra = 3;
        } else {
            throw std::runtime_error("Invalid UTF-8 sequence");
        }

        if (i + extra >= input.size()) {
            throw std::runtime_error("Truncated UTF-8 sequence");
        }

        for (std::size_t j = 1; j <= extra; ++j) {
            const unsigned char cc = static_cast<unsigned char>(input[i + j]);
            if ((cc & 0xC0) != 0x80) {
                throw std::runtime_error("Invalid UTF-8 continuation byte");
            }
            codepoint = (codepoint << 6) | (cc & 0x3F);
        }

        output.push_back(codepoint);
        i += extra + 1;
    }

    return output;
}

std::string u32ToUtf8(std::u32string_view input) {
    std::string output;

    for (char32_t cpv : input) {
        if (cpv <= 0x7F) {
            output.push_back(static_cast<char>(cpv));
        } else if (cpv <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | ((cpv >> 6) & 0x1F)));
            output.push_back(static_cast<char>(0x80 | (cpv & 0x3F)));
        } else if (cpv <= 0xFFFF) {
            output.push_back(static_cast<char>(0xE0 | ((cpv >> 12) & 0x0F)));
            output.push_back(static_cast<char>(0x80 | ((cpv >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (cpv & 0x3F)));
        } else if (cpv <= 0x10FFFF) {
            output.push_back(static_cast<char>(0xF0 | ((cpv >> 18) & 0x07)));
            output.push_back(static_cast<char>(0x80 | ((cpv >> 12) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | ((cpv >> 6) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (cpv & 0x3F)));
        } else {
            throw std::runtime_error("Invalid Unicode codepoint");
        }
    }

    return output;
}

char32_t toLowerChar(char32_t ch) {
    if (ch >= U'A' && ch <= U'Z') {
        return ch + 32;
    }

    switch (ch) {
        case cp::tr_s_upper: return cp::tr_s_lower;
        case cp::tr_c_upper: return cp::tr_c_lower;
        case cp::tr_i_dot_upper: return U'i';
        case U'I': return U'i';
        case cp::tr_u_upper: return cp::tr_u_lower;
        case cp::tr_o_upper: return cp::tr_o_lower;
        case cp::tr_g_upper: return cp::tr_g_lower;
        case cp::BG_A: return cp::bg_a;
        case cp::BG_B: return cp::bg_b;
        case cp::BG_V: return cp::bg_v;
        case cp::BG_G: return cp::bg_g;
        case cp::BG_D: return cp::bg_d;
        case cp::BG_E: return cp::bg_e;
        case cp::BG_ZH: return cp::bg_zh;
        case cp::BG_Z: return cp::bg_z;
        case cp::BG_I: return cp::bg_i;
        case cp::BG_Y: return cp::bg_y;
        case cp::BG_K: return cp::bg_k;
        case cp::BG_L: return cp::bg_l;
        case cp::BG_M: return cp::bg_m;
        case cp::BG_N: return cp::bg_n;
        case cp::BG_O: return cp::bg_o;
        case cp::BG_P: return cp::bg_p;
        case cp::BG_R: return cp::bg_r;
        case cp::BG_S: return cp::bg_s;
        case cp::BG_T: return cp::bg_t;
        case cp::BG_U: return cp::bg_u;
        case cp::BG_F: return cp::bg_f;
        case cp::BG_H: return cp::bg_h;
        case cp::BG_TS: return cp::bg_ts;
        case cp::BG_CH: return cp::bg_ch;
        case cp::BG_SH: return cp::bg_sh;
        case cp::BG_SHT: return cp::bg_sht;
        case cp::BG_HARD: return cp::bg_hard;
        case cp::BG_SOFT: return cp::bg_soft;
        case cp::BG_YU: return cp::bg_yu;
        case cp::BG_YA: return cp::bg_ya;
        default: return ch;
    }
}

char32_t toUpperChar(char32_t ch) {
    if (ch >= U'a' && ch <= U'z') {
        return ch - 32;
    }

    switch (ch) {
        case cp::tr_s_lower: return cp::tr_s_upper;
        case cp::tr_c_lower: return cp::tr_c_upper;
        case cp::tr_i_lower: return U'I';
        case U'i': return U'I';
        case cp::tr_u_lower: return cp::tr_u_upper;
        case cp::tr_o_lower: return cp::tr_o_upper;
        case cp::tr_g_lower: return cp::tr_g_upper;
        case cp::bg_a: return cp::BG_A;
        case cp::bg_b: return cp::BG_B;
        case cp::bg_v: return cp::BG_V;
        case cp::bg_g: return cp::BG_G;
        case cp::bg_d: return cp::BG_D;
        case cp::bg_e: return cp::BG_E;
        case cp::bg_zh: return cp::BG_ZH;
        case cp::bg_z: return cp::BG_Z;
        case cp::bg_i: return cp::BG_I;
        case cp::bg_y: return cp::BG_Y;
        case cp::bg_k: return cp::BG_K;
        case cp::bg_l: return cp::BG_L;
        case cp::bg_m: return cp::BG_M;
        case cp::bg_n: return cp::BG_N;
        case cp::bg_o: return cp::BG_O;
        case cp::bg_p: return cp::BG_P;
        case cp::bg_r: return cp::BG_R;
        case cp::bg_s: return cp::BG_S;
        case cp::bg_t: return cp::BG_T;
        case cp::bg_u: return cp::BG_U;
        case cp::bg_f: return cp::BG_F;
        case cp::bg_h: return cp::BG_H;
        case cp::bg_ts: return cp::BG_TS;
        case cp::bg_ch: return cp::BG_CH;
        case cp::bg_sh: return cp::BG_SH;
        case cp::bg_sht: return cp::BG_SHT;
        case cp::bg_hard: return cp::BG_HARD;
        case cp::bg_soft: return cp::BG_SOFT;
        case cp::bg_yu: return cp::BG_YU;
        case cp::bg_ya: return cp::BG_YA;
        default: return ch;
    }
}

std::u32string toLower(std::u32string_view input) {
    std::u32string out;
    out.reserve(input.size());
    for (char32_t ch : input) {
        out.push_back(toLowerChar(ch));
    }
    return out;
}

std::u32string toUpper(std::u32string_view input) {
    std::u32string out;
    out.reserve(input.size());
    for (char32_t ch : input) {
        out.push_back(toUpperChar(ch));
    }
    return out;
}

bool isUpperChar(char32_t ch) {
    return toLowerChar(ch) != ch && toUpperChar(toLowerChar(ch)) == ch;
}

bool isLowerChar(char32_t ch) {
    return toUpperChar(ch) != ch && toLowerChar(toUpperChar(ch)) == ch;
}

bool isAsciiLetter(char32_t ch) {
    return (ch >= U'a' && ch <= U'z') || (ch >= U'A' && ch <= U'Z');
}

bool isComposeChar(char32_t ch) {
    if (isAsciiLetter(ch)) {
        return true;
    }

    switch (ch) {
        case cp::tr_s_lower: case cp::tr_s_upper:
        case cp::tr_c_lower: case cp::tr_c_upper:
        case cp::tr_i_lower: case U'I':
        case cp::tr_i_dot_upper:
        case cp::tr_u_lower: case cp::tr_u_upper:
        case cp::tr_o_lower: case cp::tr_o_upper:
        case cp::tr_g_lower: case cp::tr_g_upper:
        case U'`': case U'\'':
            return true;
        default:
            return false;
    }
}

bool isCommitSeparator(char32_t ch) {
    return ch == U' ' || ch == U'\n' || ch == U'\t' ||
           ch == U'.' || ch == U',' || ch == U'!' || ch == U'?' ||
           ch == U';' || ch == U':';
}

} // namespace bgime::utf
