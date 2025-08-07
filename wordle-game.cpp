#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <bits/stdc++.h>

#define WORD_LEN 5
#define WORD_URL "D:\\Code\\Wordle Solver\\valid-wordle-words.txt"

struct WordleState;
struct WordleGame;
struct words;
enum class Status : int;
uint32_t bitmask(const std::string &str);
uint64_t encode(const std::string &str);
void ValidWords(const words &w, const WordleState &state);
void loadWords(words &w);

struct words
{
    std::vector<std::string> strings;
    std::vector<uint32_t> masks;
    std::vector<uint64_t> encoded;
    std::unordered_map<char, std::vector<int>> inv_index;
};

struct WordleState
{
    // Every new search looks at the dictionary
    // this prevents going over the same parts twice
    std::vector<int> candidates;
    std::array<bool, WORD_LEN> solvedLetters = {false};
    std::array<uint8_t, 26> maxSameChar = {0};

    std::array<char, WORD_LEN> green = {0};
    std::array<uint32_t, WORD_LEN> yellow = {0};
    uint32_t requiredCharMask = 0;
    uint32_t grey = {0};

    WordleState()
    {
        candidates.reserve(8192); // ~Biggest list, power of 2 for efficiency
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
            fprintf(stderr, "Unexpected end of life or malformed word on: %s\n", currentLine);
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < WORD_LEN; ++i)
        {
            if (!isalpha(currentLine[i]))
            {
                fprintf(stderr, "Invalid character in: %s\n", currentLine);
                exit(EXIT_FAILURE);
            }
        }

        // Log line into word struct
        w.strings.push_back(currentLine);
        uint32_t mask = bitmask(currentLine);
        w.masks.push_back(mask);
        w.encoded.push_back(encode(currentLine));
        for (int b = 0; b < 26; ++b)
        {
            if (mask & (1u << b))
                w.inv_index['a' + b].push_back(line);
        }
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
    WordleState *state;
    std::string word;
    int maxTurns = 6;
    int currentTurn = 1;

    WordleGame(const std::string &word, WordleState *state)
    {
        this->word = word;
        this->state = state;
    }

    Status turn(const std::string &guess)
    {
        if (guess.length() != WORD_LEN)
            return Status::INVALID_TURN;

        if (currentTurn >= maxTurns)
            return Status::LOSS;

        currentTurn++;

        std::array<uint8_t, 26> count_letters = {0};
        std::array<char, WORD_LEN> guess_recheck = {0};
        std::vector<uint8_t> posIndex;
        posIndex.reserve(WORD_LEN);

        // Check for matches
        for (size_t i = 0; i < WORD_LEN; ++i)
        {
            char wordChar = word[i];
            char guessChar = guess[i];

            if (guessChar == wordChar)
            {
                state->green[i] = guessChar;
                state->set_requireMask(i, guessChar - 'a');

                state->maxSameChar[guessChar - 'a']++;
            }
            else
            {
                count_letters[wordChar - 'a']++;
                guess_recheck[i] = guessChar;
                posIndex.push_back(i);
            }
        }

        // No unguessed letters, WIN
        if (posIndex.size() == 0)
            return Status::WIN;

        // Look at non-matching letters
        for (const auto i : posIndex)
        {
            uint8_t letterIndex = guess_recheck[i] - 'a';

            if (count_letters[letterIndex] > 0)
            {
                state->set_yellow(i, letterIndex);
                state->set_requireMask(i, letterIndex);
                --count_letters[letterIndex];

                state->maxSameChar[letterIndex]++;
            }
            else
            {
                state->set_grey(letterIndex);
            }
        }

        return Status::NEXT_TURN;
    }
};

// -------------------------------------------------------------------------------------------------
//                                    Dictionary Processing
// -------------------------------------------------------------------------------------------------

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

void ValidWords(const words &w, WordleState &state)
{
    std::vector<int> &candidates = state.candidates;

    // First time, get candidates
    if (candidates.empty())
    {
        // Get all words that have all required characters
        if (state.requiredCharMask != 0)
        {
            for (int i = 0; i < 26; ++i)
            {
                if (state.requiredCharMask & (1u << i))
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
        // If no required characters, use all words
        else
        {
            candidates = std::vector<int>(w.strings.size());
            std::iota(candidates.begin(), candidates.end(), 0);
        }
    }

    std::vector<int> filtered;
    filtered.reserve(candidates.size());
    uint32_t charsNotPresent = state.grey & ~state.requiredCharMask;
    uint32_t overlappingChars = state.grey & state.requiredCharMask;

    for (int word_idx : candidates)
    {
        // Word must contain all required letters
        if ((w.masks[word_idx] & state.requiredCharMask) != state.requiredCharMask)
            continue;

        // Word must NOT contain letters that do not appear in the word
        if ((w.masks[word_idx] & charsNotPresent) != 0)
            continue;

        // Check the word has it's letters in the appropriate positions
        bool ok = true;
        const std::string &word = w.strings[word_idx];
        std::array<uint8_t, 26> wordcountChars{};
        for (int pos = 0; pos < WORD_LEN; ++pos)
        {
            // Skip already solved letters for example:
            // if last turn we found the first 2 green letters to be "SO"
            // the candidates list only has valid words in the first 2 positions
            if (state.solvedLetters[pos])
                continue;

            char c = word[pos];

            wordcountChars[c - 'a']++;

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

        for (size_t i = 0; i < 26; i++)
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
            filtered.push_back(word_idx);
    }

    // Mark solved letters
    for (int i = 0; i < WORD_LEN; ++i)
    {
        if (state.green[i] != 0)
            state.solvedLetters[i] = true;
    }

    candidates = std::move(filtered);
}