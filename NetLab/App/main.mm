#import <AppKit/AppKit.h>

#import "AppDelegate.h"

// NSApplication does not retain its delegate. Keep it alive for the full process lifetime.
static AppDelegate *NLApplicationDelegate;

static void NLInstallMainMenu(void) {
    NSMenu *menuBar = [[NSMenu alloc] init];
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [menuBar addItem:appMenuItem];
    NSApp.mainMenu = menuBar;

    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"NetLab"];
    NSString *quitTitle = [NSString stringWithFormat:@"Quit %@", NSProcessInfo.processInfo.processName];
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:quitTitle
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"];
    [appMenu addItem:quitItem];
    appMenuItem.submenu = appMenu;
}

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        application.activationPolicy = NSApplicationActivationPolicyRegular;
        NLInstallMainMenu();
        NLApplicationDelegate = [[AppDelegate alloc] init];
        application.delegate = NLApplicationDelegate;
        [application run];
    }
    return 0;
}
