# macOS 네트워크 학습용 시뮬레이터 개발 작업지시서

## 1. 프로젝트 개요

### 프로젝트명
**NetLab for macOS**  
※ 프로젝트명은 임시 명칭이며 추후 변경 가능

### 개발 목적
macOS 환경에서 동작하는 개인 학습용 네트워크 토폴로지 시뮬레이터를 개발한다.

프로그램의 사용 경험과 핵심 목적은 Cisco Packet Tracer와 유사하게 구성하되, Cisco 제품의 소스코드·UI 리소스·아이콘·상표 요소를 복제하지 않는다.

사용자는 화면에 네트워크 장비를 배치하고, 장비 간 링크를 연결하고, 각 장비의 네트워크 설정을 직접 변경하면서 네트워크 동작 원리를 학습할 수 있어야 한다.

본 프로젝트는 상용 네트워크 에뮬레이터 수준의 완전한 구현보다 다음 목표를 우선한다.

1. 작은 프로그램 용량
2. macOS 네이티브 실행
3. 빠른 실행 속도
4. 낮은 메모리 사용량
5. 네트워크 기초부터 중급 수준까지 실습 가능
6. 향후 장비와 프로토콜을 추가하기 쉬운 구조

---

# 2. 개발 환경 및 기술 스택

## 2.1 지원 운영체제

1차 개발 대상:

- macOS 14 이상
- Apple Silicon arm64 우선 지원

추후 필요 시 Intel Mac용 x86_64 빌드를 추가한다.

## 2.2 개발 언어

프로그램 용량과 실행 성능을 고려하여 아래 구조를 사용한다.

### 기본 구조

- UI: **Objective-C++ / AppKit**
- 네트워크 시뮬레이션 엔진: **C++17 이상**
- 시스템 API: macOS Foundation / AppKit / Core Animation
- 데이터 저장: Foundation JSON 또는 SQLite
- 빌드: Xcode / clang

확장자는 필요에 따라 다음을 사용한다.

- `.mm` : Objective-C++
- `.m` : Objective-C
- `.cpp` : C++
- `.hpp` : C++ Header

가능한 한 외부 GUI 프레임워크를 사용하지 않는다.

### 사용하지 않을 기술

다음 기술은 특별한 이유가 없는 한 사용하지 않는다.

- Electron
- Chromium Embedded Framework
- Qt
- Python GUI
- Java
- .NET MAUI
- 대형 웹 프론트엔드 프레임워크

목적은 프로그램의 설치 용량과 런타임 메모리 사용량을 최소화하는 것이다.

---

# 3. 프로그램 기본 화면

프로그램 메인 화면은 좌우 2분할 구조로 구성한다.

단, 두 영역 사이에 시각적인 구분선은 표시하지 않는다.

```text
┌──────────────────────────────────────────────────────────────┐
│                          Toolbar                             │
├───────────────────────────────────────────────┬──────────────┤
│                                               │              │
│                                               │ Device       │
│              Topology View                    │ Palette /    │
│                                               │ Settings     │
│                                               │              │
│                                               │              │
│                                               │              │
└───────────────────────────────────────────────┴──────────────┘
```

실제 구현에서는 위 예시의 세로 구분선은 표시하지 않는다.

## 3.1 좌측 영역

좌측은 **Topology View**로 사용한다.

기본 화면 비율:

- Topology View: 약 75%
- Device / Settings 영역: 약 25%

사용자가 창 크기를 변경하면 두 영역도 자연스럽게 리사이즈한다.

Topology View에서는 다음 기능을 제공한다.

- 장비 Drag & Drop
- 장비 이동
- 장비 선택
- 다중 선택
- 장비 삭제
- 장비 복제
- 장비 이름 변경
- 장비 간 링크 연결
- 링크 삭제
- Zoom In / Out
- Canvas 이동
- 전체 토폴로지 자동 맞춤
- 장비 정렬
- Undo / Redo

---

