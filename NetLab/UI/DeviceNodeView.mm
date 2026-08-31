#import "DeviceNodeView.h"

#import "DeviceAppearance.h"
#include "../Core/Device.hpp"

#include <memory>
#include <utility>
#include <vector>

@implementation DeviceNodeView {
    NSPoint _lastDragLocation;
    NSArray<NSNumber *> *_interfaceSpeeds;
    std::unique_ptr<netlab::Device> _coreDevice;
}

- (instancetype)initWithDeviceIdentifier:(NSString *)identifier
                              displayName:(NSString *)displayName
                           interfaceNames:(NSArray<NSString *> *)interfaceNames
                          interfaceSpeeds:(NSArray<NSNumber *> *)interfaceSpeeds {
    self = [super initWithFrame:NSMakeRect(0, 0, 112, 86)];
    if (self) {
        _deviceIdentifier = [identifier copy];
        _instanceIdentifier = NSUUID.UUID.UUIDString;
        _displayName = [displayName copy];
        _interfaceNames = [interfaceNames copy];
        _interfaceSpeeds = [interfaceSpeeds copy];
        std::vector<std::pair<std::string, int>> coreInterfaces;
        for (NSUInteger index = 0; index < interfaceNames.count; ++index) {
            coreInterfaces.emplace_back(interfaceNames[index].UTF8String,
                                        static_cast<int>(interfaceSpeeds[index].integerValue));
        }
        netlab::DeviceRole role = netlab::DeviceRole::Endpoint;
        if ([identifier isEqualToString:@"l2-switch"]) role = netlab::DeviceRole::Switch;
        if ([identifier isEqualToString:@"generic-router"]) role = netlab::DeviceRole::Router;
        if ([identifier isEqualToString:@"firewall"]) role = netlab::DeviceRole::Firewall;
        if ([identifier isEqualToString:@"wireless-ap"]) role = netlab::DeviceRole::WirelessAP;
        if ([identifier isEqualToString:@"generic-server"]) role = netlab::DeviceRole::Server;
        _coreDevice = std::make_unique<netlab::Device>(_instanceIdentifier.UTF8String,
                                                       _displayName.UTF8String,
                                                       role, coreInterfaces);
        self.wantsLayer = YES;
        self.layer.cornerRadius = 9.0;
        self.accessibilityElement = YES;
        self.accessibilityRole = NSAccessibilityGroupRole;
        self.accessibilityLabel = _displayName;
    }
    return self;
}

- (netlab::Device *)coreDevice { return _coreDevice.get(); }

- (NSInteger)speedMbpsForInterfaceNamed:(NSString *)interfaceName {
    NSUInteger index = [self.interfaceNames indexOfObject:interfaceName];
    return index == NSNotFound ? 0 : _interfaceSpeeds[index].integerValue;
}

- (NSString *)macAddressForInterfaceNamed:(NSString *)interfaceName {
    const netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    return networkInterface ? [NSString stringWithUTF8String:networkInterface->macAddress().c_str()] : @"—";
}

- (NSString *)ipv4AddressForInterfaceNamed:(NSString *)interfaceName {
    const netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    return networkInterface && !networkInterface->ipv4Address().empty()
        ? [NSString stringWithUTF8String:networkInterface->ipv4Address().c_str()] : @"";
}

- (NSString *)subnetMaskForInterfaceNamed:(NSString *)interfaceName {
    const netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    return networkInterface && !networkInterface->subnetMask().empty()
        ? [NSString stringWithUTF8String:networkInterface->subnetMask().c_str()] : @"255.255.255.0";
}

- (BOOL)configureInterfaceNamed:(NSString *)interfaceName
                    ipv4Address:(NSString *)ipv4Address
                     subnetMask:(NSString *)subnetMask {
    netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    return networkInterface && networkInterface->configureIPv4(ipv4Address.UTF8String, subnetMask.UTF8String);
}

- (NSString *)defaultGateway {
    return _coreDevice->defaultGateway().empty()
        ? @"" : [NSString stringWithUTF8String:_coreDevice->defaultGateway().c_str()];
}

- (BOOL)setDefaultGatewayAddress:(NSString *)address {
    return _coreDevice->setDefaultGateway(address.UTF8String);
}

- (NSString *)arpTableText {
    if (_coreDevice->arpTable().empty()) return @"ARP cache is empty.";
    NSMutableString *text = [NSMutableString stringWithString:@"IPv4 Address             MAC Address\n"];
    for (const auto& entry : _coreDevice->arpTable()) {
        [text appendFormat:@"%-24s %s\n", entry.first.c_str(), entry.second.c_str()];
    }
    return text;
}

- (NSString *)macAddressTableText {
    if (_coreDevice->macAddressTable().empty()) return @"MAC address table is empty.";
    NSMutableString *text = [NSMutableString stringWithString:@"MAC Address              Interface\n"];
    for (const auto& entry : _coreDevice->macAddressTable()) {
        [text appendFormat:@"%-24s %s\n", entry.first.c_str(), entry.second.c_str()];
    }
    return text;
}

- (NSString *)routingTableText {
    std::vector<netlab::RouteEntry> routes = _coreDevice->routingTable();
    if (routes.empty()) return @"Routing table is empty.";
    NSMutableString *text = [NSMutableString stringWithString:@"Type       Destination       Mask              Via / Interface\n"];
    for (const auto& route : routes) {
        std::string via = route.nextHop.empty() ? route.interfaceName : route.nextHop;
        [text appendFormat:@"%-10s %-17s %-17s %s\n", route.protocol.c_str(), route.destination.c_str(),
                           route.subnetMask.c_str(), via.c_str()];
    }
    return text;
}

