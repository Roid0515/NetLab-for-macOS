#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class DeviceNodeView;
@class TopologyView;

@interface DeviceSidebarView : NSView
@property(nonatomic, assign, readonly, getter=isShowingDevicePalette) BOOL showingDevicePalette;
- (instancetype)initWithTopologyView:(TopologyView *)topologyView;
- (void)showDevice:(nullable DeviceNodeView *)device;
@end

NS_ASSUME_NONNULL_END
