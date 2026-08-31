#import "TopologyView.h"

#import "DeviceNodeView.h"
#import "DevicePaletteView.h"
#import "LinkLayerController.h"
#include "../Core/DeviceCatalog.hpp"
#include "../Core/SimulationEngine.hpp"

@interface TopologyView () <DeviceNodeViewDelegate>
@end

@implementation TopologyView {
    NSMutableArray<DeviceNodeView *> *_nodes;
    NSMutableOrderedSet<DeviceNodeView *> *_selectedNodes;
    NSMutableDictionary<NSString *, NSNumber *> *_nameCounters;
    NSMutableArray<LinkLayerController *> *_links;
    NSMutableSet<NSString *> *_connectedInterfaces;
    LinkLayerController *_selectedLink;
    DeviceNodeView *_pendingConnectNode;
    NSString *_pendingInterfaceName;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _nodes = [NSMutableArray array];
        _selectedNodes = [NSMutableOrderedSet orderedSet];
        _nameCounters = [NSMutableDictionary dictionary];
        _links = [NSMutableArray array];
        _connectedInterfaces = [NSMutableSet set];
        self.wantsLayer = YES;
        self.layer.backgroundColor = [NSColor textBackgroundColor].CGColor;
        [self registerForDraggedTypes:@[NLDeviceDefinitionPasteboardType]];
    }
    return self;
}

- (NSUInteger)deviceCount { return _nodes.count; }
- (NSUInteger)linkCount { return _links.count; }

- (void)setConnectMode:(BOOL)connectMode {
    if (_connectMode == connectMode) return;
    _connectMode = connectMode;
    _pendingConnectNode = nil;
    _pendingInterfaceName = nil;
    [self clearAllSelection];
    [self setNeedsDisplay:YES];
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    [[NSColor separatorColor] setStroke];
    NSBezierPath *grid = [NSBezierPath bezierPath];
    grid.lineWidth = 0.25;
    const CGFloat spacing = 24.0;
    for (CGFloat x = 0; x < NSWidth(self.bounds); x += spacing) {
        [grid moveToPoint:NSMakePoint(x, NSMinY(dirtyRect))];
        [grid lineToPoint:NSMakePoint(x, NSMaxY(dirtyRect))];
    }
    for (CGFloat y = 0; y < NSHeight(self.bounds); y += spacing) {
        [grid moveToPoint:NSMakePoint(NSMinX(dirtyRect), y)];
        [grid lineToPoint:NSMakePoint(NSMaxX(dirtyRect), y)];
    }
    [grid stroke];

    if (_nodes.count == 0) {
        NSString *message = @"Drag devices here to build a topology";
        NSDictionary *attributes = @{
            NSFontAttributeName: [NSFont systemFontOfSize:15 weight:NSFontWeightMedium],
            NSForegroundColorAttributeName: [NSColor tertiaryLabelColor],
        };
        NSSize size = [message sizeWithAttributes:attributes];
        [message drawAtPoint:NSMakePoint((NSWidth(self.bounds) - size.width) / 2.0,
                                         (NSHeight(self.bounds) - size.height) / 2.0)
             withAttributes:attributes];
    }

    if (self.isConnectMode) {
        NSString *status = _pendingConnectNode
            ? [NSString stringWithFormat:@"Connect: choose the destination for %@ (%@)",
                                         _pendingConnectNode.displayName, _pendingInterfaceName]
            : @"Connect: choose the first device";
        NSDictionary *attributes = @{
            NSFontAttributeName: [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold],
            NSForegroundColorAttributeName: [NSColor whiteColor],
        };
        NSSize size = [status sizeWithAttributes:attributes];
        NSRect banner = NSMakeRect(18, 16, size.width + 24, size.height + 12);
        NSBezierPath *pill = [NSBezierPath bezierPathWithRoundedRect:banner xRadius:9 yRadius:9];
        [[NSColor systemBlueColor] setFill];
        [pill fill];
        [status drawAtPoint:NSMakePoint(NSMinX(banner) + 12, NSMinY(banner) + 6)
             withAttributes:attributes];
    }
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    return [sender.draggingPasteboard availableTypeFromArray:@[NLDeviceDefinitionPasteboardType]]
        ? NSDragOperationCopy : NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSString *identifier = [sender.draggingPasteboard stringForType:NLDeviceDefinitionPasteboardType];
    if (!identifier) return NO;
    NSPoint location = [self convertPoint:sender.draggingLocation fromView:nil];
    return [self addDeviceWithIdentifier:identifier centeredAt:location];
}