# 4. 우측 패널

우측 패널은 상황에 따라 다음 두 가지 역할을 수행한다.

## 4.1 장비 추가 모드

장비가 선택되지 않은 경우 장비 추가 메뉴를 표시한다.

장비 종류별로 Category를 나눈다.

### Router

- Generic Router
- Branch Router
- Enterprise Router
- Edge Router
- Virtual Router

### Switch

- Hub
- Bridge
- L2 Switch
- L3 Switch
- Managed Switch
- PoE Switch
- Data Center Switch

### Security

- Firewall
- UTM
- IDS
- IPS
- VPN Gateway
- VPN Concentrator
- Proxy Gateway

### Wireless

- Wireless Access Point
- Wireless Router
- Wireless LAN Controller

### WAN / Carrier

- Modem
- DSL Modem
- Cable Modem
- CSU/DSU
- WAN Cloud
- MPLS Cloud
- Internet Cloud

### Server

- Generic Server
- DHCP Server
- DNS Server
- Web Server
- FTP Server
- NTP Server
- Syslog Server
- RADIUS Server
- TACACS+ Server
- Mail Server

### Endpoint

- Desktop PC
- Laptop
- macOS Client
- Linux Client
- Windows Client
- Smartphone
- Tablet
- Printer
- IP Phone
- IoT Device

### Virtual / Infrastructure

- Hypervisor
- Virtual Machine
- Container Host
- Load Balancer
- NAS
- SAN
- Wireless Controller

장비 종류는 코드에 하드코딩하지 않고 가능한 한 데이터 기반 구조로 설계한다.

향후 새로운 장비를 쉽게 추가할 수 있어야 한다.

예:

```text
DeviceDefinition
 ├─ id
 ├─ category
 ├─ displayName
 ├─ icon
 ├─ interfaces
 ├─ capabilities
 └─ defaultConfiguration
```

---

# 5. 장비 아이콘

Cisco Packet Tracer의 아이콘 파일을 프로그램에 직접 포함하거나 복사하지 않는다.

Cisco가 별도로 재사용을 허가한 라이선스가 확인되지 않는 한 Packet Tracer의 이미지 리소스를 프로젝트에 포함하지 않는다.

대신 다음 원칙을 적용한다.

1. 자체 제작한 단순 SVG 아이콘 사용
2. CC0 / Public Domain / MIT 등 상업적·비상업적 재사용이 가능한 아이콘 사용
3. 외부 아이콘을 사용할 경우 프로젝트 내부 `THIRD_PARTY_LICENSES.md`에 출처와 라이선스를 기록

아이콘 스타일은 Cisco Packet Tracer와 비슷한 의미 전달 체계를 가질 수 있으나 직접적인 이미지 복제는 하지 않는다.

기본적으로 다음 장비가 한눈에 구분되도록 한다.

- Router
- Switch
- Firewall
- AP
- Server
- PC
- Cloud
- Load Balancer
- Storage
- Mobile Device

아이콘은 가능하면 SVG 또는 macOS Vector Asset을 사용한다.

---

# 6. 장비 배치

사용자가 우측 장비 목록에서 장비를 선택하면 다음 두 가지 방법으로 Topology View에 추가할 수 있게 한다.

### 방법 A

장비를 Drag & Drop하여 원하는 위치에 배치

### 방법 B

장비 선택 후 Topology View 클릭 위치에 배치

장비가 배치되면 자동으로 기본 이름을 부여한다.

예:

```text
Router1
Router2
Switch1
Switch2
PC1
PC2
Server1
```

사용자는 이름을 변경할 수 있어야 한다.

---

# 7. 인터페이스

각 장비는 실제 네트워크 장비처럼 여러 개의 인터페이스를 가질 수 있어야 한다.

지원할 인터페이스 타입:

- Ethernet
- FastEthernet
- GigabitEthernet
- 10GigabitEthernet
- Serial
- Loopback
- VLAN Interface
- Wireless
- Management

