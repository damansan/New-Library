#pragma once

// アニメーターグラフ編集中の画面遷移要求
enum class NavigationRequest
{
	None,
	EditCondition,  // 選択中のLinkの条件グラフへ
	EditChildGraph, // 選択中のStateの子階層へ
};