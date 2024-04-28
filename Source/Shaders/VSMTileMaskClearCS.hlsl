RWTexture2D<uint> VirtualSMFlags;//64*64
RWTexture2D<uint> PhysicalShadowDepthTexture; // 1024 * 1024

[numthreads(16, 16, 1)]
void VSMTileMaskClearCS(uint2 DispatchThreadID :SV_DispatchThreadID)
{
    VirtualSMFlags[DispatchThreadID] = 0;
    
    [unroll]
    for(uint i = 0; i < 16 ; i++)
    {
        for(uint j = 0; j < 16 ;j++)
        {
            PhysicalShadowDepthTexture[DispatchThreadID * 16 + uint2(i,j)] = 0;
        }
    }
}