- (NSString *)servicesTableText {
    NSMutableString *text = [NSMutableString string];
    [text appendFormat:@"DHCP: %@ (%lu leases)\n", _coreDevice->dhcpServerEnabled() ? @"Server enabled" : @"Off",
                       (unsigned long)_coreDevice->dhcpLeases().size()];
    [text appendFormat:@"DNS: %lu A record(s)\n", (unsigned long)_coreDevice->dnsRecords().size()];
    [text appendFormat:@"NAT: %@ (%lu translation(s))\n", _coreDevice->natEnabled() ? @"Enabled" : @"Off",
                       (unsigned long)_coreDevice->natTranslations().size()];
    [text appendFormat:@"ACL: %lu rule(s)\n", (unsigned long)_coreDevice->aclRules().size()];
    return text;
}

- (NSString *)ipv6AddressForInterfaceNamed:(NSString *)interfaceName {
    const netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    if (!networkInterface || networkInterface->ipv6Address().empty()) return @"";
    return [NSString stringWithUTF8String:networkInterface->ipv6Address().c_str()];
}

- (BOOL)configureInterfaceNamed:(NSString *)interfaceName ipv6Address:(NSString *)address prefixLength:(NSInteger)prefixLength {
    netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    return networkInterface && networkInterface->configureIPv6(address.UTF8String, (int)prefixLength);
}

- (NSString *)vlanModeForInterfaceNamed:(NSString *)interfaceName {
    const netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    return networkInterface && networkInterface->switchportMode() == netlab::NetworkInterface::SwitchportMode::Trunk
        ? @"Trunk" : @"Access";
}

- (NSInteger)vlanIDForInterfaceNamed:(NSString *)interfaceName {
    const netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    if (!networkInterface) return 1;
    return networkInterface->switchportMode() == netlab::NetworkInterface::SwitchportMode::Trunk
        ? networkInterface->nativeVLAN() : networkInterface->accessVLAN();
}

- (BOOL)configureInterfaceNamed:(NSString *)interfaceName vlanMode:(NSString *)mode vlanID:(NSInteger)vlanID {
    netlab::NetworkInterface *networkInterface = _coreDevice->interfaceNamed(interfaceName.UTF8String);
    if (!networkInterface) return NO;
    return [mode isEqualToString:@"Trunk"]
        ? networkInterface->configureTrunk((int)vlanID, {(int)vlanID, 10, 20})
        : networkInterface->configureAccessVLAN((int)vlanID);
}

- (BOOL)isFlipped {
    return YES;
}

- (void)setSelected:(BOOL)selected {
    if (_selected == selected) {
        return;
    }
    _selected = selected;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSColor *fill = self.isSelected ? [NSColor selectedContentBackgroundColor]
                                    : [NSColor controlBackgroundColor];
    NSBezierPath *background = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds, 1, 1)
                                                               xRadius:8 yRadius:8];
    [fill setFill];
    [background fill];

    NSColor *accent = NLAccentColorForDeviceIdentifier(self.deviceIdentifier);
    [accent setStroke];
    background.lineWidth = self.isSelected ? 2.5 : 1.0;
    [background stroke];

    NLDrawDeviceGlyph(self.deviceIdentifier, NSMakeRect(37, 8, 38, 38),
                      self.isSelected ? [NSColor alternateSelectedControlTextColor] : accent);

    NSDictionary *attributes = @{
        NSFontAttributeName: [NSFont systemFontOfSize:12 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName: self.isSelected
            ? [NSColor alternateSelectedControlTextColor]
            : [NSColor labelColor],
    };
    NSSize textSize = [self.displayName sizeWithAttributes:attributes];
    NSPoint origin = NSMakePoint(MAX(5, (NSWidth(self.bounds) - textSize.width) / 2.0), 59);
    [self.displayName drawAtPoint:origin withAttributes:attributes];
}

- (void)mouseDown:(NSEvent *)event {
    BOOL extend = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    [self.delegate deviceNodeViewDidRequestSelection:self extendingSelection:extend];
    _lastDragLocation = [self.superview convertPoint:event.locationInWindow fromView:nil];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint location = [self.superview convertPoint:event.locationInWindow fromView:nil];
    NSPoint delta = NSMakePoint(location.x - _lastDragLocation.x, location.y - _lastDragLocation.y);
    _lastDragLocation = location;
    [self.delegate deviceNodeView:self didMoveBy:delta];
}

- (NSMenu *)menuForEvent:(NSEvent *)event {
    [self.delegate deviceNodeViewDidRequestSelection:self extendingSelection:NO];
    NSMenu *menu = [[NSMenu alloc] initWithTitle:self.displayName];
    NSMenuItem *deleteItem = [[NSMenuItem alloc] initWithTitle:@"Delete Device"
                                                       action:@selector(deleteFromContextMenu:)
                                                keyEquivalent:@""];
    deleteItem.target = self;
    deleteItem.image = [NSImage imageWithSystemSymbolName:@"trash"
                                 accessibilityDescription:@"Delete Device"];
    [menu addItem:deleteItem];
    return menu;
}

- (void)deleteFromContextMenu:(id)sender {
    [self.delegate deviceNodeViewDidRequestDeletion:self];
}

@end
