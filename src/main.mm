#include "constants.h"
#include "keypresscatcher.h"

#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>

#include <pwd.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{
    constexpr const char* kHideTrayIconSettingName = "tray_icon/hide";
    constexpr const char* kChangeLanguageOnReleaseSettingName = "language/trigger_on_key_release";
    constexpr const char* kLegacySettingsImportedKey = "migration/qt_settings_imported_v1";
    constexpr const char* kLegacyBundleIdentifier = "vasybaran.loveFromUkraine.CommandShift";

    NSString* Key(const char* key)
    {
        return [NSString stringWithUTF8String:key];
    }

    NSString* UserHomeDirectory()
    {
        passwd* user = getpwuid(getuid());
        if (user != nullptr && user->pw_dir != nullptr)
        {
            return [NSString stringWithUTF8String:user->pw_dir];
        }
        return NSHomeDirectory();
    }

    bool ParseBoolean(NSString* value, BOOL* result)
    {
        NSString* normalized = [value.lowercaseString
            stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
        if ([normalized isEqualToString:@"true"] ||
            [normalized isEqualToString:@"yes"] ||
            [normalized isEqualToString:@"1"] ||
            [normalized isEqualToString:@"on"])
        {
            *result = YES;
            return true;
        }
        if ([normalized isEqualToString:@"false"] ||
            [normalized isEqualToString:@"no"] ||
            [normalized isEqualToString:@"0"] ||
            [normalized isEqualToString:@"off"])
        {
            *result = NO;
            return true;
        }
        return false;
    }

    NSDictionary<NSString*, NSString*>* ParseLegacyIni(NSString* contents)
    {
        NSMutableDictionary<NSString*, NSString*>* values = [[NSMutableDictionary alloc] init];
        __block NSString* section = @"";
        [contents enumerateLinesUsingBlock:^(NSString* line, BOOL* stop) {
            (void)stop;
            NSString* trimmed = [line stringByTrimmingCharactersInSet:
                NSCharacterSet.whitespaceAndNewlineCharacterSet];
            if (trimmed.length == 0 || [trimmed hasPrefix:@"#"] || [trimmed hasPrefix:@";"])
            {
                return;
            }
            if ([trimmed hasPrefix:@"["] && [trimmed hasSuffix:@"]"] && trimmed.length > 2)
            {
                section = [trimmed substringWithRange:NSMakeRange(1, trimmed.length - 2)];
                return;
            }

            NSRange separator = [trimmed rangeOfString:@"="];
            if (separator.location == NSNotFound)
            {
                return;
            }
            NSString* setting = [[trimmed substringToIndex:separator.location]
                stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
            NSString* value = [[trimmed substringFromIndex:separator.location + 1]
                stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
            NSString* qualifiedSetting = section.length == 0
                ? setting
                : [NSString stringWithFormat:@"%@/%@", section, setting];
            values[qualifiedSetting] = value;
        }];
        return values;
    }

    void ImportLegacyBoolean(NSDictionary<NSString*, NSString*>* legacyValues,
                             NSUserDefaults* defaults,
                             const char* settingName)
    {
        NSString* setting = Key(settingName);
        if ([defaults objectForKey:setting] != nil)
        {
            return;
        }

        NSString* legacyValue = legacyValues[setting];
        BOOL parsedValue = NO;
        if (legacyValue != nil && ParseBoolean(legacyValue, &parsedValue))
        {
            [defaults setBool:parsedValue forKey:setting];
        }
    }

    bool ImportLegacyIniSettings(NSUserDefaults* defaults, NSString* homeDirectory)
    {
        NSString* path = [homeDirectory stringByAppendingPathComponent:@".config/commandShift.ini"];
        NSError* error = nil;
        NSString* contents = [NSString stringWithContentsOfFile:path
                                                       encoding:NSUTF8StringEncoding
                                                          error:&error];
        if (contents == nil)
        {
            if (error.code != NSFileReadNoSuchFileError && error.code != NSFileNoSuchFileError)
            {
                NSLog(@"CommandShift: could not read legacy settings at %@: %@", path, error);
                return false;
            }
            return true;
        }

        NSDictionary<NSString*, NSString*>* legacyValues = ParseLegacyIni(contents);
        ImportLegacyBoolean(legacyValues, defaults, kHideTrayIconSettingName);
        ImportLegacyBoolean(legacyValues, defaults, kChangeLanguageOnReleaseSettingName);
        return true;
    }

    bool ImportLegacyNativeSettings(NSUserDefaults* defaults, NSString* homeDirectory)
    {
        if ([defaults objectForKey:Key(CS::secondShortcutKeySettingKeyword)] != nil)
        {
            return true;
        }

        NSString* filename = [NSString stringWithFormat:@"%s.plist", kLegacyBundleIdentifier];
        NSString* path = [[homeDirectory stringByAppendingPathComponent:@"Library/Preferences"]
            stringByAppendingPathComponent:filename];
        NSError* readError = nil;
        NSData* data = [NSData dataWithContentsOfFile:path options:0 error:&readError];
        if (data == nil)
        {
            if (readError.code != NSFileReadNoSuchFileError && readError.code != NSFileNoSuchFileError)
            {
                NSLog(@"CommandShift: could not read legacy preferences at %@: %@", path, readError);
                return false;
            }
            return true;
        }

        NSError* plistError = nil;
        id plist = [NSPropertyListSerialization propertyListWithData:data
                                                             options:NSPropertyListImmutable
                                                              format:nil
                                                               error:&plistError];
        if (![plist isKindOfClass:NSDictionary.class])
        {
            NSLog(@"CommandShift: could not parse legacy preferences at %@: %@", path, plistError);
            return true;
        }

        NSNumber* storedShortcut = ((NSDictionary*)plist)[Key(CS::secondShortcutKeySettingKeyword)];
        NSInteger shortcutValue = storedShortcut.integerValue;
        if (CS::SecondShortcutKeyEnum::_FirstElem < shortcutValue &&
            shortcutValue < CS::SecondShortcutKeyEnum::_LastElem)
        {
            [defaults setInteger:shortcutValue forKey:Key(CS::secondShortcutKeySettingKeyword)];
        }
        return true;
    }

    void ImportLegacySettingsIfNeeded(NSUserDefaults* defaults)
    {
        NSString* migrationKey = Key(kLegacySettingsImportedKey);
        if ([defaults boolForKey:migrationKey])
        {
            return;
        }

        NSString* homeDirectory = UserHomeDirectory();
        bool importedIni = ImportLegacyIniSettings(defaults, homeDirectory);
        bool importedNative = ImportLegacyNativeSettings(defaults, homeDirectory);
        if (importedIni && importedNative)
        {
            [defaults setBool:YES forKey:migrationKey];
            [defaults synchronize];
        }
    }

    NSImage* LoadImageNamed(NSString* name)
    {
        NSString* path = [[NSBundle mainBundle] pathForResource:name ofType:nil];
        return path == nil ? nil : [[NSImage alloc] initWithContentsOfFile:path];
    }

    NSImage* LoadTrayImageSet(NSString* baseName)
    {
        NSArray<NSString*>* candidates = @[
            [NSString stringWithFormat:@"%@_20.png", baseName],
            [NSString stringWithFormat:@"%@_40.png", baseName],
            [NSString stringWithFormat:@"%@_60.png", baseName]
        ];
        NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(0, 0)];
        bool added = false;
        for (NSString* name in candidates)
        {
            NSImage* source = LoadImageNamed(name);
            for (NSImageRep* representation in source.representations)
            {
                [image addRepresentation:representation];
                added = true;
            }
        }
        return added ? image : nil;
    }

    bool HasInputMonitoringPermission()
    {
        return CGPreflightListenEventAccess();
    }
}

@interface AppController : NSObject <UNUserNotificationCenterDelegate>
@property(nonatomic, strong) NSStatusItem* statusItem;
@property(nonatomic, strong) NSMenu* menu;
@property(nonatomic, strong) NSMenu* shortcutMenu;
@property(nonatomic, strong) NSMenu* trayMenu;
@property(nonatomic, strong) NSMenuItem* changeLanguageOnReleaseItem;
@property(nonatomic, strong) NSArray<NSMenuItem*>* shortcutItems;
@property(nonatomic) BOOL notificationAuthorizationResolved;
@property(nonatomic) BOOL notificationsAllowed;
@property(nonatomic, strong) NSString* pendingMessageTitle;
@property(nonatomic, strong) NSString* pendingMessageBody;
@property(nonatomic) NSUserDefaults* defaults;
@property(nonatomic) KeyPressCatcher* catcher;
@end

@implementation AppController

- (instancetype)initWithDefaults:(NSUserDefaults*)defaults catcher:(KeyPressCatcher*)catcher
{
    self = [super init];
    if (self)
    {
        _defaults = defaults;
        _catcher = catcher;
    }
    return self;
}

- (void)openInputMonitoringSettings
{
    NSArray<NSURL*>* urls = @[
        [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent"],
        [NSURL URLWithString:@"x-apple.systempreferences:com.apple.Settings.PrivacySecurity?Privacy_ListenEvent"],
        [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security"],
        [NSURL URLWithString:@"x-apple.systempreferences:com.apple.Settings.PrivacySecurity"]
    ];
    for (NSURL* url in urls)
    {
        if (url != nil && [[NSWorkspace sharedWorkspace] openURL:url])
        {
            return;
        }
    }
}

- (void)openProjectPage:(id)sender
{
    (void)sender;
    [[NSWorkspace sharedWorkspace] openURL:
        [NSURL URLWithString:@"https://github.com/VasylBaran/CommandShift"]];
}

- (NSString*)settingsActionForMessageTitle:(NSString*)title
{
    if ([title isEqualToString:Key(CS::setupInputMonitoringTitle)] ||
        [title isEqualToString:Key(CS::inputMonitoringLostTitle)])
    {
        return @"open_input_monitoring";
    }
    return nil;
}

- (void)deliverNotificationWithTitle:(NSString*)title message:(NSString*)message
{
    NSString* settingsAction = [self settingsActionForMessageTitle:title];
    UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
    content.title = title;
    content.body = settingsAction == nil
        ? message
        : [message stringByAppendingString:@"Click this notification to open Input Monitoring settings."];
    if (settingsAction != nil)
    {
        content.userInfo = @{ @"action" : settingsAction };
    }

    UNTimeIntervalNotificationTrigger* trigger =
        [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:0.1 repeats:NO];
    UNNotificationRequest* request =
        [UNNotificationRequest requestWithIdentifier:NSUUID.UUID.UUIDString content:content trigger:trigger];
    [[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:request withCompletionHandler:nil];
}

- (void)presentPermissionAlertWithTitle:(NSString*)title message:(NSString*)message
{
    if ([self settingsActionForMessageTitle:title] == nil)
    {
        return;
    }

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = title;
    alert.informativeText = message;
    [alert addButtonWithTitle:@"Open Input Monitoring Settings"];
    [alert addButtonWithTitle:@"Not Now"];
    if ([alert runModal] == NSAlertFirstButtonReturn)
    {
        [self openInputMonitoringSettings];
    }
}

- (void)presentMessageWithTitle:(NSString*)title message:(NSString*)message
{
    if (self.notificationsAllowed)
    {
        [self deliverNotificationWithTitle:title message:message];
    }
    else
    {
        [self presentPermissionAlertWithTitle:title message:message];
    }
}

- (void)resolveNotificationAuthorization:(BOOL)allowed error:(NSError*)error
{
    dispatch_async(dispatch_get_main_queue(), ^{
        self.notificationAuthorizationResolved = YES;
        self.notificationsAllowed = allowed;
        if (error != nil)
        {
            NSLog(@"CommandShift: notification authorization failed: %@", error);
        }
        if (self.pendingMessageTitle != nil)
        {
            NSString* title = self.pendingMessageTitle;
            NSString* body = self.pendingMessageBody;
            self.pendingMessageTitle = nil;
            self.pendingMessageBody = nil;
            [self presentMessageWithTitle:title message:body];
        }
    });
}

- (void)configureNotifications
{
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    center.delegate = self;
    __weak typeof(self) weakSelf = self;
    [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
        if (settings.authorizationStatus != UNAuthorizationStatusNotDetermined)
        {
            BOOL allowed = settings.authorizationStatus == UNAuthorizationStatusAuthorized ||
                           settings.authorizationStatus == UNAuthorizationStatusProvisional;
            [weakSelf resolveNotificationAuthorization:allowed error:nil];
            return;
        }
        [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
                              completionHandler:^(BOOL granted, NSError* error) {
            [weakSelf resolveNotificationAuthorization:granted error:error];
        }];
    }];
}

- (void)showMessageWithTitle:(const std::string&)title message:(const std::string&)message
{
    NSString* messageTitle = [NSString stringWithUTF8String:title.c_str()];
    NSString* messageBody = [NSString stringWithUTF8String:message.c_str()];
    void (^handleMessage)(void) = ^{
        if (!self.notificationAuthorizationResolved)
        {
            self.pendingMessageTitle = messageTitle;
            self.pendingMessageBody = messageBody;
            return;
        }
        [self presentMessageWithTitle:messageTitle message:messageBody];
    };

    if (NSThread.isMainThread)
    {
        handleMessage();
    }
    else
    {
        dispatch_async(dispatch_get_main_queue(), handleMessage);
    }
}

- (void)buildMenu
{
    [self configureNotifications];
    self.menu = [[NSMenu alloc] initWithTitle:@"CommandShift"];

    NSMenuItem* projectPageItem = [[NSMenuItem alloc] initWithTitle:@"Visit CommandShift page"
                                                              action:@selector(openProjectPage:)
                                                       keyEquivalent:@""];
    projectPageItem.target = self;
    [self.menu addItem:projectPageItem];

    self.shortcutMenu = [[NSMenu alloc] initWithTitle:@"Change language with..."];
    NSMenuItem* shortcutRoot = [[NSMenuItem alloc] initWithTitle:@"Change language with..."
                                                          action:nil
                                                   keyEquivalent:@""];
    shortcutRoot.submenu = self.shortcutMenu;
    [self.menu addItem:shortcutRoot];

    std::vector<std::pair<NSString*, CS::SecondShortcutKeyEnum>> shortcuts = {
        {@"Shift + FN", CS::SecondShortcutKeyEnum::GlobalFN},
        {@"Shift + Control", CS::SecondShortcutKeyEnum::Control},
        {@"Shift + Option", CS::SecondShortcutKeyEnum::Option},
        {@"Shift + Command", CS::SecondShortcutKeyEnum::Command},
        {@"Shift", CS::SecondShortcutKeyEnum::Nothing}
    };
    NSMutableArray<NSMenuItem*>* shortcutItems = [[NSMutableArray alloc] init];
    for (const auto& shortcut : shortcuts)
    {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:shortcut.first
                                                     action:@selector(selectShortcut:)
                                              keyEquivalent:@""];
        item.tag = static_cast<NSInteger>(shortcut.second);
        item.target = self;
        [self.shortcutMenu addItem:item];
        [shortcutItems addObject:item];
    }
    self.shortcutItems = shortcutItems;

    self.changeLanguageOnReleaseItem =
        [[NSMenuItem alloc] initWithTitle:@"Change language after Shift release"
                                  action:@selector(toggleChangeOnRelease:)
                           keyEquivalent:@""];
    self.changeLanguageOnReleaseItem.target = self;
    [self.menu addItem:self.changeLanguageOnReleaseItem];

    self.trayMenu = [[NSMenu alloc] initWithTitle:@"Hide this from menu bar..."];
    NSMenuItem* trayRoot = [[NSMenuItem alloc] initWithTitle:@"Hide this from menu bar..."
                                                      action:nil
                                               keyEquivalent:@""];
    trayRoot.submenu = self.trayMenu;
    [self.menu addItem:trayRoot];

    NSMenuItem* hidePermanently = [[NSMenuItem alloc] initWithTitle:@"permanently"
                                                             action:@selector(hideIconPermanently:)
                                                      keyEquivalent:@""];
    hidePermanently.target = self;
    [self.trayMenu addItem:hidePermanently];

    NSMenuItem* hideUntilRestart = [[NSMenuItem alloc] initWithTitle:@"until restart"
                                                              action:@selector(hideIconUntilRestart:)
                                                       keyEquivalent:@""];
    hideUntilRestart.target = self;
    [self.trayMenu addItem:hideUntilRestart];
    [self.menu addItem:NSMenuItem.separatorItem];

    NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                      action:@selector(quitApp:)
                                               keyEquivalent:@""];
    quitItem.target = self;
    [self.menu addItem:quitItem];
}

- (void)applyInitialState
{
    CS::SecondShortcutKeyEnum selectedKey = self.catcher->getSecondShortcutKey();
    for (NSMenuItem* item in self.shortcutItems)
    {
        item.state = item.tag == static_cast<NSInteger>(selectedKey)
                         ? NSControlStateValueOn
                         : NSControlStateValueOff;
    }

    bool changeOnRelease = [self.defaults boolForKey:Key(kChangeLanguageOnReleaseSettingName)];
    self.catcher->setChangeLanguageOnRelease(changeOnRelease);
    self.changeLanguageOnReleaseItem.state = changeOnRelease
        ? NSControlStateValueOn
        : NSControlStateValueOff;

    if (![self.defaults boolForKey:Key(kHideTrayIconSettingName)])
    {
        [self createStatusItemIfNeeded];
    }
}

- (void)createStatusItemIfNeeded
{
    if (self.statusItem != nil)
    {
        return;
    }

    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    self.statusItem.menu = self.menu;
    NSImage* image = LoadTrayImageSet(@"icon");
    if (image != nil)
    {
        CGFloat size = NSStatusBar.systemStatusBar.thickness * 0.90;
        image.size = NSMakeSize(size, size);
        self.statusItem.button.image = image;
    }
    self.statusItem.button.toolTip = @"CommandShift (1.06) - developed by Vasyl Baran";
}

- (void)selectShortcut:(id)sender
{
    NSMenuItem* item = static_cast<NSMenuItem*>(sender);
    self.catcher->setSecondShortcutKey(static_cast<CS::SecondShortcutKeyEnum>(item.tag));
    [self.defaults setInteger:item.tag forKey:Key(CS::secondShortcutKeySettingKeyword)];
    for (NSMenuItem* shortcutItem in self.shortcutItems)
    {
        shortcutItem.state = shortcutItem == item ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

- (void)toggleChangeOnRelease:(id)sender
{
    (void)sender;
    bool enabled = !self.catcher->changeLanguageOnRelease();
    self.catcher->setChangeLanguageOnRelease(enabled);
    [self.defaults setBool:enabled forKey:Key(kChangeLanguageOnReleaseSettingName)];
    self.changeLanguageOnReleaseItem.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)hideIconPermanently:(id)sender
{
    (void)sender;
    [self.defaults setBool:YES forKey:Key(kHideTrayIconSettingName)];
    [self removeStatusItem];
}

- (void)hideIconUntilRestart:(id)sender
{
    (void)sender;
    [self removeStatusItem];
}

- (void)removeStatusItem
{
    if (self.statusItem != nil)
    {
        [[NSStatusBar systemStatusBar] removeStatusItem:self.statusItem];
        self.statusItem = nil;
    }
}

- (void)quitApp:(id)sender
{
    (void)sender;
    [NSApp terminate:nil];
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
    (void)center;
    (void)notification;
    if (@available(macOS 11.0, *))
    {
        completionHandler(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList);
    }
    else
    {
        completionHandler(UNNotificationPresentationOptionAlert);
    }
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
 didReceiveNotificationResponse:(UNNotificationResponse*)response
         withCompletionHandler:(void (^)(void))completionHandler
{
    (void)center;
    NSString* action = response.notification.request.content.userInfo[@"action"];
    if ([action isEqualToString:@"open_input_monitoring"] ||
        [action isEqualToString:@"open_required_permissions"])
    {
        [self openInputMonitoringSettings];
    }
    if (completionHandler != nil)
    {
        completionHandler();
    }
}

@end

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
        ImportLegacySettingsIfNeeded(defaults);

        AppController* controller = [[AppController alloc] initWithDefaults:defaults catcher:nil];
        KeyPressCatcher catcher(defaults, [controller](const std::string& title, const std::string& message) {
            [controller showMessageWithTitle:title message:message];
        });
        controller.catcher = &catcher;
        [controller buildMenu];
        [controller applyInitialState];
        [NSApp run];
    }
    return 0;
}
