#import "MainWindowController.h"

#import "../UI/DeviceSidebarView.h"
#import "../UI/TopologyView.h"

static NSToolbarItemIdentifier const NLToolbarNew = @"netlab.new";
static NSToolbarItemIdentifier const NLToolbarDemo = @"netlab.demo";
static NSToolbarItemIdentifier const NLToolbarAddDevice = @"netlab.add-device";
static NSToolbarItemIdentifier const NLToolbarConnect = @"netlab.connect";
static NSToolbarItemIdentifier const NLToolbarDelete = @"netlab.delete";
static NSToolbarItemIdentifier const NLToolbarFit = @"netlab.fit";

@interface NLMainContentView : NSView
- (instancetype)initWithTopologyView:(NSView *)topology paletteView:(NSView *)palette;
@end

@implementation NLMainContentView {
    NSView *_topology;
    NSView *_palette;
}

- (instancetype)initWithTopologyView:(NSView *)topology paletteView:(NSView *)palette {
    self = [super initWithFrame:NSZeroRect];
    if (self) {
        _topology = topology;
        _palette = palette;
        [self addSubview:_topology];
        [self addSubview:_palette];
    }
    return self;
}

- (void)layout {
    [super layout];
    // Keep a borderless 75:25 layout; the palette retains a practical minimum width.
    CGFloat paletteWidth = MAX(250.0, floor(NSWidth(self.bounds) * 0.25));
    paletteWidth = MIN(paletteWidth, MAX(250.0, NSWidth(self.bounds) - 520.0));
    CGFloat topologyWidth = NSWidth(self.bounds) - paletteWidth;
    _topology.frame = NSMakeRect(0, 0, topologyWidth, NSHeight(self.bounds));
    _palette.frame = NSMakeRect(topologyWidth, 0, paletteWidth, NSHeight(self.bounds));
}

@end

@interface MainWindowController () <NSToolbarDelegate, TopologyViewSelectionDelegate>
@end

