# NetLab 리팩터링 작업 결과

- 릴리즈 버전: v1.3 (앱 버전 1.3.0, 빌드 3)
- 작업일: 2026-08-31
- 대상: Core 네트워크 모델, 시뮬레이션 엔진, UI 연결 계층, 테스트 구조

## 사용 설명서 호환성 분석

기존 `NetLab_간단_사용설명서.md`에 안내된 노드 추가·선택·이동·삭제, 링크 연결, IP 설정, DHCP 요청, Ping, Demo 불러오기 흐름은 그대로 유지된다.

다음 두 항목은 정확한 시뮬레이션을 위해 함께 조정했다.

1. Trunk 포트는 더 이상 VLAN 10과 20을 암묵적으로 모두 허용하지 않는다. 입력한 VLAN만 전달하며 사용 설명서도 같은 내용으로 수정했다.
2. DNS는 토폴로지와 무관한 전역 검색을 하지 않는다. 클라이언트에 설정된 DNS 서버가 실제 활성 링크와 같은 VLAN에서 도달 가능할 때만 조회한다. Milestone 7 Demo의 PC1에는 DNS 서버 `10.10.10.53`을 지정해 기존 `pc2.netlab` Ping 학습 흐름을 유지했다.

따라서 사용 설명서의 주요 학습 기능이 사라지거나 조작 방식이 틀어지는 큰 호환성 문제는 없다.

## 변경 파일

### Core

- `Device.hpp`, `Device.cpp`: 장비 기능(capability) 검사, 원자적 인터페이스 설정, DHCP/DNS/NAT/ACL/라우팅 입력 검증
- `DeviceDefinition.hpp`, `DeviceCatalog.cpp`: 장비 역할과 기능을 카탈로그 정의의 단일 기준으로 통합
- `NetworkInterface.hpp`, `NetworkInterface.cpp`: IPv4/IPv6 및 prefix 검증, IPv6 설정 해제, 원자적 설정 지원
- `Link.hpp`, `Link.cpp`: 링크 유형과 상태를 Core 링크 모델에 통합
- `SimulationEngine.hpp`, `SimulationEngine.cpp`: 실제 링크 기반 경로 탐색, VLAN/DHCP/DNS 정확도 개선, 중복 장비·링크·인터페이스 연결 차단

### UI와 앱 시작 코드

- `DeviceNodeView.h`, `DeviceNodeView.mm`: 카탈로그 역할/기능 전달, 원자적 설정 API 연결
- `DeviceInspectorView.mm`: 잘못된 값이 일부만 적용되지 않도록 일괄 검증 후 적용
- `LinkLayerController.h`, `LinkLayerController.mm`: Core `Link`를 링크 상태·속도의 기준 모델로 사용
- `TopologyView.h`, `TopologyView.mm`: 시뮬레이션 엔진 생성 중복 제거, Demo DNS 설정 보존
- `MainWindowController.h`, `MainWindowController.mm`, `main.mm`: 제품 실행 경로에 섞여 있던 자체 테스트 분기 제거

### 테스트와 프로젝트

- `NetLabTests/main.cpp`: Core 자동 검사
- `NetLabTests/UISmokeTests.mm`: 창 구성, Demo, DHCP, DNS Ping, 노드 삭제 UI 스모크 검사
- `NetLabTests` Xcode 타깃 및 공유 Scheme 추가
- `README.md`, `NetLab/Tests/README.md`: 새 테스트 실행 방법 반영
- `.gitignore`: 일반적인 비밀키·환경 설정 파일 제외 규칙 보강

## 삭제한 코드

- 제품 실행 파일의 `--self-test`, `--ui-self-test` 분기와 내장 테스트 코드
- Milestone 2/3 전용 Demo·레이아웃 테스트 진입점
- 시뮬레이션 엔진의 중복 연결 구조체 `EthernetConnection`
- 장비 식별자 문자열로 역할을 추측하던 Core/UI 연결 코드
- DHCP의 `/24` 고정 마스크 및 고정 호스트 범위 가정

## 중복 제거

- 링크 상태와 속도는 `Link` 모델을 단일 기준으로 사용한다.
- 장비 역할과 capability는 `DeviceCatalog` 정의에서 전달한다.
- Topology UI의 Ping, DHCP, 상태 출력은 공통 엔진 구성 함수를 사용한다.
- 네트워크 주소 문자열은 공통 IPv4 파서와 formatter를 사용한다.

## 보안 및 입력 검증 개선

- IPv4 주소, subnet mask, gateway, IPv6 주소와 prefix, VLAN 범위를 적용 전에 검증한다.
- 인터페이스 변경은 전체 값이 유효할 때만 반영되어 부분 적용 상태가 남지 않는다.
- DHCP 서버, DNS, NAT, ACL, 라우팅, 무선, VPN 기능은 해당 장비 capability가 있을 때만 활성화된다.
- 잘못된 DHCP pool, DNS 레코드, ACL 주소, 중복 장비/링크/인터페이스 연결을 거부한다.
- `.env`, 인증서, 개인키, provisioning 파일과 일반적인 secrets 파일이 Git에 포함되지 않도록 했다.

## 시뮬레이션 정확도 개선

- `Up` 상태이고 양쪽 인터페이스가 활성화된 실제 `Link`만 패킷 전달에 사용한다.
- Access/Trunk VLAN 일치 여부를 실제 경로마다 확인한다.
- DHCP 서버는 동일한 활성 L2/VLAN 경로에서 도달 가능한 서버만 선택한다.
- DHCP는 `/16`, `/24`, `/25`, `/26` 등 유효한 mask에서 network, broadcast, gateway, 사용 중 주소를 제외하고 임대한다.
- DNS는 클라이언트에 구성된 서버 주소와 실제 도달 가능성을 확인한다.
- 연결 경로 및 라우팅은 중복 장비·링크로 인해 모호해지지 않도록 등록 단계에서 검증한다.

## 테스트 결과

- Debug 앱 빌드: 성공
- Release 앱 빌드: 성공
- Clang 정적 분석: 성공, 분석 경고 0건
- Debug 자동 검사: 59개 통과
- Release 자동 검사: 59개 통과
- 검사 범위: IPv4/IPv6 검증, 원자적 롤백, 라우팅, capability, VLAN, 링크 상태, DHCP 여러 mask와 주소 제외, DNS 도달성, Demo UI, 노드 삭제

Xcode 없이도 다음 명령으로 테스트할 수 있다.

```sh
xcodebuild -project NetLab.xcodeproj -scheme NetLabTests -configuration Debug -derivedDataPath build build
./build/Build/Products/Debug/NetLabTests
```

## 남아 있는 기술 부채

- DNS 도달성은 현재 교육용 L2/VLAN 경로를 기준으로 한다. 라우터를 여러 대 통과하는 일반화된 DNS 요청 시뮬레이션은 별도 확장이 필요하다.
- 테스트는 독립 실행형 Xcode 테스트 타깃이다. Xcode Test Navigator와 세밀한 리포팅이 필요하면 XCTest bundle로 이전할 수 있다.
- Developer ID 서명, Hardened Runtime, Notarization, App Sandbox는 배포 인증서와 권한 설계가 필요한 항목이라 이번 리팩터링에서 강제로 활성화하지 않았다.
- 토폴로지 영속 저장 형식 전체를 재설계하지 않고 현재 모델 구조를 유지했다.
