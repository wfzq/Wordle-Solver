#include <iostream>
#include <random>
#include "wordle-game.cpp"

// WEIGHTS
int MIN_LIST_SIZE = 0;
int CANDIDATE_BONUS = 150;
int REPEATING_PENALTY = 80;
int UNPLAYED_CHAR_BONUS = 200;
int YELLOW_CHAR_BONUS = 250;

struct VectorHash;
const std::string &algo_idxfirst(const words &w, const WordleState &state);
const std::string &algo_idxmiddle(const words &w, const WordleState &state);
const std::string &algo_idxlast(const words &w, const WordleState &state);
const std::string &algo_rand(const words &w, const WordleState &state);
const std::string &algo_entropy(const words &w, const WordleState &state);
const std::string &algo_normal(const words &w, const WordleState &state);

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

// Matty's code, interfaced
const std::string &algo_entropy(const words &w, const WordleState &state)
{
    if (state.candidates.size() == 1)
        return w.strings[state.candidates[0]];

    double max = -1.0;
    int best_idx = state.candidates[0];

    for (int idx : state.candidates)
    {
        const std::string &candidate = w.strings[idx];

        double entropy = 0.0;
        std::unordered_map<std::vector<short>, int, VectorHash> pattern_count;

        // Compute entropy
        for (int idx2 : state.candidates)
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
            double p = kv.second / (double)state.candidates.size();
            if (p > 0.0)
            {
                entropy += -p * std::log2(p);
            }
        }

        if (entropy > max)
        {
            max = entropy;
            best_idx = idx;
        }
    }

    return w.strings[best_idx];
}

const std::string &algo_normal(const words &w, const WordleState &state)
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
    if (sortedChFreq.size() > 0)
    {
        for (int i = 0; i < 26; ++i)
            if (temp_chFreq[i] > 0)
                sortedChFreq.emplace_back('a' + i, temp_chFreq[i]);

        std::sort(
            sortedChFreq.begin(),
            sortedChFreq.end(),
            [](const auto &a, const auto &b)
            {
                return a.second > b.second;
            });

        
    }

    /* for (int i = 0; i < sortedChFreq.size() && shortlist.size() > MIN_LIST_SIZE; ++i)
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
    } */

    std::vector<int> shortlist;
    shortlist.resize(w.strings.size() - 1);
    std::iota(shortlist.begin(), shortlist.end(), 1);

    // Count Green and Yellow chars
    int greenLetters = 0;
    int yellowLetters = 0;
    int YellowLettersMask = 0;
    for (size_t i = 0; i < WORD_LEN; i++)
    {
        if (solvedLettersLocal[i])
            greenLetters++;

        YellowLettersMask |= state.yellow[i];
    }

    yellowLetters = std::__popcount(YellowLettersMask);

    // Adjust weights
    if (greenLetters >= 3)
    {
        REPEATING_PENALTY = 30;
        YELLOW_CHAR_BONUS = 50;
        UNPLAYED_CHAR_BONUS = 200;
        CANDIDATE_BONUS = 30;
    }

    if (yellowLetters > 3)
    {
        YELLOW_CHAR_BONUS = 400;
    }

    if (state.candidates.size() < 20 && greenLetters <= 3)
    {
        CANDIDATE_BONUS = 500;
    }

    // Rate Words to select the best one
    int word_score;
    int best_score = 0;
    int best_word_idx;
    std::unordered_set<int> candidatesSet(state.candidates.begin(), state.candidates.end());
    for (int word_idx : shortlist)
    {
        const auto &word = w.strings[word_idx];
        int word_char_frq[26] = {0};
        word_score = 0;

        // Word candidate bonus
        if (candidatesSet.count(word_idx))
            word_score += CANDIDATE_BONUS;

        uint32_t localNewChars = unplayedChars;
        for (int i = 0; i < WORD_LEN; i++)
        {
            char ch = word[i];
            uint32_t mask = 1u << (ch - 'a');
            int occurances = ++word_char_frq[ch - 'a'];

            if ((localNewChars & mask) && (unplayedCharsMask & mask))
            {
                word_score += UNPLAYED_CHAR_BONUS + 10 * sortedChFreq[ch - 'a'].second;
                localNewChars |= (1u << ch - 'a');
            }

            if (YellowLettersMask & (1u << (ch - 'a')))
                word_score += YELLOW_CHAR_BONUS;

            if (occurances > 1)
            {
                // Word is punished more for each occurance
                word_score -= REPEATING_PENALTY * occurances;
            }
        }

        if (word_score > best_score)
        {
            best_score = word_score;
            best_word_idx = word_idx;
        }
    }
    return w.strings[best_word_idx];
}