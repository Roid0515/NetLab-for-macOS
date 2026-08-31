# NetLab v1.3

Core 네트워크 모델과 시뮬레이션 안정성을 개선하고 자동 테스트를 제품 코드에서 분리한 릴리즈입니다.

## 주요 변경 사항

- IPv4, IPv6, subnet mask, gateway, VLAN 입력 검증 강화
- 인터페이스 설정을 전체 검증 후 한 번에 반영하도록 변경
- 장비 역할과 capability를 `DeviceCatalog`의 단일 정의로 통합
- 실제 활성 링크와 VLAN 경로를 사용하는 DHCP 및 DNS 시뮬레이션
- 다양한 subnet mask에서 network, broadcast, gateway, 사용 중 주소를 제외하는 DHCP 임대
- 링크 상태와 속도를 Core `Link` 모델로 통합
- 중복 장비, 링크 ID 및 인터페이스 재사용 연결 방지
- 제품 실행 코드의 자체 테스트 분기를 별도 `NetLabTests` 타깃으로 분리
- Demo의 DHCP, DNS Ping 및 노드 삭제를 포함한 UI 스모크 테스트 추가
- 일반적인 환경 파일, 인증서, 개인키 및 secrets 파일의 Git 추적 방지

## 호환성

- 노드 추가·선택·이동·삭제, 링크 연결, IP 설정, DHCP 요청, Ping, Demo 사용 방법은 유지됩니다.
- Trunk 포트는 사용자가 입력한 VLAN만 전달합니다.
- Demo의 `pc2.netlab` DNS Ping 동작은 유지됩니다.

## 검증 결과

- Debug 및 Release 빌드 성공
- Debug 및 Release 자동 검사 각각 59개 통과
- Clang 정적 분석 경고 0건
- Apple Silicon 및 Intel을 포함한 Universal Binary
- 앱 버전 `1.3.0`, 빌드 `3`

## 참고

배포 앱은 개인 실행용 ad-hoc 서명입니다. Developer ID 서명과 Apple 공증은 포함하지 않습니다.
