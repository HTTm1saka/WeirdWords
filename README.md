# 炸绷生成器

每次随机生成四条诡词。`AB`、`ABC`、`ABCCD`、`AB是个啥CD` 各占一个固定位置。不同字母使用不同的字，`CC` 重复同一个字。熟词偶尔作为片段参与拼接。

## 下载

| 词库 | Windows x64 | Android 12 及以上 |
| --- | --- | --- |
| 27 字原版 | [WeirdWords-27.exe](https://github.com/HTTm1saka/WeirdWords/releases/latest/download/WeirdWords-27.exe) | [WeirdWords-27-Android.apk](https://github.com/HTTm1saka/WeirdWords/releases/latest/download/WeirdWords-27-Android.apk) |
| 45 字扩充版 | [WeirdWords.exe](https://github.com/HTTm1saka/WeirdWords/releases/latest/download/WeirdWords.exe) | [WeirdWords-Android.apk](https://github.com/HTTm1saka/WeirdWords/releases/latest/download/WeirdWords-Android.apk) |

Windows 版双击运行。Android 版使用调试签名，两个词库版本可同时安装。点词卡复制单条，点「复制全部」复制当前四条。Android 界面采用 Material 3，跟随系统配色和浅色、深色模式。

## 从源码构建

- **Windows** 使用 MinGW-w64，将 `g++` 与 `windres` 加入 `PATH`。在项目根目录运行 `./build.ps1 -WordCount 27` 或 `./build.ps1 -WordCount 45`。
- **Android** 使用 JDK 17 和 Android SDK 37。在 `android` 目录运行 `./gradlew assembleRelease -PsignWithDebugKey=true`，Windows 上使用 `./gradlew.bat`。两份 APK 输出在 `android/app/build/outputs/apk/classic/release/` 和 `android/app/build/outputs/apk/expanded/release/`，使用本机调试密钥签名。

作者：[m1saka](https://github.com/HTTm1saka)