- (BOOL)addDeviceWithIdentifier:(NSString *)identifier centeredAt:(NSPoint)location {
    return [self createDeviceWithIdentifier:identifier centeredAt:location] != nil;
}

- (nullable DeviceNodeView *)createDeviceWithIdentifier:(NSString *)identifier centeredAt:(NSPoint)location {
    const netlab::DeviceDefinition *match = nullptr;
    for (const auto& definition : netlab::DeviceCatalog::defaultDefinitions()) {
        if ([identifier isEqualToString:[NSString stringWithUTF8String:definition.identifier.c_str()]]) {
            match = &definition;
            break;
        }
    }
    if (!match) return nil;

    NSString *prefix = [NSString stringWithUTF8String:match->defaultNamePrefix.c_str()];
    NSInteger next = [_nameCounters[prefix] integerValue] + 1;
    _nameCounters[prefix] = @(next);
    NSString *name = [NSString stringWithFormat:@"%@%ld", prefix, (long)next];
    NSMutableArray<NSString *> *interfaceNames = [NSMutableArray array];
    NSMutableArray<NSNumber *> *interfaceSpeeds = [NSMutableArray array];
    for (const auto& interfaceDefinition : match->interfaces) {
        [interfaceNames addObject:[NSString stringWithUTF8String:interfaceDefinition.name.c_str()]];
        [interfaceSpeeds addObject:@(interfaceDefinition.speedMbps)];
    }
    DeviceNodeView *node = [[DeviceNodeView alloc] initWithDeviceIdentifier:identifier
                                                                displayName:name
                                                             interfaceNames:interfaceNames
                                                            interfaceSpeeds:interfaceSpeeds];
    node.delegate = self;
    CGFloat x = MIN(MAX(0, location.x - NSWidth(node.frame) / 2.0),
                    MAX(0, NSWidth(self.bounds) - NSWidth(node.frame)));
    CGFloat y = MIN(MAX(0, location.y - NSHeight(node.frame) / 2.0),
                    MAX(0, NSHeight(self.bounds) - NSHeight(node.frame)));
    [node setFrameOrigin:NSMakePoint(x, y)];
    [_nodes addObject:node];
    [self addSubview:node];
    [self selectOnlyNode:node];
    [self.window makeFirstResponder:self];
    [self setNeedsDisplay:YES];
    return node;
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    [self clearAllSelection];
    for (LinkLayerController *link in _links.reverseObjectEnumerator) {
        if ([link containsPoint:point tolerance:8.0]) {
            _selectedLink = link;
            link.selected = YES;
            break;
        }
    }
    [self.window makeFirstResponder:self];
}

- (void)keyDown:(NSEvent *)event {
    if (event.keyCode == 51 || event.keyCode == 117) {
        [self deleteSelection];
        return;
    }
    if (event.keyCode == 53 && self.isConnectMode) {
        self.connectMode = NO;
        return;
    }
    [super keyDown:event];
}

- (void)deviceNodeViewDidRequestSelection:(DeviceNodeView *)node extendingSelection:(BOOL)extend {
    if (self.isConnectMode) {
        [self handleConnectSelectionForNode:node];
        return;
    }
    [self clearLinkSelection];
    if (!extend) {
        [self selectOnlyNode:node];
    } else if ([_selectedNodes containsObject:node]) {
        [_selectedNodes removeObject:node];
        node.selected = NO;
    } else {
        [_selectedNodes addObject:node];
        node.selected = YES;
    }
    [self.selectionDelegate topologyView:self didSelectDevice:node];
    [self.window makeFirstResponder:self];
}

