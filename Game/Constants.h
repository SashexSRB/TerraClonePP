#pragma once

namespace Constants {
    constexpr float TileSize        = 32.0f;
    constexpr float VisibleTilesX   = 160.0f;
    constexpr int   AtlasWidth      = 256;
    constexpr int   AtlasTileSize   = 8;
    constexpr float PlayerZ         = 0.1f;
    constexpr float PlayerWidth     = 64.0f;
    constexpr float PlayerHeight    = 96.0f;
    constexpr float PlayerHBWidth   = 44.0f;
    constexpr float PlayerHBHeight  = 90.0f;
    constexpr float PlrHbOffsetW    = (PlayerWidth - PlayerHBWidth) / 2.0f;
    constexpr float PlrHbOffsetH    = PlayerHeight - PlayerHBHeight;
    constexpr float PlayerMoveSpeed = 300.0f;
    constexpr float PlayerJumpSpeed = 575.0f;
    constexpr float BlockReach      = 6.0f;
}
