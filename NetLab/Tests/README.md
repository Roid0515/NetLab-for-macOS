# Tests

Automated Core and simulation tests live in the separate `NetLabTests` target at the repository root. Production startup code contains no test command-line branches.

```sh
xcodebuild -project NetLab.xcodeproj -scheme NetLabTests -configuration Debug -derivedDataPath build build
./build/Build/Products/Debug/NetLabTests
```