인터페이스 객체 예:

```text
Interface
 ├─ id
 ├─ name
 ├─ type
 ├─ macAddress
 ├─ ipv4Address
 ├─ ipv4SubnetMask
 ├─ ipv6Address
 ├─ speed
 ├─ duplex
 ├─ adminStatus
 ├─ operationalStatus
 └─ connectedLink
```

---

# 8. 링크 연결

Toolbar에 **Connect Mode**를 제공한다.

사용 방법:

1. Connect Mode 선택
2. 첫 번째 장비 클릭
3. 사용할 Interface 선택
4. 두 번째 장비 클릭
5. 사용할 Interface 선택
6. Link 생성

지원 링크:

- Ethernet
- Fiber
- Serial
- Wireless
- Logical Tunnel

초기 버전에서는 Ethernet 링크를 가장 먼저 구현한다.

---

# 9. 링크 상태 표시

링크 상태는 시각적으로 확인할 수 있어야 한다.

링크 연결 직후에는 초기화 과정을 거친 뒤 Up/Down 상태를 결정한다.

## Link Down

- 회색
- 점멸 없음

## 100 Mbps Link Up

- 주황색
- 링크 양 끝 포트 또는 장비 연결 지점에서 점멸 효과

## 1 Gbps Link Up

- 녹색
- 링크 양 끝 포트 또는 장비 연결 지점에서 점멸 효과

점멸 속도는 실제 패킷 트래픽 또는 가상의 링크 사용량에 따라 변화하도록 한다.

예:

```text
Traffic 없음
→ 천천히 점멸

Traffic 적음
→ 일반 점멸

Traffic 많음
→ 빠르게 점멸
```

CPU 사용량을 줄이기 위해 Core Animation 기반으로 구현한다.

초기 요구사항에서는 다음 색상을 반드시 유지한다.

```text
100 Mbps = Orange
1 Gbps   = Green
```

10 Gbps 이상의 링크는 향후 확장을 위해 별도 상태값으로 구현하되, 초기 버전에서는 고정된 색상 규칙을 강제하지 않는다.

---

# 10. 장비 선택 및 설정 메뉴

Topology View에서 장비를 클릭하면 우측 패널이 **Device Configuration Mode**로 변경된다.

예:

```text
Switch1
────────────────────

[Overview]
[Interfaces]
[IPv4]
[IPv6]
[VLAN]
[Routing]
[Switching]
[DHCP]
[DNS]
[ACL]
[NAT]
[VPN]
[Wireless]
[Services]
[CLI]
[Logs]
```

장비 종류에 따라 사용할 수 없는 메뉴는 숨기거나 비활성화한다.

---

# 11. 네트워크 설정 기능

교육용 프로그램이므로 가능한 한 다양한 네트워크 설정을 직접 실습할 수 있도록 설계한다.

기능은 모듈 방식으로 구현한다.

## 11.1 기본 Interface 설정

- Interface Enable / Disable
- MAC Address 표시
- Speed
- Duplex
- MTU
- Description

## 11.2 IPv4

- Static IP
- Subnet Mask
- Default Gateway
- Secondary IP
- Static ARP

## 11.3 IPv6

- IPv6 Address
- Prefix Length
- Link Local Address
- Default Route

## 11.4 VLAN

- VLAN 생성
- VLAN 삭제
- VLAN 이름
- Access Port
- Trunk Port
- Native VLAN
- Allowed VLAN

## 11.5 Switching

- MAC Address Table
- ARP Table
- STP 기본 동작
- Port State
- Port Security

향후:

- RSTP
- MSTP
- LACP
- EtherChannel

## 11.6 Routing

초기:

- Connected Route
- Static Route
- Default Route

추후:

- RIP
- OSPF
- BGP

Routing Table을 GUI에서 확인할 수 있도록 한다.

예:

```text
Destination
Gateway
Interface
Metric
Protocol
```

## 11.7 DHCP

