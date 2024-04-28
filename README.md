# CommandShift
CommandShift is a free and open-source app that allows you to change input source using Windows-style shortcut (e.g. Command + Shift, Option + Shift, Control + Shift or Fn + Shift. It's customizable). By default MacOS doesn't support shortcuts that consist of modifier keys only.

# How-to use
1. Download Intel or ARM64(aka M1) dmg
2. Drag'n'drop CommandShift application from dmg-file to your Applications folder
3. Add CommandShift to "Security & Privacy -> Privacy -> Accessibility" in order for it to work properly
4. Enjoy!
5. (optionally) Add CommandShift to your startup items ("Users & Groups -> Login Items")

# FAQ:
* Q: I've added CommandShift to "Security & Privacy -> Privacy -> Accessibility" but it doesn't work =(
* A: Please make sure that your settings in "Keyboard -> Shortcuts -> Input Sources" are set to default (you can easily do that by pressing "Restore Defaults" button there)
* Q: How do I choose which shortcut should switch language?
* A: Find CommandShift ('CS' icon) in menu bar at the top-right and click on it, there you'll find "Change language with..." drop-down. Select the one you prefer =) 
* Q: I get "CommandShift is damaged and can't be opened" error
* A: Please run this command in Terminal: _xattr -cr /Applications/CommandShift.app_
* Q: How to automatically launch CommandShift when computer starts?
* A: You can add CommandShift to your list of auto-start items by opening System Settings panel -> Users and Groups OR General (depending on what version of macOS you're using) -> Login Items -> [+] -> [select CommandShift app]
* A: I use 3+ languages and CommandShift switches only between the last 2. How to make it cycle between all 3+ languages?
* Q: Press and hold second key (the one you chose from the "Change language with..." drop-down) the then press Shift multiple times in order to cycle through your languages. 
* Q: I want to switch language after releasing Shift key instead of when pressing it
* A: From CommandShift's menu bar at the top-right you can toggle 'Change language after Shift release'
* Q: I want to switch language by just pressing Shift key alone
* A: From CommandShift's menu bar go to "Change language with..." drop-down and select "Shift"
* Q: I want to hide CommandShift icon from menu tray
* A: From CommandShift's menu bar select "Hide icon from tray menu..." and choose whether you want to hide it "Permanently" or "Until restart"
* Q: I have Arm/Intel based Mac. Is CommandShift going to work on both? 
* A: Yes, CommandShift is a universal app and runs natively on both Arm and Intel Macs.

# Downloads
* Bitbucket: https://bitbucket.org/vasylbaran7/downloads/downloads/
* Sourceforge: https://sourceforge.net/projects/commandshift/files/
* GitHub: https://github.com/VasylBaran/CommandShift/releases/

**Latest installer checksums (MD5)**
* ARM64: 3d5bc5abce1ffac17503955b4398e4ab
* Intel: e0b725bf85666ef517145ce12d8da519

# Support 
If you have found CommandShift useful and would like to say 'thank you' please consider supporting my endeavors. It encourages me to further support and update CommandShift as new versions of MacOS come out. 
* PayPal (one-time): https://www.paypal.com/donate/?hosted_button_id=WZAJV3PYPWUHA
* Patreon (monthly): https://www.patreon.com/Vasyl_Baran


I'm a Software engineer and Digital artist and I'm glad to be useful.
Besides CommandShift I also have a few other projects in different mediums and even more new and exiting ideas. 
Subscribe to me on Patreon and I'll keep you posted, I promise! ;-)
