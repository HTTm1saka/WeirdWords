#pragma once

#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <string_view>

namespace weird_words {

using Batch = std::array<std::wstring, 4>;

inline constexpr std::wstring_view kCharacters = L"雷霆痞绷乐咪塞蛋猪炸阴爆刚球大骚裂爽狂蒸尿猖闹麻鬼飞嫩";
inline constexpr std::array<std::wstring_view, 10> kFamiliarWords = {
    L"猪咪", L"阴间", L"骚刚", L"雷霆", L"老弟",
    L"闹麻", L"舒嫩", L"炸绷", L"炸刚", L"破解",
};

struct WordFormat {
    std::wstring_view pattern;
    std::array<std::size_t, 2> familiarPositions;
    std::size_t positionCount;
};

// The format order matches the cards from left to right, then top to bottom.
inline constexpr std::array<WordFormat, 4> kFormats = {{
    {L"AB", {}, 0},
    {L"ABC", {0, 1}, 2},
    {L"ABCCD", {0}, 1},
    {L"AB是个啥CD", {0, 5}, 2},
}};

inline std::size_t RandomIndex(std::mt19937& random, std::size_t count) {
    return std::uniform_int_distribution<std::size_t>(0, count - 1)(random);
}

inline std::size_t CountFamiliarWords(std::wstring_view word) {
    std::size_t count = 0;
    for (std::size_t index = 0; index + 1 < word.size(); ++index) {
        const auto pair = word.substr(index, 2);
        count += std::find(kFamiliarWords.begin(), kFamiliarWords.end(), pair) != kFamiliarWords.end();
    }
    return count;
}

inline std::wstring MakeWord(std::mt19937& random, const WordFormat& format,
                             std::wstring_view familiarWord) {
    const std::size_t position = familiarWord.empty()
        ? 0 : format.familiarPositions[RandomIndex(random, format.positionCount)];
    const std::size_t expectedFamiliarWords = familiarWord.empty() ? 0 : 1;

    std::wstring word;
    do {
        std::array<wchar_t, 4> letters;
        for (auto current = letters.begin(); current != letters.end(); ++current) {
            wchar_t candidate;
            do {
                candidate = kCharacters[RandomIndex(random, kCharacters.size())];
            } while (familiarWord.find(candidate) != std::wstring_view::npos ||
                     std::find(letters.begin(), current, candidate) != current);
            *current = candidate;
        }

        word = format.pattern;
        for (auto& character : word) {
            if (character >= L'A' && character <= L'D') {
                character = letters[character - L'A'];
            }
        }
        if (!familiarWord.empty()) {
            word.replace(position, familiarWord.size(), familiarWord);
        }
        // Keep the intended frequency even when a random pair happens to be familiar.
    } while (CountFamiliarWords(word) != expectedFamiliarWords);

    return word;
}

inline Batch GenerateBatch(std::mt19937& random) {
    const bool useFamiliarWord = RandomIndex(random, 4) == 0;
    const auto familiarFormat = useFamiliarWord ? 1 + RandomIndex(random, kFormats.size() - 1) : 0;
    const auto familiarWord = useFamiliarWord
        ? kFamiliarWords[RandomIndex(random, kFamiliarWords.size())] : std::wstring_view{};

    Batch batch;
    for (std::size_t index = 0; index < batch.size(); ++index) {
        batch[index] = MakeWord(random, kFormats[index], index == familiarFormat ? familiarWord : L"");
    }
    return batch;
}

}  // namespace weird_words