- DHCP Pool
- Network
- Default Gateway
- DNS Server
- Lease
- Excluded Address

## 11.8 DNS

- A Record
- AAAA Record
- CNAME
- PTR

## 11.9 NAT

- Static NAT
- Dynamic NAT
- PAT

## 11.10 ACL

- Standard ACL
- Extended ACL 개념에 대응하는 필터 규칙
- Source
- Destination
- Protocol
- Port
- Permit / Deny

## 11.11 Firewall

- Zone
- Rule
- Source
- Destination
- Service
- Action
- Logging

## 11.12 VPN

향후 학습용 구현:

- Site-to-Site VPN
- Remote Access VPN
- Tunnel Interface

실제 암호화 구현보다 논리적인 패킷 전달과 정책 학습을 우선한다.

## 11.13 Wireless

- SSID
- Security Type
- Password
- Channel
- VLAN
- Client Association

---

# 12. CLI 기능

Cisco Packet Tracer와 비슷하게 장비를 CLI에서도 설정할 수 있도록 한다.

단, Cisco IOS를 복제하지 않고 학습용 명령 체계를 자체 구현한다.

우측 설정 메뉴에 **CLI** 탭을 제공한다.

예:

```text
NetLab> enable

NetLab# configure terminal

NetLab(config)# interface gigabitEthernet 0/1

NetLab(config-if)# ip address 192.168.10.1 255.255.255.0

NetLab(config-if)# no shutdown
```

지원 명령의 예:

```text
show interfaces
show ip interface
show ip route
show arp
show mac-address-table
show vlan
show running-config
show startup-config
ping
traceroute
configure terminal
interface
ip address
ipv6 address
shutdown
no shutdown
vlan
switchport
route
hostname
```

내부적으로 GUI 설정과 CLI 설정은 동일한 Configuration Model을 수정해야 한다.

즉:

```text
GUI
   ↓
Configuration Model
   ↑
CLI
```

GUI와 CLI에 별도의 설정 상태를 만들지 않는다.

---

# 13. 네트워크 시뮬레이션 엔진

네트워크 동작은 C++ 기반의 독립적인 Simulation Engine으로 구현한다.

UI 코드와 네트워크 엔진을 분리한다.

기본 구조:

```text
AppKit UI
   │
   ▼
Topology Controller
   │
   ▼
Simulation Engine
   │
   ├─ Device
   ├─ Interface
   ├─ Link
   ├─ EthernetFrame
   ├─ ARP
   ├─ IPv4
   ├─ ICMP
   ├─ Switching
   └─ Routing
```

---

# 14. 패킷 처리

초기 버전에서는 다음 순서로 구현한다.

## Phase 1

- Ethernet
- MAC Address
- ARP
- IPv4
- ICMP
- Ping

## Phase 2

- L2 Switching
- MAC Learning
- VLAN
- Access / Trunk

## Phase 3

- Routing
- Static Route
- Default Route
- TTL

## Phase 4

- DHCP
- DNS
- NAT
- ACL

## Phase 5

- STP
- OSPF
- RIP

## Phase 6

- IPv6
- Wireless
- VPN
- Firewall Simulation

실제 운영체제 Network Stack을 사용하는 것이 아니라 프로그램 내부에서 논리적으로 패킷을 생성하고 전달한다.

---

# 15. Ping 학습 기능

사용자가 PC 또는 장비에서 다음과 같이 Ping을 수행할 수 있도록 한다.

```text
ping 192.168.1.1
```

Simulation Engine은 다음 과정을 재현한다.

```text
Destination 확인
      ↓
Subnet 판단
      ↓
ARP 확인
      ↓
ARP Request
      ↓
MAC 학습
      ↓
ICMP Echo Request
      ↓
Switch 전달
      ↓
Router Routing
      ↓
ICMP Echo Reply
```

학습자가 원하는 경우 **Packet Step Mode**를 켜서 이 과정을 단계별로 확인할 수 있도록 설계한다.

