#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class DeviceNodeView;

@interface LinkLayerController : NSObject

@property(nonatomic, copy, readonly) NSString *linkIdentifier;
@property(nonatomic, weak, readonly) DeviceNodeView *firstNode;
@property(nonatomic, weak, readonly) DeviceNodeView *secondNode;
@property(nonatomic, copy, readonly) NSString *firstInterfaceName;
@property(nonatomic, copy, readonly) NSString *secondInterfaceName;
@property(nonatomic, assign, getter=isSelected) BOOL selected;

- (instancetype)initWithFirstNode:(DeviceNodeView *)firstNode
                   firstInterface:(NSString *)firstInterface
                       secondNode:(DeviceNodeView *)secondNode
                  secondInterface:(NSString *)secondInterface
                        speedMbps:(NSInteger)speedMbps;
- (void)installInTopologyLayer:(CALayer *)topologyLayer;
- (void)updateGeometry;
- (BOOL)containsPoint:(NSPoint)point tolerance:(CGFloat)tolerance;
- (BOOL)isAttachedToNode:(DeviceNodeView *)node;
- (void)invalidate;

@end

NS_ASSUME_NONNULL_END
