#import <AppKit/AppKit.h>

#import "../NetLab/App/MainWindowController.h"
#import "../NetLab/UI/DeviceNodeView.h"
#import "../NetLab/UI/DeviceSidebarView.h"
#import "../NetLab/UI/TopologyView.h"

extern "C" bool NLRunUISmokeTests(void) {
    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        application.activationPolicy = NSApplicationActivationPolicyProhibited;
        MainWindowController *controller = [[MainWindowController alloc] init];
        [controller.window.contentView layoutSubtreeIfNeeded];

        TopologyView *topology = [controller valueForKey:@"topologyView"];
        DeviceSidebarView *sidebar = [controller valueForKey:@"sidebarView"];
        if (!topology || !sidebar || controller.window.contentView.subviews.count != 2 ||
            NSWidth(topology.frame) <= 0 || NSHeight(topology.frame) <= 0) return false;

        [topology loadMilestone7DemoTopology];
        NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:0.8];
        while (deadline.timeIntervalSinceNow > 0) {
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:deadline];
        }
        if (topology.deviceCount != 8 || topology.linkCount != 7) return false;

        NSArray<DeviceNodeView *> *nodes = [topology valueForKey:@"nodes"];
        DeviceNodeView *source = nil;
        for (DeviceNodeView *node in nodes) {
            if ([node.displayName isEqualToString:@"PC1"]) source = node;
        }
        if (!source) return false;
        NSString *dhcp = [topology requestDHCPForDevice:source];
        NSString *ping = [topology runPingFromDevice:source targetAddress:@"pc2.netlab"];
        if (![dhcp containsString:@"SUCCESS"] || ![ping containsString:@"SUCCESS"] ||
            ![ping containsString:@"Routing"] || ![ping containsString:@"NAT"]) return false;

        [topology deleteDevice:source];
        if (topology.deviceCount != 7 || topology.linkCount != 6) return false;
        [topology showDevicePalette];
        return sidebar.isShowingDevicePalette;
    }
}
