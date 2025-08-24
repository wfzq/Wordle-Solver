#include <iostream>
#include <random>
#include "wordle-game.cpp"

// WEIGHTS
int CANDIDATE_BONUS = 150;
int REPEATING_PENALTY = 200;
int UNPLAYED_CHAR_BONUS = 200;
int YELLOW_CHAR_BONUS = 250;

// Utility
struct VectorHash;
static inline uint16_t encode_pattern_base3(const std::array<uint8_t, 5> &pattern);
static inline uint16_t compute_pattern(const char *guess, const char *target, const std::array<uint8_t, 26> &target_counts_pre);

// Algorithms
const std::string &algo_idxfirst(const words &w, const WordleState &state);
const std::string &algo_idxmiddle(const words &w, const WordleState &state);
const std::string &algo_idxlast(const words &w, const WordleState &state);
const std::string &algo_rand(const words &w, const WordleState &state);
const std::string &algo_normal(const words &w, const WordleState &state);
const std::string &algo_test1(const words &w, const WordleState &state);
const std::string &algo_entropy(const words &w, const WordleState &state);
const std::string &algo_entropy_fast(const words &w, const WordleState &state);

struct VectorHash
{
    size_t operator()(const std::vector<short> &v) const
    {
        std::size_t hash = 0;
        for (short i : v)
        {
            hash ^= std::hash<short>()(i) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

static inline uint16_t compute_pattern(const char *guess, const char *secret_word, const std::array<uint8_t, 26> &secret_precounts)
{
    std::array<uint8_t, 26> remaining = secret_precounts;
    std::array<uint8_t, 5> pattern{};

    for (int i = 0; i < 5; ++i)
    {
        if (guess[i] == secret_word[i])
        {
            pattern[i] = 2; // Green
            remaining[(uint8_t)(secret_word[i] - 'a')]--;
        }
    }
    for (int i = 0; i < 5; ++i)
    {
        if (pattern[i] == 2)
            continue;

        uint8_t c = (uint8_t)(guess[i] - 'a');
        if (remaining[c] > 0)
        {
            pattern[i] = 1; // Yellow
            remaining[c]--;
        }
        else
            pattern[i] = 0; // Gray
    }

    // Encode pattern in base 3
    uint16_t encoding = 0;
    for (int i = 0; i < 5; ++i)
        encoding = encoding * 3 + pattern[i];
    return encoding;
}

// -------------------------------------------------------------------------------------------------
//                                   Algorithm Implementations
// -------------------------------------------------------------------------------------------------

const std::string &algo_idxfirst(const words &w, const WordleState &state)
{
    return w.strings[state.candidates[0]];
}

const std::string &algo_idxmiddle(const words &w, const WordleState &state)
{
    return w.strings[state.candidates[state.candidates.size() / 2]];
}

const std::string &algo_idxlast(const words &w, const WordleState &state)
{
    return w.strings[state.candidates[state.candidates.size() - 1]];
}

const std::string &algo_rand(const words &w, const WordleState &state)
{
    std::random_device rd;                                                // Non-deterministic random seed
    std::mt19937 gen(rd());                                               // Mersenne Twister engine
    std::uniform_int_distribution<> dist(0, state.candidates.size() - 1); // Range: 0 to N inclusive

    int random_number = dist(gen);
    return w.strings[state.candidates[random_number]];
}

const std::string &algo_normal(const words &w, const WordleState &state)
{
    if (state.candidates.size() == 1)
        return w.strings[state.candidates[0]];

    uint32_t unplayedChars = ~(state.grey | state.requiredCharMask);

    int charFrequency[26] = {0};
    std::array<uint32_t, 5> positionCharMask = {0};
    for (int word_idx : state.candidates)
    {
        const auto &word = w.strings[word_idx];
        for (int i = 0; i < WORD_LEN; i++)
        {
            if (state.solvedLetters[i])
                continue;

            char ch = word[i];
            uint32_t chBit = 1u << (ch - 'a');

            // Log char in given position
            positionCharMask[i] |= chBit;

            // Log unplayed char frequency
            if (unplayedChars & chBit)
                charFrequency[ch - 'a']++;
        }
    }

    // Do all candidates share a single letter at a position, not marked as green
    auto solvedLettersLocal = state.solvedLetters;
    for (int i = 0; i < 5; i++)
    {
        auto mask = positionCharMask[i];
        if (mask && !(mask & (mask - 1)))
        {
            solvedLettersLocal[i] = true;

            // Remove char because it's mandatory
            uint8_t charIndex = __builtin_ctz(mask);
            charFrequency[charIndex] = 0;
        }
    }

    std::vector<int> shortlist;
    shortlist.resize(w.strings.size() - 1);
    std::iota(shortlist.begin(), shortlist.end(), 1);

    // Count green & yellows
    int greenLetters = 0;
    int yellowLetters = 0;
    int YellowLettersMask = 0;
    int unplayedCharsCount = 0;
    for (size_t i = 0; i < WORD_LEN; i++)
    {
        if (solvedLettersLocal[i])
            greenLetters++;

        YellowLettersMask |= state.yellow[i];
    }

    yellowLetters = std::__popcount(YellowLettersMask);
    unplayedCharsCount = std::__popcount(unplayedChars) - 6; // 32 - 6 = 26

    // Early Game
    if ((greenLetters + yellowLetters) <= 3)
    {
        YELLOW_CHAR_BONUS = 300;
        UNPLAYED_CHAR_BONUS = 1000;
    }
    // Middle Game
    else if ((greenLetters + yellowLetters) > 3)
    {
        YELLOW_CHAR_BONUS = 550;
        UNPLAYED_CHAR_BONUS = 500;
    }

    REPEATING_PENALTY = (unplayedCharsCount / 21) * 400;
    CANDIDATE_BONUS = 700 - (state.candidates.size() - 1) * (700 - 150) / (w.strings.size() - 1);

    int best_score = 0, current_score, best_idx;
    std::unordered_set<int> candidatesSet(state.candidates.begin(), state.candidates.end());
    for (int word_idx : shortlist)
    {
        const auto &word = w.strings[word_idx];

        current_score = 0;
        int repeatChars[26] = {0};
        auto local_unplayedChars = unplayedChars;

        // Candidate Bonus
        if (candidatesSet.count(word_idx))
            current_score += CANDIDATE_BONUS;

        for (int i = 0; i < WORD_LEN; i++)
        {
            char ch = word[i];
            uint8_t ch_idx = ch - 'a';
            uint8_t ch_count = ++charFrequency[ch_idx];
            uint32_t ch_mask = 1u << (ch_idx);

            // Unplayed chars Bonus
            if ((unplayedChars & ch_mask) && (local_unplayedChars & ch_mask))
            {
                current_score += UNPLAYED_CHAR_BONUS;
                current_score += charFrequency[ch_idx];

                local_unplayedChars &= ~ch_mask;
            }

            // Yellow bonus
            if (YellowLettersMask & ch_mask)
                current_score += YELLOW_CHAR_BONUS;

            // Repeat penalty
            if (ch_count > 1)
                current_score -= REPEATING_PENALTY * ch_count;
        }

        if (current_score > best_score)
        {
            // std::cout << "\nword: " << w.strings[word_idx] << " - " << current_score << "\n";
            best_score = current_score;
            best_idx = word_idx;
        }
    }
    return w.strings[best_idx];
}

const std::string &algo_test1(const words &w, const WordleState &state)
{
    if (state.candidates.size() == 1)
        return w.strings[state.candidates[0]];

    // Variables:
    std::array<bool, 5> solvedLettersLocal = state.solvedLetters;
    uint32_t unplayedChars = ~(state.grey | state.requiredCharMask);
    uint32_t unplayedCharsMask = 0;
    std::vector<std::pair<char, int>> sortedChFreq;
    sortedChFreq.reserve(26);

    int temp_chFreq[26] = {0};
    std::array<uint32_t, 5> temp_posCharMask = {0};
    for (int word_idx : state.candidates)
    {
        const auto &word = w.strings[word_idx];
        for (int i = 0; i < WORD_LEN; i++)
        {
            if (state.solvedLetters[i])
                continue;

            char ch = word[i];
            uint32_t chBit = 1u << (ch - 'a');

            // Log char in given position
            temp_posCharMask[i] |= chBit;

            // Log unplayed char frequency
            if (unplayedChars & chBit)
            {
                temp_chFreq[ch - 'a']++;
                unplayedCharsMask |= chBit;
            }
        }
    }

    // Do all candidates use only one letter in a position not marked as green?
    for (int i = 0; i < 5; i++)
    {
        uint32_t charMask = temp_posCharMask[i];
        if (charMask && !(charMask & (charMask - 1)))
        {
            solvedLettersLocal[i] = true;

            // Remove letter from char frequency array
            // because it is mandatory
            uint8_t idx = __builtin_ctz(charMask) + 1;
            temp_chFreq[idx] = 0;
        }
    }

    // Prepare the frequency char vector
    for (int i = 0; i < 26; ++i)
        if (temp_chFreq[i] > 0)
            sortedChFreq.emplace_back('a' + i, temp_chFreq[i]);

    if (sortedChFreq.size() > 0)
    {
        std::sort(
            sortedChFreq.begin(),
            sortedChFreq.end(),
            [](const auto &a, const auto &b)
            {
                return a.second > b.second;
            });
    }

    std::vector<int> shortlist = state.candidates;
    for (int i = 0; i < sortedChFreq.size() && shortlist.size() > 0; ++i)
    {
        char ch = sortedChFreq[i].first;
        const auto &current = w.inv_index.find(ch)->second;

        // intersect current shortlist with words that contain ch
        std::vector<int> next;
        next.reserve(shortlist.size());
        std::set_intersection(
            shortlist.begin(), shortlist.end(),
            current.begin(), current.end(),
            std::back_inserter(next));

        if (!next.empty())
            shortlist = std::move(next);
    }

    if (shortlist.size() == 1)
        return w.strings[shortlist[0]];
    else
        return w.strings[shortlist[shortlist.size() / 2]];
}

// -------------------------------------------------------------------------------------------------
//                                   Entropy Implementations
// -------------------------------------------------------------------------------------------------

// Matty's code, interfaced
const std::string &algo_entropy(const words &w, const WordleState &state)
{
    const auto &candidates = state.candidates;

    if (candidates.size() == 1)
        return w.strings[candidates[0]];

    double max = -1.0;
    int best_idx = candidates[0];
    for (int idx : candidates)
    {
        const std::string &candidate = w.strings[idx];

        double entropy = 0.0;
        std::unordered_map<std::vector<short>, int, VectorHash> pattern_count;

        // Compute entropy
        for (int idx2 : candidates)
        {
            const std::string &target = w.strings[idx2];

            std::vector<short> pattern(5);
            std::vector<bool> green_char(5, false);

            // 1 - Green
            // 2 - Grey
            // 3 - Yellow
            for (int i = 0; i < 5; i++)
            {
                if (candidate[i] == target[i])
                {
                    pattern[i] = 1;
                    green_char[i] = true;
                }
            }

            for (int i = 0; i < 5; i++)
            {
                if (pattern[i] == 1)
                    continue;

                bool found = false;
                for (int j = 0; j < 5; j++)
                {
                    if (green_char[j])
                        continue;

                    if (candidate[i] == target[j])
                    {
                        found = true;
                        green_char[j] = true;
                        break;
                    }
                }
                pattern[i] = found ? 3 : 2;
            }

            pattern_count[pattern]++;
        }

        for (auto &kv : pattern_count)
        {
            double p = kv.second / (double)candidates.size();

            if (p > 0.0)
                entropy += -p * std::log2(p);
        }

        if (entropy > max)
        {
            max = entropy;
            best_idx = idx;
        }
    }
    return w.strings[best_idx];
}

/**
 *
 * @warning REQUIRES precomputing entropy
 * @warning WORD_LEN is limited to 5 because uint8_t is used when precomputing
 */
const std::string &algo_entropy_fast(const words &w, const WordleState &state)
{
    if (w.e == nullptr)
    {
        std::cerr << "ERROR: Entropy precomputation missing";
        exit(1);
    }

    const auto &candidates = state.candidates;
    if (candidates.size() == 1)
        return w.strings[candidates[0]];

    const auto &klogk = w.e->klogk;
    const auto &ptable = w.e->pattern_table;
    const int wordCount = w.strings.size();
    const double log2N = std::log2((double)candidates.size());

    double best_entropy = -1.0;
    int best_idx = candidates[0];
    std::array<int, 243> pattern_count; // Assuming 5 chars
    for (int guess_idx : candidates)
    {
        pattern_count.fill(0);

        const int base = guess_idx * wordCount;
        // count patterns
        for (int target_idx : candidates)
        {
            uint8_t code = ptable[base + (size_t)target_idx];
            ++pattern_count[code];
        }

        // compute S = sum_k (k * log2(k)) via table, then H = log2N - S/N
        double S = 0.0;
        for (int p = 0; p < 243; ++p)
        {
            int k = pattern_count[p];
            if (k)
                S += klogk[k];
        }
        double entropy = log2N - (S / (double)candidates.size());

        if (entropy > best_entropy)
        {
            best_entropy = entropy;
            best_idx = guess_idx;
        }
    }
    return w.strings[best_idx];
}