---

# 16. Packet Visualization

Toolbar에 다음 모드를 제공한다.

```text
Realtime
Step
Pause
```

Step Mode에서는 패킷이 이동하는 경로를 토폴로지 위에서 확인할 수 있도록 한다.

예:

```text
PC1
 ↓
Switch1
 ↓
Router1
 ↓
Switch2
 ↓
PC2
```

패킷은 작은 점 또는 간단한 애니메이션으로 표현한다.

과도한 그래픽 효과는 사용하지 않는다.

---

# 17. 상태 및 학습 정보

장비를 선택하면 다음 상태 정보를 확인할 수 있어야 한다.

### PC

- IP
- Subnet
- Gateway
- DNS
- ARP Table

### Switch

- Interface State
- MAC Address Table
- VLAN
- STP State

### Router

- Interface
- ARP Table
- Routing Table
- NAT Table
- ACL

### Firewall

- Interface
- Zone
- Policy
- Session

---

# 18. 프로젝트 저장

현재 토폴로지를 파일로 저장할 수 있어야 한다.

확장자 예:

```text
.netlab
```

파일 내용은 JSON 기반으로 구현한다.

예:

```json
{
  "version": 1,
  "devices": [],
  "links": [],
  "simulation": {}
}
```

지원 기능:

- New
- Open
- Save
- Save As
- Autosave

---

# 19. 프로그램 용량 최적화

본 프로그램은 개인 학습용 도구이므로 작은 설치 용량을 중요하게 취급한다.

다음 원칙을 따른다.

1. AppKit 사용
2. 외부 GUI Framework 사용 금지
3. Chromium 계열 Runtime 사용 금지
4. 불필요한 이미지 포함 금지
5. SVG / Vector Asset 우선
6. macOS 기본 Framework 적극 활용
7. 외부 라이브러리는 반드시 필요성을 검토
8. Debug Asset을 Release Build에 포함하지 않음
9. 로그 및 샘플 파일을 앱 번들에 과도하게 포함하지 않음

목표:

```text
초기 MVP 앱 번들: 가능한 한 30 MB 이하
```

단, 기능 안정성을 희생하면서 억지로 용량을 줄이지 않는다.

---

# 20. UI 디자인 방향

UI는 최대한 단순하게 구성한다.

스타일:

- macOS Native
- Flat
- Minimal
- 과도한 그림자 금지
- 과도한 애니메이션 금지
- 불필요한 Border 금지

두 패널 사이에는 Divider Line을 표시하지 않는다.

Topology View 배경은 밝은 회색 또는 macOS System Background를 사용한다.

Dark Mode도 macOS 시스템 설정을 따라 자동 대응한다.

---

# 21. Toolbar

상단 Toolbar 기본 구성:

```text
New
Open
Save

Select
Move
Connect
Delete

Zoom In
Zoom Out
Fit

Realtime
Step
Pause
```

---

# 22. 장비 Context Menu

Topology View에서 장비를 우클릭하면 다음 메뉴를 제공한다.

```text
Configure
Rename
Duplicate
Delete

Ping
Open CLI
Show Interfaces
Show Routing Table
Show ARP Table
```

장비 종류에 따라 관련 없는 항목은 숨긴다.

---

# 23. 내부 코드 구조

권장 디렉터리 구조:

```text
NetLab/
│
├─ App/
│  ├─ AppDelegate
│  └─ MainWindowController
│
├─ UI/
│  ├─ TopologyView
│  ├─ DevicePaletteView
│  ├─ DeviceInspectorView
│  ├─ CLIView
│  └─ ToolbarController
│
├─ Core/
│  ├─ Device
│  ├─ Interface
│  ├─ Link
│  ├─ Packet
│  ├─ SimulationEngine
│  └─ EventQueue
│
├─ Protocol/
│  ├─ Ethernet
│  ├─ ARP
│  ├─ IPv4
│  ├─ IPv6
│  ├─ ICMP
│  ├─ DHCP
│  ├─ DNS
│  ├─ VLAN
│  ├─ STP
│  ├─ NAT
│  ├─ ACL
│  └─ Routing
│
├─ CLI/
│  ├─ CLIParser
│  ├─ CLICommand
│  └─ Commands
│
├─ Model/
│  ├─ Topology
│  ├─ Configuration
│  └─ ProjectFile
│
├─ Assets/
│  └─ DeviceIcons
│
└─ Tests/
```

