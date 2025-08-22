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

    // UNUSED VARIABLES
    std::array<char, 26> dictionaryFrequency = {'e', 'a', 'o', 'r', 'i', 'l', 't', 'n', 's', 'u', 'd', 'p', 'm', 'y', 'c', 'h', 'g', 'b', 'k', 'f', 'w', 'v', 'z', 'j', 'x', 'q'};
    std::array<bool, 5> solvedLettersLocal = state.solvedLetters;
    // VARIABLES
    uint32_t unplayedChars = ~(state.grey | state.requiredCharMask);
    std::vector<std::pair<char, int>> sortedChFreq;
    sortedChFreq.reserve(26);

    int tmp_chFreq[26] = {0};
    std::array<uint32_t, 5> tmp_posCharMask = {0};
    for (int word_idx : state.candidates)
    {
        const std::string &word = w.strings[word_idx];
        for (int i = 0; i < WORD_LEN; i++)
        {
            if (state.solvedLetters[i])
                continue;

            char ch = word[i];
            uint32_t chBit = 1u << (ch - 'a');

            // Log char bit
            tmp_posCharMask[i] |= chBit;

            // Log char frequency
            if (unplayedChars & chBit)
                tmp_chFreq[ch - 'a']++;
        }
    }

    // Do all candidates use only one letter in a position not marked as green?
    // aka checks if only 1 bit is set for each position
    for (int i = 0; i < 5; i++)
    {
        uint32_t charMask = tmp_posCharMask[i];
        if (charMask && !(charMask & (charMask - 1)))
        {
            solvedLettersLocal[i] = true;

            // Remove letter from char frequency array
            // because it is mandatory
            uint8_t idx = __builtin_ctz(charMask) + 1;
            tmp_chFreq[idx] = 0;
        }
    }

    // Prepare the frequency char vector
    for (int i = 0; i < 26; ++i)
        if (tmp_chFreq[i] > 0)
            sortedChFreq.emplace_back('a' + i, tmp_chFreq[i]);

    std::sort(sortedChFreq.begin(), sortedChFreq.end(),
              [](const auto &a, const auto &b)
              {
                  return a.second > b.second;
              });

    // Get a shortlist containing the most amount of high value letters
    std::vector<int> shortlist;
    shortlist.resize(w.strings.size() - 1);
    std::iota(shortlist.begin(), shortlist.end(), 1);
    // for (int i = 0; i < sortedChFreq.size() && shortlist.size() > MIN_LIST_SIZE; ++i)
    //{
    //     char ch = sortedChFreq[i].first;
    //     const auto &current = w.inv_index.find(ch)->second;
    //
    //     // intersect current shortlist with words that contain ch
    //     std::vector<int> next;
    //     next.reserve(shortlist.size());
    //     std::set_intersection(
    //         shortlist.begin(), shortlist.end(),
    //         current.begin(), current.end(),
    //         std::back_inserter(next));
    //
    //     if (!next.empty())
    //         shortlist = std::move(next);
    // }

    int sl = 0;
    int yl = 0;
    int ylm = 0;
    for (size_t i = 0; i < WORD_LEN; i++)
    {
        if (solvedLettersLocal[i])
            sl++;

        ylm |= state.yellow[i];
    }

    yl = std::__popcount(ylm);

    if (sl >= 3)
    {
        REPEATING_PENALTY = 30;
        YELLOW_CHAR_BONUS = 50;
        UNPLAYED_CHAR_BONUS = 200;
        CANDIDATE_BONUS = 30;
    }

    if (yl > 3)
    {
        YELLOW_CHAR_BONUS = 400;
    }

    if (state.candidates.size() < 20 && sl <= 3)
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
        const std::string &word = w.strings[word_idx];
        int letter_frq[26] = {0};
        word_score = 0;

        // Prefer candidates
        if (candidatesSet.count(word_idx))
            word_score += CANDIDATE_BONUS;

        uint32_t localNewChars = unplayedChars;
        for (int i = 0; i < WORD_LEN; i++)
        {
            char ch = word[i];
            int occurances = ++letter_frq[ch - 'a'];

            if (localNewChars & (1u << (ch - 'a')))
            {
                word_score += UNPLAYED_CHAR_BONUS;
                localNewChars |= (1u << ch - 'a');
            }

            if (ylm & (1u << (ch - 'a')))
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