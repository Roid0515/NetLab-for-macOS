# NetLab 간단 사용 설명서

NetLab은 실제 네트워크 패킷을 외부로 보내지 않고, 프로그램 안에서 장비·링크·프로토콜의 동작을 학습하는 macOS 앱입니다.

## 1. 실행하기

프로젝트 폴더의 `NetLab.app`을 더블 클릭합니다. macOS가 실행을 막으면 Finder에서 앱을 Control-클릭한 뒤 **열기**를 선택합니다.

처음 동작을 빠르게 확인하려면 상단의 **Demo**를 누르세요. 서로 다른 두 네트워크가 Router로 연결된 Milestone 7 학습 토폴로지가 자동으로 만들어집니다.

## 2. 노드 추가하기

이미 배치한 노드가 선택되어 오른쪽에 설정 화면이 보인다면, 상단 **Add Device**를 누르세요. 설정 화면이 장비 목록으로 바뀝니다. 왼쪽 격자의 빈 곳을 한 번 클릭해도 같은 동작을 합니다.

1. 오른쪽 **Devices** 목록에서 Router, Switch, Firewall, Wireless AP, Services Server 또는 Desktop PC를 찾습니다.
2. 원하는 장비를 마우스로 끌어 왼쪽 격자 화면에 놓습니다.
3. 장비에는 `PC1`, `Switch1`, `Router1`처럼 이름이 자동으로 붙습니다.
4. 배치한 장비는 마우스로 다시 끌어 이동할 수 있습니다.

다른 노드를 계속 추가하려면 **Add Device → 장비 Drag & Drop**을 반복합니다. 기존 노드와 설정값은 그대로 유지됩니다.

장비를 삭제하려면 장비를 클릭한 뒤 Delete/Backspace 키 또는 상단 **Delete**를 누릅니다. **New**는 전체 토폴로지를 비웁니다.

## 3. 노드 연결하기

1. 상단 **Connect**를 누릅니다.
2. 첫 번째 장비를 클릭합니다.
3. 사용할 Interface를 선택하고 **Choose**를 누릅니다.
4. 두 번째 장비를 클릭하고 Interface를 선택합니다.
5. 링크가 만들어지면 Connect 모드가 계속 유지됩니다. 끝낼 때 상단 **Cancel Connect** 또는 Esc를 누릅니다.

회색 링크는 초기화 중, 초록 링크는 1 Gbps, 주황 링크는 100 Mbps 연결을 뜻합니다. 연결된 Interface는 다른 링크에서 중복 선택할 수 없습니다.

## 4. IP를 설정하고 통신 확인하기

1. 배치된 PC를 클릭합니다.
2. 오른쪽 Inspector에서 Interface를 선택합니다.
3. **IPv4 Address**, **Subnet Mask**, 필요하면 **Default Gateway**를 입력합니다.
4. **Apply IPv4 Configuration**을 누릅니다.
5. 다른 PC에도 IP를 설정합니다.
6. 출발 PC를 다시 선택하고 **PING** 칸에 목적지 IPv4 주소 또는 Demo의 DNS 이름 `pc2.netlab`을 입력합니다.
7. **Ping**을 누릅니다.

같은 Subnet에서는 ARP → Switching → ICMP 순서를 볼 수 있습니다. 서로 다른 Subnet에서는 PC의 Gateway와 Router Interface를 올바르게 설정해야 하며, Routing·ACL·NAT·TTL 이벤트가 추가로 표시됩니다.

Demo의 기본 통신 시험은 `PC1`에서 `pc2.netlab` 또는 `20.20.20.10`으로 Ping하는 것입니다.

## 5. VLAN 실습하기

Switch를 클릭하고 Interface마다 **Access** 또는 **Trunk**, VLAN ID를 선택한 뒤 **Apply IPv4 Configuration**을 누릅니다.

- PC가 연결된 포트는 보통 Access로 설정합니다.
- Switch끼리 연결된 포트는 Trunk로 설정합니다.
- Trunk는 학습용 기본 허용 목록으로 VLAN 10과 20을 전달합니다.
- 경로 중 한 포트라도 해당 VLAN을 허용하지 않으면 Ping이 실패하고 `VLAN ... blocks` 이벤트가 표시됩니다.

## 6. DHCP와 학습 상태 보기

- **Request DHCP**: 선택한 장비가 토폴로지의 Services Server에 주소를 요청합니다. Demo에서는 `10.10.10.100/24`부터 임대됩니다.
- **Lab Status**: 선택 장비의 STP, OSPF/RIP, IPv6, Wireless, Firewall, VPN 학습 상태를 보여줍니다.
- 아래 출력 영역: ARP Table, MAC Address Table, Routing Table, DHCP/DNS/NAT/ACL 상태를 한곳에서 확인합니다.

## 7. Demo에서 확인할 내용

Demo에는 `PC1 — Switch1 — Router1 — Switch2 — PC2` 경로와 Services Server, Wireless AP, Firewall이 포함됩니다.

1. PC1을 선택하고 `pc2.netlab`으로 Ping해 DNS와 Router 경유 통신을 확인합니다.
2. Router1을 선택해 Connected/Default Routing Table과 NAT Translation을 확인합니다.
3. Switch를 선택해 VLAN과 학습된 MAC Address Table을 확인합니다.
4. PC1에서 Request DHCP를 눌러 임대 과정을 확인합니다.
5. AP, Firewall, Router에서 Lab Status를 눌러 Milestone 7 상태를 비교합니다.

## 현재 범위

NetLab 1.0은 교육용 논리 시뮬레이터입니다. STP, OSPF, RIP, IPv6, Wireless, Firewall, VPN은 구성 상태와 핵심 판정 과정을 학습하도록 단순화되어 있습니다. 실제 무선 전파, 운영체제 네트워크 스택, VPN 암호화 또는 실제 인터넷 통신을 수행하지 않습니다.
