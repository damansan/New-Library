# 設計メモ（New-Library）

このファイルは collect_all.bat で collected_all.txt の先頭に書き出される。
構成のルールはここが正。コードと食い違ったら、このメモを更新する。

## 0. 目的と判断の物差し

- 目的：パルクールゲーム（構想まとめ）を実現する
- 新しい仕組みを作る・決めるのは、構想の優先度の高い項目に必要なときだけ。それ以外は 9章に書いて今は作らない
- 迷ったら Unity を参考にする（データ・アセット・Prefab の扱い）

## 1. 層（プロジェクト）と依存

| 層 | 種類 | 参照してよい層 |
|---|---|---|
| Engine | lib | Engine |
| Game | lib | Engine, Game |
| Tool | lib | Engine, Tool |
| GameTool | lib | Engine, Game, Tool, GameTool |
| App（GameApp / EditorApp） | exe | すべて |
| Launcher | exe（3dgp.exe） | なし |

- GameApp = Engine + Game（RUN_GAME_APP 定義あり）、EditorApp = Engine + Game + Tool + GameTool
- 依存の向きは Projects/Scripts/CheckIncludes.ps1 がビルド前に検査する（違反は LAYER001）

## 2. ファイルの置き場所と include

- ソースは Source/Code/<層>/ 以下。vcxproj は Projects/<層>/
- include の起点は Source/Code。層の名前から書く（例：#include "Engine/ECS/Components/MovementComponent.h"）
- VS で新規追加するときは、場所を Source/Code/<層>/... に変える
- 共通のビルド設定は Projects/Common.props

## 3. 層ごとのテーマ

| 層 | テーマ |
|---|---|
| Engine / Game | ECS らしく（4章） |
| Tool / GameTool | ゲームを止めずに触る。再生／停止は作らない。Studio は別の世界（自分の registry・System・ResourceManager）で確かめ、保存したファイルで動いているゲームへ届ける |

## 4. ECS の決まり

### 4.1 原則

| # | 原則 | 例外・注意 |
|---|---|---|
| E1 | データは Component、処理は System | System がキャッシュやログの重複防止を持つのはよい |
| E2 | 設定と状態を分ける | 設定＝人が決める（保存する）、状態＝System が決める（保存しない） |
| E3 | 「今〜中」は Component の有無で表す | 毎フレーム切り替わる値には使わない |
| E4 | 識別は型で行う。文字列で探さない | エンティティは EntityCategory で探す |
| E5 | 1つの値の書き手は1人 | 譲るときはデータで明示（例：LocomotionMode::External） |
| E6 | System どうしはデータで伝え合う | 別の System を直接呼ばない |

### 4.2 Component の付け外し

| 関係 | 例 | ルール |
|---|---|---|
| 派生 | Mesh → SkinnedMesh / StaticMesh、Physics(Character) → CharacterController＋Movement | 作った System が外す。元が変わる・外れたら一緒に外す |
| 依存 | Animator は SkinnedMesh が必要 | 自動で付けない・外さない。足りなければ Inspector で知らせる |
| 排他 | StaticBody と CharacterController | 種類を決める設定が1つだけを派生させる |

- 人が付けたものは人しか外さない
- 派生した Component は状態だけを持つ。設定（重力、カプセルの大きさなど）は元の Component に置く

### 4.3 宣言と登録

- Component は <層>/ECS/Components/、System は <層>/ECS/Systems/
- 調整値・Inspector に出す値は REFLECT_FIELD。見せるだけの状態は FieldFlags::VisibleOnly | FieldFlags::Transient
- 登録は EngineComponents::Register() / GameComponents::Register()。Game 独自の Inspector は GameTool の GameInspectors

### 4.4 名前と識別

- エンティティの名前は人が確かめるためのもの。EditorContext が持ち、ゲームのコードは読まない
- コードでの識別は EntityCategory

