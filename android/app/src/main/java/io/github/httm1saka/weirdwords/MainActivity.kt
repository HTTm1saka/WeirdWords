package io.github.httm1saka.weirdwords

import android.content.ActivityNotFoundException
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Intent
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.clickable
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.ListItem
import androidx.compose.material3.ListItemDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.listSaver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalResources
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import androidx.core.net.toUri
import kotlinx.coroutines.launch

private val wordsSaver = listSaver<List<String>, String>(save = { it }, restore = { it })

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val context = LocalContext.current
            val colors = if (isSystemInDarkTheme()) {
                dynamicDarkColorScheme(context)
            } else {
                dynamicLightColorScheme(context)
            }
            MaterialTheme(colorScheme = colors) {
                WeirdWordsApp()
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun WeirdWordsApp() {
    val context = LocalContext.current
    val resources = LocalResources.current
    val scope = rememberCoroutineScope()
    val snackbar = remember { SnackbarHostState() }
    val githubUrl = stringResource(R.string.github_url)
    var showingAbout by rememberSaveable { mutableStateOf(false) }
    var words by rememberSaveable(stateSaver = wordsSaver) { mutableStateOf(generateBatch()) }

    fun showMessage(message: String) {
        snackbar.currentSnackbarData?.dismiss()
        scope.launch { snackbar.showSnackbar(message) }
    }

    fun copy(text: String, confirmation: String) {
        val clipboard = context.getSystemService(ClipboardManager::class.java)
        if (clipboard == null) {
            showMessage(resources.getString(R.string.copy_unavailable))
            return
        }
        clipboard.setPrimaryClip(ClipData.newPlainText(resources.getString(R.string.app_name), text))
        // Android 13 and later already show a system clipboard confirmation.
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.S_V2) showMessage(confirmation)
    }

    BackHandler(enabled = showingAbout) { showingAbout = false }
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(stringResource(if (showingAbout) R.string.about else R.string.app_name)) },
                navigationIcon = {
                    if (showingAbout) {
                        IconButton(onClick = { showingAbout = false }) {
                            Icon(painterResource(R.drawable.ic_back), stringResource(R.string.back))
                        }
                    }
                },
                actions = {
                    if (!showingAbout) {
                        TextButton(onClick = { showingAbout = true }) {
                            Text(stringResource(R.string.about))
                        }
                    }
                },
            )
        },
        snackbarHost = { SnackbarHost(snackbar) },
    ) { insets ->
        if (showingAbout) {
            AboutPage(
                url = githubUrl,
                onOpenLink = {
                    try {
                        context.startActivity(Intent(Intent.ACTION_VIEW, githubUrl.toUri()))
                    } catch (_: ActivityNotFoundException) {
                        showMessage(resources.getString(R.string.link_unavailable))
                    }
                },
                modifier = Modifier.padding(insets),
            )
        } else {
            GeneratorPage(
                words = words,
                onGenerate = {
                    snackbar.currentSnackbarData?.dismiss()
                    words = generateBatch()
                },
                onCopyWord = { word -> copy(word, resources.getString(R.string.copied_word, word)) },
                onCopyAll = { copy(words.joinToString("\n"), resources.getString(R.string.copied_all)) },
                modifier = Modifier.padding(insets),
            )
        }
    }
}

@Composable
private fun GeneratorPage(
    words: List<String>,
    onGenerate: () -> Unit,
    onCopyWord: (String) -> Unit,
    onCopyAll: () -> Unit,
    modifier: Modifier = Modifier,
) {
    BoxWithConstraints(modifier.fillMaxSize(), contentAlignment = Alignment.TopCenter) {
        val viewportHeight = maxHeight
        Column(
            modifier = Modifier
                .widthIn(max = 560.dp)
                .fillMaxWidth()
                .verticalScroll(rememberScrollState())
                .heightIn(min = viewportHeight)
                .padding(horizontal = 20.dp, vertical = 24.dp),
            verticalArrangement = Arrangement.Center,
        ) {
            Text(
                text = stringResource(R.string.results),
                style = MaterialTheme.typography.headlineMedium,
                modifier = Modifier.semantics { heading() },
            )
            Spacer(Modifier.height(16.dp))
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                for (rowStart in words.indices step 2) {
                    Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                        for (index in rowStart..rowStart + 1) {
                            val word = words[index]
                            WordCard(
                                format = WordFormat.entries[index],
                                word = word,
                                onCopy = { onCopyWord(word) },
                                modifier = Modifier.weight(1f),
                            )
                        }
                    }
                }
            }
            Spacer(Modifier.height(20.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                Button(onClick = onGenerate, modifier = Modifier.weight(1f).heightIn(min = 48.dp)) {
                    Icon(painterResource(R.drawable.ic_refresh), null, Modifier.size(18.dp))
                    Spacer(Modifier.width(8.dp))
                    Text(stringResource(R.string.generate))
                }
                OutlinedButton(onClick = onCopyAll, modifier = Modifier.weight(1f).heightIn(min = 48.dp)) {
                    Icon(painterResource(R.drawable.ic_copy), null, Modifier.size(18.dp))
                    Spacer(Modifier.width(8.dp))
                    Text(stringResource(R.string.copy_all))
                }
            }
        }
    }
}

@Composable
private fun WordCard(format: WordFormat, word: String, onCopy: () -> Unit, modifier: Modifier = Modifier) {
    val colors = MaterialTheme.colorScheme
    val copyLabel = stringResource(R.string.copy_word, word)
    Card(
        onClick = onCopy,
        modifier = modifier.semantics {
            contentDescription = copyLabel
            role = Role.Button
        },
        shape = MaterialTheme.shapes.large,
        colors = CardDefaults.cardColors(
            containerColor = colors.surfaceContainerHigh,
            contentColor = colors.onSurface,
        ),
    ) {
        Column(
            modifier = Modifier.fillMaxWidth().heightIn(min = 116.dp).padding(14.dp),
            verticalArrangement = Arrangement.SpaceBetween,
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(6.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = format.pattern,
                    modifier = Modifier.weight(1f),
                    style = MaterialTheme.typography.labelMedium,
                    color = colors.onSurfaceVariant,
                )
                Icon(painterResource(R.drawable.ic_copy), null, Modifier.size(16.dp), colors.onSurfaceVariant)
            }
            Spacer(Modifier.height(10.dp))
            Text(
                text = word,
                style = if (format == WordFormat.PHRASE) {
                    MaterialTheme.typography.bodyLarge
                } else {
                    MaterialTheme.typography.headlineSmall
                },
            )
        }
    }
}

@Composable
private fun AboutPage(url: String, onOpenLink: () -> Unit, modifier: Modifier = Modifier) {
    Column(
        modifier = modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(20.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Surface(
            modifier = Modifier.widthIn(max = 560.dp).fillMaxWidth(),
            color = MaterialTheme.colorScheme.surfaceContainer,
            shape = MaterialTheme.shapes.large,
        ) {
            val itemColors = ListItemDefaults.colors(containerColor = Color.Transparent)
            Column {
                ListItem(headlineContent = { Text(stringResource(R.string.author)) }, colors = itemColors)
                HorizontalDivider(modifier = Modifier.padding(horizontal = 16.dp))
                ListItem(
                    headlineContent = { Text(stringResource(R.string.github_label)) },
                    supportingContent = { Text(url, color = MaterialTheme.colorScheme.primary) },
                    trailingContent = { Icon(painterResource(R.drawable.ic_open_link), null) },
                    colors = itemColors,
                    modifier = Modifier.clickable(role = Role.Button, onClick = onOpenLink),
                )
            }
        }
    }
}
