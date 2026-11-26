package io.github.httm1saka.weirdwords

import kotlin.random.Random

internal enum class WordFormat(val pattern: String, vararg val familiarPositions: Int) {
    PAIR("AB"),
    TRIPLE("ABC", 0, 1),
    REPEATED("ABCCD", 0),
    PHRASE("AB是个啥CD", 0, 5),
}

private const val characters = "雷霆痞绷乐咪塞蛋猪炸阴爆刚球大骚裂爽狂蒸尿猖闹麻鬼飞嫩" +
    "轰嘣崩劈砸甩蹦癫嗷嚎嘎咕呱噗嗝哐糊脆"
private val familiarWords = listOf("猪咪", "阴间", "骚刚", "雷霆", "老弟", "闹麻", "舒嫩", "炸绷", "炸刚", "破解")

internal fun generateBatch(random: Random = Random.Default): List<String> {
    val familiarFormat = if (random.nextInt(4) == 0) {
        WordFormat.entries[random.nextInt(1, WordFormat.entries.size)]
    } else {
        null
    }
    val familiarWord = if (familiarFormat != null) familiarWords.random(random) else ""
    return WordFormat.entries.map { format ->
        generateWord(format, if (format == familiarFormat) familiarWord else "", random)
    }
}

private fun generateWord(format: WordFormat, familiarWord: String, random: Random): String {
    val position = if (familiarWord.isNotEmpty()) format.familiarPositions.random(random) else -1
    val expectedFamiliarCount = if (position >= 0) 1 else 0

    var word: String
    do {
        val letters = CharArray(4)
        for (index in letters.indices) {
            var candidate: Char
            do {
                candidate = characters.random(random)
            } while (candidate in familiarWord || (0 until index).any { letters[it] == candidate })
            letters[index] = candidate
        }

        word = buildString(format.pattern.length) {
            for (character in format.pattern) {
                append(if (character in 'A'..'D') letters[character - 'A'] else character)
            }
        }
        if (position >= 0) word = word.replaceRange(position, position + familiarWord.length, familiarWord)
        // An accidental familiar pair must not change the per-batch probability.
    } while (word.windowed(2).count { it in familiarWords } != expectedFamiliarCount)
    return word
}
