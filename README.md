# cis165-lab7
/**
 * Program: Body Mass Index (BMI) Calculator
 * File: bmi.cpp
 * Description: Calculates BMI from weight in pounds and height in inches,
 *              validates inputs and classifies results into categories.
 *
 * Input rules (each value is entered on its own line):
 *   - Must be a plain decimal number: optional +/- sign, digits, and at most
 *     one decimal point. Anything else (letters, "150abc", "1e2", "nan",
 *     "inf", hex, commas, blank lines, end-of-input) is rejected.
 *   - Weight and height must be positive.
 *   - Weight must be 1 to 1500 pounds; height must be 20 to 200 inches
 *     (both ends inclusive).
 *
 * Categories (decided from the SAME rounded value that is displayed):
 *   Underweight      BMI <  18.50
 *   Optimal weight   18.50 <= BMI <= 25.00
 *   Overweight       25.00 <  BMI <  30.00
 *   Obese            BMI >= 30.00
 *
 * Rounding (to 2 decimal places) so the displayed number and the category
 * can never disagree:
 *   - Just below an upper cutoff (25.00 or 30.00): round DOWN.
 *       24.996 -> 24.99     29.996 -> 29.99
 *   - Just below the lower cutoff (18.50): round UP.
 *       18.496 -> 18.50
 *   - Everywhere else: normal round-half-up.
 *       14.3469 -> 14.35
 *   "Just below" means within ROUNDING_WINDOW hundredths of the cutoff.
 */

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

// Plausibility limits (inclusive).
const double MIN_WEIGHT_LB = 1.0;
const double MAX_WEIGHT_LB = 1500.0;
const double MIN_HEIGHT_IN = 20.0;
const double MAX_HEIGHT_IN = 200.0;

// Category cutoffs, expressed in hundredths of a BMI point (18.50 -> 1850)
// so that comparisons are done on exact integers, not floating point.
const long UNDERWEIGHT_BELOW = 1850;  // below this: Underweight
const long OPTIMAL_MAX       = 2500;  // up to and including this: Optimal
const long OBESE_FROM        = 3000;  // this and above: Obese

// How close (in hundredths) to a cutoff the directional rounding applies.
// 0.5 = only values that ordinary rounding would push ONTO the cutoff.
const double ROUNDING_WINDOW = 0.5;
const double EPSILON         = 1e-9;  // guards against 1849.4999999999998

enum ReadStatus { READ_OK, READ_NO_INPUT, READ_INVALID };

std::string trim(const std::string& s) {
    const char* whitespace = " \t\r\n\f\v";
    std::string::size_type first = s.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return "";
    }
    std::string::size_type last = s.find_last_not_of(whitespace);
    return s.substr(first, last - first + 1);
}

// True only for: [+|-] digits [. digits], with at least one digit.
// Rejects exponents, hex, "nan"/"inf", commas, and any trailing junk.
bool isPlainNumber(const std::string& s) {
    std::string::size_type i = 0;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        ++i;
    }
    bool sawDigit = false;
    bool sawDot = false;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (c >= '0' && c <= '9') {
            sawDigit = true;
        } else if (c == '.' && !sawDot) {
            sawDot = true;
        } else {
            return false;
        }
    }
    return sawDigit;
}

// Prompts, reads ONE line, and parses the whole line as a number.
ReadStatus readNumber(const char* prompt, double& value) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return READ_NO_INPUT;
    }
    const std::string text = trim(line);
    if (!isPlainNumber(text)) {
        return READ_INVALID;
    }
    const double parsed = std::strtod(text.c_str(), NULL);
    if (!std::isfinite(parsed)) {  // e.g. a 400-digit number overflows
        return READ_INVALID;
    }
    value = parsed;
    return READ_OK;
}

void reportReadError(const char* name, const char* example, ReadStatus status) {
    if (status == READ_NO_INPUT) {
        std::string lowered(name);
        lowered[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[0])));
        std::cerr << "Error: No input received for " << lowered << "." << std::endl;
    } else {
        std::cerr << "Error: " << name << " must be a valid number (e.g. "
                  << example << ")." << std::endl;
    }
}

// Rounds a BMI to 2 decimals, returned as an integer count of hundredths.
long roundBmiToHundredths(double bmi) {
    const double scaled = bmi * 100.0;

    // Upper cutoffs (25.00, 30.00): round DOWN just below them.
    const long upperCutoffs[2] = { OPTIMAL_MAX, OBESE_FROM };
    for (int i = 0; i < 2; ++i) {
        const double cutoff = static_cast<double>(upperCutoffs[i]);
        if (scaled < cutoff && scaled >= cutoff - ROUNDING_WINDOW) {
            return upperCutoffs[i] - 1;
        }
    }

    // Lower cutoff (18.50): round UP just below it.
    const double lower = static_cast<double>(UNDERWEIGHT_BELOW);
    if (scaled < lower && scaled >= lower - ROUNDING_WINDOW - EPSILON) {
        return UNDERWEIGHT_BELOW;
    }

    // Everywhere else: normal round-half-up.
    return static_cast<long>(std::floor(scaled + 0.5 + EPSILON));
}

const char* classify(long bmiHundredths) {
    if (bmiHundredths < UNDERWEIGHT_BELOW) {
        return "Underweight";
    }
    if (bmiHundredths <= OPTIMAL_MAX) {
        return "Optimal weight";
    }
    if (bmiHundredths < OBESE_FROM) {
        return "Overweight";
    }
    return "Obese";
}

}  // namespace

int main() {
    double weight = 0.0;
    double height = 0.0;

    // Prompt and read user inputs (fail fast on bad input)
    ReadStatus status = readNumber("Enter weight in pounds: ", weight);
    if (status != READ_OK) {
        reportReadError("Weight", "150 or 150.5", status);
        return 1;
    }

    status = readNumber("Enter height in inches: ", height);
    if (status != READ_OK) {
        reportReadError("Height", "70 or 70.5", status);
        return 1;
    }

    // Validate that both weight and height are positive numbers
    if (weight <= 0.0 || height <= 0.0) {
        std::cerr << "Error: Weight and height must be positive numbers." << std::endl;
        return 1;
    }

    // Validate plausible ranges (report every problem found)
    bool inRange = true;
    if (weight < MIN_WEIGHT_LB || weight > MAX_WEIGHT_LB) {
        std::cerr << "Error: Weight must be between " << MIN_WEIGHT_LB << " and "
                  << MAX_WEIGHT_LB << " pounds." << std::endl;
        inRange = false;
    }
    if (height < MIN_HEIGHT_IN || height > MAX_HEIGHT_IN) {
        std::cerr << "Error: Height must be between " << MIN_HEIGHT_IN << " and "
                  << MAX_HEIGHT_IN << " inches." << std::endl;
        inRange = false;
    }
    if (!inRange) {
        return 1;
    }

    // Calculate BMI using the standard formula, then round for display
    const double bmi = weight * 703.0 / (height * height);
    const long hundredths = roundBmiToHundredths(bmi);

    // Display the rounded BMI with the category chosen from that same value
    std::cout << "BMI: " << hundredths / 100 << '.'
              << std::setw(2) << std::setfill('0') << hundredths % 100
              << " — " << classify(hundredths) << std::endl;

    return 0;
}
