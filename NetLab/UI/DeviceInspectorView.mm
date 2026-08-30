#import "DeviceInspectorView.h"

#import "DeviceNodeView.h"
#import "TopologyView.h"

@implementation DeviceInspectorView {
    __weak TopologyView *_topologyView;
    __weak DeviceNodeView *_device;
    NSTextField *_titleLabel;
    NSPopUpButton *_interfacePopup;
    NSTextField *_macLabel;
    NSTextField *_ipv4Field;
    NSTextField *_subnetField;
    NSTextField *_gatewayField;
    NSTextField *_ipv6Field;
    NSPopUpButton *_vlanModePopup;
    NSTextField *_vlanField;
    NSTextField *_pingField;
    NSTextField *_statusLabel;
    NSTextView *_outputView;
}

- (instancetype)initWithTopologyView:(TopologyView *)topologyView {
    self = [super initWithFrame:NSZeroRect];
    if (self) {
        _topologyView = topologyView;
        self.wantsLayer = YES;
        self.layer.backgroundColor = NSColor.controlBackgroundColor.CGColor;
        [self buildInterface];
    }
    return self;
}

- (NSTextField *)caption:(NSString *)text {
    NSTextField *label = [NSTextField labelWithString:text];
    label.font = [NSFont systemFontOfSize:11 weight:NSFontWeightSemibold];
    label.textColor = NSColor.secondaryLabelColor;
    return label;
}

