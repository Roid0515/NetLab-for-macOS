#import "LinkLayerController.h"

#import <QuartzCore/QuartzCore.h>

#import "DeviceNodeView.h"
#include "../Core/Link.hpp"

#include <memory>

@implementation LinkLayerController {
    std::unique_ptr<netlab::Link> _model;
    CAShapeLayer *_lineLayer;
    CAShapeLayer *_firstPulseLayer;
    CAShapeLayer *_secondPulseLayer;
    BOOL _valid;
}

- (instancetype)initWithFirstNode:(DeviceNodeView *)firstNode
                   firstInterface:(NSString *)firstInterface
                       secondNode:(DeviceNodeView *)secondNode
                  secondInterface:(NSString *)secondInterface
                        speedMbps:(NSInteger)speedMbps {
    self = [super init];
    if (self) {
        _linkIdentifier = NSUUID.UUID.UUIDString;
        _firstNode = firstNode;
        _secondNode = secondNode;
        _firstInterfaceName = [firstInterface copy];
        _secondInterfaceName = [secondInterface copy];
        _valid = YES;

        netlab::LinkEndpoint firstEndpoint{firstNode.instanceIdentifier.UTF8String,
                                            firstInterface.UTF8String};
        netlab::LinkEndpoint secondEndpoint{secondNode.instanceIdentifier.UTF8String,
                                             secondInterface.UTF8String};
        _model = std::make_unique<netlab::Link>(_linkIdentifier.UTF8String,
                                                std::move(firstEndpoint),
                                                std::move(secondEndpoint),
                                                static_cast<int>(speedMbps));

        _lineLayer = [CAShapeLayer layer];
        _lineLayer.fillColor = NSColor.clearColor.CGColor;
        _lineLayer.lineCap = kCALineCapRound;
        _lineLayer.lineWidth = 3.0;

        _firstPulseLayer = [CAShapeLayer layer];
        _secondPulseLayer = [CAShapeLayer layer];
        for (CAShapeLayer *pulse in @[_firstPulseLayer, _secondPulseLayer]) {
            pulse.bounds = CGRectMake(0, 0, 10, 10);
            pulse.path = [NSBezierPath bezierPathWithOvalInRect:NSMakeRect(0, 0, 10, 10)].CGPath;
            pulse.opacity = 0.0;
        }
        [self applyStateColor:NSColor.systemGrayColor animated:NO];
    }
    return self;
}

- (void)installInTopologyLayer:(CALayer *)topologyLayer {
    [topologyLayer insertSublayer:_lineLayer atIndex:0];
    [topologyLayer insertSublayer:_firstPulseLayer above:_lineLayer];
    [topologyLayer insertSublayer:_secondPulseLayer above:_lineLayer];
    [self updateGeometry];

    __weak LinkLayerController *weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.65 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        LinkLayerController *strongSelf = weakSelf;
        if (!strongSelf || !strongSelf->_valid) return;
        if (strongSelf->_model->speedMbps() <= 0) {
            strongSelf->_model->setState(netlab::LinkState::Down);
            [strongSelf applyStateColor:NSColor.systemGrayColor animated:NO];
            return;
        }
        strongSelf->_model->setState(netlab::LinkState::Up);
        NSColor *color = strongSelf->_model->speedMbps() <= 100
            ? NSColor.systemOrangeColor : NSColor.systemGreenColor;
        [strongSelf applyStateColor:color animated:YES];
    });
}

- (const netlab::Link &)model { return *_model; }

- (void)applyStateColor:(NSColor *)color animated:(BOOL)animated {
    NSColor *deviceColor = [color colorUsingColorSpace:NSColorSpace.deviceRGBColorSpace] ?: color;
    _lineLayer.strokeColor = deviceColor.CGColor;
    _firstPulseLayer.fillColor = deviceColor.CGColor;
    _secondPulseLayer.fillColor = deviceColor.CGColor;

    for (CAShapeLayer *pulse in @[_firstPulseLayer, _secondPulseLayer]) {
        [pulse removeAnimationForKey:@"trafficPulse"];
        pulse.opacity = animated ? 0.35 : 0.0;
        if (animated) {
            CABasicAnimation *animation = [CABasicAnimation animationWithKeyPath:@"opacity"];
            animation.fromValue = @0.2;
            animation.toValue = @1.0;
            animation.duration = 1.35;  // Idle traffic indication; later traffic changes this rate.
            animation.autoreverses = YES;
            animation.repeatCount = HUGE_VALF;
            [pulse addAnimation:animation forKey:@"trafficPulse"];
        }
    }
}

- (void)setSelected:(BOOL)selected {
    _selected = selected;
    _lineLayer.lineWidth = selected ? 5.0 : 3.0;
}

- (void)updateGeometry {
    DeviceNodeView *firstNode = self.firstNode;
    DeviceNodeView *secondNode = self.secondNode;
    if (!firstNode || !secondNode) return;
    NSPoint first = NSMakePoint(NSMidX(firstNode.frame), NSMidY(firstNode.frame));
    NSPoint second = NSMakePoint(NSMidX(secondNode.frame), NSMidY(secondNode.frame));
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathMoveToPoint(path, nullptr, first.x, first.y);
    CGPathAddLineToPoint(path, nullptr, second.x, second.y);
    _lineLayer.path = path;
    CGPathRelease(path);
    _firstPulseLayer.position = first;
    _secondPulseLayer.position = second;
}

- (BOOL)containsPoint:(NSPoint)point tolerance:(CGFloat)tolerance {
    NSPoint a = NSMakePoint(NSMidX(self.firstNode.frame), NSMidY(self.firstNode.frame));
    NSPoint b = NSMakePoint(NSMidX(self.secondNode.frame), NSMidY(self.secondNode.frame));
    CGFloat dx = b.x - a.x;
    CGFloat dy = b.y - a.y;
    CGFloat lengthSquared = dx * dx + dy * dy;
    if (lengthSquared < 0.001) return NO;
    CGFloat t = ((point.x - a.x) * dx + (point.y - a.y) * dy) / lengthSquared;
    t = MIN(1.0, MAX(0.0, t));
    CGFloat nearestX = a.x + t * dx;
    CGFloat nearestY = a.y + t * dy;
    return hypot(point.x - nearestX, point.y - nearestY) <= tolerance;
}

- (BOOL)isAttachedToNode:(DeviceNodeView *)node {
    return self.firstNode == node || self.secondNode == node;
}

- (void)invalidate {
    _valid = NO;
    [_firstPulseLayer removeAllAnimations];
    [_secondPulseLayer removeAllAnimations];
    [_lineLayer removeFromSuperlayer];
    [_firstPulseLayer removeFromSuperlayer];
    [_secondPulseLayer removeFromSuperlayer];
}

@end
