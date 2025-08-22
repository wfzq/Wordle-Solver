#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <bits/stdc++.h>

#define MAX_TURNS 6
#define WORD_LEN 5
#define WORD_URL "D:\\Code\\Wordle Solver\\valid-wordle-words.txt"

struct words;
struct WordleState;
struct WordleGame;
enum class Status : int;
void loadWords(words &w);
uint64_t encode(const std::string &str);
uint32_t bitmask(const std::string &str);
void getCandidates(const words &w, WordleState &state);

struct words
{
    std::vector<std::string> strings;
    std::vector<uint32_t> masks;
    std::vector<uint64_t> encoded;
    std::unordered_map<char, std::vector<int>> inv_index;
};

struct WordleState
{
    std::vector<int> candidates;
    std::array<bool, WORD_LEN> solvedLetters = {false};
    std::array<char, WORD_LEN> green = {0};
    std::array<uint8_t, 26> maxSameChar = {0};
    std::array<uint32_t, WORD_LEN> yellow = {0};
    uint32_t requiredCharMask = 0;
    uint32_t grey = {0};

    WordleState()
    {
        // Roughly biggest list size
        candidates.reserve(8192);
    }

    inline void set_yellow(int position, uint32_t letterindex)
    {
        yellow[position] |= (1u << letterindex);
    }
    inline void set_requireMask(int position, uint32_t letterindex)
    {
        requiredCharMask |= (1u << letterindex);
    }
    inline void set_grey(uint32_t letterindex)
    {
        grey |= (1u << letterindex);
    }
};

enum class Status : int
{
    INVALID_TURN = -1,
    NEXT_TURN = 0,
    WIN = 1,
    LOSS = 2
};

uint32_t bitmask(const std::string &str)
{
    uint32_t mask = 0;
    for (char c : str)
        mask |= 1u << (c - 'a');
    return mask;
}

uint64_t encode(const std::string &str)
{
    uint64_t key = 0;
    for (char c : str)
        key = (key << 5) | (uint64_t)(c - 'a');
    return key;
}

// -------------------------------------------------------------------------------------------------
//                                       Load Dictionary
// -------------------------------------------------------------------------------------------------