- (void)buildInterface {
    _titleLabel = [NSTextField labelWithString:@"Device"];
    _titleLabel.font = [NSFont systemFontOfSize:20 weight:NSFontWeightSemibold];

    _interfacePopup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    _interfacePopup.target = self;
    _interfacePopup.action = @selector(interfaceChanged:);
    _interfacePopup.accessibilityLabel = @"Interface";

    _macLabel = [NSTextField labelWithString:@"—"];
    _macLabel.font = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    _macLabel.selectable = YES;

    _ipv4Field = [NSTextField textFieldWithString:@""];
    _ipv4Field.placeholderString = @"192.168.1.10";
    _ipv4Field.accessibilityLabel = @"IPv4 Address";
    _subnetField = [NSTextField textFieldWithString:@"255.255.255.0"];
    _subnetField.accessibilityLabel = @"Subnet Mask";
    _gatewayField = [NSTextField textFieldWithString:@""];
    _gatewayField.placeholderString = @"192.168.1.1";
    _gatewayField.accessibilityLabel = @"Default Gateway";
    _ipv6Field = [NSTextField textFieldWithString:@""];
    _ipv6Field.placeholderString = @"2001:db8:10::10";
    _ipv6Field.accessibilityLabel = @"IPv6 Address";

    _vlanModePopup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [_vlanModePopup addItemsWithTitles:@[@"Access", @"Trunk"]];
    _vlanModePopup.accessibilityLabel = @"VLAN Mode";
    _vlanField = [NSTextField textFieldWithString:@"1"];
    _vlanField.placeholderString = @"VLAN ID";
    _vlanField.accessibilityLabel = @"VLAN ID";
    NSStackView *vlanRow = [NSStackView stackViewWithViews:@[_vlanModePopup, _vlanField]];
    vlanRow.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    vlanRow.spacing = 8;
    [_vlanModePopup.widthAnchor constraintEqualToConstant:105].active = YES;

    NSButton *applyButton = [NSButton buttonWithTitle:@"Apply IPv4 Configuration"
                                               target:self action:@selector(applyConfiguration:)];
    applyButton.bezelStyle = NSBezelStyleRounded;
    applyButton.accessibilityLabel = @"Apply IPv4 Configuration";

    _pingField = [NSTextField textFieldWithString:@""];
    _pingField.placeholderString = @"Destination IPv4 address";
    _pingField.accessibilityLabel = @"Ping Destination";
    NSButton *pingButton = [NSButton buttonWithTitle:@"Ping" target:self action:@selector(runPing:)];
    pingButton.bezelStyle = NSBezelStyleRounded;
    pingButton.keyEquivalent = @"\r";
    pingButton.accessibilityLabel = @"Run Ping";
    NSStackView *pingRow = [NSStackView stackViewWithViews:@[_pingField, pingButton]];
    pingRow.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    pingRow.spacing = 8;
    [_pingField.widthAnchor constraintGreaterThanOrEqualToConstant:150].active = YES;

    NSButton *dhcpButton = [NSButton buttonWithTitle:@"Request DHCP" target:self action:@selector(requestDHCP:)];
    dhcpButton.accessibilityLabel = @"Request DHCP Lease";
    NSButton *statusButton = [NSButton buttonWithTitle:@"Lab Status" target:self action:@selector(showLabStatus:)];
    statusButton.accessibilityLabel = @"Show Milestone 7 Status";
    NSStackView *toolsRow = [NSStackView stackViewWithViews:@[dhcpButton, statusButton]];
    toolsRow.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    toolsRow.distribution = NSStackViewDistributionFillEqually;
    toolsRow.spacing = 8;

    _statusLabel = [NSTextField wrappingLabelWithString:@"Select an interface and configure IPv4."];
    _statusLabel.font = [NSFont systemFontOfSize:11];
    _statusLabel.textColor = NSColor.secondaryLabelColor;

    _outputView = [[NSTextView alloc] initWithFrame:NSZeroRect];
    _outputView.editable = NO;
    _outputView.selectable = YES;
    _outputView.font = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    _outputView.backgroundColor = NSColor.textBackgroundColor;
    _outputView.textContainerInset = NSMakeSize(8, 8);
    _outputView.string = @"Ping events and ARP state appear here.";
    _outputView.accessibilityLabel = @"Simulation Output";
    NSScrollView *outputScroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    outputScroll.borderType = NSBezelBorder;
    outputScroll.hasVerticalScroller = YES;
    outputScroll.autohidesScrollers = YES;
    outputScroll.documentView = _outputView;

    NSBox *separator = [[NSBox alloc] initWithFrame:NSZeroRect];
    separator.boxType = NSBoxSeparator;
    NSStackView *controls = [NSStackView stackViewWithViews:@[
        _titleLabel,
        [self caption:@"INTERFACE"], _interfacePopup,
        [self caption:@"MAC ADDRESS"], _macLabel,
        [self caption:@"IPv4 ADDRESS"], _ipv4Field,
        [self caption:@"SUBNET MASK"], _subnetField,
        [self caption:@"DEFAULT GATEWAY"], _gatewayField,
        [self caption:@"IPv6 ADDRESS (/64)"], _ipv6Field,
        [self caption:@"SWITCHPORT / VLAN"], vlanRow,
        applyButton,
        separator,
        [self caption:@"PING"], pingRow,
        toolsRow,
        _statusLabel,
        [self caption:@"PACKET / TABLE OUTPUT"],
    ]];
    controls.orientation = NSUserInterfaceLayoutOrientationVertical;
    controls.alignment = NSLayoutAttributeLeading;
    controls.spacing = 7;
    controls.translatesAutoresizingMaskIntoConstraints = NO;
    for (NSView *view in @[_titleLabel, _interfacePopup, _macLabel, _ipv4Field,
                            _subnetField, _gatewayField, _ipv6Field, vlanRow, applyButton, pingRow, toolsRow,
                            _statusLabel]) {
        [view.widthAnchor constraintEqualToAnchor:controls.widthAnchor].active = YES;
    }
    outputScroll.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:controls];
    [self addSubview:outputScroll];
    [NSLayoutConstraint activateConstraints:@[
        [controls.topAnchor constraintEqualToAnchor:self.topAnchor constant:20],
        [controls.leadingAnchor constraintEqualToAnchor:self.leadingAnchor constant:18],
        [controls.trailingAnchor constraintEqualToAnchor:self.trailingAnchor constant:-18],
        [outputScroll.topAnchor constraintEqualToAnchor:controls.bottomAnchor constant:8],
        [outputScroll.leadingAnchor constraintEqualToAnchor:controls.leadingAnchor],
        [outputScroll.trailingAnchor constraintEqualToAnchor:controls.trailingAnchor],
        [outputScroll.bottomAnchor constraintEqualToAnchor:self.bottomAnchor constant:-16],
        [outputScroll.heightAnchor constraintGreaterThanOrEqualToConstant:120],
    ]];
}

