#import "DevicePaletteView.h"

#import "DeviceAppearance.h"
#include "../Core/DeviceCatalog.hpp"

NSPasteboardType const NLDeviceDefinitionPasteboardType = @"com.netlab.device-definition";

@interface NLPaletteItemView : NSView <NSDraggingSource>
- (instancetype)initWithIdentifier:(NSString *)identifier title:(NSString *)title;
@end

@implementation NLPaletteItemView {
    NSString *_identifier;
    NSString *_title;
    NSPoint _mouseDownLocation;
}

- (instancetype)initWithIdentifier:(NSString *)identifier title:(NSString *)title {
    self = [super initWithFrame:NSMakeRect(0, 0, 220, 58)];
    if (self) {
        _identifier = [identifier copy];
        _title = [title copy];
        self.wantsLayer = YES;
        self.layer.cornerRadius = 7.0;
        self.toolTip = [NSString stringWithFormat:@"Drag %@ onto the topology", title];
    }
    return self;
}

- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    NSBezierPath *background = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds, 1, 1)
                                                               xRadius:7 yRadius:7];
    [[NSColor windowBackgroundColor] setFill];
    [background fill];
    [[NSColor separatorColor] setStroke];
    background.lineWidth = 0.5;
    [background stroke];

    NSColor *accent = NLAccentColorForDeviceIdentifier(_identifier);
    NLDrawDeviceGlyph(_identifier, NSMakeRect(12, 9, 38, 38), accent);
    NSDictionary *attributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:13 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: [NSColor labelColor],
    };
    [_title drawAtPoint:NSMakePoint(62, 20) withAttributes:attributes];
}

- (void)mouseDown:(NSEvent *)event {
    _mouseDownLocation = [self convertPoint:event.locationInWindow fromView:nil];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint current = [self convertPoint:event.locationInWindow fromView:nil];
    if (hypot(current.x - _mouseDownLocation.x, current.y - _mouseDownLocation.y) < 3.0) {
        return;
    }

    NSPasteboardItem *pasteboardItem = [[NSPasteboardItem alloc] init];
    [pasteboardItem setString:_identifier forType:NLDeviceDefinitionPasteboardType];
    NSDraggingItem *draggingItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboardItem];

    NSImage *image = [self bitmapImageRepForCachingDisplayInRect:self.bounds]
        ? [[NSImage alloc] initWithSize:self.bounds.size]
        : nil;
    if (image) {
        [image lockFocus];
        [self drawRect:self.bounds];
        [image unlockFocus];
    }
    [draggingItem setDraggingFrame:NSMakeRect(event.locationInWindow.x - 90,
                                               event.locationInWindow.y - 25, 180, 50)
                          contents:image];
    [self beginDraggingSessionWithItems:@[draggingItem] event:event source:self];
}

- (NSDragOperation)draggingSession:(NSDraggingSession *)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
    return NSDragOperationCopy;
}

- (BOOL)ignoreModifierKeysForDraggingSession:(NSDraggingSession *)session { return YES; }

@end

@implementation DevicePaletteView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.wantsLayer = YES;
        self.layer.backgroundColor = [NSColor controlBackgroundColor].CGColor;
        [self buildInterface];
    }
    return self;
}

- (void)buildInterface {
    NSTextField *heading = [NSTextField labelWithString:@"Devices"];
    heading.font = [NSFont systemFontOfSize:20 weight:NSFontWeightSemibold];
    heading.translatesAutoresizingMaskIntoConstraints = NO;

    NSTextField *hint = [NSTextField wrappingLabelWithString:@"Drag a device onto the topology canvas."];
    hint.font = [NSFont systemFontOfSize:11];
    hint.textColor = [NSColor secondaryLabelColor];
    hint.translatesAutoresizingMaskIntoConstraints = NO;

    NSStackView *stack = [NSStackView stackViewWithViews:@[]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.alignment = NSLayoutAttributeLeading;
    stack.spacing = 10;
    stack.edgeInsets = NSEdgeInsetsMake(4, 0, 12, 0);
    stack.translatesAutoresizingMaskIntoConstraints = NO;

    netlab::DeviceCategory previousCategory = netlab::DeviceCategory::Endpoint;
    BOOL first = YES;
    for (const auto& definition : netlab::DeviceCatalog::defaultDefinitions()) {
        if (first || definition.category != previousCategory) {
            NSString *category = @"Endpoint";
            if (definition.category == netlab::DeviceCategory::Router) category = @"Router";
            if (definition.category == netlab::DeviceCategory::Switch) category = @"Switch";
            if (definition.category == netlab::DeviceCategory::Security) category = @"Security";
            if (definition.category == netlab::DeviceCategory::Wireless) category = @"Wireless";
            if (definition.category == netlab::DeviceCategory::Server) category = @"Server";
            NSTextField *label = [NSTextField labelWithString:category];
            label.font = [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold];
            label.textColor = [NSColor secondaryLabelColor];
            [stack addArrangedSubview:label];
            previousCategory = definition.category;
            first = NO;
        }
        NSString *identifier = [NSString stringWithUTF8String:definition.identifier.c_str()];
        NSString *title = [NSString stringWithUTF8String:definition.displayName.c_str()];
        NLPaletteItemView *item = [[NLPaletteItemView alloc] initWithIdentifier:identifier title:title];
        item.translatesAutoresizingMaskIntoConstraints = NO;
        [stack addArrangedSubview:item];
        [item.widthAnchor constraintEqualToAnchor:stack.widthAnchor].active = YES;
        [item.heightAnchor constraintEqualToConstant:58].active = YES;
    }

    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    scrollView.drawsBackground = NO;
    scrollView.hasVerticalScroller = YES;
    scrollView.autohidesScrollers = YES;
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    NSView *documentView = [[NSView alloc] initWithFrame:NSZeroRect];
    documentView.translatesAutoresizingMaskIntoConstraints = NO;
    [documentView addSubview:stack];
    scrollView.documentView = documentView;

    [self addSubview:heading];
    [self addSubview:hint];
    [self addSubview:scrollView];

    [NSLayoutConstraint activateConstraints:@[
        [heading.topAnchor constraintEqualToAnchor:self.topAnchor constant:22],
        [heading.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [heading.trailingAnchor constraintLessThanOrEqualToAnchor:self.trailingAnchor constant:-20],
        [hint.topAnchor constraintEqualToAnchor:heading.bottomAnchor constant:5],
        [hint.leadingAnchor constraintEqualToAnchor:heading.leadingAnchor],
        [hint.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-20],
        [scrollView.topAnchor constraintEqualToAnchor:hint.bottomAnchor constant:14],
        [scrollView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:20],
        [scrollView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-16],
        [scrollView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
        [documentView.widthAnchor constraintEqualToAnchor:scrollView.contentView.widthAnchor],
        [stack.topAnchor constraintEqualToAnchor:documentView.topAnchor],
        [stack.leadingAnchor constraintEqualToAnchor:documentView.leadingAnchor],
        [stack.trailingAnchor constraintEqualToAnchor:documentView.trailingAnchor],
        [stack.bottomAnchor constraintEqualToAnchor:documentView.bottomAnchor],
    ]];
}

@end
