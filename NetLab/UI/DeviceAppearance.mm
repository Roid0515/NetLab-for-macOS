#import "DeviceAppearance.h"

NSColor *NLAccentColorForDeviceIdentifier(NSString *identifier) {
    if ([identifier isEqualToString:@"generic-router"]) {
        return [NSColor systemBlueColor];
    }
    if ([identifier isEqualToString:@"l2-switch"]) {
        return [NSColor systemTealColor];
    }
    if ([identifier isEqualToString:@"firewall"]) return [NSColor systemRedColor];
    if ([identifier isEqualToString:@"wireless-ap"]) return [NSColor systemOrangeColor];
    if ([identifier isEqualToString:@"generic-server"]) return [NSColor systemPurpleColor];
    return [NSColor systemIndigoColor];
}

static void NLStrokeArrow(NSPoint start, NSPoint end, NSColor *color) {
    NSBezierPath *path = [NSBezierPath bezierPath];
    path.lineWidth = 1.8;
    path.lineCapStyle = NSLineCapStyleRound;
    [path moveToPoint:start];
    [path lineToPoint:end];
    [color setStroke];
    [path stroke];
}

void NLDrawDeviceGlyph(NSString *identifier, NSRect bounds, NSColor *color) {
    NSRect glyph = NSInsetRect(bounds, 5.0, 5.0);
    [color setStroke];

    if ([identifier isEqualToString:@"generic-router"]) {
        NSBezierPath *circle = [NSBezierPath bezierPathWithOvalInRect:glyph];
        circle.lineWidth = 2.0;
        [circle stroke];
        NSPoint center = NSMakePoint(NSMidX(glyph), NSMidY(glyph));
        NLStrokeArrow(NSMakePoint(center.x - 7, center.y), NSMakePoint(center.x + 7, center.y), color);
        NLStrokeArrow(NSMakePoint(center.x, center.y - 7), NSMakePoint(center.x, center.y + 7), color);
        return;
    }

    if ([identifier isEqualToString:@"l2-switch"]) {
        NSBezierPath *box = [NSBezierPath bezierPathWithRoundedRect:glyph xRadius:4 yRadius:4];
        box.lineWidth = 2.0;
        [box stroke];
        for (NSInteger index = 0; index < 4; ++index) {
            CGFloat x = NSMinX(glyph) + 6.0 + index * 7.0;
            NSRect port = NSMakeRect(x, NSMidY(glyph) - 2.0, 4.0, 4.0);
            NSBezierPath *portPath = [NSBezierPath bezierPathWithRect:port];
            [portPath fill];
        }
        return;
    }

    if ([identifier isEqualToString:@"firewall"]) {
        NSBezierPath *shield = [NSBezierPath bezierPath];
        [shield moveToPoint:NSMakePoint(NSMidX(glyph), NSMaxY(glyph))];
        [shield lineToPoint:NSMakePoint(NSMaxX(glyph), NSMaxY(glyph) - 7)];
        [shield lineToPoint:NSMakePoint(NSMaxX(glyph) - 4, NSMinY(glyph) + 8)];
        [shield lineToPoint:NSMakePoint(NSMidX(glyph), NSMinY(glyph))];
        [shield lineToPoint:NSMakePoint(NSMinX(glyph) + 4, NSMinY(glyph) + 8)];
        [shield lineToPoint:NSMakePoint(NSMinX(glyph), NSMaxY(glyph) - 7)];
        [shield closePath]; shield.lineWidth = 2.0; [shield stroke];
        return;
    }

    if ([identifier isEqualToString:@"wireless-ap"]) {
        NSBezierPath *base = [NSBezierPath bezierPathWithOvalInRect:NSMakeRect(NSMidX(glyph)-3, NSMinY(glyph), 6, 6)];
        [base fill];
        for (CGFloat inset = 3; inset <= 11; inset += 4) {
            NSBezierPath *arc = [NSBezierPath bezierPath];
            [arc appendBezierPathWithArcWithCenter:NSMakePoint(NSMidX(glyph), NSMinY(glyph)+3)
                                            radius:inset startAngle:35 endAngle:145];
            arc.lineWidth = 2.0; [arc stroke];
        }
        return;
    }

    if ([identifier isEqualToString:@"generic-server"]) {
        NSBezierPath *rack = [NSBezierPath bezierPathWithRoundedRect:glyph xRadius:3 yRadius:3];
        rack.lineWidth = 2.0; [rack stroke];
        for (NSInteger row = 1; row < 3; ++row) {
            CGFloat y = NSMinY(glyph) + row * NSHeight(glyph) / 3.0;
            NLStrokeArrow(NSMakePoint(NSMinX(glyph)+3, y), NSMakePoint(NSMaxX(glyph)-3, y), color);
        }
        return;
    }

    NSRect screen = NSMakeRect(NSMinX(glyph) + 3.0, NSMinY(glyph) + 7.0,
                               NSWidth(glyph) - 6.0, NSHeight(glyph) - 10.0);
    NSBezierPath *monitor = [NSBezierPath bezierPathWithRoundedRect:screen xRadius:3 yRadius:3];
    monitor.lineWidth = 2.0;
    [monitor stroke];
    NLStrokeArrow(NSMakePoint(NSMidX(glyph), NSMinY(glyph) + 2.0),
                  NSMakePoint(NSMidX(glyph), NSMinY(screen)), color);
    NLStrokeArrow(NSMakePoint(NSMidX(glyph) - 7.0, NSMinY(glyph) + 2.0),
                  NSMakePoint(NSMidX(glyph) + 7.0, NSMinY(glyph) + 2.0), color);
}