- (void)deviceNodeView:(DeviceNodeView *)node didMoveBy:(NSPoint)delta {
    NSArray<DeviceNodeView *> *movingNodes = node.isSelected ? _selectedNodes.array : @[node];
    for (DeviceNodeView *movingNode in movingNodes) {
        NSPoint origin = movingNode.frame.origin;
        origin.x = MIN(MAX(0, origin.x + delta.x), MAX(0, NSWidth(self.bounds) - NSWidth(movingNode.frame)));
        origin.y = MIN(MAX(0, origin.y + delta.y), MAX(0, NSHeight(self.bounds) - NSHeight(movingNode.frame)));
        [movingNode setFrameOrigin:origin];
    }
    [self updateLinkGeometry];
}

- (void)deviceNodeViewDidRequestDeletion:(DeviceNodeView *)node {
    [self deleteDevice:node];
}

- (void)selectOnlyNode:(DeviceNodeView *)node {
    [self clearAllSelection];
    [_selectedNodes addObject:node];
    node.selected = YES;
    [self.selectionDelegate topologyView:self didSelectDevice:node];
}

- (void)clearSelection {
    for (DeviceNodeView *node in _selectedNodes) node.selected = NO;
    [_selectedNodes removeAllObjects];
}

- (void)clearLinkSelection {
    _selectedLink.selected = NO;
    _selectedLink = nil;
}

- (void)clearAllSelection {
    [self clearSelection];
    [self clearLinkSelection];
    [self.selectionDelegate topologyView:self didSelectDevice:nil];
}

- (void)showDevicePalette {
    self.connectMode = NO;
    [self clearAllSelection];
    [self.window makeFirstResponder:self];
}

- (void)deleteSelection {
    NSArray<DeviceNodeView *> *nodesToDelete = [_selectedNodes.array copy];
    [self clearSelection];
    if (nodesToDelete.count == 0 && _selectedLink) {
        [self removeLink:_selectedLink];
        [self clearLinkSelection];
        return;
    }
    for (DeviceNodeView *node in nodesToDelete) [self deleteDevice:node];
    [self setNeedsDisplay:YES];
}

- (void)deleteDevice:(DeviceNodeView *)device {
    if (!device || ![_nodes containsObject:device]) return;
    [_selectedNodes removeObject:device];
    device.selected = NO;
    NSArray<LinkLayerController *> *attached = [_links filteredArrayUsingPredicate:
        [NSPredicate predicateWithBlock:^BOOL(LinkLayerController *link, NSDictionary *bindings) {
            return [link isAttachedToNode:device];
        }]];
    for (LinkLayerController *link in attached) [self removeLink:link];
    [_nodes removeObject:device];
    [device removeFromSuperview];
    [self.selectionDelegate topologyView:self didSelectDevice:nil];
    [self setNeedsDisplay:YES];
}

- (void)clearTopology {
    [self clearAllSelection];
    for (LinkLayerController *link in [_links copy]) [self removeLink:link];
    for (DeviceNodeView *node in _nodes) [node removeFromSuperview];
    [_nodes removeAllObjects];
    [_nameCounters removeAllObjects];
    _pendingConnectNode = nil;
    _pendingInterfaceName = nil;
    [self setNeedsDisplay:YES];
}

- (void)fitTopology {
    if (_nodes.count == 0) return;
    NSRect unionRect = _nodes.firstObject.frame;
    for (DeviceNodeView *node in _nodes) unionRect = NSUnionRect(unionRect, node.frame);
    NSPoint delta = NSMakePoint(MAX(20, (NSWidth(self.bounds) - NSWidth(unionRect)) / 2.0) - NSMinX(unionRect),
                                MAX(20, (NSHeight(self.bounds) - NSHeight(unionRect)) / 2.0) - NSMinY(unionRect));
    for (DeviceNodeView *node in _nodes) {
        [node setFrameOrigin:NSMakePoint(node.frame.origin.x + delta.x, node.frame.origin.y + delta.y)];
    }
    [self updateLinkGeometry];
}

- (void)layout {
    [super layout];
    [self updateLinkGeometry];
}

