\# 設計メモ（New-Library）



このファイルは collect\_all.bat で collected\_all.txt の先頭に書き出される。

構成のルールはここが正。コードと食い違ったらこのメモを更新する。



\## 1. 層（プロジェクト）と依存

| 層 | 種類 | 参照してよい層 |

|---|---|---|

| Engine   | lib | Engine |

| Game     | lib | Engine, Game |

| Tool     | lib | Engine, Tool |

| GameTool | lib | Engine, Game, Tool, GameTool |

| App（GameApp / EditorApp） | exe | すべて |

| Launcher | exe（3dgp.exe） | なし（単独） |



\- GameApp = Engine + Game（RUN\_GAME\_APP 定義あり）

\- EditorApp = Engine + Game + Tool + GameTool

\- 依存の向きはビルド前に Projects/Scripts/CheckIncludes.ps1 が検査する（違反は LAYER001）



\## 2. ファイルの置き場所と include

\- ソースはすべて Source/Code/<層>/ 以下。vcxproj は Projects/<層>/ にある

\- include の起点は Source/Code のみ。必ず層の名前から書く

&#x20; 例: #include "Engine/ECS/Components/MovementComponent.h"

\- Visual Studio で新規追加するときは、場所を Source/Code/<層>/... に変更すること

&#x20; （初期値の Projects/<層>/ に置くと、CheckIncludes の検査対象から外れる）

\- 共通のビルド設定（include パス、出力先、定義）は Projects/Common.props に集約



\## 3. ECS の決まり

\- Component: <層>/ECS/Components/、System: <層>/ECS/Systems/

\- 調整値・インスペクターに出す値は REFLECT\_FIELD で宣言する

\- 実行中だけの値は FieldFlags::VisibleOnly | FieldFlags::Transient

\- Game の Component は GameComponents::Register() で登録する

\- Game 独自のインスペクターは GameTool の GameInspectors に置く

\- windows.h の min/max マクロとぶつかるため、(std::max) のように括弧で囲む



\## 4. 1フレームの実行順（SceneGame）

1\. PhysX シーン更新

2\. ControllerSystem → GameInputSystem（生の入力）

3\. StateMachineSystem::UpdateState（ステート遷移）

4\. Player の各ステート System

5\. MovementSystem（重力）

6\. StateMachineSystem::UpdateAnimation → AnimationSpeedSystem → AnimationSystem（ルートモーションは velocity を上書き）

7\. SequencerSystem → IKSystem

8\. MeshLoaderSystem → PhysicsSystem（CCT が velocity × dt で移動）

9\. GameCameraSystem



\## 5. 誰が何を書くか（Player 移動の作り直しで決定中）

\- 【予定】velocity の XZ と rotation.y を書くのは PlayerLocomotionSystem の1か所だけ

\- 【予定】各ステートの System は「どう動きたいか」の設定を渡すだけで、velocity は書かない

\- 【予定】ステートグラフの遷移条件は生の入力ではなく、集約済みの意図（PlayerIntent）を見る

\- 例外: ルートモーションのクリップ再生中は AnimationSystem が velocity を書く