void loadWords(words &w)
{
    std::ifstream wWords(WORD_URL);

    if (!wWords.is_open())
    {
        std::cerr << "Error: Could not open valid-wordle-words.txt" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string currentLine;
    size_t line = 0;
    while (std::getline(wWords, currentLine))
    {
        if (currentLine.length() != WORD_LEN)
        {
            std::cerr << "Malformed word on: " << currentLine << std::endl;
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < WORD_LEN; ++i)
        {
            if (!isalpha(currentLine[i]))
            {
                std::cerr << "Invalid character in: " << currentLine << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        // Log word into struct
        uint32_t mask = bitmask(currentLine);
        w.strings.push_back(currentLine);
        w.masks.push_back(mask);
        w.encoded.push_back(encode(currentLine));
        for (int b = 0; b < 26; ++b)
            if (mask & (1u << b))
                w.inv_index['a' + b].push_back(line);

        line++;
    }

    // Order inverse index
    for (auto &kv : w.inv_index)
        std::sort(kv.second.begin(), kv.second.end());
}

// -------------------------------------------------------------------------------------------------
//                                    Wordle Game Implementation
// -------------------------------------------------------------------------------------------------

struct WordleGame
{
    std::string word;
    WordleState *state;
    int currentTurn = 1;

    WordleGame(const std::string &word, WordleState *state)
    {
        this->word = word;
        this->state = state;
    }

    /**
     * @warning On LOSS, object's currentTurn will be 7
     */
    Status turn(const std::string &guess)
    {
        if (guess.length() != WORD_LEN)
            return Status::INVALID_TURN;

        if (currentTurn > MAX_TURNS)
            return Status::LOSS;

        currentTurn++;

        std::array<uint8_t, 26> tmp_maxSameChar = {0};
        std::array<uint8_t, 26> count_letters = {0};
        std::array<char, WORD_LEN> guess_recheck = {0};
        std::vector<uint8_t> posIndex;
        posIndex.reserve(WORD_LEN);

        // Check for matches
        for (size_t i = 0; i < WORD_LEN; ++i)
        {
            const char &wrd_ch = word[i];
            const char &gue_ch = guess[i];

            if (gue_ch == wrd_ch)
            {
                state->green[i] = gue_ch;
                state->set_requireMask(i, gue_ch - 'a');

                tmp_maxSameChar[gue_ch - 'a']++;
            }
            else
            {
                count_letters[wrd_ch - 'a']++;
                guess_recheck[i] = gue_ch;
                posIndex.push_back(i);
            }
        }

        // Go through non-matching letters
        for (const auto i : posIndex)
        {
            uint8_t char_idx = guess_recheck[i] - 'a';

            if (count_letters[char_idx] > 0)
            {
                state->set_yellow(i, char_idx);
                state->set_requireMask(i, char_idx);
                --count_letters[char_idx];

                tmp_maxSameChar[char_idx]++;
            }
            else
            {
                state->set_grey(char_idx);
            }
        }

        // Update maxSameChar array
        for (size_t i = 0; i < 26; ++i)
            if (tmp_maxSameChar[i] > state->maxSameChar[i])
                state->maxSameChar[i] = tmp_maxSameChar[i];

        // All characters matched
        if (posIndex.size() == 0)
            return Status::WIN;

        return Status::NEXT_TURN;
    }
};

/**
 * Find all words who fit within the wordle requirements of a given WordleState.
 *
 * @param w Active dictionary object
 * @param state Candidates array is directly updated
 *
 * @note Prunes existing candidates
 */
void getCandidates(const words &w, WordleState &state)
{
    auto &candidates = state.candidates;
    const auto &reqCharMask = state.requiredCharMask;

    // First time, get candidates
    if (candidates.empty())
    {
        // Get all words with Green/Yellow chars
        if (reqCharMask != 0)
        {
            for (int i = 0; i < 26; ++i)
            {
                if (reqCharMask & (1u << i))
                {
                    char ch = 'a' + i;
                    const std::vector<int> &current = w.inv_index.find(ch)->second;

                    if (candidates.size() == 0)
                    {
                        candidates = current;
                    }
                    else
                    {
                        // Get only words in both lists
                        std::vector<int> intersection;
                        intersection.reserve(candidates.size());

                        std::set_intersection(
                            candidates.begin(), candidates.end(),
                            current.begin(), current.end(),
                            std::back_inserter(intersection));

                        candidates = std::move(intersection);
                    }
                }
            }
        }
        // Get all words not containing the 5 grey chars
        else
        {
            // uint8_t is used instead of bool because it's faster
            std::vector<uint8_t> excluded_idx(w.strings.size(), 0);
            for (int i = 0; i < 26; ++i)
            {
                if (state.grey & (1u << i))
                {
                    char ch = 'a' + i;
                    const std::vector<int> &current = w.inv_index.find(ch)->second;

                    for (int idx : current)
                        excluded_idx[idx] = 1;
                }
            }

            for (int i = 0; i < w.strings.size(); i++)
                if (!excluded_idx[i])
                    candidates.push_back(i);

            // Done!
            return;
        }
    }

    std::vector<int> answer;
    answer.reserve(candidates.size());
    uint32_t overlappingChars = state.grey & reqCharMask;
    uint32_t charsNotPresent = state.grey & ~reqCharMask;

    for (int word_idx : candidates)
    {
        // Word must contain all required letters
        if ((w.masks[word_idx] & reqCharMask) != reqCharMask)
            continue;

        // Word must NOT contain letters that do not appear in the word
        if ((w.masks[word_idx] & charsNotPresent) != 0)
            continue;

        bool ok = true;
        const auto &word = w.strings[word_idx];
        std::array<uint8_t, 26> wordcountChars = {};

        for (int pos = 0; pos < WORD_LEN; ++pos)
        {
            char c = word[pos];
            wordcountChars[c - 'a']++;

            // Skip already solved (green) letters
            if (state.solvedLetters[pos])
                continue;

            // green check
            if (state.green[pos] != 0 && state.green[pos] != c)
            {
                ok = false;
                break;
            }
            // yellow check
            else if (state.yellow[pos] != 0)
            {
                uint32_t c_bit = 1u << (c - 'a');
                if (state.yellow[pos] & c_bit)
                {
                    ok = false;
                    break;
                }
            }
        }

        if (!ok)
            continue;

        // Check character quanitity in word
        for (size_t i = 0; i < 26; ++i)
        {
            if (overlappingChars & (1u << i))
            {
                if (wordcountChars[i] > state.maxSameChar[i])
                {
                    ok = false;
                    break;
                }
            }
        }

        if (ok)
            answer.push_back(word_idx);
    }

    // Update solved letters
    for (int i = 0; i < WORD_LEN; ++i)
    {
        if (state.green[i] != 0)
            state.solvedLetters[i] = true;
    }

    candidates = std::move(answer);
}