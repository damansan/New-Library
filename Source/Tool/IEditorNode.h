#pragma once

class IEditorNode
{
public:
    virtual ~IEditorNode() = default;

    virtual int GetId() const = 0;

    // ノード中身の描画
    virtual void DrawContent() = 0;

    // ピンの設定
    virtual void DrawPins() = 0;
};