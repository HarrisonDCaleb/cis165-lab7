/**
 * Program: Color Mixer
 * File: color_mixer.cpp
 * Description: Asks for two colors (red, blue, yellow, black, or white) using character arrays,
 *              validates them using strcmp, and displays their mixed color result.
 *
 * Input rules (each color is entered on its own line):
 *   - Must be red, blue, yellow, black, or white. Capitalization is ignored
 *     ("Red" and "RED" are treated as "red").
 *   - Spaces, tabs, and Windows line endings around the word are ignored.
 *   - Anything else is rejected: other colors (including mixed results such
 *     as "pink" or "navy"), extra words ("red blue", "blue 123"), blank
 *     lines, embedded null bytes, and end-of-input.
 *   - A line may hold at most 19 characters (COLOR_SIZE of 20 includes the
 *     terminating '\0'). Longer lines are rejected, never truncated, so
 *     leftover characters can't spill into the next prompt.
 *
 * Mixing results (order does not matter):
 *   red + blue    = purple          red + black    = maroon
 *   red + yellow  = orange          red + white    = pink
 *   blue + yellow = green           blue + black   = navy
 *   black + white = gray            blue + white   = light blue
 *                                   yellow + black = mustard
 *                                   yellow + white = light yellow
 *   Mixing a color with itself gives that same color.
 */

#include <cctype>
#include <cstddef>
#include <cstring>
#include <iostream>

namespace {

const int COLOR_SIZE = 20;  // 19 characters + the terminating '\0'

enum ReadStatus { READ_OK, READ_NO_INPUT, READ_INVALID };

// Every color the user is allowed to enter.
const char* const VALID_COLORS[] = { "red", "blue", "yellow", "black", "white" };
const std::size_t VALID_COLOR_COUNT = sizeof(VALID_COLORS) / sizeof(VALID_COLORS[0]);

// Every mix of two DIFFERENT colors. The pair can be entered in either order.
struct Mix {
    const char* first;
    const char* second;
    const char* result;
};

const Mix MIXES[] = {
    { "red",    "blue",   "purple"       },
    { "red",    "yellow", "orange"       },
    { "blue",   "yellow", "green"        },
    { "red",    "black",  "maroon"       },
    { "red",    "white",  "pink"         },
    { "blue",   "black",  "navy"         },
    { "blue",   "white",  "light blue"   },
    { "yellow", "black",  "mustard"      },
    { "yellow", "white",  "light yellow" },
    { "black",  "white",  "gray"         }
};
const std::size_t MIX_COUNT = sizeof(MIXES) / sizeof(MIXES[0]);

bool is_valid_color(const char* color) {
    for (std::size_t i = 0; i < VALID_COLOR_COUNT; ++i) {
        if (std::strcmp(color, VALID_COLORS[i]) == 0) {
            return true;
        }
    }
    return false;
}

// True when {a, b} is the pair {x, y} in either order.
bool is_pair(const char* a, const char* b, const char* x, const char* y) {
    return (std::strcmp(a, x) == 0 && std::strcmp(b, y) == 0) ||
           (std::strcmp(a, y) == 0 && std::strcmp(b, x) == 0);
}

// Returns the mixed color name, or nullptr if the pair has no defined result.
const char* mix_colors(const char* first, const char* second) {
    if (std::strcmp(first, second) == 0) {
        return first;  // a color mixed with itself stays the same
    }
    for (std::size_t i = 0; i < MIX_COUNT; ++i) {
        if (is_pair(first, second, MIXES[i].first, MIXES[i].second)) {
            return MIXES[i].result;
        }
    }
    return nullptr;
}

// Strips leading/trailing whitespace and lowercases the text, in place.
void normalize(char* text) {
    std::size_t start = 0;
    std::size_t end = std::strlen(text);
    while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    std::size_t out = 0;
    for (std::size_t i = start; i < end; ++i) {
        text[out++] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    }
    text[out] = '\0';
}

// Prompts, reads ONE whole line into the array, and checks it is an allowed color.
// On READ_OK the array holds the cleaned, lowercase color name.
ReadStatus read_color(const char* prompt, char* color, int size) {
    std::cout << prompt;
    color[0] = '\0';  // never leave the array uninitialized, whatever happens below

    std::cin.getline(color, size);
    const std::streamsize extracted = std::cin.gcount();

    if (std::cin.eof() && extracted == 0) {
        return READ_NO_INPUT;  // nothing was typed at all
    }
    if (std::cin.fail()) {
        return READ_INVALID;   // line did not fit in the array
    }

    // getline counts the newline it consumed; at end-of-input there is none.
    // If strlen() disagrees with the characters read, a '\0' byte was typed,
    // which strcmp would silently treat as the end of the word.
    const std::streamsize text_length = std::cin.eof() ? extracted : extracted - 1;
    if (static_cast<std::streamsize>(std::strlen(color)) != text_length) {
        return READ_INVALID;
    }

    normalize(color);
    return is_valid_color(color) ? READ_OK : READ_INVALID;
}

void report_read_error(const char* which, ReadStatus status) {
    if (status == READ_NO_INPUT) {
        std::cerr << "Error: No input received for " << which << " color." << std::endl;
    } else {
        std::cerr << "Error: Enter red, blue, yellow, black, or white." << std::endl;
    }
}

}  // namespace

int main() {
    char first_color[COLOR_SIZE] = "";
    char second_color[COLOR_SIZE] = "";

    // Prompt, read, and validate each input (fail fast on bad input)
    ReadStatus status = read_color("Enter first color (red, blue, yellow, black, or white): ",
                                   first_color, COLOR_SIZE);
    if (status != READ_OK) {
        report_read_error("first", status);
        return 1;
    }

    status = read_color("Enter second color (red, blue, yellow, black, or white): ",
                        second_color, COLOR_SIZE);
    if (status != READ_OK) {
        report_read_error("second", status);
        return 1;
    }

    // Determine the color mixing result
    const char* result = mix_colors(first_color, second_color);

    // Safety net: validation guarantees a result, but never print a blank one.
    if (result == nullptr) {
        std::cerr << "Error: Unable to mix those colors." << std::endl;
        return 1;
    }

    std::cout << "Result: " << result << std::endl;
    return 0;
}

