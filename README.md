# NetLab for macOS

NetLab 1.2는 AppKit + Objective-C++ UI와 C++17 시뮬레이션 엔진으로 만든 가벼운 macOS 네트워크 학습 앱입니다. `macOS_NetLab_Codex_작업지시서.md`의 Milestone 1~7을 교육용 논리 모델로 구현합니다.

## 실행 및 빌드

프로젝트 루트의 `NetLab.app`을 더블 클릭하면 Xcode 없이 실행할 수 있습니다.

```sh
xcodebuild -project NetLab.xcodeproj -scheme NetLab -configuration Release build
```

요구 환경은 macOS 14 이상이며 Apple Silicon을 우선 지원합니다.

## 구현 범위

- M1~M3: 네이티브 75:25 UI, Drag & Drop, 선택/이동/삭제, Ethernet 링크, MAC/ARP/IPv4/Gateway/ICMP Ping
- M4: Connected/Static/Default Route와 서로 다른 Subnet의 Router 경유 Ping
- M5: 포트별 Access/Trunk VLAN, 허용/차단 판정, Switch MAC 학습
- M6: DHCP DORA 이벤트와 Lease, DNS A Record, NAT Translation, 순서형 ACL
- M7: STP 상태, OSPF/RIP 수렴 상태, IPv6 구성, Wireless SSID, Stateful Firewall, VPN Tunnel의 교육용 논리 상태
- Palette: Router, Switch, Firewall, Wireless AP, Services Server, Desktop PC용 자체 벡터 아이콘
- Demo: 두 Subnet, Router, Services, AP, Firewall을 포함한 통합 학습 토폴로지
- Add Device: 선택된 노드의 Inspector에서 장비 목록으로 즉시 전환
- Delete Device: Toolbar, Delete/Backspace 키, 노드 우클릭 메뉴, Inspector 버튼 지원

STP/동적 라우팅/IPv6/Wireless/Firewall/VPN은 실제 운영체제 네트워크나 암호화를 사용하는 에뮬레이션이 아니라 핵심 개념과 상태를 보여 주는 단순화 모델입니다.

## 사용법과 테스트

학습자가 장비를 추가하고 연결한 뒤 Ping하는 과정은 [NetLab_간단_사용설명서.md](NetLab_간단_사용설명서.md)에 정리되어 있습니다.

```sh
NetLab.app/Contents/MacOS/NetLab --self-test
NetLab.app/Contents/MacOS/NetLab --ui-self-test
```

외부 라이브러리나 외부 이미지 자산은 포함하지 않습니다.