- (NSString *)interfaceKeyForNode:(DeviceNodeView *)node interfaceName:(NSString *)interfaceName {
    return [NSString stringWithFormat:@"%@/%@", node.instanceIdentifier, interfaceName];
}

- (NSArray<NSString *> *)availableInterfacesForNode:(DeviceNodeView *)node {
    NSMutableArray<NSString *> *available = [NSMutableArray array];
    for (NSString *interfaceName in node.interfaceNames) {
        if (![_connectedInterfaces containsObject:[self interfaceKeyForNode:node interfaceName:interfaceName]]) {
            [available addObject:interfaceName];
        }
    }
    return available;
}

- (void)handleConnectSelectionForNode:(DeviceNodeView *)node {
    if (_pendingConnectNode == node) {
        NSBeep();
        return;
    }
    [self presentInterfaceChooserForNode:node completion:^(NSString *interfaceName) {
        if (!interfaceName) return;
        if (!self->_pendingConnectNode) {
            self->_pendingConnectNode = node;
            self->_pendingInterfaceName = interfaceName;
            [self selectOnlyNode:node];
            [self setNeedsDisplay:YES];
            return;
        }
        DeviceNodeView *firstNode = self->_pendingConnectNode;
        NSString *firstInterface = self->_pendingInterfaceName;
        [self createLinkFromNode:firstNode
                       interface:firstInterface
                          toNode:node
                       interface:interfaceName];
        self->_pendingConnectNode = nil;
        self->_pendingInterfaceName = nil;
        [self clearAllSelection];
        [self setNeedsDisplay:YES];
    }];
}

- (void)presentInterfaceChooserForNode:(DeviceNodeView *)node
                             completion:(void (^)(NSString * _Nullable interfaceName))completion {
    NSArray<NSString *> *available = [self availableInterfacesForNode:node];
    if (available.count == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"No available Ethernet interfaces";
        alert.informativeText = [NSString stringWithFormat:@"All interfaces on %@ are already connected.",
                                                           node.displayName];
        [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
            completion(nil);
        }];
        return;
    }

    NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 260, 28) pullsDown:NO];
    for (NSString *interfaceName in available) {
        NSInteger speed = [node speedMbpsForInterfaceNamed:interfaceName];
        [popup addItemWithTitle:[NSString stringWithFormat:@"%@ — %@", interfaceName,
                                 speed >= 1000 ? @"1 Gbps" : @"100 Mbps"]];
        popup.lastItem.representedObject = interfaceName;
    }
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = [NSString stringWithFormat:@"Choose an interface on %@", node.displayName];
    alert.informativeText = @"The selected interface will be reserved for this Ethernet link.";
    alert.accessoryView = popup;
    [alert addButtonWithTitle:@"Choose"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
        completion(response == NSAlertFirstButtonReturn ? popup.selectedItem.representedObject : nil);
    }];
}

- (void)createLinkFromNode:(DeviceNodeView *)firstNode
                 interface:(NSString *)firstInterface
                    toNode:(DeviceNodeView *)secondNode
                 interface:(NSString *)secondInterface {
    NSInteger speed = MIN([firstNode speedMbpsForInterfaceNamed:firstInterface],
                          [secondNode speedMbpsForInterfaceNamed:secondInterface]);
    LinkLayerController *link = [[LinkLayerController alloc] initWithFirstNode:firstNode
                                                                firstInterface:firstInterface
                                                                    secondNode:secondNode
                                                               secondInterface:secondInterface
                                                                     speedMbps:speed];
    [_links addObject:link];
    [_connectedInterfaces addObject:[self interfaceKeyForNode:firstNode interfaceName:firstInterface]];
    [_connectedInterfaces addObject:[self interfaceKeyForNode:secondNode interfaceName:secondInterface]];
    [link installInTopologyLayer:self.layer];
}