- (void)inspectDevice:(DeviceNodeView *)device {
    _device = device;
    _titleLabel.stringValue = device.displayName;
    [_interfacePopup removeAllItems];
    [_interfacePopup addItemsWithTitles:device.interfaceNames];
    [self interfaceChanged:nil];
    _gatewayField.stringValue = device.defaultGateway;
    _pingField.stringValue = @"";
    _statusLabel.stringValue = @"Ready.";
    [self refreshOutputWithPrefix:@"Current tables"];
}

- (void)interfaceChanged:(id)sender {
    NSString *interfaceName = _interfacePopup.titleOfSelectedItem ?: @"";
    _macLabel.stringValue = [_device macAddressForInterfaceNamed:interfaceName];
    _ipv4Field.stringValue = [_device ipv4AddressForInterfaceNamed:interfaceName];
    _subnetField.stringValue = [_device subnetMaskForInterfaceNamed:interfaceName];
    _ipv6Field.stringValue = [_device ipv6AddressForInterfaceNamed:interfaceName];
    NSString *mode = [_device vlanModeForInterfaceNamed:interfaceName];
    [_vlanModePopup selectItemWithTitle:mode];
    _vlanField.integerValue = [_device vlanIDForInterfaceNamed:interfaceName];
}

- (void)applyConfiguration:(id)sender {
    NSString *interfaceName = _interfacePopup.titleOfSelectedItem ?: @"";
    BOOL interfaceValid = [_device configureInterfaceNamed:interfaceName
                                               ipv4Address:_ipv4Field.stringValue
                                                subnetMask:_subnetField.stringValue];
    BOOL gatewayValid = [_device setDefaultGatewayAddress:_gatewayField.stringValue];
    BOOL ipv6Valid = _ipv6Field.stringValue.length == 0 ||
        [_device configureInterfaceNamed:interfaceName ipv6Address:_ipv6Field.stringValue prefixLength:64];
    BOOL vlanValid = [_device configureInterfaceNamed:interfaceName
                                             vlanMode:_vlanModePopup.titleOfSelectedItem ?: @"Access"
                                               vlanID:_vlanField.integerValue];
    if (interfaceValid && gatewayValid && ipv6Valid && vlanValid) {
        _statusLabel.textColor = NSColor.systemGreenColor;
        _statusLabel.stringValue = @"IPv4 configuration applied to the shared device model.";
    } else {
        _statusLabel.textColor = NSColor.systemRedColor;
        _statusLabel.stringValue = @"Invalid IPv4 address, subnet mask, or gateway.";
    }
}

- (void)requestDHCP:(id)sender {
    NSString *result = [_topologyView requestDHCPForDevice:_device];
    [self interfaceChanged:nil];
    [self refreshOutputWithPrefix:result];
    BOOL succeeded = [result containsString:@"SUCCESS"];
    _statusLabel.textColor = succeeded ? NSColor.systemGreenColor : NSColor.systemRedColor;
    _statusLabel.stringValue = succeeded ? @"DHCP lease applied." : @"DHCP failed; inspect the events below.";
}

- (void)showLabStatus:(id)sender {
    NSString *status = [_topologyView advancedStatusForDevice:_device];
    [self refreshOutputWithPrefix:status];
    _statusLabel.textColor = NSColor.systemBlueColor;
    _statusLabel.stringValue = @"Milestone 7 learning state displayed.";
}

- (void)runPing:(id)sender {
    if (_pingField.stringValue.length == 0) {
        _statusLabel.textColor = NSColor.systemRedColor;
        _statusLabel.stringValue = @"Enter a destination IPv4 address.";
        return;
    }
    NSString *result = [_topologyView runPingFromDevice:_device targetAddress:_pingField.stringValue];
    [self refreshOutputWithPrefix:result];
    BOOL succeeded = [result containsString:@"SUCCESS"];
    _statusLabel.textColor = succeeded ? NSColor.systemGreenColor : NSColor.systemRedColor;
    _statusLabel.stringValue = succeeded ? @"Ping completed successfully." : @"Ping failed; inspect the events below.";
}

- (void)refreshOutputWithPrefix:(NSString *)prefix {
    _outputView.string = [NSString stringWithFormat:
        @"%@\n\nARP TABLE\n%@\n\nMAC ADDRESS TABLE\n%@\n\nROUTING TABLE\n%@\n\nSERVICES\n%@",
        prefix, _device.arpTableText, _device.macAddressTableText,
        _device.routingTableText, _device.servicesTableText];
}

@end
