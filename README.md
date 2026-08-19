# CommandShift

CommandShift is a free and open-source macOS app that lets you change input source
with a Windows-style shortcut made of modifier keys only — Command + Shift,
Option + Shift, Control + Shift, Fn + Shift, or even Shift on its own. macOS does not
support modifier-only shortcuts by itself; CommandShift adds them.

## About this fork

The original CommandShift was written by **Vasyl Baran** and lives at
[VasylBaran/CommandShift](https://github.com/VasylBaran/CommandShift). All the credit
for the application belongs to him — this is a fork that builds on his work, kept for
personal use, and it remains under the same GPL-3.0 license.

What this fork changes:

* **No dependency on macOS' own input-source shortcut.** The original simulated a
  Ctrl+Space keystroke and relied on that binding still being in place, which meant
  giving up Ctrl+Space — the autocomplete shortcut in most IDEs. This fork asks Text
  Input Services to change the source directly, so no system shortcut needs to stay
  bound, and switching is immediate rather than going through the system's hotkey
  machinery.
* **Other shortcuts are left alone.** The language only changes when the shortcut's
  modifiers are pressed and released on their own. Press another key, click or scroll
  while holding them and CommandShift stays out of the way, so Cmd+Shift+A still does
  what the app under it expects. In Shift-only mode this also means typing capital
  letters no longer switches the layout.
* **Switching returns to the previously used input source**, the way Alt+Tab returns
  you to your last window, including when you changed source from the menu bar.
* **One settings file.** Every preference now lives in `~/.config/commandShift.ini`.
* **A build script**, so the app can be built, signed and installed in one command.

Because switching now has to distinguish a language change from the start of another
shortcut, it happens when the keys are released rather than when they are pressed.

## Installing

This fork has no prebuilt releases — build it from source with the instructions below.
Vasyl's original builds are available from
[his releases page](https://github.com/VasylBaran/CommandShift/releases/), but they
predate everything listed above.

Once installed:

1. Add CommandShift to "Privacy & Security -> Accessibility" so it can see key presses.
2. (optionally) Add CommandShift to your login items ("General -> Login Items").

## FAQ

* **Q:** I've added CommandShift to "Privacy & Security -> Accessibility" but it doesn't work =(
* **A:** Check that you have at least two input sources enabled in "Keyboard -> Text Input -> Input Sources", and that the switch next to CommandShift is actually turned on (adding the app to the list is not enough). If you have just replaced the app with a newer version, remove the old entry with the "-" button and add it again.
* **Q:** Do I need to keep macOS' own "Select the previous input source" shortcut (Ctrl+Space) bound?
* **A:** No. CommandShift changes the input source directly, so you are free to unbind Ctrl+Space in "Keyboard -> Keyboard Shortcuts -> Input Sources" and use it for something else.
* **Q:** Does CommandShift interfere with shortcuts like Cmd+Shift+A?
* **A:** No. If you press any other key, or click or scroll, while holding the modifiers, CommandShift leaves that shortcut alone and does not change the language. This is also why the language changes when you release the keys rather than when you press them: until you let go, there is no way to tell a language switch from the start of another shortcut.
* **Q:** How do I choose which shortcut should switch language?
* **A:** Find CommandShift ('CS' icon) in the menu bar at the top-right and click on it, there you'll find a "Change language with..." drop-down. Select the one you prefer.
* **Q:** I use 3+ languages. Which one does CommandShift switch to?
* **A:** The one you used before the current one, the same way Alt+Tab returns you to your last window. To reach a third language, pick it once from the input menu in the menu bar; CommandShift will then switch between that one and whichever you came from.
* **Q:** I want to switch language by just pressing the Shift key alone
* **A:** From CommandShift's menu bar go to the "Change language with..." drop-down and select "Shift". Typing capital letters will not switch anything, since Shift is then being held together with another key.
* **Q:** I want to hide the CommandShift icon from the menu bar
* **A:** From CommandShift's menu select "Tray menu icon..." and choose whether to hide it "permanently" or "until restart".
* **Q:** How do I automatically launch CommandShift when the computer starts?
* **A:** Add it to your login items: System Settings -> General -> Login Items -> [+] -> [select CommandShift].
* **Q:** I get a "CommandShift is damaged and can't be opened" error
* **A:** Run this in Terminal: `xattr -cr /Applications/CommandShift.app`

## Building from source (macOS)

### One-time setup

1. Xcode Command Line Tools:

   ```
   xcode-select --install
   ```

2. Qt 6:

   ```
   brew install qt
   ```

3. A code-signing identity. CommandShift needs Accessibility permission, and macOS ties
   that permission to the code signature. Without a stable signing identity you have to
   re-grant Accessibility after *every* rebuild. Creating a self-signed certificate once
   avoids that:

   ```
   openssl req -x509 -newkey rsa:2048 -nodes -days 3650 \
     -keyout key.pem -out cert.pem -subj "/CN=CommandShift Dev" \
     -addext "basicConstraints=critical,CA:false" \
     -addext "keyUsage=critical,digitalSignature" \
     -addext "extendedKeyUsage=critical,codeSigning"
   openssl pkcs12 -export -inkey key.pem -in cert.pem -out identity.p12 \
     -passout pass:csdev -name "CommandShift Dev"
   security import identity.p12 -k ~/Library/Keychains/login.keychain-db \
     -P csdev -T /usr/bin/codesign
   security add-trusted-cert -r trustRoot -p codeSign \
     -k ~/Library/Keychains/login.keychain-db cert.pem
   ```

   Verify with `security find-identity -v -p codesigning`.

### Build

```
./build.sh            # build + sign into ./build/CommandShift.app
./build.sh install    # ...and replace /Applications/CommandShift.app, then launch
```

Override the defaults with `QT_PREFIX` and `CS_SIGN_IDENTITY` if needed.

Note: a Homebrew Qt build is arm64-only and targets macOS 14+. Use the official Qt
installer if you need a universal binary or a lower deployment target.

## License

GPL-3.0, inherited from the original project. See [LICENSE](LICENSE).