---

# 24. 코드 작성 규칙

1. UI와 Simulation Engine을 분리한다.
2. 네트워크 엔진은 가능한 한 순수 C++로 작성한다.
3. macOS UI 연동이 필요한 부분만 Objective-C++를 사용한다.
4. 하나의 파일에 지나치게 많은 기능을 넣지 않는다.
5. Device별 기능을 거대한 switch-case 하나로 만들지 않는다.
6. 장비 Capability 방식으로 기능을 분리한다.
7. Protocol Module을 독립적으로 추가할 수 있게 설계한다.
8. 모든 주요 클래스에 최소한의 주석을 작성한다.
9. 메모리 소유권을 명확하게 관리한다.
10. UI Thread에서 무거운 Simulation 작업을 수행하지 않는다.

---

# 25. 장비 Capability 구조

장비 종류를 기능 조합으로 구현한다.

예:

```text
L2_SWITCHING
L3_ROUTING
VLAN
STP
DHCP_CLIENT
DHCP_SERVER
DNS_SERVER
NAT
ACL
FIREWALL
WIRELESS
VPN
```

예를 들어 L3 Switch는 다음 Capability를 가진다.

```text
L2_SWITCHING
L3_ROUTING
VLAN
STP
ACL
```

Router:

```text
L3_ROUTING
DHCP_SERVER
NAT
ACL
VPN
```

이 방식으로 새 장비 추가 시 코드 중복을 최소화한다.

---

# 26. 개발 단계

처음부터 Packet Tracer 전체 기능을 구현하려고 하지 않는다.

## Milestone 1 — UI Prototype

구현:

- macOS App 실행
- 좌우 2분할 UI
- Divider 미표시
- Device Palette
- Topology Canvas
- 장비 Drag & Drop
- 장비 이동
- Delete

완료 조건:

Topology View에 Router, Switch, PC를 자유롭게 배치할 수 있다.

## Milestone 2 — Link

구현:

- Interface
- Ethernet Link
- Link Up / Down
- 100 Mbps Orange
- 1 Gbps Green
- Link Animation

완료 조건:

PC - Switch - Router 구조를 화면에 구성할 수 있다.

## Milestone 3 — IPv4 / Ping

구현:

- MAC
- ARP
- IPv4
- Gateway
- ICMP
- Ping

완료 조건:

```text
PC1 → Switch → PC2
```

구성에서 같은 Subnet의 Ping이 성공한다.

## Milestone 4 — Router

구현:

- Routing Table
- Connected Route
- Static Route
- Default Route

완료 조건:

서로 다른 Subnet의 PC 간 Ping이 Router를 통해 성공한다.

## Milestone 5 — VLAN

구현:

- VLAN
- Access
- Trunk
- MAC Table

완료 조건:

VLAN에 따라 통신 성공/실패를 구분한다.

## Milestone 6 — Services

구현:

- DHCP
- DNS
- NAT
- ACL

## Milestone 7 — Advanced

구현 후보:

- STP
- OSPF
- RIP
- IPv6
- Wireless
- Firewall
- VPN

---

# 27. 테스트용 기본 토폴로지

프로그램 테스트를 위해 다음 토폴로지를 자동 생성할 수 있도록 한다.

```text
PC1
 │
 │ 1Gbps
 │
Switch1
 │
 │ 1Gbps
 │
Router1
 │
 │ 1Gbps
 │
Switch2
 │
 │ 100Mbps
 │
PC2
```