- (void)removeLink:(LinkLayerController *)link {
    [_connectedInterfaces removeObject:[self interfaceKeyForNode:link.firstNode
                                                    interfaceName:link.firstInterfaceName]];
    [_connectedInterfaces removeObject:[self interfaceKeyForNode:link.secondNode
                                                    interfaceName:link.secondInterfaceName]];
    [link invalidate];
    [_links removeObject:link];
    if (_selectedLink == link) _selectedLink = nil;
}

- (void)updateLinkGeometry {
    for (LinkLayerController *link in _links) [link updateGeometry];
}

- (void)loadMilestone2DemoTopology {
    [self clearTopology];
    CGFloat y = MAX(120, NSHeight(self.bounds) * 0.48);
    DeviceNodeView *pc = [self createDeviceWithIdentifier:@"desktop-pc"
                                                centeredAt:NSMakePoint(NSWidth(self.bounds) * 0.20, y)];
    DeviceNodeView *switchNode = [self createDeviceWithIdentifier:@"l2-switch"
                                                        centeredAt:NSMakePoint(NSWidth(self.bounds) * 0.50, y)];
    DeviceNodeView *router = [self createDeviceWithIdentifier:@"generic-router"
                                                    centeredAt:NSMakePoint(NSWidth(self.bounds) * 0.80, y)];
    if (!pc || !switchNode || !router) return;
    [self createLinkFromNode:pc interface:@"G0" toNode:switchNode interface:@"G0/1"];
    [self createLinkFromNode:switchNode interface:@"F0/1" toNode:router interface:@"G0/0"];
    [self clearAllSelection];
}

- (void)loadMilestone3DemoTopology {
    [self clearTopology];
    CGFloat y = MAX(120, NSHeight(self.bounds) * 0.48);
    DeviceNodeView *pc1 = [self createDeviceWithIdentifier:@"desktop-pc"
                                                 centeredAt:NSMakePoint(NSWidth(self.bounds) * 0.20, y)];
    DeviceNodeView *switchNode = [self createDeviceWithIdentifier:@"l2-switch"
                                                        centeredAt:NSMakePoint(NSWidth(self.bounds) * 0.50, y)];
    DeviceNodeView *pc2 = [self createDeviceWithIdentifier:@"desktop-pc"
                                                 centeredAt:NSMakePoint(NSWidth(self.bounds) * 0.80, y)];
    if (!pc1 || !switchNode || !pc2) return;
    [pc1 configureInterfaceNamed:@"G0" ipv4Address:@"192.168.1.10" subnetMask:@"255.255.255.0"];
    [pc1 setDefaultGatewayAddress:@"192.168.1.1"];
    [pc2 configureInterfaceNamed:@"G0" ipv4Address:@"192.168.1.20" subnetMask:@"255.255.255.0"];
    [pc2 setDefaultGatewayAddress:@"192.168.1.1"];
    [self createLinkFromNode:pc1 interface:@"G0" toNode:switchNode interface:@"G0/1"];
    [self createLinkFromNode:switchNode interface:@"F0/1" toNode:pc2 interface:@"G0"];
    [self clearAllSelection];
}

