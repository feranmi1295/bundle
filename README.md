# Bundle

> The Android build system that actually works. No Gradle. No cryptic errors.

Bundle is a fast, framework-agnostic Android build system written in C. It replaces Gradle with a clean CLI, honest error messages, and a built-in compatibility matrix that tells you exactly what's wrong before anything fails.

## The Problem

Gradle fails like this:
FAILURE: Build failed with an exception.

What went wrong: Execution failed for task ':app:processDebugMainManifest'.


Bundle fails like this:
[Bundle Error] react-native v0.73 is NOT compatible with Kotlin v2.0
-> Required Kotlin : 1.9.x
-> Fix in bundle.nextgen: kotlin = "1.9.x"

## Commands

| Command | Description |
|---|---|
| `bundle init` | Initialize project structure + bundle.nextgen config |
| `bundle template --blank` | Pick a framework and scaffold your project |
| `bundle make` | Resolve dependencies with compatibility check |
| `bundle build` | Compile and produce a signed APK |

## Supported Frameworks

- React Native (0.73 - 0.75)
- Flutter (3.16 - 3.22)
- Kotlin (1.9 - 2.0)
- Java (11, 17)

## bundle.nextgen

Every Bundle project has a `bundle.nextgen` config file:

```toml
[project]
name = "my-app"
version = "1.0.0"
author = ""

[framework]
name = "react-native"
version = "0.73"

[android]
min_sdk = 21
target_sdk = 34
ndk = "25.x"
kotlin = "1.9.x"

[dependencies]
# add your dependencies here
```

## Build from Source

```bash
git clone https://github.com/feranmi1295/bundle.git
cd bundle
make
sudo cp bundle /usr/local/bin/
```

## Requirements

- Android SDK with build-tools installed
- `ANDROID_HOME` set in your environment
- `javac` (JDK)
- `keytool` (JDK)
- `zip`

## Status

- [x] bundle init
- [x] bundle template
- [x] bundle make + compatibility matrix
- [x] bundle build (aapt2 + javac + d8 + zipalign + apksigner)
- [ ] bundle make --framework flag
- [ ] Kotlin source compilation
- [ ] dependency downloading
- [ ] bundle install

## License

MIT