예시 주소:

```text
PC1
192.168.10.10/24
GW 192.168.10.1

Router1
G0/0 192.168.10.1/24
G0/1 192.168.20.1/24

PC2
192.168.20.10/24
GW 192.168.20.1
```

PC1 → PC2 Ping 성공 여부를 자동 테스트한다.

---

# 28. 반드시 구현해야 하는 핵심 요구사항

다음 요구사항은 임의로 삭제하거나 변경하지 않는다.

- macOS Native Application
- C 계열 언어 사용
- C++ + Objective-C++ 기반
- Topology View
- 우측 Device Palette
- 2분할 영역 사이 Divider Line 없음
- 장비 Drag & Drop
- Router / Switch / Firewall / AP / Server / Client 등 주요 네트워크 장비 지원
- 새로운 장비 추가가 쉬운 구조
- 장비 간 Link 연결
- 100 Mbps Link = Orange
- 1 Gbps Link = Green
- Link Traffic Animation
- Device Configuration Panel
- CLI
- IPv4
- IPv6 확장 구조
- VLAN
- Routing
- DHCP
- DNS
- NAT
- ACL
- Firewall 확장 구조
- Packet Simulation
- Ping
- Routing Table
- ARP Table
- MAC Address Table
- Project Save / Load
- 프로그램 용량 최소화

---

# 29. 저작권 및 라이선스 주의사항

Cisco Packet Tracer를 학습 경험의 참고 대상으로 삼되 다음은 복사하지 않는다.

- Cisco Packet Tracer 소스코드
- Cisco Packet Tracer 앱 내부 리소스
- Cisco 전용 아이콘 파일
- Cisco 로고
- Cisco 제품 이미지
- Cisco UI의 픽셀 단위 복제
- Cisco IOS의 코드 또는 바이너리

프로토콜 개념, 토폴로지 방식, 네트워크 학습 구조와 같은 일반적인 아이디어는 자체 구현한다.

외부 오픈소스 또는 이미지 리소스를 사용하면 반드시 라이선스를 기록한다.

---

# 30. Codex 작업 원칙

Codex는 아래 순서로 작업한다.

1. 먼저 프로젝트 전체 구조를 생성한다.
2. Milestone 1만 구현한다.
3. 컴파일 오류를 해결한다.
4. 실행 가능한 상태를 확인한다.
5. Milestone 2를 구현한다.
6. 이후 Milestone 단위로 기능을 추가한다.

한 번에 전체 기능을 구현하려고 하지 않는다.

각 Milestone 완료 시 다음 내용을 확인한다.

```text
Build 성공 여부
Runtime 오류 여부
UI 동작 여부
메모리 누수 여부
기존 기능 Regression 여부
```

기능 추가 시 기존 구조를 무시하고 임시 코드를 덧붙이지 않는다.

기존 Architecture와 Capability 구조를 유지하면서 구현한다.

---

# 31. 최종 목표

최종적으로 사용자가 다음과 같은 순서로 네트워크 학습을 수행할 수 있어야 한다.

```text
장비 선택
   ↓
Topology 배치
   ↓
Interface 연결
   ↓
IP 설정
   ↓
VLAN 설정
   ↓
Routing 설정
   ↓
Ping
   ↓
Packet 이동 확인
   ↓
Routing / ARP / MAC Table 확인
   ↓
문제 발생 시 설정 수정
```

프로그램의 핵심 정체성은 다음과 같다.

> macOS에서 가볍게 실행할 수 있는 네이티브 기반의 개인용 네트워크 토폴로지 및 패킷 시뮬레이션 학습 도구.

Cisco Packet Tracer의 상용 기능 전체를 복제하는 것이 목적이 아니라, 네트워크 관리자가 실제로 학습해야 할 L2/L3 네트워크 구조와 주요 서비스의 동작 원리를 직접 구성하고 관찰할 수 있는 프로그램을 만드는 것을 목표로 한다.
