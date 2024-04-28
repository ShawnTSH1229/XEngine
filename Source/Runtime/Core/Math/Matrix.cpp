#include "Matrix.h"
namespace DirectX
{
    namespace SimpleMath
    {
        LMatrix LMatrix::Transpose() const noexcept
        {
            using namespace DirectX;
            const XMMATRIX M = XMLoadFloat4x4(this);
            LMatrix R;
            XMStoreFloat4x4(&R, XMMatrixTranspose(M));
            return R;
        }
        Matrix LMatrix::Invert() const noexcept
        {
            using namespace DirectX;
            const XMMATRIX M = XMLoadFloat4x4(this);
            Matrix R;
            XMVECTOR det;
            XMStoreFloat4x4(&R, XMMatrixInverse(&det, M));
            return R;
        }

        LMatrix DirectX::SimpleMath::LMatrix::CreateFromAxisAngle(const Vector3& axis, float angle) noexcept
        {
            using namespace DirectX;
            Matrix R;
            const XMVECTOR a = XMLoadFloat3(&axis);
            XMStoreFloat4x4(&R, XMMatrixRotationAxis(a, angle));
            return R;
        }

        LMatrix LMatrix::CreateScale(const Vector3& scales) noexcept
        {
            using namespace DirectX;
            LMatrix R;
            XMStoreFloat4x4(&R, XMMatrixScaling(scales.x, scales.y, scales.z));
            return R;
        }

        LMatrix LMatrix::CreateTranslation(const Vector3& position) noexcept
        {
            using namespace DirectX;
            LMatrix R;
            XMStoreFloat4x4(&R, XMMatrixTranslation(position.x, position.y, position.z));
            return R;
        }

        LMatrix DirectX::SimpleMath::LMatrix::CreateMatrixLookAtLH(const Vector3& EyePos, const Vector3& TargetPos, const Vector3& UpDir)
        {
            using namespace DirectX;
            Matrix R;
            const XMVECTOR eyev = XMLoadFloat3(&EyePos);
            const XMVECTOR targetv = XMLoadFloat3(&TargetPos);
            const XMVECTOR upv = XMLoadFloat3(&UpDir);
            XMStoreFloat4x4(&R, XMMatrixLookAtLH(eyev, targetv, upv));
            return R;
        }
        LMatrix LMatrix::CreateOrthographicOffCenterLH(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane) noexcept
        {
            using namespace DirectX;
            Matrix R;
            XMStoreFloat4x4(&R, XMMatrixOrthographicOffCenterLH(left, right, bottom, top, zNearPlane, zFarPlane));
            return R;
        }

        static XMMATRIX XM_CALLCONV SimpleMath_MMatrixPerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
        {
            assert(NearZ > 0.f && FarZ > 0.f);
            assert(!XMScalarNearEqual(FovAngleY, 0.0f, 0.00001f * 2.0f));
            assert(!XMScalarNearEqual(AspectRatio, 0.0f, 0.00001f));
            assert(!XMScalarNearEqual(FarZ, NearZ, 0.00001f));

#if defined(_XM_NO_INTRINSICS_)

            float    SinFov;
            float    CosFov;
            XMScalarSinCos(&SinFov, &CosFov, 0.5f * FovAngleY);

            float Height = CosFov / SinFov;
            float Width = Height / AspectRatio;
            float fRange = NearZ / (NearZ - FarZ);

            XMMATRIX M;
            M.m[0][0] = Width;
            M.m[0][1] = 0.0f;
            M.m[0][2] = 0.0f;
            M.m[0][3] = 0.0f;

            M.m[1][0] = 0.0f;
            M.m[1][1] = Height;
            M.m[1][2] = 0.0f;
            M.m[1][3] = 0.0f;

            M.m[2][0] = 0.0f;
            M.m[2][1] = 0.0f;
            M.m[2][2] = fRange;
            M.m[2][3] = 1.0f;

            M.m[3][0] = 0.0f;
            M.m[3][1] = 0.0f;
            M.m[3][2] = -fRange * FarZ;
            M.m[3][3] = 0.0f;
            return M;

#elif defined(_XM_ARM_NEON_INTRINSICS_)
            float    SinFov;
            float    CosFov;
            XMScalarSinCos(&SinFov, &CosFov, 0.5f * FovAngleY);

            float fRange = NearZ / (NearZ - FarZ);
            float Height = CosFov / SinFov;
            float Width = Height / AspectRatio;
            const XMVECTOR Zero = vdupq_n_f32(0);

            XMMATRIX M;
            M.r[0] = vsetq_lane_f32(Width, Zero, 0);
            M.r[1] = vsetq_lane_f32(Height, Zero, 1);
            M.r[2] = vsetq_lane_f32(fRange, g_XMIdentityR3.v, 2);
            M.r[3] = vsetq_lane_f32(-fRange * FarZ, Zero, 2);
            return M;
#elif defined(_XM_SSE_INTRINSICS_)
            float    SinFov;
            float    CosFov;
            XMScalarSinCos(&SinFov, &CosFov, 0.5f * FovAngleY);

            float fRange = NearZ / (NearZ - FarZ);
            // Note: This is recorded on the stack
            float Height = CosFov / SinFov;
            XMVECTOR rMem = {
                Height / AspectRatio,
                Height,
                fRange,
                -fRange * FarZ
            };
            // Copy from memory to SSE register
            XMVECTOR vValues = rMem;
            XMVECTOR vTemp = _mm_setzero_ps();
            // Copy x only
            vTemp = _mm_move_ss(vTemp, vValues);
            // CosFov / SinFov,0,0,0
            XMMATRIX M;
            M.r[0] = vTemp;
            // 0,Height / AspectRatio,0,0
            vTemp = vValues;
            vTemp = _mm_and_ps(vTemp, g_XMMaskY);
            M.r[1] = vTemp;
            // x=fRange,y=-fRange * NearZ,0,1.0f
            vTemp = _mm_setzero_ps();
            vValues = _mm_shuffle_ps(vValues, g_XMIdentityR3, _MM_SHUFFLE(3, 2, 3, 2));
            // 0,0,fRange,1.0f
            vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(3, 0, 0, 0));
            M.r[2] = vTemp;
            // 0,0,-fRange * NearZ,0.0f
            vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(2, 1, 0, 0));
            M.r[3] = vTemp;
            return M;
#endif
        }

        LMatrix LMatrix::MMatrixPerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
        {
            LMatrix Ret;
            XMMATRIX P = SimpleMath_MMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);
            XMStoreFloat4x4(&Ret, P);
            return Ret;
        }


    }
}