@implementation MainWindowController {
    TopologyView *_topologyView;
    DeviceSidebarView *_sidebarView;
    NSToolbarItem *_connectToolbarItem;
}

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, 1180, 760);
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                              NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    self = [super initWithWindow:window];
    if (self) {
        window.title = @"NetLab";
        window.subtitle = @"Topology Lab";
        window.minSize = NSMakeSize(800, 520);
        window.releasedWhenClosed = NO;
        window.titlebarAppearsTransparent = NO;
        [window center];

        _topologyView = [[TopologyView alloc] initWithFrame:NSZeroRect];
        _topologyView.selectionDelegate = self;
        _sidebarView = [[DeviceSidebarView alloc] initWithTopologyView:_topologyView];
        NLMainContentView *contentView = [[NLMainContentView alloc] initWithTopologyView:_topologyView
                                                                             paletteView:_sidebarView];
        contentView.frame = NSMakeRect(0, 0, NSWidth(frame), NSHeight(frame));
        window.contentView = contentView;
        [contentView layoutSubtreeIfNeeded];

        NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:@"netlab.main-toolbar"];
        toolbar.delegate = self;
        toolbar.displayMode = NSToolbarDisplayModeIconAndLabel;
        toolbar.allowsUserCustomization = NO;
        window.toolbar = toolbar;
    }
    return self;
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar {
    return @[NLToolbarNew, NLToolbarDemo, NLToolbarAddDevice, NSToolbarFlexibleSpaceItemIdentifier,
             NLToolbarConnect, NLToolbarDelete, NLToolbarFit];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar {
    return @[NLToolbarNew, NLToolbarDemo, NLToolbarAddDevice, NSToolbarFlexibleSpaceItemIdentifier,
             NLToolbarConnect, NLToolbarDelete, NLToolbarFit];
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
     itemForItemIdentifier:(NSToolbarItemIdentifier)identifier
 willBeInsertedIntoToolbar:(BOOL)flag {
    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:identifier];
    item.target = self;
    if ([identifier isEqualToString:NLToolbarNew]) {
        item.label = @"New";
        item.image = [NSImage imageWithSystemSymbolName:@"doc.badge.plus" accessibilityDescription:@"New topology"];
        item.action = @selector(newTopology:);
    } else if ([identifier isEqualToString:NLToolbarDemo]) {
        item.label = @"Demo";
        item.image = [NSImage imageWithSystemSymbolName:@"point.3.connected.trianglepath.dotted"
                               accessibilityDescription:@"Load link demo"];
        item.action = @selector(loadDemoTopology:);
    } else if ([identifier isEqualToString:NLToolbarAddDevice]) {
        item.label = @"Add Device";
        item.image = [NSImage imageWithSystemSymbolName:@"plus.square.on.square"
                               accessibilityDescription:@"Show device palette"];
        item.toolTip = @"Deselect the current node and show the device palette";
        item.action = @selector(showDevicePalette:);
    } else if ([identifier isEqualToString:NLToolbarConnect]) {
        item.label = @"Connect";
        item.image = [NSImage imageWithSystemSymbolName:@"cable.connector"
                               accessibilityDescription:@"Connect devices"];
        item.action = @selector(toggleConnectMode:);
        _connectToolbarItem = item;
    } else if ([identifier isEqualToString:NLToolbarDelete]) {
        item.label = @"Delete";
        item.image = [NSImage imageWithSystemSymbolName:@"trash" accessibilityDescription:@"Delete selection"];
        item.action = @selector(deleteSelection:);
    } else if ([identifier isEqualToString:NLToolbarFit]) {
        item.label = @"Fit";
        item.image = [NSImage imageWithSystemSymbolName:@"arrow.up.left.and.arrow.down.right" accessibilityDescription:@"Fit topology"];
        item.action = @selector(fitTopology:);
    }
    return item;
}

- (void)newTopology:(id)sender {
    if (_topologyView.subviews.count == 0) return;
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Create a new topology?";
    alert.informativeText = @"The current topology will be cleared.";
    [alert addButtonWithTitle:@"New"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertFirstButtonReturn) [self->_topologyView clearTopology];
    }];
}

- (void)deleteSelection:(id)sender { [_topologyView deleteSelection]; }
- (void)showDevicePalette:(id)sender { [_topologyView showDevicePalette]; }
- (void)fitTopology:(id)sender { [_topologyView fitTopology]; }
- (void)loadDemoTopology:(id)sender { [_topologyView loadMilestone7DemoTopology]; }

- (void)toggleConnectMode:(id)sender {
    _topologyView.connectMode = !_topologyView.isConnectMode;
    _connectToolbarItem.label = _topologyView.isConnectMode ? @"Cancel Connect" : @"Connect";
    _connectToolbarItem.image = [NSImage imageWithSystemSymbolName:
        (_topologyView.isConnectMode ? @"xmark.circle" : @"cable.connector")
        accessibilityDescription:@"Connect devices"];
}

- (BOOL)runLayoutSelfTest {
    [self.window.contentView layoutSubtreeIfNeeded];
    if (NSWidth(self.window.contentView.bounds) <= 0 || NSHeight(self.window.contentView.bounds) <= 0) return NO;
    if (NSWidth(_topologyView.frame) <= 0 || NSHeight(_topologyView.frame) <= 0) return NO;
    if (self.window.contentView.subviews.count != 2) return NO;
    NSView *palette = self.window.contentView.subviews[1];
    if (NSWidth(palette.frame) < 240 || NSHeight(palette.frame) <= 0) return NO;
    BOOL demoPassed = [_topologyView runMilestone7SelfTest] &&
                      _topologyView.deviceCount == 8 && _topologyView.linkCount == 7;
    [_topologyView showDevicePalette];
    return demoPassed && _sidebarView.isShowingDevicePalette;
}

- (void)topologyView:(TopologyView *)topologyView didSelectDevice:(DeviceNodeView *)device {
    [_sidebarView showDevice:device];
}

@end
