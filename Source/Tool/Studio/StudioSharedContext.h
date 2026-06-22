#pragma once
#include <d3d11.h>
#include "Rendering/SkyBox.h"
#include "Resource/ResourceManager.h"

struct StudioSharedContext
{
    SkyBox          skyBox;
    ResourceManager resourceManager;

    void Initialize(ID3D11Device* device)
    {
        skyBox.Initialize(
            device, 		
            "Data/SkyBox/BlueSky"
            //"Data/SkyBox/DynamicSky"
        );
    }
};