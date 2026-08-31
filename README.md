# Subway Puzzle Prototype

Unreal Engine 5.8.2용 순수 C++ 프로토타입입니다. Blueprint 및 프로젝트 전용 콘텐츠 에셋을 사용하지 않습니다. 실행 시 C++가 엔진 기본 메시로 보드와 카메라를 생성합니다.

## 구현 범위

1. 10개 노드에서 WASD/방향키 한 칸 이동
2. 클릭한 노드까지 동일 그래프를 사용하는 A* 경로 탐색
3. 빨간 장벽으로 표시되는 차단 간선과 화면 피드백

## 실행

1. `SubwayPuzzlePrototype.uproject`를 Unreal Engine 5.8.2로 엽니다.
2. 프로젝트가 열리면 Play 버튼을 누릅니다.
3. WASD/방향키 또는 파란색·초록색 노드를 클릭합니다.

초록색 9번 노드가 목표입니다. 1-2번 및 5-6번 사이의 빨간 장벽은 그래프에서 차단된 연결이며, 클릭 이동은 자동으로 우회합니다.

## 주요 C++ 클래스

- `APuzzleBoard`: 노드/간선 데이터, 방향 판정, A* 경로 탐색, 회색박스 생성
- `APuzzlePawn`: 이동 명령과 경로 실행
- `APuzzlePlayerController`: C++에서 생성한 Enhanced Input 매핑 및 클릭 판정
- `APuzzleHUD`: 조작법, 노드 번호, 이동 불가 사유 표시
- `APuzzleGameMode`: 보드, 조명 리그, 고정 카메라 생성
- `UPuzzleGraphAsset`: 노드/간선/시작/목표를 담는 Data Asset (스테이지 단위)
- `APuzzleNodeMarker`: 뷰포트 배치용 노드 마커 (저작 전용, 런타임에 안 쓰임)
- `APuzzleGraphBaker`: 레벨의 마커를 모아 Data Asset으로 굽고 되읽는 저작 도구
- `UPuzzleSettings`: Project Settings > Game > Subway Puzzle. 기본 스테이지 지정

## 그래프 저작 (뷰포트 배치 → Data Asset)

1. 저작용 레벨을 만듭니다: File > New Level > Empty Level → `Content/Maps/GraphAuthoring`으로 저장
2. Content Browser에서 우클릭 > **Data > Data Asset** (UE 5.8 기준. 우클릭 후 검색창에 `data asset`을 쳐도 됩니다) → "Pick Class For Data Asset Instance" 창에서 `PuzzleGraphAsset` 선택 → `Content/Graphs/Stage01` 등으로 저장
3. 레벨에 `PuzzleGraphBaker` 하나를 배치하고, 디테일 패널의 **Target Asset**에 2번 애셋을 지정합니다. 이 액터의 트랜스폼이 보드 원점입니다.
4. `PuzzleNodeMarker`를 배치합니다. 기본 이동 기즈모로 자유롭게 옮기고 Ctrl+D로 복제하면 됩니다.
   - **Links**: 배열에 항목을 추가하고 Target에 연결할 마커를 지정합니다(스포이드로 뷰포트에서 직접 찍거나 아웃라이너에서 드래그)
   - **Block Reason**: `Open`이면 통행 가능, `Stairs` / `Narrow Doorway` / `Step Gap`이면 차단 + 사유별 피드백
   - **Is Start Node** / **Is Goal Node**: 시작·목표 지정
   - 뷰포트에 간선이 실시간으로 그려집니다 (청록 = 통행, 빨강 = 차단, 노란 구 = 시작, 초록 구 = 목표)
5. Baker의 **Bake Level -> Asset** 버튼을 누르고 애셋을 저장합니다. 반대로 **Load Asset -> Level** 은 애셋에서 마커를 복원합니다.

실행할 스테이지는 Project Settings > Game > Subway Puzzle > **Default Graph Asset** 에서 고릅니다. 비워두면 코드에 내장된 10노드 테스트 그래프가 쓰입니다.

> 노드를 격자에서 크게 벗어나게 배치하면 방향키 판정이 실패할 수 있습니다. `APuzzleBoard::FindDirectionalTarget`의 임계값(`BestDot = 0.7f`) 때문에 누른 방향과 45.6° 이상 어긋난 간선은 선택되지 않습니다.

## 에디터 뷰포트가 검게 보이는 경우

시작 맵인 `/Engine/Maps/Entry`는 조명도 하늘도 없는 빈 엔진 맵입니다. 보드·조명·카메라는 모두 실행 시 C++가 생성하므로, Play를 누르기 전 에디터 뷰포트는 원래 검은 화면입니다. Play를 누르면 화면이 나와야 하며, 그래도 검다면 Output Log에서 `LogSubwayPuzzle` 오류를 확인하세요.
