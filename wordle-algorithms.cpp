#include <iostream>
#include <random>
#include "wordle-game.cpp"

// Constants
const int MIN_LIST_SIZE = 0;
const int CANDIDATE_BONUS = 30;
const int REPEATING_PENTALTY = 10;
const int UNPLAYED_CHAR_BONUS = 20;
const int YELLOW_CHAR_BONUS = 30;

template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess);
template <typename Algo>
void runAlgorithm_stepthrough(const words &w, Algo algorithm, std::string firstGuess);
void autoWordle(const words &w, std::string word, std::array<std::string, 6> guess_args);
void playWordle(const words &w, std::string word);
void playSecretWord(const words &w);

const std::string &algo_idxfirst(const words &w, const WordleState &state);
const std::string &algo_idxmiddle(const words &w, const WordleState &state);
const std::string &algo_idxlast(const words &w, const WordleState &state);
const std::string &algo_rand(const words &w, const WordleState &state);
const std::string &algo_normal(const words &w, const WordleState &state);

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
    if (state.candidates.size() == 1)
        return w.strings[state.candidates[0]];

    return w.strings[state.candidates.size() - 1];
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
    std::vector<int> shortlist = state.candidates;
    for (int i = 0; i < sortedChFreq.size() && shortlist.size() > MIN_LIST_SIZE; ++i)
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

        for (int i = 0; i < WORD_LEN; i++)
        {
            char ch = word[i];
            int occurances = letter_frq[ch - 'a']++;

            if (unplayedChars & (1u << (ch - 'a')))
                word_score += UNPLAYED_CHAR_BONUS;

            if (state.requiredCharMask & (1u << (ch - 'a')))
                word_score += YELLOW_CHAR_BONUS;

            if (occurances > 1)
            {
                // Word is punished more for each occurance
                word_score -= REPEATING_PENTALTY * occurances;
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

// -------------------------------------------------------------------------------------------------
//                                   Wordle Type
// -------------------------------------------------------------------------------------------------

template <typename Algo>
void runAlgorithm(const words &w, Algo algorithm, std::string firstGuess)
{
    int wins = 0;
    int turns = 0;
    for (int i = 0; i < w.strings.size(); ++i)
    {
        WordleState state;
        WordleGame game(w.strings[i], &state);

        Status gameOver = game.turn(firstGuess);

        while ((int)gameOver <= 0)
        {
            ValidWords(w, state);
            const std::string &choice = algorithm(w, state);
            gameOver = game.turn(choice);
        }

        turns += game.currentTurn;
        if (gameOver == Status::WIN)
            wins++;
    }

    double Avgwinrate = static_cast<double>(wins) / w.strings.size() * 100.0;
    double Avgturns = static_cast<double>(turns) / w.strings.size();

    std::cout << "\n";
    std::cout << "Winrate: " << Avgwinrate << " %\n";
    std::cout << "Av.turn: " << Avgturns << "\n";
    std::cout << "\n";
}

template <typename Algo>
void runAlgorithm_stepthrough(const words &w, Algo algorithm, std::string firstGuess)
{
    int wins = 0;
    int turns = 0;
    for (int i = 0; i < w.strings.size(); ++i)
    {
        WordleState state;
        WordleGame game(w.strings[i], &state);

        Status gameOver = game.turn(firstGuess);

        std::cout << "-------------------------------------------------------------------------------------\n";
        std::cout << game.currentTurn - 1 << "/6 - " << firstGuess;

        while ((int)gameOver <= 0)
        {
            ValidWords(w, state);

            std::cout << " - [" << state.candidates.size() << " possible words]\n\n";

            if (state.candidates.size() == 1)
                std::cout << w.strings[state.candidates[0]];
            else
            {
                for (auto &str : state.candidates)
                {
                    std::cout << ", ";
                    std::cout << w.strings[str];
                }
            }
            std::cout << "\n";

            const std::string &choice = algorithm(w, state);

            std::cout << "-------------------------------------------------------------------------------------\n";
            std::cout << game.currentTurn << "/6 - " << choice;

            gameOver = game.turn(choice);
        }

        turns += game.currentTurn;
        if (gameOver == Status::WIN)
            wins++;
        system("pause");
    }

    double Avgwinrate = static_cast<double>(wins) / w.strings.size() * 100.0;
    double Avgturns = static_cast<double>(turns) / w.strings.size();

    std::cout << "\n";
    std::cout << "Winrate: " << Avgwinrate << " %\n";
    std::cout << "Av.turn: " << Avgturns << "\n";
    std::cout << "\n";
}

void playWordle(const words &w, std::string word)
{
    if (word.length() == 0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 14800);
        int rand = dist(gen);

        // Randomize word
        word = w.strings[rand];
    }

    WordleState state;
    WordleGame game1(word, &state);
    std::string guess;
    while (true)
    {
        std::cout << "-------------------------------------------------------------------------------------\n";

        std::cin >> guess;

        std::cout << "-------------------------------------------------------------------------------------\n";
        std::cout << game1.currentTurn << "/6 - " << guess;

        int status = static_cast<int>(game1.turn(guess));

        switch (status)
        {
        case -1:
            std::cout << "invalid" << std::endl;
            break;
        case 0:
            break;
        case 1:
            std::cout << "\n\nWINNER!\n\n"
                      << std::endl;
            return;
        case 2:
            std::cout << "\n\nLOSER!\n\n"
                      << std::endl;
            return;
        }

        ValidWords(w, state);

        std::cout << " - [" << state.candidates.size() << " possible words]\n\n";

        if (state.candidates.size() == 1)
            std::cout << w.strings[state.candidates[0]];
        else
        {
            for (auto &str : state.candidates)
            {
                std::cout << ", ";
                std::cout << w.strings[str];
            }
        }
        std::cout << "\n";
    }
}

void autoWordle(const words &w, std::string word, std::array<std::string, 6> guess_args)
{
    WordleState state;
    WordleGame game1(word, &state);

    std::array<std::string, 6> guesses = guess_args;

    for (auto &&guess : guesses)
    {
        std::cout << "-------------------------------------------------------------------------------------\n";
        std::cout << game1.currentTurn << "/6 - " << guess;

        int status = static_cast<int>(game1.turn(guess));

        switch (status)
        {
        case -1:
            std::cout << "invalid" << std::endl;
            break;
        case 0:
            break;
        case 1:
            std::cout << "\n\nWINNER!\n\n"
                      << std::endl;
            return;
        case 2:
            std::cout << "\n\nLOSER!\n\n"
                      << std::endl;
            return;
        }

        ValidWords(w, state);
        std::cout << " - [" << state.candidates.size() << " possible words]\n\n";

        if (state.candidates.size() == 1)
            std::cout << w.strings[state.candidates[0]];
        else
        {
            for (auto &str : state.candidates)
            {
                std::cout << ", ";
                std::cout << w.strings[str];
            }
        }
        std::cout << "\n";
    }
}

void playSecretWord(const words &w)
{
    WordleState state;

    int turn = 1;
    std::string input, guess, result;
    while (turn <= 6)
    {
        // Get input
        std::getline(std::cin, input);
        if (input.length() != WORD_LEN * 2 + 1)
            continue;

        guess = input.substr(0, WORD_LEN);
        result = input.substr(WORD_LEN + 1, WORD_LEN);

        // Input guess
        for (int i = 0; i < WORD_LEN; i++)
        {
            char c = result[i];

            switch (c)
            {
            case 'G':
            case 'g':
                state.green[i] = guess[i];
                state.set_requireMask(i, guess[i] - 'a');
                state.maxSameChar[guess[i] - 'a']++;
                break;

            case 'Y':
            case 'y':
                state.set_yellow(i, guess[i] - 'a');
                state.set_requireMask(i, guess[i] - 'a');
                state.maxSameChar[guess[i] - 'a']++;
                break;

            case 'X':
            case 'x':
                state.set_grey(guess[i] - 'a');
                break;

            default:
                std::cerr << "followup error: " << c << std::endl;
                break;
            }
        }

        turn++;

        ValidWords(w, state);
        std::cout << " - [" << state.candidates.size() << " possible words]\n\n";

        if (state.candidates.size() == 1)
            std::cout << w.strings[state.candidates[0]];
        else
        {
            for (auto &str : state.candidates)
            {
                std::cout << ", ";
                std::cout << w.strings[str];
            }
        }
        std::cout << "\n-------------------------------------------------------------------------------------\n";
        std::cout << "\n";
    }
}

int main(int argc, char const *argv[])
{
    words w;
    loadWords(w);

    switch (4)
    {
    case 1:
        /*
            G|g - green
            Y|g - yellow
            X|x - Grey

            Input Format: (GUESS) (CHAR/SPACE) (RESULT)
            -------------------------------------------
                            Examples:
            -------------------------------------------
            1) crane|xxgxy
            2) SALET>GGGGX
            3) MilKY xyXxG
        */
        playSecretWord(w);
        break;

    case 2:
        // "" - Random Word
        playWordle(w, "");
        break;

    case 3:
        // arg1 - Word to find
        // arg2 - Array of guesses
        autoWordle(w, "lumpy", {"crane", "milky", "smily", "dumped", "lumpy"});
        break;

    case 4:
        // arg1 - Algorithm, name always starts with algo_
        // arg2 - First guess
        runAlgorithm(w, algo_normal, "salet");
        break;
    }
}