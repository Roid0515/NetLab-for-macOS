#import "AppDelegate.h"

#import "MainWindowController.h"

@implementation AppDelegate {
    MainWindowController *_mainWindowController;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    _mainWindowController = [[MainWindowController alloc] init];
    [_mainWindowController showWindow:self];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}

@end