- (void)loadMilestone7DemoTopology {
    [self clearTopology];
    CGFloat width = NSWidth(self.bounds);
    CGFloat upperY = MAX(130, NSHeight(self.bounds) * 0.36);
    CGFloat lowerY = MIN(NSHeight(self.bounds) - 90, upperY + 190);
    DeviceNodeView *pc1 = [self createDeviceWithIdentifier:@"desktop-pc" centeredAt:NSMakePoint(width * 0.08, upperY)];
    DeviceNodeView *switch1 = [self createDeviceWithIdentifier:@"l2-switch" centeredAt:NSMakePoint(width * 0.27, upperY)];
    DeviceNodeView *router = [self createDeviceWithIdentifier:@"generic-router" centeredAt:NSMakePoint(width * 0.50, upperY)];
    DeviceNodeView *switch2 = [self createDeviceWithIdentifier:@"l2-switch" centeredAt:NSMakePoint(width * 0.73, upperY)];
    DeviceNodeView *pc2 = [self createDeviceWithIdentifier:@"desktop-pc" centeredAt:NSMakePoint(width * 0.92, upperY)];
    DeviceNodeView *server = [self createDeviceWithIdentifier:@"generic-server" centeredAt:NSMakePoint(width * 0.27, lowerY)];
    DeviceNodeView *accessPoint = [self createDeviceWithIdentifier:@"wireless-ap" centeredAt:NSMakePoint(width * 0.67, lowerY)];
    DeviceNodeView *firewall = [self createDeviceWithIdentifier:@"firewall" centeredAt:NSMakePoint(width * 0.84, lowerY)];
    if (!pc1 || !switch1 || !router || !switch2 || !pc2 || !server || !accessPoint || !firewall) return;

    [pc1 configureInterfaceNamed:@"G0" ipv4Address:@"10.10.10.10" subnetMask:@"255.255.255.0"];
    [pc1 setDefaultGatewayAddress:@"10.10.10.1"];
    [pc2 configureInterfaceNamed:@"G0" ipv4Address:@"20.20.20.10" subnetMask:@"255.255.255.0"];
    [pc2 setDefaultGatewayAddress:@"20.20.20.1"];
    [server configureInterfaceNamed:@"G0" ipv4Address:@"10.10.10.53" subnetMask:@"255.255.255.0"];
    [server setDefaultGatewayAddress:@"10.10.10.1"];
    [router configureInterfaceNamed:@"G0/0" ipv4Address:@"10.10.10.1" subnetMask:@"255.255.255.0"];
    [router configureInterfaceNamed:@"G0/1" ipv4Address:@"20.20.20.1" subnetMask:@"255.255.255.0"];
    [router configureInterfaceNamed:@"G0/0" ipv6Address:@"2001:db8:10::1" prefixLength:64];

    [switch1 configureInterfaceNamed:@"G0/1" vlanMode:@"Access" vlanID:10];
    [switch1 configureInterfaceNamed:@"G0/2" vlanMode:@"Access" vlanID:10];
    [switch1 configureInterfaceNamed:@"F0/1" vlanMode:@"Access" vlanID:10];
    [switch2 configureInterfaceNamed:@"G0/1" vlanMode:@"Access" vlanID:20];
    [switch2 configureInterfaceNamed:@"G0/2" vlanMode:@"Access" vlanID:20];
    [switch2 configureInterfaceNamed:@"F0/1" vlanMode:@"Access" vlanID:20];
    [switch2 configureInterfaceNamed:@"F0/2" vlanMode:@"Access" vlanID:20];

    netlab::Device *routerDevice = router.coreDevice;
    routerDevice->addStaticRoute("0.0.0.0", "0.0.0.0", "203.0.113.1");
    routerDevice->setNATEnabled(true, "203.0.113.10");
    routerDevice->addACLRule({true, "any", "any", "Allow learning lab traffic"});
    routerDevice->setDynamicRoutingProtocol("OSPF");
    routerDevice->setVPNTunnel("198.51.100.2", true);
    server.coreDevice->setDHCPServer(true, "10.10.10.0", "255.255.255.0", "10.10.10.1", "10.10.10.53");
    server.coreDevice->addDNSRecord("pc2.netlab", "20.20.20.10");
    switch1.coreDevice->enableSTP(true);
    switch2.coreDevice->enableSTP(true);
    accessPoint.coreDevice->setWireless("NetLab-Class", true);
    firewall.coreDevice->setFirewallEnabled(true);
    firewall.coreDevice->setVPNTunnel("Branch-Lab", true);

    [self createLinkFromNode:pc1 interface:@"G0" toNode:switch1 interface:@"G0/1"];
    [self createLinkFromNode:server interface:@"G0" toNode:switch1 interface:@"F0/1"];
    [self createLinkFromNode:switch1 interface:@"G0/2" toNode:router interface:@"G0/0"];
    [self createLinkFromNode:router interface:@"G0/1" toNode:switch2 interface:@"G0/1"];
    [self createLinkFromNode:switch2 interface:@"G0/2" toNode:pc2 interface:@"G0"];
    [self createLinkFromNode:switch2 interface:@"F0/1" toNode:accessPoint interface:@"G0"];
    [self createLinkFromNode:switch2 interface:@"F0/2" toNode:firewall interface:@"INSIDE"];
    [self clearAllSelection];
}

