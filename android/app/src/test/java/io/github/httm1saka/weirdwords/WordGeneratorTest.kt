package io.github.httm1saka.weirdwords

import kotlin.random.Random
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class WordGeneratorTest {
    @Test
    fun batchesKeepTheirFormatsAndDistinctSlots() {
        val random = Random(281156)
        val seen = mutableSetOf<Char>()
        repeat(10_000) {
            val (pair, triple, repeated, phrase) = generateBatch(random)
            assertEquals(listOf(2, 3, 5, 7), listOf(pair.length, triple.length, repeated.length, phrase.length))
            assertEquals(repeated[2], repeated[3])
            assertEquals("是个啥", phrase.substring(2, 5))
            val slots = listOf(pair, triple, repeated.removeRange(3, 4), phrase.removeRange(2, 5))
            for (letters in slots) {
                assertEquals(letters.length, letters.toSet().size)
                seen.addAll(letters.toList())
            }
        }
        assertTrue(seen.containsAll("轰嘣崩劈砸甩蹦癫嗷嚎嘎咕呱噗嗝哐糊脆".toSet()))
    }

    @Test
    fun familiarWordsAreOnlyEmbeddedAndAppearInAboutOneQuarterOfBatches() {
        val familiar = setOf("猪咪", "阴间", "骚刚", "雷霆", "老弟", "闹麻", "舒嫩", "炸绷", "炸刚", "破解")
        val seen = mutableSetOf<String>()
        val random = Random(731_029)
        var familiarRounds = 0
        repeat(10_000) {
            val batch = generateBatch(random)
            assertTrue(batch.first() !in familiar)
            val found = batch.flatMap { word -> word.windowed(2).filter { it in familiar } }
            assertTrue(found.size <= 1)
            if (found.isNotEmpty()) familiarRounds++
            seen.addAll(found)
        }
        assertEquals(familiar, seen)
        assertTrue(familiarRounds in 2_300..2_700)
    }
}
