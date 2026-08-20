[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner2-direct.svg)](https://vshymanskyy.github.io/StandWithUkraine/)

# Backstory
Initially I wrote this program for myself since I bought my first Macbook and quickly realized that I can't use 'Alt'+Shift to change language on macOS. 
After a while a friend of mine bought his first Macbook and asked me "How can I change language using 'Alt'+Shift? I can't seem to change it in system preferences". I shared my app with him and at that point I realized that other people might find it useful as well so decided to share this app with the world.

# CommandShift
CommandShift is a free and open-source app that allows you to change input source using Windows-style shortcut (e.g. Command + Shift, Option + Shift, Control + Shift, Fn + Shift or even just Shift. It's quite customizable). 
By default MacOS doesn't support shortcuts that consist of modifier keys only. CommandShift solves this problem.

# Why Input Monitoring permission is required
CommandShift listens for the selected modifier-key combination even when another app is focused. MacOS requires Input Monitoring permission to observe these global keyboard events.
CommandShift uses this permission only to detect modifier-only shortcuts.

# Support
I'm a single software engineer from Ukraine. If you found CommandShift useful and would like to say 'thank you' please consider supporting me on [Patreon](https://www.patreon.com/Vasyl_Baran) or [PayPal](https://www.paypal.com/donate/?hosted_button_id=WZAJV3PYPWUHA). It encourages me to further improve, support and update CommandShift as new versions of MacOS come out.

# How-to use
1. Download CommandShift-universal.dmg
2. Drag'n'drop CommandShift application from dmg-file to your Applications folder
3. Allow CommandShift in "Privacy & Security -> Input Monitoring" when prompted
4. Enjoy!
5. (optionally) Add CommandShift to your startup items ("General -> Login Items" on newer MacOS versions or "Users & Groups -> Login Items" on older versions)

# Build from source
CommandShift is a native MacOS application built using CMake and Xcode:

```bash
cmake -S . -B build -G Xcode
open build/CommandShift.xcodeproj
```

The generated project builds a universal app for Arm and Intel Macs and supports MacOS 10.15+.

# FAQ:
* **Q:** I've upgraded to macOS Tahoe and now language input source changes by itself
* **A:** Try CommandShift v1.06 ;-)
* **Q:** I've enabled CommandShift in "Privacy & Security -> Input Monitoring" but it doesn't work =(
* **A:** If MacOS asks you to restart CommandShift after granting permission, quit and reopen the app
* **Q:** Do my settings in "Keyboard -> Shortcuts -> Input Sources" need to use the default shortcut?
* **A:** No. CommandShift now selects the next enabled input source directly using the public MacOS Text Input Source APIs
* **Q:** How do I choose which shortcut should switch language?
* **A:** Find CommandShift ('CS' icon) in menu bar at the top-right and click on it, there you'll find "Change language with..." drop-down. Select the one you prefer =) 
* **Q:** I get "CommandShift is damaged and can't be opened" error
* **A:** Please run this command in Terminal: _xattr -cr /Applications/CommandShift.app_
* **Q:** How to automatically launch CommandShift when computer starts?
* **A:** You can add CommandShift to your list of auto-start items by opening System Settings panel -> Users and Groups OR General (depending on what version of macOS you're using) -> Login Items -> [+] -> [select CommandShift app]
* **Q:** I use 3+ languages. Can CommandShift cycle through all of them?
* **A:** Yes, CommandShift cycles through all currently enabled input sources
* **Q:** I want to switch language by just pressing Shift key alone
* **A:** From CommandShift's menu bar go to "Change language with..." drop-down and select "Shift"
* **Q:** I want to hide CommandShift icon from menu tray
* **A:** From CommandShift's menu bar select "Hide this from menu bar..." and choose whether you want to hide it "permanently" or "until restart"
* **Q:** I hid the CommandShift icon permanently but changed my mind. How do I make it visible again?
* **A:** Run `killall CommandShift; defaults write vasybaran.loveFromUkraine.CommandShift "tray_icon/hide" -bool false; open -a CommandShift` in Terminal
* **Q:** I have Arm/Intel based Mac. Is CommandShift going to work on both?
* **A:** Yes, CommandShift is a universal app and runs natively on both Arm and Intel Macs.

# Downloads
* Sourceforge: https://sourceforge.net/projects/commandshift/files/
* GitHub: https://github.com/VasylBaran/CommandShift/releases/

**Latest (v1.06) CommandShift checksum (MD5)**
* CommandShift-universal.dmg: 1181579b479d484781d224c859c5d976

Again, if you have found CommandShift useful please consider supporting my endeavors. It encourages me to further improve, support and update CommandShift as new versions of MacOS come out:
* **Buy Me a Coffee (Stripe) (one-time):** https://buymeacoffee.com/vasylbaran
* **Patreon (monthly):** https://www.patreon.com/Vasyl_Baran

And remember, stay Safe and stay Strong! 🇺🇦
