# Exe Integrity Bypass Against RGL

This mod disables the integrity check (version check) against the exe of GTA V, where the game will test if it is the latest version as of the game launch using the function that calls `CryptAcquireContextA`, `CryptMsgGetParam`, and `CryptQueryObject`. Looks like Rockstar started testing even in Steam and EGS versions starting from April 4th, 2023 UTC at the latest.

In short words, this mod just makes GTA5.exe not listen to the result of version check.

### License
Exe Integrity Bypass Against RGL is licensed under the BSD Zero Clause License. Too few pieces of materials to be worth keeping secret.
