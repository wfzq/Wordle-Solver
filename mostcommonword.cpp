#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

#define WORD_URL "D:\\Code\\Wordle Solver\\valid-wordle-words.txt"

int main(int argc, char const *argv[])
{
    std::ifstream wWords(WORD_URL);

    std::string currentLine;
    size_t line = 0;
    std::unordered_map<char, int> letterFrequency;
    while (std::getline(wWords, currentLine))
    {
        for (int i = 0; i < currentLine.length() - 1; ++i)
        {
            letterFrequency[currentLine[i]]++;
        }
        
        if (currentLine[4] != 's')
        {
            letterFrequency[currentLine[4]]++;
        }
    }

    std::vector<std::pair<char, int>> sortedLetters(letterFrequency.begin(), letterFrequency.end());

    // Step 2: Sort vector by value (int), descending
    std::sort(sortedLetters.begin(), sortedLetters.end(),
              [](const auto &a, const auto &b)
              {
                  return a.second > b.second; // change to < for ascending
              });
    
    std::cout << "{";
    for (const auto &pair : sortedLetters)
    {
        std::cout << " \'" << pair.first << "\', ";
    }
    std::cout << "}";
}
