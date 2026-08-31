#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class DeviceNodeView;
@class TopologyView;

@protocol TopologyViewSelectionDelegate <NSObject>
- (void)topologyView:(TopologyView *)topologyView didSelectDevice:(nullable DeviceNodeView *)device;
@end

@interface TopologyView : NSView

@property(nonatomic, assign, getter=isConnectMode) BOOL connectMode;
@property(nonatomic, assign, readonly) NSUInteger deviceCount;
@property(nonatomic, assign, readonly) NSUInteger linkCount;
@property(nonatomic, weak, nullable) id<TopologyViewSelectionDelegate> selectionDelegate;

- (void)deleteSelection;
- (void)deleteDevice:(DeviceNodeView *)device;
- (void)showDevicePalette;
- (void)clearTopology;
- (void)fitTopology;
- (void)loadMilestone2DemoTopology;
- (void)loadMilestone3DemoTopology;
- (void)loadMilestone7DemoTopology;
- (NSString *)runPingFromDevice:(DeviceNodeView *)source targetAddress:(NSString *)targetAddress;
- (NSString *)requestDHCPForDevice:(DeviceNodeView *)device;
- (NSString *)advancedStatusForDevice:(DeviceNodeView *)device;
- (BOOL)runMilestone3SelfTest;
- (BOOL)runMilestone7SelfTest;
- (BOOL)runNodeDeletionSelfTest;

@end

NS_ASSUME_NONNULL_END
