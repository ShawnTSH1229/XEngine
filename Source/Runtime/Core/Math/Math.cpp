#include "Math.h"

namespace DirectX
{
    namespace SimpleMath
    {
        const LMatrix LMatrix::Identity = { 1.f, 0.f, 0.f, 0.f,
                                  0.f, 1.f, 0.f, 0.f,
                                  0.f, 0.f, 1.f, 0.f,
                                  0.f, 0.f, 0.f, 1.f };
    }
}

XBoundingBox& XBoundingBox::Transform(XMatrix& MatrixIn)
{
    DirectX::BoundingBox BBCom(Center, Extent);
    DirectX::BoundingBox BBOut;
    BBCom.Transform(BBOut, MatrixIn);

    XBoundingBox XBBOut;
    XBBOut.Center = BBOut.Center;
    XBBOut.Extent = BBOut.Extents;
    return XBBOut;
}