- (NSString *)runPingFromDevice:(DeviceNodeView *)source targetAddress:(NSString *)targetAddress {
    netlab::SimulationEngine engine;
    for (DeviceNodeView *node in _nodes) engine.addDevice(*node.coreDevice);
    for (LinkLayerController *link in _links) {
        engine.addEthernetConnection({link.firstNode.coreDevice,
                                      link.firstInterfaceName.UTF8String,
                                      link.secondNode.coreDevice,
                                      link.secondInterfaceName.UTF8String});
    }
    netlab::PingResult result = engine.ping(*source.coreDevice, targetAddress.UTF8String);
    NSMutableString *output = [NSMutableString stringWithFormat:@"ping %@\n\n", targetAddress];
    for (const auto& event : result.events) {
        [output appendFormat:@"[%s] %s\n", event.stage.c_str(), event.detail.c_str()];
    }
    [output appendFormat:@"\n%@ %@\n", result.success ? @"SUCCESS" : @"FAILED",
                         [NSString stringWithUTF8String:result.summary.c_str()]];
    return output;
}

- (NSString *)requestDHCPForDevice:(DeviceNodeView *)device {
    netlab::SimulationEngine engine;
    for (DeviceNodeView *node in _nodes) engine.addDevice(*node.coreDevice);
    for (LinkLayerController *link in _links) {
        engine.addEthernetConnection({link.firstNode.coreDevice, link.firstInterfaceName.UTF8String,
                                      link.secondNode.coreDevice, link.secondInterfaceName.UTF8String});
    }
    netlab::ServiceResult result = engine.requestDHCP(*device.coreDevice);
    NSMutableString *output = [NSMutableString stringWithString:@"DHCP CLIENT\n\n"];
    for (const auto& event : result.events) [output appendFormat:@"[%s] %s\n", event.stage.c_str(), event.detail.c_str()];
    [output appendFormat:@"\n%@ %s\n", result.success ? @"SUCCESS" : @"FAILED", result.summary.c_str()];
    return output;
}

- (NSString *)advancedStatusForDevice:(DeviceNodeView *)device {
    netlab::SimulationEngine engine;
    for (DeviceNodeView *node in _nodes) engine.addDevice(*node.coreDevice);
    return [NSString stringWithUTF8String:engine.advancedStatus(*device.coreDevice).c_str()];
}

- (BOOL)runMilestone3SelfTest {
    [self loadMilestone3DemoTopology];
    DeviceNodeView *source = nil;
    for (DeviceNodeView *node in _nodes) {
        if ([node.displayName isEqualToString:@"PC1"]) {
            source = node;
            break;
        }
    }
    if (!source) return NO;
    NSString *result = [self runPingFromDevice:source targetAddress:@"192.168.1.20"];
    return [result containsString:@"SUCCESS"] && [source.arpTableText containsString:@"192.168.1.20"];
}

- (BOOL)runMilestone7SelfTest {
    [self loadMilestone7DemoTopology];
    DeviceNodeView *source = nil;
    for (DeviceNodeView *node in _nodes) if ([node.displayName isEqualToString:@"PC1"]) source = node;
    if (!source) return NO;
    NSString *dhcp = [self requestDHCPForDevice:source];
    NSString *ping = [self runPingFromDevice:source targetAddress:@"pc2.netlab"];
    return [dhcp containsString:@"SUCCESS"] && [ping containsString:@"SUCCESS"] &&
           [ping containsString:@"Routing"] && [ping containsString:@"NAT"] &&
           _nodes.count == 8 && _links.count == 7;
}

- (BOOL)runNodeDeletionSelfTest {
    [self loadMilestone7DemoTopology];
    if (_nodes.count != 8 || _links.count != 7) return NO;
    DeviceNodeView *node = _nodes.firstObject;
    [self deleteDevice:node];
    return _nodes.count == 7 && _links.count == 6 && ![self.subviews containsObject:node];
}

@end
