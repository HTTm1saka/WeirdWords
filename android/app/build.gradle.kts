plugins {
    id("com.android.application") version "9.5.1"
    id("org.jetbrains.kotlin.plugin.compose") version "2.2.10"
}

android {
    namespace = "io.github.httm1saka.weirdwords"
    compileSdk = 37

    defaultConfig {
        applicationId = "io.github.httm1saka.weirdwords"
        minSdk = 31
        targetSdk = 37
        versionCode = 5
        versionName = "1.1.0"
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    flavorDimensions += "vocabulary"
    productFlavors {
        create("classic") {
            dimension = "vocabulary"
            applicationIdSuffix = ".classic"
            buildConfigField("boolean", "EXPANDED_VOCABULARY", "false")
        }
        create("expanded") {
            dimension = "vocabulary"
            buildConfigField("boolean", "EXPANDED_VOCABULARY", "true")
        }
    }

    buildTypes {
        release {
            optimization {
                enable = true
            }
            if (providers.gradleProperty("signWithDebugKey").orNull == "true") {
                signingConfig = signingConfigs.getByName("debug")
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

dependencies {
    implementation(platform("androidx.compose:compose-bom:2025.12.01"))
    implementation("androidx.activity:activity-compose:1.11.0")
    implementation("androidx.compose.foundation:foundation")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.ui:ui")
    testImplementation("junit:junit:4.13.2")
}
