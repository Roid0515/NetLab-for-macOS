#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class DeviceNodeView;
@class TopologyView;

@interface DeviceInspectorView : NSView

- (instancetype)initWithTopologyView:(TopologyView *)topologyView;
- (void)inspectDevice:(DeviceNodeView *)device;

@end

NS_ASSUME_NONNULL_END