## 5. 1フレームの流れ（SceneGame::Update）

1. PhysX の更新
2. Gameplay：Controller → GameInput → PlayerIntent → StateMachine.UpdateState → Player の各 State の System → PlayerLocomotion → Movement
3. Animation：StateMachine.UpdateAnimation → Animation → Sequencer → IK
4. Resources：MeshLoader → Physics（CCT を velocity × dt で動かす、isGrounded）
5. Camera

**書き手**
- velocity.xz・rotation.y：PlayerLocomotion（ルートモーション中は AnimationSystem が書く）
- velocity.y：Jump（入った瞬間）→ Movement（重力）→ Physics（接地の押し付け）

**1フレーム遅れ（意図通り）**
- Sequencer のフラグ、Physics の isGrounded・接触の結果は、次のフレームの StateMachine から使う

## 6. データファイル

| | 決まり |
|---|---|
| 区分 | Scene＝どこに何を置くか（Prefab の参照＋差分）／Prefab＝そのエンティティは何者か（全部の値）／Studio の Json＝どう動くか |
| 読み書き | JsonFileCache 1つ（読む・保存する）。結果は「読み込み済み／ファイルなし／壊れている」 |
| 壊れている | ログを出し、データは初期値 |
| ログ | 「指定したのに無い」と「壊れている」だけ。無いのが普通のものは Inspector で確かめる |
| 保存 | 一時ファイルに書いてから置き換える |
| 読み直し | 更新日時が変わったら読み直す。シーンは読み直さない |
| 拡張子 | `.種類.json`（scene、prefab、stategraph、animedit、sequencer）。種類の判断は GetDataFileKind の1か所 |
| 先頭 | type、version。モデルに属するものは model（モデルのパス） |
| 例外 | ProjectSettings.json（固定のパス、手書き） |

**モデルに属するデータ**
- .animedit、.sequencer、.stategraph は 1モデルに1種ずつ
- 置き場所は Data/Model/<モデル名>/Json/。名前は自由
- そのフォルダから、先頭の model が一致するものに自動で結び付く。Inspector には表示だけ
- 同じ種類が2つあれば警告し、どちらも使わない

**アニメーション**
- 読み込み時に、生データ ＋ .animedit から焼き込む（トリミング → ルートのずれ除去 → 区間 → 向き）。生データは控えておく
- .animedit・.sequencer の時刻は生データの時刻で保存する。変換は Animation::ToRawTime / ToBakedTime だけ

## 7. ツール

| ツール | 単位 | やること |
|---|---|---|
| Studio | モデル1つ | アニメ・シーケンサー・ステートグラフ・判定を作る |
| ステージ作成ツール（メインのエディタ） | シーン1つ | パーツを置く・保存する・試す |

- Studio のタイムラインは共通の部品（StudioTimelineWidget）を使う

## 8. 用語と書き方

- ブレンド：上半身と下半身などに分けてポーズを混ぜること
- レイヤー：物理の衝突の組み合わせ（レイヤー表）
- ログは簡潔に。敬語を使わない
- `using namespace DirectX;` は使わない。`DirectX::` を付ける
- windows.h の max/min マクロを避けるため `(std::max)` と書く

## 9. 作るときに決めること・保留

**作るときに決める**
- Y を誰が書くか（壁走り・よじ登り・ワイヤー）。External で譲る形の作り
- 設定と状態を分ける範囲（Animator、Sequencer、StateMachine、Movement、Physics の設定）
- Any State（除外付き）、Exit → Entry
- ブレンド（上下半身）

**保留**
- 自動の結び付けの上書き（ドロップ）
- 武器によるステートの切り替え
- 派生キャッシュ（.cereal、.dds）の自動作り直し
- State 名・クリップ名の守り方
- ECS の登録忘れの検出、表示名の一本化、名前の付け方
- Studio・Editor の作りの整理（パネルの登録、時計、伝え方）