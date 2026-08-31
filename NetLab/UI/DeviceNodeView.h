#import <AppKit/AppKit.h>

#ifdef __cplusplus
namespace netlab { class Device; }
#endif

NS_ASSUME_NONNULL_BEGIN

@class DeviceNodeView;

@protocol DeviceNodeViewDelegate <NSObject>
- (void)deviceNodeViewDidRequestSelection:(DeviceNodeView *)node extendingSelection:(BOOL)extend;
- (void)deviceNodeView:(DeviceNodeView *)node didMoveBy:(NSPoint)delta;
- (void)deviceNodeViewDidRequestDeletion:(DeviceNodeView *)node;
@end

@interface DeviceNodeView : NSView

@property(nonatomic, copy, readonly) NSString *deviceIdentifier;
@property(nonatomic, copy, readonly) NSString *instanceIdentifier;
@property(nonatomic, copy, readonly) NSString *displayName;
@property(nonatomic, copy, readonly) NSArray<NSString *> *interfaceNames;
@property(nonatomic, assign, getter=isSelected) BOOL selected;
@property(nonatomic, weak, nullable) id<DeviceNodeViewDelegate> delegate;

- (instancetype)initWithDeviceIdentifier:(NSString *)identifier
                              displayName:(NSString *)displayName
                           interfaceNames:(NSArray<NSString *> *)interfaceNames
                          interfaceSpeeds:(NSArray<NSNumber *> *)interfaceSpeeds;
- (NSInteger)speedMbpsForInterfaceNamed:(NSString *)interfaceName;
- (NSString *)macAddressForInterfaceNamed:(NSString *)interfaceName;
- (NSString *)ipv4AddressForInterfaceNamed:(NSString *)interfaceName;
- (NSString *)subnetMaskForInterfaceNamed:(NSString *)interfaceName;
- (BOOL)configureInterfaceNamed:(NSString *)interfaceName
                    ipv4Address:(NSString *)ipv4Address
                     subnetMask:(NSString *)subnetMask;
- (NSString *)defaultGateway;
- (BOOL)setDefaultGatewayAddress:(NSString *)address;
- (NSString *)arpTableText;
- (NSString *)macAddressTableText;
- (NSString *)routingTableText;
- (NSString *)servicesTableText;
- (NSString *)ipv6AddressForInterfaceNamed:(NSString *)interfaceName;
- (BOOL)configureInterfaceNamed:(NSString *)interfaceName ipv6Address:(NSString *)address prefixLength:(NSInteger)prefixLength;
- (NSString *)vlanModeForInterfaceNamed:(NSString *)interfaceName;
- (NSInteger)vlanIDForInterfaceNamed:(NSString *)interfaceName;
- (BOOL)configureInterfaceNamed:(NSString *)interfaceName vlanMode:(NSString *)mode vlanID:(NSInteger)vlanID;

#ifdef __cplusplus
- (netlab::Device *)coreDevice;
#endif

@end

NS_ASSUME_NONNULL_END
