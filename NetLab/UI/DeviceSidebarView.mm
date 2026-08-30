#import "DeviceSidebarView.h"

#import "DeviceInspectorView.h"
#import "DevicePaletteView.h"

@implementation DeviceSidebarView {
    DevicePaletteView *_paletteView;
    DeviceInspectorView *_inspectorView;
}

- (BOOL)isShowingDevicePalette { return !_paletteView.hidden; }

- (instancetype)initWithTopologyView:(TopologyView *)topologyView {
    self = [super initWithFrame:NSZeroRect];
    if (self) {
        _paletteView = [[DevicePaletteView alloc] initWithFrame:NSZeroRect];
        _inspectorView = [[DeviceInspectorView alloc] initWithTopologyView:topologyView];
        _inspectorView.hidden = YES;
        [self addSubview:_paletteView];
        [self addSubview:_inspectorView];
    }
    return self;
}

- (void)layout {
    [super layout];
    _paletteView.frame = self.bounds;
    _inspectorView.frame = self.bounds;
}

- (void)showDevice:(DeviceNodeView *)device {
    _paletteView.hidden = device != nil;
    _inspectorView.hidden = device == nil;
    if (device) [_inspectorView inspectDevice:device];
}

@end
